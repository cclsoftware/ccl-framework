//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2025 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/platform/win/gui/textbox.win.cpp
// Description : Platform-specific Text Control implementation
//
//************************************************************************************************

#include "ccl/platform/win/gui/textbox.win.h"
#include "ccl/platform/win/gui/window.win.h"

#include "ccl/gui/keyevent.h"
#include "ccl/gui/help/helpmanager.h"
#include "ccl/gui/theme/visualstyle.h"
#include "ccl/gui/theme/themerenderer.h"
#include "ccl/gui/windows/window.h"

#include "ccl/base/message.h"

#include "ccl/public/base/buffer.h"
#include "ccl/public/math/mathprimitives.h"
#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/graphics/dpiscale.h"

#include "ccl/platform/win/gui/win32graphics.h"

#include <windowsx.h>

using namespace CCL;

static LRESULT CALLBACK CCLTextEditProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static void* EditWindowProc = nullptr;

//************************************************************************************************
// NativeTextControl
//************************************************************************************************

NativeTextControl* NativeTextControl::create (Control& owner, const Rect& clientRect, int returnKeyType, int keyboardType)
{
	return NEW WindowsTextControl (owner, clientRect, returnKeyType, keyboardType);
}

//************************************************************************************************
// WindowsTextControl
//************************************************************************************************

WindowsTextControl* WindowsTextControl::fromHWND (void* hwnd)
{
	return (WindowsTextControl*)::GetWindowLongPtr ((HWND)hwnd, GWLP_USERDATA);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowsTextControl::WindowsTextControl (Control& owner, const Rect& clientRect, int returnKeyType, int keyboardType)
: NativeTextControl (owner, returnKeyType, keyboardType),
  handle (nullptr),
  hFont (nullptr),
  hBrush (nullptr),
  lastScrollPos (kMinCoord, kMinCoord)
{
	// owning control must be attached!
	Window* w = owner.getWindow ();
	SOFT_ASSERT (w != nullptr, "Win32 text control owner not attached!!!\n")
	if(!w)
		return;

	const IVisualStyle& visualStyle = getVisualStyle ();

	DWORD xstyle = 0;
	DWORD wstyle = WS_CHILD|ES_AUTOHSCROLL;

	bool isMultiLine = owner.getStyle ().isCustomStyle (Styles::kTextBoxAppearanceMultiLine);

	if(isMultiLine)
	{
		wstyle |= ES_MULTILINE|ES_WANTRETURN|ES_AUTOVSCROLL;

		// MSDN: If you do not specify ES_AUTOHSCROLL, the control automatically
		// wraps words to the beginning of the next line when necessary.
		if(visualStyle.getTextFormat ().isWordBreak ())
			wstyle &= ~ES_AUTOHSCROLL;

		if(owner.getStyle ().isHorizontal ())
			wstyle |= WS_HSCROLL;
		if(owner.getStyle ().isVertical ())
			wstyle |= WS_VSCROLL;

		// add border to indicate edit mode, OS scrollbar style is different from application theme anyway
		if(owner.getStyle ().isHorizontal () || owner.getStyle ().isVertical ())
			wstyle |= WS_BORDER;
		
		canceled = false;
	}

	if(owner.getStyle ().isCustomStyle (Styles::kTextBoxBehaviorPasswordEdit))
		wstyle |= ES_PASSWORD;

	if(!owner.isEnabled ())
		wstyle |= ES_READONLY;// was: WS_DISABLED

	switch(visualStyle.getTextAlignment ().getAlignH ())
	{
	case Alignment::kHCenter : wstyle |= ES_CENTER; break;
	case Alignment::kRight : wstyle |= ES_RIGHT;  break;
	default: wstyle |= ES_LEFT; break;
	}

	DWORD windowXStyle = ::GetWindowLong ((HWND)w->getSystemWindow (), GWL_EXSTYLE);
	if(windowXStyle & WS_EX_LAYERED)
		xstyle |= WS_EX_LAYERED; // edit control must be layered as well

	handle = ::CreateWindowEx (xstyle, L"EDIT", L"", wstyle, 0, 0, 0, 0,
							   (HWND)w->getSystemWindow (), nullptr, g_hMainInstance, nullptr);
	
	::SetWindowLongPtr ((HWND)handle, GWLP_USERDATA, (LONG_PTR)this);

	if(xstyle & WS_EX_LAYERED)
		::SetLayeredWindowAttributes ((HWND)handle, 0, (BYTE)255, LWA_ALPHA);

	if(!EditWindowProc)
		EditWindowProc = (void*)::GetWindowLongPtr ((HWND)handle, GWLP_WNDPROC);
	::SetWindowLongPtr ((HWND)handle, GWLP_WNDPROC, (LONG_PTR)CCLTextEditProc);


	if(owner.getStyle ().isCustomStyle (Styles::kTextBoxBehaviorPasswordEdit))
	{
		uchar passwordChar = TextBox::getPasswordReplacementString ().at (0);
		::SendMessage((HWND)handle, EM_SETPASSWORDCHAR, passwordChar, 0);
	}

	if(isMultiLine)
	{
		DWORD tabWidth = 14;
		::SendMessage ((HWND)handle, EM_SETTABSTOPS, 1, (LPARAM)&tabWidth);	
	}

	updateVisualStyle ();
	updateText ();
	setSize (clientRect);

	if(!isMultiLine)
		setSelection (0, -1); // select all

	::ShowWindow ((HWND)handle, SW_SHOW);
	::SetFocus ((HWND)handle);

	// it sometimes happens that an edit control in a layered window isn't painted as part of ::ShowWindow (above) and so appears black initially
	// invalidating helps but would be be too early here
	if((xstyle & WS_EX_LAYERED) && !owner.hasBeenDrawn ())
		(NEW Message ("invalidate"))->post (this, 100);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowsTextControl::~WindowsTextControl ()
{
	if(handle)
	{
		// avoid focus recursion by restoring original window procedure...
		::SetWindowLongPtr ((HWND)handle, GWLP_USERDATA, 0);
		::SetWindowLongPtr ((HWND)handle, GWLP_WNDPROC, (LONG_PTR)EditWindowProc);
		::DestroyWindow ((HWND)handle);
	}

	if(hFont)
		::DeleteObject (hFont);
	if(hBrush)
		::DeleteObject (hBrush);

	cancelSignals ();

	// set focus back to parent window (e.g. for mouse wheel)
	if(Window* w = owner.getWindow ())
		if(w->isActive () && !w->isInCloseEvent ()) // but only if window is active and not closing
			::SetFocus ((HWND)w->getSystemWindow ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float WindowsTextControl::getContentScaleFactor () const
{
	Window* w = owner.getWindow ();
	return w ? w->getContentScaleFactor () : 1.f;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsTextControl::updateText ()
{
	String text;
	IParameter* p = getTextParameter ();
	if(p)	
		p->toString (text);
	::SetWindowText ((HWND)handle, StringChars (text));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsTextControl::getControlText (String& string)
{
	#define kWindowTextLimit STRING_STACK_SPACE_MAX
	int length = ::GetWindowTextLength ((HWND)handle);
	if(length == 0) // empty text
	{
		string = String::kEmpty;
	}
	else if(length < kWindowTextLimit)
	{
		uchar text[kWindowTextLimit] = {0};
		if(::GetWindowText ((HWND)handle, text, kWindowTextLimit))
			string.assign (text);
	}
	else
	{
		Buffer buffer;
		buffer.resize ((length + 1) * sizeof(uchar));
		if(::GetWindowText ((HWND)handle, buffer.as<uchar> (), length + 1))
			string.assign (buffer.as<uchar> ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsTextControl::setSelection (int start, int length)
{
	int end = length < 0 ? -1 : start + length;
	::SendMessage ((HWND)handle, EM_SETSEL, start, end);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsTextControl::setScrollPosition (PointRef _where)
{
	PixelPoint where (_where, getContentScaleFactor ());

	SCROLLINFO scrollInfo;
	scrollInfo.cbSize = sizeof(scrollInfo);
	scrollInfo.fMask  = SIF_ALL;
	::GetScrollInfo ((HWND)handle, SB_VERT, &scrollInfo);

	RECT rect;
	::GetWindowRect ((HWND)handle, &rect);
	int lineH = scrollInfo.nPage > 0 ? (rect.bottom - rect.top) / scrollInfo.nPage : 1;

	int numLines = (int)::SendMessage ((HWND)handle, EM_GETLINECOUNT, 0, 0);

	int lines = (int)ccl_round<0> (float (where.y) / lineH);
	int characters = 0; // todo

	::SendMessage((HWND)handle, EM_LINESCROLL, characters, lines);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Point WindowsTextControl::getScrollPosition () const
{
	if(lastScrollPos != Point (kMinCoord, kMinCoord)) // return last valid position saved in WM_KILLFOCUS
		return lastScrollPos;

	SCROLLINFO scrollInfo;
	scrollInfo.cbSize = sizeof(scrollInfo);
	scrollInfo.fMask  = SIF_ALL;
	::GetScrollInfo ((HWND)handle, SB_VERT, &scrollInfo);

	RECT rect;
	::GetWindowRect ((HWND)handle, &rect);
	int lineH =  scrollInfo.nPage > 0 ? (rect.bottom - rect.top) / scrollInfo.nPage : 1;
	
	Point where (0, scrollInfo.nPos * lineH);
	DpiScale::toCoordPoint (where, getContentScaleFactor ());
	return where;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsTextControl::setSize (RectRef clientRect)
{
	if(!handle)
		return;

	Rect rect (clientRect);
	const IVisualStyle& visualStyle = getVisualStyle ();
	if(!owner.getStyle ().isCustomStyle (Styles::kTextBoxAppearanceMultiLine))
	{
		#define ADDSIZE 5
		#if 0
		Coord minHeight = (Coord) visualStyle.getTextFont ().getSize () + ADDSIZE;
		#else
		Rect stringSize;
		Font::measureString (stringSize, "Xgjpq", visualStyle.getTextFont ()); // todo: add any taller character you know

		CCL_PRINTF ("text height: %d (ADDSIZE), %d (measureString) \n", (Coord) visualStyle.getTextFont ().getSize () + ADDSIZE, stringSize.bottom)
		Coord minHeight = stringSize.bottom;
		#endif

		if(1)//rect.getHeight () < minHeight)
		{
			// center the minHeight rect vertically with rounding (up)
			rect.top += (Coord) ccl_round<0> (rect.getHeight () / 2.f) - (Coord) ccl_round<0> (minHeight / 2.f);
			rect.setHeight (minHeight);
		}
	}

	Point offset;
	owner.clientToWindow (offset);
	rect.offset (offset);

	if(owner.getStyle ().isCommonStyle (Styles::kBorder))
		rect.contract (1);

	Window* w = owner.getWindow ();
	DpiScale::toPixelRect (rect, w->getContentScaleFactor ());

	int noRedrawFlag = 0;
	switch(visualStyle.getTextAlignment ().align)
	{
	case Alignment::kLeftTop:
		if(rect.getLeftTop () == clientRect.getLeftTop ())
			noRedrawFlag = SWP_NOREDRAW;
		break;
	case Alignment::kRightTop:
		if(rect.getRightTop () == clientRect.getRightTop ())
			noRedrawFlag = SWP_NOREDRAW;
		break;
	case Alignment::kLeftBottom:
		if(rect.getLeftBottom () == clientRect.getLeftBottom ())
			noRedrawFlag = SWP_NOREDRAW;
		break;
	case Alignment::kRightBottom:
		if(rect.getRightBottom () == clientRect.getRightBottom ())
			noRedrawFlag = SWP_NOREDRAW;
		break;
	case Alignment::kCenter:
		if(rect.getCenter () == clientRect.getCenter ())
			if(clientRect.right - rect.right == clientRect.left - rect.left)
				noRedrawFlag = SWP_NOREDRAW;
		break;
	}
	::SetWindowPos ((HWND)handle, HWND_TOP, rect.left, rect.top, rect.getWidth (), rect.getHeight (), SWP_NOZORDER | noRedrawFlag);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsTextControl::updateVisualStyle ()
{
	if(!handle)
		return;

	Window* w = owner.getWindow ();
	const IVisualStyle& visualStyle = getVisualStyle ();

	// Set colors
	backColor = visualStyle.getBackColor ();
	Color textColor = visualStyle.getTextColor ();

	// Set font
	Font font (visualStyle.getTextFont ());
	font.setSize (font.getSize () * w->getContentScaleFactor ());

	if(hFont)
		::DeleteObject (hFont);
	if(hBrush)
		::DeleteObject (hBrush);

	hBrush = Win32::GdiInterop::makeSystemBrush (SolidBrush (backColor));
	hFont = Win32::GdiInterop::makeSystemFont (font);

	SetWindowFont ((HWND) handle, hFont, TRUE);

	// Update padding
	Coord paddingLeft = visualStyle.getMetric<Coord> (StyleID::kPaddingLeft, 0);
	Coord paddingRight = visualStyle.getMetric<Coord> (StyleID::kPaddingRight, 0);
	::SendMessage ((HWND)handle, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG (paddingLeft, paddingRight));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EventResult WindowsTextControl::handleEvent (SystemEvent& e)
{
	switch(e.msg)
	{
	case WM_GETDLGCODE :
		return (EventResult)DLGC_WANTALLKEYS;

	case WM_KEYDOWN :
	case WM_SYSKEYDOWN :
		{
			KeyEvent keyEvent;
			VKey::fromSystemEvent (keyEvent, e);

			if(owner.getStyle ().isCustomStyle (Styles::kTextBoxAppearanceMultiLine)
				&& keyEvent.character == 'a'
				&& keyEvent.state.getModifiers () == KeyState::kCommand)
			{
				::SendMessage ((HWND)handle, EM_SETSEL, 0, -1);
				return (EventResult)nullptr;
			}

			if(handleKeyDown (keyEvent))
				return (EventResult)nullptr;
		}
		break;

	case WM_SETFOCUS :
		if(Window* w = owner.getWindow ())
			w->setFocusView (&owner, false);
		break;

	case WM_KILLFOCUS :
		{
			lastScrollPos = getScrollPosition (); // save scroll position for later access, will get lost in submitText () -> updateText ()

			SharedPtr<Control> ownerKeeper (&owner); // keep owner alive
			SharedPtr<NativeTextControl> selfKeeper (this); // keep us alive
			if(!canceled)
				submitText ();

			if(Window* w = owner.getWindow ())
				if(w->getFocusView () == &owner)
					w->setFocusView (nullptr, false);

			if(getRetainCount () == 1) // we are about to be destroyed, suppress call of default window procedure
				return (EventResult)nullptr;
		}
		break;

	case WM_HELP :
		HelpManager::instance ().showContextHelp (owner.asUnknown ());
		return (EventResult)TRUE;
	}

	e.notHandled = true;
	return (EventResult)-1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void* WindowsTextControl::getBrush () const
{
	return hBrush;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

unsigned int WindowsTextControl::getColor () const
{
	return Win32::GdiInterop::toSystemColor (getVisualStyle ().getTextColor ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

unsigned int WindowsTextControl::getBackColor () const
{
	return Win32::GdiInterop::toSystemColor (backColor);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WindowsTextControl::notify (ISubject* subject, MessageRef msg)
{
	if(msg == "invalidate")
		::InvalidateRect (HWND(handle), NULL, FALSE);

	NativeTextControl::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// CCLTextEditProc
//////////////////////////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK CCLTextEditProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	WindowsTextControl* edit = WindowsTextControl::fromHWND (hwnd);
	ASSERT (edit != nullptr)
	if(edit)
	{
		SystemEvent e (hwnd, msg, (void*)wParam, (void*)lParam);
		LRESULT result = (LRESULT)edit->handleEvent (e);
		if(e.wasHandled ())
			return result;
	}
	
	ASSERT (EditWindowProc != nullptr)
	return ::CallWindowProc ((WNDPROC)EditWindowProc, hwnd, msg, wParam, lParam);
}
