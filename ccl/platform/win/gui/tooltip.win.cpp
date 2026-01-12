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
// Filename    : ccl/platform/win/gui/tooltip.win.cpp
// Description : Windows Tooltips
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/windows/tooltip.h"
#include "ccl/gui/windows/window.h"
#include "ccl/gui/windows/desktop.h"

#include "ccl/platform/win/gui/windowhelper.h"
#include "ccl/platform/win/gui/win32graphics.h"
#include "ccl/platform/win/gui/screenscaling.h"
#include "ccl/platform/win/gui/dpihelper.h"

#include <commctrl.h>

namespace CCL {

//************************************************************************************************
// WindowsTooltip
//************************************************************************************************

class WindowsTooltip: public TooltipPopup
{
public:
	DECLARE_CLASS (WindowsTooltip, TooltipPopup)

	WindowsTooltip ();
	~WindowsTooltip ();

	// TooltipPopup
	void setBackColor (Color color) override;
	void setTextColor (Color color) override;
	void CCL_API construct (IView* view) override;
	void CCL_API show () override;
	void CCL_API hide () override;
	void CCL_API setPosition (PointRef pos, IView* view = nullptr) override;
	void CCL_API setText (StringRef text) override;

private:
	static const int kToolID = 100;

	HWND nativeWindow;
	float savedDpiFactor;
	HFONT nativeFontHandle;
	
	void onDpiChanged (float dpiFactor);
	void trackPosition (PointRef p);
	bool constrainPosition (Point& pos);
};

//************************************************************************************************
// WindowsTooltipFactory
//************************************************************************************************

class WindowsTooltipFactory: public TooltipFactory
{
public:
	// TooltipFactory
	ITooltipPopup* createTooltipPopup () override;
};

static const UINT kToolInfoSize = CCSIZEOF_STRUCT (TTTOOLINFO, lParam); // use this size for compatiblity with 3rd party host applications

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

void TooltipFactory::linkTooltipFactory () {} // force linkage of this file

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_INIT_LEVEL (WindowTooltip, kFrameworkLevelFirst)
{
	static WindowsTooltipFactory theFactory;
	TooltipPopup::setFactory (&theFactory);
	return true;
}

//************************************************************************************************
// WindowsTooltipFactory
//************************************************************************************************

ITooltipPopup* WindowsTooltipFactory::createTooltipPopup ()
{
	return NEW WindowsTooltip;
}

//************************************************************************************************
// WindowsTooltip
//************************************************************************************************

DEFINE_CLASS (WindowsTooltip, TooltipPopup)
// ClassID::TooltipPopup
DEFINE_CLASS_UID (WindowsTooltip, 0xA077C193, 0x3A76, 0x4834, 0xB2, 0x34, 0x05, 0x78, 0xF1, 0x13, 0xAA, 0x32)

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowsTooltip::WindowsTooltip ()
: nativeWindow (nullptr),
  savedDpiFactor (1.f),
  nativeFontHandle (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowsTooltip::~WindowsTooltip ()
{
	if(nativeWindow)
		::DestroyWindow (nativeWindow);

	if(nativeFontHandle)
		::DeleteObject (nativeFontHandle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WindowsTooltip::construct (IView* iview)
{
	// @@DPI_AWARENESS_CONTEXT: make sure calculations happen in physical pixels!
	Win32::DpiAwarenessScope dpiScope (Win32::gDpiInfo, Win32::kDpiContextDefault);

	View* view = unknown_cast<View> (iview);

	// find top-level parent in case it's a child window...
	IWindow* parentWindow = view ? view->getWindow () : nullptr;
	HWND hwndParent = parentWindow ? (HWND)parentWindow->getSystemWindow () : nullptr;
	hwndParent = Win32::FindTopLevelWindow (hwndParent);

	DWORD xstyle = WS_EX_TOPMOST;
	DWORD wstyle = WS_POPUP | TTS_NOPREFIX | TTS_NOANIMATE | TTS_NOFADE; // without TTS_NOANIMATE | TTS_NOFADE the GUI thread is blocked for 125 ms

	nativeWindow = ::CreateWindowEx (xstyle, TOOLTIPS_CLASS, nullptr, wstyle,
									 CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
									 hwndParent, nullptr, g_hMainInstance, nullptr);

	TOOLINFO toolInfo = {0};
	toolInfo.cbSize = kToolInfoSize;
	toolInfo.uFlags = TTF_TRACK|TTF_ABSOLUTE;
	toolInfo.hwnd = hwndParent;
	toolInfo.hinst = g_hMainInstance;
	toolInfo.uId = kToolID;

	BOOL result = (BOOL)::SendMessage (nativeWindow, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);
	#if DEBUG
	if(!result)
		CCL_DEBUGGER ("Tooltip creation failed! Manifest missing?\n")
	#endif

	float dpiFactor = Win32::gScreens.getPrimaryScreen ().scaleFactor;
	onDpiChanged (dpiFactor);

	initColors (view);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsTooltip::onDpiChanged (float dpiFactor)
{
	savedDpiFactor = dpiFactor;

	// enable multiline tooltips by limiting max. width
	int maxPixelWidth = DpiScale::coordToPixel (500, dpiFactor);
	::SendMessage (nativeWindow, TTM_SETMAXTIPWIDTH, 0, maxPixelWidth);

	// set font corrected for per-monitor dpi
	Font font;
	if(const VisualStyle* style = ThemePainter::getStandardStyle (ThemePainter::kLabelRenderer))
		font = style->getTextFont ();
	font.setSize (font.getSize () * dpiFactor);

	HFONT oldFontHandle = nativeFontHandle;
	nativeFontHandle = Win32::GdiInterop::makeSystemFont (font);
	::SendMessage (nativeWindow, WM_SETFONT, (WPARAM)nativeFontHandle, 0);
	if(oldFontHandle)
		::DeleteObject (oldFontHandle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsTooltip::setBackColor (Color color)
{
	COLORREF backColor = Win32::GdiInterop::toSystemColor (color);
	::SendMessage (nativeWindow, TTM_SETTIPBKCOLOR, (WPARAM)backColor, 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsTooltip::setTextColor (Color color)
{
	COLORREF textColor = Win32::GdiInterop::toSystemColor (color);
	::SendMessage (nativeWindow, TTM_SETTIPTEXTCOLOR, (WPARAM)textColor, 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsTooltip::trackPosition (PointRef p)
{
	// @@DPI_AWARENESS_CONTEXT: make sure calculations happen in physical pixels!
	Win32::DpiAwarenessScope dpiScope (Win32::gDpiInfo, Win32::kDpiContextDefault);

	const Win32::ScreenInformation& screen = Win32::gScreens.screenForCoord (p);
	if(screen.scaleFactor != savedDpiFactor)
		onDpiChanged (screen.scaleFactor);

	Point p2 (p);
	screen.toPixelPoint (p2);
	::SendMessage (nativeWindow, TTM_TRACKPOSITION, 0, (LPARAM)MAKELONG (p2.x, p2.y));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WindowsTooltip::show ()
{
	TOOLINFO toolInfo = {0};
	toolInfo.cbSize = kToolInfoSize;
	toolInfo.hwnd = ::GetParent (nativeWindow);
	toolInfo.uId = kToolID;

	CCL_PROFILE_START (TTM_TRACKACTIVATE)
	::SendMessage (nativeWindow, TTM_TRACKACTIVATE, TRUE, (LPARAM)&toolInfo);
	CCL_PROFILE_STOP (TTM_TRACKACTIVATE)

	// reposition if necessary make tooltip completely visible
	Point p (-kMaxCoord, -kMaxCoord);
	if(constrainPosition (p))
	{
		trackPosition (p);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WindowsTooltip::hide ()
{
	savedText.empty ();
	savedPosition (kMinCoord, kMinCoord);

	TOOLINFO toolInfo = {0};
	toolInfo.cbSize = kToolInfoSize;
	toolInfo.hwnd = ::GetParent (nativeWindow);
	toolInfo.uId = kToolID;

	::SendMessage (nativeWindow, TTM_TRACKACTIVATE, FALSE, (LPARAM)&toolInfo);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WindowsTooltip::setPosition (PointRef pos, IView* view)
{
	Point p (pos);
	if(view)
		view->clientToScreen (p);

	// reposition if necessary to keep tooltip completely visible
	if(::IsWindowVisible (nativeWindow))
		constrainPosition (p);

	if(p != savedPosition)
	{
		savedPosition = p;
		trackPosition (p);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WindowsTooltip::setText (StringRef text)
{
	if(text != savedText)
	{
		savedText = text;

		TOOLINFO toolInfo = {0};
		toolInfo.cbSize = kToolInfoSize;
		toolInfo.hwnd = ::GetParent (nativeWindow);
		toolInfo.uId = kToolID;
		StringChars chars (text);
		toolInfo.lpszText = (LPWSTR)(const uchar*)chars;

		::SendMessage (nativeWindow, TTM_UPDATETIPTEXT, 0, (LPARAM)&toolInfo);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WindowsTooltip::constrainPosition (Point& pos)
{
	if(nativeWindow)
	{
		// @@DPI_AWARENESS_CONTEXT: make sure calculations happen in physical pixels!
		Win32::DpiAwarenessScope dpiScope (Win32::gDpiInfo, Win32::kDpiContextDefault);

		RECT winRect;
		::GetWindowRect (nativeWindow, &winRect);
		
		Rect rect;
		Win32::GdiInterop::fromSystemRect (rect, winRect);
		const Win32::ScreenInformation& screen = Win32::gScreens.screenForWindowHandle (nativeWindow);
		screen.toCoordRect (rect);

		Point oldPos (pos);
		if(oldPos == Point (-kMaxCoord, -kMaxCoord))
			oldPos (rect.left, rect.top);

		Rect monitorSize;
		Desktop.getMonitorSize (monitorSize, Desktop.findMonitor (oldPos, true), false);

		pos = oldPos;
		ccl_upper_limit (pos.x, Coord (monitorSize.right - (rect.right - rect.left)));
		ccl_upper_limit (pos.y, Coord (monitorSize.bottom - (rect.bottom - rect.top)));
		return pos != oldPos;
	}
	return false;
}
