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
// Filename    : ccl/platform/win/gui/window.win.cpp
// Description : Platform-specific Window implementation
//
//************************************************************************************************

#define DEBUG_LOG 0
#define DEBUG_EVENTS 0
#define DEBUG_EVENTS_CLASS Window

#include "ccl/platform/win/gui/window.win.h"
#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/windows/childwindow.h"
#include "ccl/gui/windows/transparentwindow.h"
#include "ccl/gui/popup/popupselector.h"

#include "ccl/gui/gui.h"
#include "ccl/gui/keyevent.h"
#include "ccl/gui/help/helpmanager.h"
#include "ccl/gui/system/dragndrop.h"
#include "ccl/gui/graphics/nativegraphics.h"

#include "ccl/base/message.h"
#include "ccl/base/signalsource.h"
#include "ccl/base/storage/configuration.h"

#include "ccl/public/gui/iapplication.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/system/isignalhandler.h"
#include "ccl/public/system/ilocalemanager.h"
#include "ccl/public/math/mathprimitives.h"

#include "ccl/platform/win/system/registry.h"
#include "ccl/platform/win/direct2d/dcompengine.h"
#include "ccl/platform/win/gui/oledragndrop.h"
#include "ccl/platform/win/gui/touchhelper.h"
#include "ccl/platform/win/gui/screenscaling.h"
#include "ccl/platform/win/gui/dpihelper.h"
#include "ccl/platform/win/gui/textbox.win.h"
#include "ccl/platform/win/gui/menu.win.h"
#include "ccl/platform/win/gui/windowhelper.h"
#include "ccl/platform/win/gui/windowclasses.h"
#include "ccl/platform/win/gui/layeredwindowrendertarget.h"
#include "ccl/platform/win/gui/accessibility.win.h"
#include "ccl/public/cclversion.h"

#include <windowsx.h>
#include <dwmapi.h>

#pragma comment (lib, "Dwmapi.lib")

#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0
#endif

static const CCL::Configuration::BoolValue coloredTitlebarConfiguration ("CCL.Win32", "ColoredTitlebar", false);

namespace CCL {
namespace Win32 {

//////////////////////////////////////////////////////////////////////////////////////////////////

static void TranslateWindowStyle (DWORD& wstyle, DWORD& xstyle, StyleRef style)
{
	const DWORD kNoFrameStyle = WS_POPUP|WS_CLIPCHILDREN|WS_CLIPSIBLINGS;

	wstyle = kNoFrameStyle|WS_SYSMENU;
	xstyle = 0;

	if(style.isCustomStyle (Styles::kWindowBehaviorFloating) || style.isCustomStyle (Styles::kWindowBehaviorPopupSelector))
	{
		xstyle |= WS_EX_TOOLWINDOW; // A tool window does not appear in the taskbar
	}
	else
		xstyle |= WS_EX_APPWINDOW;

	if(style.isCustomStyle (Styles::kWindowBehaviorPopupSelector))
	{
		xstyle |= WS_EX_TOPMOST;
		const_cast<StyleFlags&>(style).setCustomStyle (Styles::kWindowAppearanceDropShadow);
	}

	if(style.isCustomStyle (Styles::kWindowAppearanceCustomFrame))
	{
		// clear all styles to remove window frame
		wstyle = kNoFrameStyle;
		xstyle = 0;

		const_cast<StyleFlags&>(style).setCustomStyle (Styles::kWindowAppearanceDropShadow);
	}
	else
	{
		if(style.isCustomStyle (Styles::kWindowBehaviorSizable))
			wstyle |= WS_SIZEBOX;

		if(style.isCustomStyle (Styles::kWindowAppearanceTitleBar))
			wstyle |= WS_CAPTION|WS_MINIMIZEBOX;

		if(style.isCustomStyle (Styles::kWindowBehaviorMaximizable))
			wstyle |= WS_MAXIMIZEBOX;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static void AdjustWindowSizeInPixels (Rect& size, StyleRef style, DWORD wstyle, DWORD xstyle, bool hasMenu, float contentScaleFactor)
{
	if(style.isCustomStyle (Styles::kWindowAppearanceCustomFrame))
		return;

	// does not apply content scale factor (only takes it into account)
	RECT ar;
	::SetRect (&ar, 0, 0, size.getWidth (), size.getHeight ());
	if(gDpiInfo.adjustWindowRectForDpiFactor (&ar, wstyle, hasMenu, xstyle, contentScaleFactor) == false)
		::AdjustWindowRectEx (&ar, wstyle, hasMenu, xstyle);
	size.setWidth (ar.right - ar.left);
	size.setHeight (ar.bottom - ar.top);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static void AdjustWindowSize (Rect& size, StyleRef style, DWORD wstyle, DWORD xstyle, bool hasMenu, float contentScaleFactor)
{
	DpiScale::toPixelRect (size, contentScaleFactor);
	AdjustWindowSizeInPixels (size, style, wstyle, xstyle, hasMenu, contentScaleFactor);
	DpiScale::toCoordRect (size, contentScaleFactor);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static bool CheckIsMatchingDpiAwareness (void* handle)
{
	return Win32::gDpiInfo.getCurrentDpiAwarenessContext () == Win32::gDpiInfo.getWindowDpiAwarenessContext (handle);
}

} // namespace Win32
} // namespace CCL

using namespace CCL;

//************************************************************************************************
// Win32Window
//************************************************************************************************

Win32Window* Win32Window::windowInResize = nullptr;
Rect Win32Window::pendingWindowSize;

//////////////////////////////////////////////////////////////////////////////////////////////////

Win32Window::Win32Window (const Rect& size, StyleRef style, StringRef title)
: Window (size, style, title),
  savedDpiFactor (1.f),
  inWheelEvent (false),
  fullscreenRestoreState (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Win32Window::~Win32Window ()
{
	destruct ();
	delete fullscreenRestoreState;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Win32Window::fromNativeWindow (void* nativeHandle)
{
	// init DPI
	const Win32::ScreenInformation& screen = Win32::gScreens.screenForWindowHandle (nativeHandle);
	savedDpiFactor = screen.scaleFactor;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Win32Window::makeNativePopupWindow (IWindow* parent)
{	
	ASSERT (Win32::gDpiInfo.isThreadDpiUnaware () == false) // code isn't prepared for DPI-unaware top-level windows

	// init screens for applications without main window
	if(Desktop.isWindowlessApplication () && Desktop.countWindows () == 0)
		Win32::gScreens.update ();

	HWND hwndParent = nullptr;
	if(parent)
		hwndParent = (HWND)parent->getSystemWindow ();

	// handle progress window in modal dialogs
	if(!hwndParent && style.isCustomStyle (Styles::kWindowBehaviorProgressDialog))
		if(IWindow* modalWindow = Desktop.getTopWindow (kDialogLayer))
		{
			hwndParent = (HWND)modalWindow->getSystemWindow ();
			::EnableWindow (hwndParent, FALSE); // workaround: otherwise progress window does not receive any mouse input
		}
	
	if(!hwndParent)
		if(IWindow* appWindow = Desktop.getApplicationWindow ())
			hwndParent = (HWND)appWindow->getSystemWindow ();

	// prepare for foreign views that aren't DPI-aware (Windows 10 1803 and later)
	bool dpiHostingChanged = false;
	if(style.isCustomStyle (Styles::kWindowBehaviorPluginViewHost) && Win32::gDpiInfo.canSwitchDpiHostingBehavior ())
	{
		if(Win32::gDpiInfo.switchToDpiHostingBehavior (Win32::kDpiHostingMixed))
			dpiHostingChanged = true;
	}

	const Win32::ScreenInformation& screen = Win32::gScreens.screenForCoordRect (size);
	savedDpiFactor = screen.scaleFactor;

	DWORD wstyle = 0;
	DWORD xstyle = 0;
	Win32::TranslateWindowStyle (wstyle, xstyle, style);
	const WCHAR* className = style.isCustomStyle (Styles::kWindowAppearanceDropShadow) ? Win32::kShadowWindowClass : Win32::kDefaultWindowClass;

	Rect r (size);
	Win32::AdjustWindowSize (r, style, wstyle, xstyle, hasVisibleMenuBar (), getContentScaleFactor ());
	limitSizeToScreen (r);
	moveWindowRectInsideScreen (r);
	screen.toPixelRect (r);

	handle = ::CreateWindowEx (xstyle, className, StringChars (title), wstyle,
							   r.left, r.top, r.right - r.left, r.bottom - r.top,
							   hwndParent, nullptr, g_hMainInstance,
							   this);
	setWindowTitle (title);

	if(dpiHostingChanged) // switch back to default
		Win32::gDpiInfo.switchToDpiHostingBehavior (Win32::kDpiHostingDefault);

	TransparentWindow::create (this, TransparentWindow::kKeepOnTop);

	Win32::DropTarget* dropTarget = NEW Win32::DropTarget (this);
	::RegisterDragDrop ((HWND)handle, dropTarget); // calls addRef
	dropTarget->release ();

	Win32::TouchHelper::prepareWindow (*this);

	if(auto* topModal = ccl_cast<Dialog> (Desktop.getTopWindow (kDialogLayer))) // note: kDialogLayer can also contain non-dialog windows, e.g. a progress "dialog"
		if(getTitle () != CCL_SPY_NAME			
			&& !style.isCustomStyle (Styles::kWindowBehaviorPopupSelector)) // NonModalPopupSelectorWindow must not be disabled
		{
			// disable window if a modal dialog is open (see Win32Dialog::beginModalMode)
			::EnableWindow ((HWND)handle, FALSE);
			::SendMessage ((HWND)handle, WM_NCACTIVATE, FALSE, 0);
		}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Win32Window::makeNativeChildWindow (void* nativeParent)
{
	// init with parent DPI
	const Win32::ScreenInformation& screen = Win32::gScreens.screenForWindowHandle (nativeParent);
	savedDpiFactor = screen.scaleFactor;

	ASSERT (nativeParent != nullptr)

	DWORD xstyle = 0;
	DWORD wstyle = WS_CHILD|WS_CLIPSIBLINGS|WS_CLIPCHILDREN;

	RECT r = {0};
	if(Win32::gDpiInfo.isThreadDpiUnaware ())
	{
		// Child window will be created DPI-unaware, i.e. DPI virtualization is active
		// and logical coordinates are based on 96 DPI, not physical pixels
		Win32::GdiInterop::toSystemRect (r, size);
	}
	else
	{
		PixelRect sizeInPixel (size, getContentScaleFactor ());
		Win32::GdiInterop::toSystemRect (r, sizeInPixel);
	}

	handle = ::CreateWindowEx (xstyle, Win32::kDefaultWindowClass, StringChars (title), wstyle,
							   r.left, r.top, r.right - r.left, r.bottom - r.top,
							   (HWND)nativeParent, nullptr, g_hMainInstance,
							   this);
	ASSERT (handle != nullptr)

	Win32::DropTarget* dropTarget = NEW Win32::DropTarget (this);
	::RegisterDragDrop ((HWND)handle, dropTarget); // calls addRef
	dropTarget->release ();

	Win32::TouchHelper::prepareWindow (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Win32Window::showPlatformInformation ()
{
	#if DEBUG
	Debugger::println ("*** Window Information ***");
	
	Win32::DpiAwarenessContext threadContext = Win32::gDpiInfo.getCurrentDpiAwarenessContext ();
	bool isDpiVirtualized = threadContext == Win32::kDpiContextUnaware;
	Debugger::printf ("DPI virtualization enabled: %s\n", isDpiVirtualized ? "YES" : "NO");

	Win32::DpiAwarenessContext windowContext = Win32::gDpiInfo.getWindowDpiAwarenessContext (handle);
	Debugger::printf ("Windows DPI aware: %s\n", windowContext == Win32::kDpiContextDefault ? "YES" : "NO");

	auto dumpRectangles = [&]
	{
		RECT wr = {0};
		::GetWindowRect ((HWND)handle, &wr);
		RECT cr = {0};
		::GetClientRect ((HWND)handle, &cr);

		Rect windowRect;
		Win32::GdiInterop::fromSystemRect (windowRect, wr);
		dumpRect (windowRect, "Window rect");

		Rect clientRect;
		Win32::GdiInterop::fromSystemRect (clientRect, cr);
		dumpRect (clientRect, "Client rect");
	};

	dumpRectangles ();

	#if 1
	isDpiVirtualized = !isDpiVirtualized;
	Win32::gDpiInfo.switchToDpiAwarenessContext (isDpiVirtualized ? Win32::kDpiContextUnaware : Win32::kDpiHostingDefault);
	Debugger::printf ("DPI virtualization enabled: %s\n", isDpiVirtualized ? "YES" : "NO");

	dumpRectangles ();
	#endif

	Win32::gDpiInfo.switchToDpiAwarenessContext (threadContext);
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Win32Window::sendNCActivate ()
{
	HWND hwndToplevel = Win32::FindTopLevelWindow ((HWND)handle, true);
	if(hwndToplevel)
		SendMessage (hwndToplevel, WM_NCACTIVATE, 1, 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Win32Window::captureMouse (bool state)
{
	if(state)
	{
		::SetCapture ((HWND)handle);
	}
	else
	{
		if(::GetCapture () == (HWND)handle)
			::ReleaseCapture ();
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Win32Window::suspendParent (bool state, void*& protectedData)
{
	HWND parent = (HWND)getSystemWindow ();
	if(state)
	{		
		HWND child = ::GetWindow (parent, GW_CHILD);
		if(child)
		{
			protectedData = child;
			::SetParent (child, nullptr);	
		}
	}
	else 
	{
		if(protectedData)
			::SetParent ((HWND)protectedData, parent);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float CCL_API Win32Window::getContentScaleFactor () const
{
	return savedDpiFactor;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CCL::Win32Window::setContentScaleFactor (float factor)
{
	if(factor == savedDpiFactor)
		return true;

	Rect newPixelRect (size);
	DpiScale::toPixelRect (newPixelRect, factor);
	onDpiChanged (factor, newPixelRect);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Win32Window::onDpiChanged (float dpiFactor, RectRef newPixelRect, bool suppressAdjustment)
{
	// @@DPI_AWARENESS_CONTEXT: make sure calculations happen in physical pixels!
	Win32::DpiAwarenessScope dpiScope (Win32::gDpiInfo, Win32::kDpiContextDefault);

	if(dpiFactor != savedDpiFactor)
	{
		savedDpiFactor = dpiFactor;

		if(suppressAdjustment == false)
		{
			// do some corrections to the new rectangle suggested by Windows
			Rect newSize (size);

			if(windowInResize == this) // currently in setWindowSize(), pick up the new size
				newSize = pendingWindowSize;

			// we need pixels for SetWindowPos, so use the pixel variant of AdjustWindowSize (avoid rounding up in DpiScale::pixelToCoord when Windows tells a pixel size that is not a multiple of dpiFactor)
			DpiScale::toPixelRect (newSize, dpiFactor);

			DWORD wstyle = ::GetWindowLong ((HWND)handle, GWL_STYLE);
			DWORD xstyle = ::GetWindowLong ((HWND)handle, GWL_EXSTYLE);
			Win32::AdjustWindowSizeInPixels (newSize, style, wstyle, xstyle, hasVisibleMenuBar (), dpiFactor);

			newSize.moveTo (newPixelRect.getLeftTop ());		

			::SetWindowPos ((HWND)handle, nullptr, newSize.left, newSize.top, newSize.getWidth (), newSize.getHeight (), SWP_NOACTIVATE|SWP_NOZORDER);
		}
		else
		{
			// caller has calculated an explicit new rectangle already, don't do any further adjustments
			::SetWindowPos ((HWND)handle, nullptr, newPixelRect.left, newPixelRect.top, newPixelRect.getWidth (), newPixelRect.getHeight (), SWP_NOACTIVATE|SWP_NOZORDER);
		}
		
		onDisplayPropertiesChanged (DisplayChangedEvent (savedDpiFactor, DisplayChangedEvent::kResolutionChanged));

		updateSize ();
			
		invalidate ();
		redraw ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Point& Win32Window::screenPixelToClientCoord (Point& pos) const
{
	// @@DPI_AWARENESS_CONTEXT: make sure calculations happen in physical pixels!
	Win32::DpiAwarenessScope dpiScope (Win32::gDpiInfo, Win32::kDpiContextDefault);

	POINT p = {pos.x, pos.y};
	::ScreenToClient ((HWND)handle, &p);
	pos (p.x, p.y);

	DpiScale::toCoordPoint (pos, getContentScaleFactor ());
	return pos;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PointF& Win32Window::screenPixelToClientCoord (PointF& pos) const
{
	// separate integer and fractional part
	Point posInt ((int)pos.x, (int)pos.y);
	PointF fraction (pos - pointIntToF (posInt));

	// @@DPI_AWARENESS_CONTEXT: make sure calculations happen in physical pixels!
	Win32::DpiAwarenessScope dpiScope (Win32::gDpiInfo, Win32::kDpiContextDefault);

	// translate integer part, then add fractional part
	POINT p = {posInt.x, posInt.y};
	::ScreenToClient ((HWND)handle, &p);
	pos (p.x + fraction.x, p.y + fraction.y);

	DpiScale::toCoordPointF (pos, getContentScaleFactor ());
	return pos;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Win32Window::updateMenuBar ()
{
	disableSizeMode (true);
	Rect r (getSize ());

	WindowsMenuBar* windowsMenuBar = ccl_cast<WindowsMenuBar> (menuBar);
	::SetMenu ((HWND)handle, windowsMenuBar ? windowsMenuBar->getHandle () : nullptr);
	::DrawMenuBar ((HWND)handle);

	setSize (r);
	disableSizeMode (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Win32Window::hasVisibleMenuBar () const
{
	return menuBar && menuBar->countMenus () > 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Win32Window::updateBackgroundColor ()
{
	if(!handle)
		return;

	if(coloredTitlebarConfiguration)
	{		
		const VisualStyle* backgroundStyle = this->visualStyle;
		if(backgroundStyle == nullptr) // might be too early
			backgroundStyle = getTheme ().getStandardStyle (ThemePainter::kBackgroundRenderer);
		
		ASSERT (backgroundStyle) 
		if(backgroundStyle)
		{
			// Supported starting with Windows 11
			Color backColor = backgroundStyle->getBackColor ();
			COLORREF captionColor = Win32::GdiInterop::toSystemColor (backColor);
			::DwmSetWindowAttribute ((HWND)handle, DWMWA_CAPTION_COLOR, &captionColor, sizeof(captionColor));
		}
	}

	setLayeredRenderTarget (needsLayeredRenderTarget ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Win32Window::needsLayeredMode () const
{
	return getOpacity () < 1.f || needsLayeredRenderTarget ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Win32Window::needsLayeredRenderTarget () const
{
	// Suppress layered render target when the only purpose is rounded window corners
	// and this can be handled by the OS much more efficiently
	if(style.isCustomStyle (Styles::kWindowAppearanceCustomFrame) && 
	   style.isCustomStyle (Styles::kWindowAppearanceRoundedCorners) &&
	   GUI.isRoundedWindowCornersSupported ())
		return false;

	return shouldBeTranslucent ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Win32Window::hasLayeredRenderTarget () const
{
	return ccl_cast<Win32::LayeredWindowRenderTarget> (renderTarget) != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Win32Window::setLayeredRenderTarget (bool state)
{
	if(state != hasLayeredRenderTarget ())
	{
		safe_release (renderTarget);
		if(state)
			renderTarget = NEW Win32::LayeredWindowRenderTarget (*this);
			// otherwise default target will be created in getRenderTarget

		setLayeredMode (needsLayeredMode ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Win32Window::setLayeredMode (bool state)
{
	DWORD xstyle = ::GetWindowLong ((HWND)handle, GWL_EXSTYLE);

	if(state)
		::SetWindowLong ((HWND)handle, GWL_EXSTYLE, xstyle | WS_EX_LAYERED);
	else
		::SetWindowLong ((HWND)handle, GWL_EXSTYLE, xstyle & ~WS_EX_LAYERED);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Win32Window::setWindowSize (Rect& size)
{
	if(!handle)
		return;

	// @@DPI_AWARENESS_CONTEXT: make sure calculations happen in physical pixels!
	Win32::DpiAwarenessScope dpiScope (Win32::gDpiInfo, Win32::kDpiContextDefault);

	DWORD wstyle = ::GetWindowLong ((HWND)handle, GWL_STYLE);
	Rect newSize (size);

	if(wstyle & WS_CHILD)
	{
		PixelRect sizeInPixel (newSize, getContentScaleFactor ());

		#if (0 && DEBUG)//DEBUG_LOG
		Debugger::printf ("Child window set size l = %d t = %d w = %d h = %d\n",
					sizeInPixel.left, sizeInPixel.top, sizeInPixel.getWidth (), sizeInPixel.getHeight ());
		#endif

		::SetWindowPos ((HWND)handle, nullptr, sizeInPixel.left, sizeInPixel.top, sizeInPixel.getWidth (), sizeInPixel.getHeight (), SWP_NOCOPYBITS);
	}
	else
	{
		DWORD xstyle = ::GetWindowLong ((HWND)handle, GWL_EXSTYLE);
		Win32::AdjustWindowSize (newSize, style, wstyle, xstyle, hasVisibleMenuBar (), getContentScaleFactor ());

		Rect unlimited (newSize);
		if(!isSizeModeDisabled ()) // when autosizing to childs, limiting our size would break the attachment relationship (parts of the child could be clipped)
			if(!isMaximized () && !fullscreenRestoreState)    // limitSizeToScreen would be to restrictive when maximized, since a part of the nonclient area can be outside the screen
				limitSizeToScreen (newSize);

		moveWindowRectInsideScreen (newSize);

		// communicate the size adjustment to caller
		size.right -= (unlimited.getWidth () - newSize.getWidth ());
		size.bottom -= (unlimited.getHeight () - newSize.getHeight ());

		// quick fix: SetWindowPos() might cause WM_DPICHANGED before requested window size is updated!
		ScopedVar<Win32Window*> scope1 (windowInResize, this);
		ScopedVar<Rect> scope2 (pendingWindowSize, size);

		Win32::gScreens->toPixelRect (newSize);
		::SetWindowPos ((HWND)handle, nullptr, newSize.left, newSize.top, newSize.getWidth (), newSize.getHeight (), SWP_NOACTIVATE | SWP_NOZORDER);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Win32Window::moveWindow (PointRef _pos)
{
	// TODO: @@DPI_AWARENESS_CONTEXT???
	ASSERT (Win32::CheckIsMatchingDpiAwareness (handle))

	Point pos (_pos);
	Win32::gScreens->toPixelPoint (pos);
	::SetWindowPos ((HWND)handle, nullptr, pos.x, pos.y, 0, 0, SWP_NOSIZE|SWP_NOACTIVATE|SWP_NOZORDER);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Win32Window::setWindowTitle (StringRef title)
{
	::SetWindowText ((HWND)handle, StringChars (title));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Win32Window::setStyle (StyleRef style)
{
	StyleFlags oldStyle (this->style);
	if(style != oldStyle)
	{
		SuperClass::setStyle (style);

		DWORD wstyle = 0;
		DWORD xstyle = 0;
		Win32::TranslateWindowStyle (wstyle, xstyle, style);

		wstyle |= WS_VISIBLE;
		if(needsLayeredMode ())
			xstyle |= WS_EX_LAYERED;

		Rect oldSize (getSize ());
		::SetWindowLong ((HWND)handle, GWL_STYLE, wstyle);
		::SetWindowLong ((HWND)handle, GWL_EXSTYLE, xstyle);

		// frame may have changed, notify the system about it
		::SetWindowPos ((HWND)handle, nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED|SWP_NOMOVE|SWP_NOSIZE|SWP_NOOWNERZORDER|SWP_NOACTIVATE);

		// but our client area should keep it's size
		if(collectResize ())
			resizeDeferred (true);
		else
			setWindowSize (oldSize);

		// update top-most option
		if(oldStyle.isCustomStyle (Styles::kWindowBehaviorTopMost) != style.isCustomStyle (Styles::kWindowBehaviorTopMost))
			Win32::SetAlwaysOnTop ((HWND)handle, style.isCustomStyle (Styles::kWindowBehaviorTopMost));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Win32Window::invalidate (RectRef rect)
{
	if(!handle)
		return;

	ASSERT (inDrawEvent == false)

	// @@DPI_AWARENESS_CONTEXT: make sure calculations happen in physical pixels!
	Win32::DpiAwarenessScope dpiScope (Win32::gDpiInfo, Win32::kDpiContextDefault);

	RECT r;
	PixelRect pixelRect (rect, getContentScaleFactor ());
	Win32::GdiInterop::toSystemRect (r, pixelRect);
	::InvalidateRect ((HWND)handle, &r, FALSE);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Win32Window::showWindow (bool state)
{
	if(state)
	{
		::ShowWindow ((HWND)handle, SW_SHOW);
		::UpdateWindow ((HWND)handle);

		// don't automatically give focus to a child window (e.g. plug-in), it must be activated explicitely
		bool isChildWindow = GetWindowLong ((HWND)handle, GWL_STYLE) & WS_CHILD;
		if(!isChildWindow)
			::SetFocus ((HWND)handle); // for mouse wheel

		updateSize (); // seems necessary, size was not always correct here.

		// apply top-most option
		if(style.isCustomStyle (Styles::kWindowBehaviorTopMost))
			Win32::SetAlwaysOnTop ((HWND)handle, true);
	}
	else
	{
		captureMouse (false);
		::ShowWindow ((HWND)handle, SW_HIDE);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Win32Window::maximize (tbool state)
{
	::ShowWindow ((HWND)handle, state ? SW_MAXIMIZE : SW_RESTORE);
	
	if(state && getStyle ().isCustomStyle (Styles::kWindowAppearanceCustomFrame))
	{
		Rect rect;
		getClientRect (rect);
		invalidate (rect);
	}

	WindowEvent maximizeEvent (*this, state ? WindowEvent::kMaximize : WindowEvent::kUnmaximize);
	signalWindowEvent (maximizeEvent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Win32Window::isMaximized () const
{
	return (tbool)::IsZoomed ((HWND)handle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Win32Window::isMinimized () const
{
	return (tbool)::IsIconic ((HWND)handle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Win32Window::setUserSize (RectRef _size)
{
	// TODO: @@DPI_AWARENESS_CONTEXT???
	ASSERT (Win32::CheckIsMatchingDpiAwareness (handle))

	Rect size (_size);
	limitSizeToScreen (size);
	moveWindowRectInsideScreen (size);

	Win32::gScreens->toPixelRect (size);

	WINDOWPLACEMENT placement = {0};
	placement.length = sizeof(WINDOWPLACEMENT);
	placement.ptMaxPosition.x = -1;
	placement.ptMaxPosition.y = -1;
	placement.ptMinPosition.x = -1;
	placement.ptMinPosition.y = -1;
	Win32::GdiInterop::toSystemRect (placement.rcNormalPosition, size);

	if(isMaximized ())
		placement.showCmd = SW_MAXIMIZE;

	::SetWindowPlacement ((HWND)handle, &placement);

	// also set the size for returning from fullscreen
	if(fullscreenRestoreState)
		fullscreenRestoreState->size = _size;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Win32Window::getUserSize (Rect& size) const
{
	if(fullscreenRestoreState)
	{
		size = fullscreenRestoreState->size;
		return;
	}

	// TODO: @@DPI_AWARENESS_CONTEXT???
	ASSERT (Win32::CheckIsMatchingDpiAwareness (handle))

	WINDOWPLACEMENT placement;
	placement.length = sizeof(WINDOWPLACEMENT);
	::GetWindowPlacement ((HWND)handle, &placement);
	
	Win32::GdiInterop::fromSystemRect (size, placement.rcNormalPosition);
	Win32::gScreens->toCoordRect (size);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Win32Window::setFullscreen (tbool state)
{
	tbool wasFullscreen = isFullscreen ();
	if(state != wasFullscreen)
	{
		if(state)
		{
			StyleFlags windowStyle (getStyle ());
			bool maximized = isMaximized ();
			
			Rect sizeToRestore;
			if(maximized)
				getUserSize (sizeToRestore);
			else
				sizeToRestore = getSize ();

			// remember size & style flags for restoring
			ASSERT (!fullscreenRestoreState)
			fullscreenRestoreState = NEW FullscreenRestoreState;
			fullscreenRestoreState->style = windowStyle;
			fullscreenRestoreState->maximized = maximized;
			fullscreenRestoreState->size = sizeToRestore;
			
			// remove OS frame, set topmost
			windowStyle.setCustomStyle (Styles::kWindowAppearanceCustomFrame|Styles::kWindowBehaviorTopMost, true);
			windowStyle.setCustomStyle (Styles::kWindowBehaviorSizable, false);
			setStyle (windowStyle);

			// remove menu
			::SetMenu ((HWND)handle, nullptr);

			// set size to full monitor size
			Rect monitorSize;
			int monitor = Desktop.findMonitor (getSize ().getCenter (), true);
			Desktop.getMonitorSize (monitorSize, monitor, false);
			setSize (monitorSize);

			invalidate (); // fixes missing redraw (black area) with multiple stacked fullscreen windows
		}
		else
		{
			ASSERT (fullscreenRestoreState)
			if(fullscreenRestoreState)
			{
				// restore menu
				updateMenuBar ();

				// restore previous style & size
				setStyle (fullscreenRestoreState->style);

				if(fullscreenRestoreState->maximized)
				{
					maximize (true);
					setUserSize (fullscreenRestoreState->size);
				}
				else
					setSize (fullscreenRestoreState->size);
			}

			delete fullscreenRestoreState;
			fullscreenRestoreState = nullptr;
		}
		WindowEvent event (*this, state ? WindowEvent::kFullscreenEnter : WindowEvent::kFullscreenLeave);
		signalWindowEvent (event);
	}
	return wasFullscreen;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Win32Window::isFullscreen () const
{
	return fullscreenRestoreState != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Win32Window::isVisible () const
{
	return (tbool)::IsWindowVisible ((HWND)handle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Win32Window::center ()
{
	if(isMaximized ())
		return;

	// TODO: @@DPI_AWARENESS_CONTEXT???
	ASSERT (Win32::CheckIsMatchingDpiAwareness (handle))

	if(layer == kDialogLayer)
	{
		if(auto appWindow = unknown_cast<Window> (Desktop.getApplicationWindow ()))
		{
			Rect r;
			Point p;
			appWindow->getFrameSize (r);
			appWindow->getPosition (p);
			r.moveTo (p);

			Rect r2 (0, 0, getWidth (), getHeight ());
			r2.center (r);
			moveWindow (r2.getLeftTop ());
			return;
		}
	}

	Rect r (Win32::gScreens.getPrimaryScreen ().pixelWorkArea);
	PixelPoint sizeInPixel (Point (getWidth (), getHeight ()), getContentScaleFactor ());

	::SetWindowPos ((HWND)handle, nullptr,
					r.getWidth ()/2 - sizeInPixel.x/2,
					r.getHeight ()/2 - sizeInPixel.y/2,
					0, 0,
					SWP_NOSIZE|SWP_NOZORDER);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Win32Window::redraw ()
{
	::UpdateWindow ((HWND)handle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Win32Window::activate ()
{
	// could be called from PopupSelector::doPopup -> Window::setFocusView before popupSelector is open
	// calling SetActiveWindow (0) could then result in WM_ACTIVATEAPP (false)!
	if(!handle)
		return;

	if(!hasBeenDrawn () && Desktop.hasFullScreenWindow ())
	{
		// this fixes an issue with windows that were invisible after being opened on top of a fullscreen window
		// (previous EnforceWindowOrder didn't handle the new window because it was not added to the Desktop yet)
		invalidate ();
		Win32::EnforceWindowOrder ();
	}

	::SetActiveWindow ((HWND)handle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Win32Window::isActive () const
{
	return ::GetForegroundWindow () == (HWND)handle;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Win32Window::isEnabled () const
{
	return SuperClass::isEnabled () && ::IsWindowEnabled ((HWND)handle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Win32Window::close ()
{
	return ::SendMessage ((HWND)handle, WM_CLOSE, 0, 0) == 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Win32Window::updateSize ()
{
	if(collectResize ())
		return;
	
	if(!handle)
		return;

	if(::IsIconic ((HWND)handle)) // window size would be empty if minimized!
		return;

	// @@DPI_AWARENESS_CONTEXT: make sure calculations happen in physical pixels!
	Win32::DpiAwarenessScope dpiScope (Win32::gDpiInfo, Win32::kDpiContextDefault);

	RECT wr;
	::GetWindowRect ((HWND)handle, &wr);

	RECT cr;
	::GetClientRect ((HWND)handle, &cr);

	Point pos (wr.left, wr.top);
	Win32::gScreens->toCoordPoint (pos);

	Rect r (0, 0, cr.right - cr.left, cr.bottom - cr.top);
	DpiScale::toCoordRect (r, getContentScaleFactor ());
	r.offset (pos);

	View::setSize (r);

	// inform render target
	if(NativeWindowRenderTarget* t = getRenderTarget ())
		t->onSize ();

	// commit pending layer changes
	if(graphicsLayer)
		graphicsLayer->flush ();

	Win32::EnforceWindowOrder ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Win32Window::getFrameSize (Rect& size) const
{
	// @@DPI_AWARENESS_CONTEXT: make sure calculations happen in physical pixels!
	Win32::DpiAwarenessScope dpiScope (Win32::gDpiInfo, Win32::kDpiContextDefault);

	RECT wr;
	::GetWindowRect ((HWND)handle, &wr);

	Point pos (wr.left, wr.top);
	Win32::gScreens->toCoordPoint (pos);

	size (0, 0, wr.right - wr.left, wr.bottom - wr.top);
	DpiScale::toCoordRect (size, getContentScaleFactor ());
	size.offset (pos);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Win32Window::moveWindow ()
{
	auto adjustRestoredWindowRect = [&] (Rect& size)
	{
		// determine mouse pos relative to client
		Point oldMousePos;
		GUI.getMousePosition (oldMousePos);
		screenToClient (oldMousePos);

		// adjust mouse pos to stay in the (likely shrinked) window
		constexpr Coord kMargin = 50;
		Point newMousePos (oldMousePos);
		ccl_upper_limit (newMousePos.x, size.getWidth () - kMargin);
		ccl_upper_limit (newMousePos.y, size.getHeight () - kMargin);
		ccl_lower_limit (newMousePos.x, 0);
		ccl_lower_limit (newMousePos.y, 0);

		Point oldWindowPos (getSize ().getLeftTop ());
		size.moveTo (oldWindowPos + (oldMousePos - newMousePos));
	};

	if(isFullscreen ())
	{
		// adjust window size to be restored
		adjustRestoredWindowRect (fullscreenRestoreState->size);

		// leave fullscreen
		setFullscreen (false);
	}
	else if(isMaximized ())
	{
		// adjust user size to be restored
		Rect userSize;
		getUserSize (userSize);
		adjustRestoredWindowRect (userSize);
		setUserSize (userSize);

		// leave maximized state
		maximize (false);
	}

	POINT p;
	::GetCursorPos (&p);
	LPARAM lParam = p.x | (p.y << 16);
	ScopedVar<bool> scope (inMoveLoop, true);
	::SendMessage ((HWND)handle, WM_SYSCOMMAND, SC_MOVE|HTCAPTION, lParam);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Win32Window::resizeWindow (int edge)
{
	POINT p;
	::GetCursorPos (&p);
	LPARAM lParam = p.x | (p.y << 16);	
	switch(edge)
	{
	case kEdgeBottomRight:
		::SendMessage ((HWND)handle, WM_SYSCOMMAND, SC_SIZE|WMSZ_BOTTOMRIGHT, lParam);
		break;
	case kEdgeLeft:
		::SendMessage ((HWND)handle, WM_SYSCOMMAND, SC_SIZE|WMSZ_LEFT, lParam);
		break;
	case kEdgeRight:
		::SendMessage ((HWND)handle, WM_SYSCOMMAND, SC_SIZE|WMSZ_RIGHT, lParam);
		break;
	case kEdgeTop:
		::SendMessage ((HWND)handle, WM_SYSCOMMAND, SC_SIZE|WMSZ_TOP, lParam);
		break;
	case kEdgeBottom:
		::SendMessage ((HWND)handle, WM_SYSCOMMAND, SC_SIZE|WMSZ_BOTTOM, lParam);
		break;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Point& CCL_API Win32Window::clientToScreen (Point& pos) const
{
	// @@DPI_AWARENESS_CONTEXT: make sure calculations happen in physical pixels!
	Win32::DpiAwarenessScope dpiScope (Win32::gDpiInfo, Win32::kDpiContextDefault);

	// translate origin of window client to screen (pos can be outside of window, and even on another monitor with different scale factor!)
	POINT p = {0, 0};
	::ClientToScreen ((HWND)handle, &p);
	Point origin (p.x, p.y);

	Win32::gScreens->toCoordPoint (origin);

	pos += origin;
	return pos;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Point& CCL_API Win32Window::screenToClient (Point& pos) const
{
	Win32::gScreens->toPixelPoint (pos);

	return screenPixelToClientCoord (pos);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Win32Window::setOpacity (float _opacity)
{
	_opacity = ccl_bound<float> (_opacity, 0.f, 1.f);
	if(opacity != _opacity)
	{
		opacity = _opacity;
	
		bool layered = needsLayeredMode ();
		setLayeredMode (layered);

		if(layered)
			::SetLayeredWindowAttributes ((HWND)handle, 0, (BYTE)(opacity * 255.f), LWA_ALPHA);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Win32Window::scrollClient (RectRef rect, PointRef delta)
{
	if(collectUpdates)
	{
		// don't scroll, just invalidate
		Rect r (rect);
		r.offset (delta);
		r.join (rect);
		invalidate (r);
		return;
	}

	CCL_PRINTF ("scroll %d %d -> ", delta.x, delta.y)
	CCL_PRINTF ("%d %d %d %d\n", rect.left, rect.top, rect.getWidth (), rect.getHeight ())
	ASSERT (delta.x < getWidth () && delta.y < getHeight ())

	if(NativeWindowRenderTarget* target = getRenderTarget ())
	{
		Rect r (rect);

		// avoid artifacts when the source rect touches the last pixel line in the window: invalidate that line instead
		// (only seen this for vertical scrolling in dialogs, with both D2DWindowRenderTarget & LayeredWindowRenderTarget)
		if(delta.y < 0)
		{
			Coord outside = r.bottom - (getHeight () - 1);
			if(outside > 0)
			{
				r.bottom -= outside;
				invalidate (Rect (r.left, r.bottom, rect.right, getHeight ()));
			}
		}
		else if(delta.y > 0)
		{
			if(r.top <= 0)
			{
				r.top = 1;
				invalidate (Rect (r.left, 0, r.right, 1));
			}
		}

		target->onScroll (r, delta);
	}
	else
	{
		RECT r = {};
		Win32::GdiInterop::toSystemRect (r, rect);
		ASSERT (getContentScaleFactor () == 1.f)
		::ScrollWindowEx ((HWND)handle, delta.x, delta.y, &r, nullptr, NULL, nullptr, 0);
		finishScroll (rect, delta);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EventResult Win32Window::handleEvent (SystemEvent& e)
{
	HWND hwnd = (HWND)e.hwnd;
	WPARAM wParam = (WPARAM)e.wParam;
	LPARAM lParam = (LPARAM)e.lParam;

	static bool ignoreSCKeyMenu = false;
	static bool handledAltWheel = false;
	static LCID oldInputLanguage = LOWORD (::GetKeyboardLayout (0));

	#if DEBUG_EVENTS
	if(e.msg != WM_ENTERIDLE && ccl_cast<DEBUG_EVENTS_CLASS> (this))
	{
		static int counter = 0;
		CCL_PRINTF ("MSG %05d 0x%03x W %d L %d\n", counter++, e.msg, (int)wParam, (int)lParam)
	}
	#endif

	switch(e.msg)
	{
	case WM_PAINT :
		{
			ASSERT (inDrawEvent == false)
			if(inDrawEvent)
				return nullptr;
			
			//CCL_PROFILE_START(Paint)
			ScopedVar<bool> inDrawEventScope (inDrawEvent, true);
			if(NativeWindowRenderTarget* t = getRenderTarget ())
				t->onRender ();
			//CCL_PROFILE_STOP(Paint)
		}
		return nullptr;

	//case WM_CTLCOLORSTATIC :
	case WM_CTLCOLOREDIT :
		{
			WindowsTextControl* edit = WindowsTextControl::fromHWND ((void*)lParam);
			if(edit && edit->getBrush ())
			{
				::SetTextColor ((HDC)wParam, edit->getColor ());
				//::SetBkMode ((HDC)wParam, TRANSPARENT); does not work with multiline controls 
				::SetBkColor  ((HDC)wParam, edit->getBackColor ());
				return (EventResult)edit->getBrush ();
			}
		}
		break;

	case WM_NOTIFY :
		return nullptr;

	case WM_MBUTTONDOWN :
	case WM_LBUTTONDOWN :
	case WM_RBUTTONDOWN :
		{
			Point p (GET_X_LPARAM (lParam), GET_Y_LPARAM (lParam));
			if(Win32::TouchHelper::didHandleButtonMessage (*this, p))
				return nullptr;

			MouseEvent event (MouseEvent::kMouseDown, p, 0, System::GetProfileTime ());
			DpiScale::toCoordPoint (event.where, getContentScaleFactor ());
			VKey::fromSystemModifiers (event.keys, (uint32)wParam);
			::SetFocus ((HWND)handle); // a (foreign) child window might have the focus
			if(onMouseDown (event))
				return nullptr;
		}
		break;

	case WM_MOUSEMOVE :
	{	
		Point p (GET_X_LPARAM (lParam), GET_Y_LPARAM (lParam));
		if(Win32::TouchHelper::didHandleButtonMessage (*this, p))
			return nullptr;

		MSG msg;
		if(mouseHandler != nullptr && PeekMessage (&msg, nullptr, WM_LBUTTONUP, WM_LBUTTONUP, PM_REMOVE))
		{
			CCL_PRINTF ("Button M up\n")

			MouseEvent event (MouseEvent::kMouseUp, p, 0, System::GetProfileTime ());
			DpiScale::toCoordPoint (event.where, getContentScaleFactor ());
			VKey::fromSystemModifiers (event.keys, (uint32)wParam);
			onMouseUp (event);
			return nullptr;
		}
		else
		{
			while(PeekMessage (&msg, nullptr, WM_MOUSEMOVE, WM_MOUSEMOVE, PM_REMOVE))
			{
				lParam = msg.lParam;
				wParam = msg.wParam;
			}

			MouseEvent event (MouseEvent::kMouseMove, Point (GET_X_LPARAM (lParam), GET_Y_LPARAM (lParam)), 0, System::GetProfileTime ());
			DpiScale::toCoordPoint (event.where, getContentScaleFactor ());
			VKey::fromSystemModifiers (event.keys, (uint32)wParam);
			onMouseMove (event);
			return nullptr;
		}
	}

	case WM_LBUTTONUP :
	case WM_MBUTTONUP :
	case WM_RBUTTONUP :
		{
			Point p (GET_X_LPARAM (lParam), GET_Y_LPARAM (lParam));
			if(Win32::TouchHelper::didHandleButtonMessage (*this, p))
				return nullptr;

			CCL_PRINTF ("Button W up\n")

			MouseEvent event (MouseEvent::kMouseUp, p, 0, System::GetProfileTime ());
			DpiScale::toCoordPoint (event.where, getContentScaleFactor ());
			VKey::fromSystemModifiers (event.keys, (uint32)wParam);

			// in mouse-up the wParam does not tell which mouse button is up (because it is not down anymore)
			// since the onMouseUp/MouseEvent has no specific information about the mouse button, the only way is to set the key manually
			switch(e.msg)
			{
			case WM_LBUTTONUP : event.keys.keys |= KeyState::kLButton; break;
			case WM_MBUTTONUP : event.keys.keys |= KeyState::kMButton; break;
			case WM_RBUTTONUP : event.keys.keys |= KeyState::kRButton; break;	
			}

			if(onMouseUp (event))
				return nullptr;
		}
		break;

	case WM_XBUTTONDOWN :
	case WM_XBUTTONUP :
		{
			// TODO: implement me!
		}
		break;

	case WM_CONTEXTMENU :
		{
			Point where (GET_X_LPARAM (lParam), GET_Y_LPARAM (lParam)); // in screen coordinates
			bool wasKeyPressed = where.x == -1 && where.y == -1;
			if(!wasKeyPressed)
				screenPixelToClientCoord (where);

			popupContextMenu (where, wasKeyPressed);
		}
		return nullptr;

	case WM_HELP :
		{
			 HELPINFO* hi = (HELPINFO*)lParam;
			 if(hi->iContextType == HELPINFO_MENUITEM)
			 {
	 			Menu* menu = WindowsPopupMenu::fromSystemMenu (hi->hItemHandle);
				MenuItemID id = hi->iCtrlId;
				MenuItem* item = menu ? menu->findItem (id) : nullptr;
				if(item)
					HelpManager::instance ().showContextHelp (item->asUnknown ());
			 }
		}
		return (EventResult)TRUE;

	case WM_SETCURSOR :
		if(LOWORD (lParam) == HTCLIENT)
		{
			// ignore request for foreign (child) windows; note: both CCL windows in the parent chain (ChildWindow and toplevel window) receive the message for a foreign plug-in child window
			HWND cursorWindow = (HWND)wParam;
			if(cursorWindow != (HWND)handle && !Win32::GetWindowFromNativeHandle (cursorWindow))
				break;

			GUI.updateCursor ();
			return (EventResult)TRUE;
		}
		break;

	case WM_MOUSEWHEEL :
	case WM_MOUSEHWHEEL :
		if(!inWheelEvent)
		{
			CCL_PRINTF ("mouse wheel in %s\n", myClass ().getPersistentName ());
			ScopedVar<bool> scope (inWheelEvent, true);
			
			short delta = HIWORD (wParam);
			int type = 0;
			if(e.msg == WM_MOUSEWHEEL)
				type = delta > 0 ? MouseWheelEvent::kWheelUp : MouseWheelEvent::kWheelDown;
			else
			{
				type = delta > 0 ? MouseWheelEvent::kWheelRight : MouseWheelEvent::kWheelLeft;
				delta = -delta;
			}
			
			Point location (GET_X_LPARAM (lParam), GET_Y_LPARAM (lParam)); // in screen coordinates
			Win32::gScreens->toCoordPoint (location);

			Window* windowUnderMouse = unknown_cast<Window> (Desktop.findWindow (location));
			if(windowUnderMouse == nullptr)
				return nullptr;

			Window* dialog = Desktop.getTopWindow (kDialogLayer);
			if(dialog && dialog != windowUnderMouse && ccl_cast<Dialog> (dialog))
				return nullptr;

			MouseWheelEvent event (type, windowUnderMouse->screenToClient (location));
			VKey::fromSystemModifiers (event.keys, LOWORD (wParam));
			event.delta = (float)delta / (float)WHEEL_DELTA;

			// toggle axis
			if(event.keys.isSet (KeyState::kShift))
			{
				event.eventType = (event.eventType + 2) % 4;
				event.keys.keys &= ~KeyState::kShift;
				event.wheelFlags |= MouseWheelEvent::kAxisToggled;
			}
			
			bool handled = windowUnderMouse->onMouseWheel (event);

			if(handled && event.keys.isSet (KeyState::kOption))
				handledAltWheel = true;
		}
		return nullptr;

	case WM_CAPTURECHANGED :
		if((HWND)lParam != (HWND)handle) // cancel mouse handler
			setMouseHandler (nullptr);
		return nullptr;

	case WM_GESTURE : // Windows 7 or later
	case WM_GESTURENOTIFY :
		if(Win32::TouchHelper::processGestureEvent (*this, e))
			return nullptr;
		break;

	case WM_POINTERDOWN :
	case WM_POINTERUPDATE :
	case WM_POINTERUP :
	case WM_POINTERENTER:
	case WM_POINTERLEAVE:
		if(Win32::TouchHelper::processPointerEvent (*this, e))
			return nullptr;
		break;

	case WM_SETFOCUS :
		{
			CCL_PRINTF ("WM_SETFOCUS %s %s\n", myClass ().getPersistentName (), MutableCString (getTitle ()).str ())
			HWND hwndLostFocus = (HWND)wParam;
			if(!hwndLostFocus || !::IsChild (hwnd, hwndLostFocus))
				onFocus (FocusEvent::kSetFocus);
		}
		break;

	case WM_KILLFOCUS :
		{
			CCL_PRINTF ("WM_KILLFOCUS %s %s\n", myClass ().getPersistentName (), MutableCString (getTitle ()).str ())
			HWND hwndFocus = (HWND)wParam;
			if(!hwndFocus || !::IsChild (hwnd, hwndFocus))
				onFocus (FocusEvent::kKillFocus);
		}
		break;

	case WM_KEYDOWN :
	case WM_SYSKEYDOWN :
	case WM_KEYUP :
	case WM_SYSKEYUP :
		{
			ignoreSCKeyMenu = false;

			KeyEvent key;
			VKey::fromSystemEvent (key, e);

			if(handledAltWheel && key.eventType == KeyEvent::kKeyUp && key.vKey == VKey::kOption)
			{
				// Alt-Key released after it was involved in a handled mousewheel message: ignore a following SC_KEYMENU
				ignoreSCKeyMenu = true;
				handledAltWheel = false;
			}

			static bool capsLockDownHandled = false;
			enum { kSimulatedCapsLockEvent = 666 };

			// ignore CapsLock events sent by ourselves
			if(key.vKey == VKey::kCapsLock && ::GetMessageExtraInfo () == kSimulatedCapsLockEvent)
				return nullptr;

			bool result = (key.eventType == KeyEvent::kKeyDown) ? onKeyDown (key) : onKeyUp (key);

			auto isDeadKey = [] (const KeyEvent& key)
			{
				return key.vKey == VKey::kCircumflex
					|| key.vKey == VKey::kAcute
					|| key.vKey == VKey::kGrave;
			};

			if(key.eventType == KeyEvent::kKeyUp && isDeadKey (key))
			{
				KeyEvent dummy;
				VKey::fromSystemEvent (dummy, e); // when a dead key is released, flush the internal state of ::ToUnicode (see keyevent.win.cpp) 
			}

			if(result)
			{
				if(key.vKey == VKey::kCapsLock && key.eventType == KeyEvent::kKeyDown)
					capsLockDownHandled = true;

				if(e.msg == WM_SYSKEYDOWN)
					ignoreSCKeyMenu = true; // we handled the key, ignore a following SC_KEYMENU that could popup a menu from the menubar
				return nullptr;
			}

			if(key.vKey == VKey::kCapsLock && capsLockDownHandled && key.eventType == KeyEvent::kKeyUp)
			{
				// CapsLock "up" event received after "down" was handled: send another pair of events (down/up) to restore the previous CapsLock state
				INPUT ip[2] = { 0 };
				ip[0].type = INPUT_KEYBOARD;
				ip[0].ki.wVk = VK_CAPITAL;
				ip[0].ki.dwExtraInfo = kSimulatedCapsLockEvent;

				ip[1].type = INPUT_KEYBOARD;
				ip[1].ki.wVk = VK_CAPITAL;
				ip[1].ki.dwFlags = KEYEVENTF_KEYUP;
				ip[1].ki.dwExtraInfo = kSimulatedCapsLockEvent;

				// the windows setting "To turn off Caps Lock, Press the Shift key" is reflected in the registry value "HKEY_CURRENT_USER\Keyboard Layout" "Attributes" = 0x00010000
				// if that flag is set, we have to simulate Shift instead of CapsLock
				uint32 value = 0;
				Registry::Accessor (Registry::kKeyCurrentUser, "Keyboard Layout").readDWORD (value, nullptr, "Attributes");
				CCL_PRINTF ("CapsLock up received: %x\n", value)
				if(value & 0x10000)
					ip[0].ki.wVk = ip[1].ki.wVk = VK_SHIFT;

				::SendInput (2, ip, sizeof(INPUT));
						
				capsLockDownHandled = false;
			}
		}
		break;

	case WM_DEADCHAR :
		CCL_PRINTF ("WM_DEADCHAR %x  %x\n", wParam, lParam)
		break;

	case WM_SYSCOMMAND :
		CCL_PRINTF ("WM_SYSCOMMAND %x\n", wParam)
		if(wParam == SC_KEYMENU && ignoreSCKeyMenu)
		{
			ignoreSCKeyMenu = false;
			return nullptr;
		}
		break;

	case WM_APPCOMMAND :
		{
			KeyEvent key;
			switch (GET_APPCOMMAND_LPARAM(lParam))
			{
			case APPCOMMAND_VOLUME_MUTE:        key.vKey = VKey::kVolumeMute; break;
			case APPCOMMAND_VOLUME_DOWN:        key.vKey = VKey::kVolumeUp; break;
			case APPCOMMAND_VOLUME_UP:          key.vKey = VKey::kVolumeDown; break;
			case APPCOMMAND_MEDIA_STOP:         key.vKey = VKey::kStop; break;
			case APPCOMMAND_MEDIA_PLAY_PAUSE:   key.vKey = VKey::kPlayPause; break;
			case APPCOMMAND_MEDIA_PAUSE:        key.vKey = VKey::kPause; break;
			case APPCOMMAND_MEDIA_RECORD:       key.vKey = VKey::kRecord; break;
			case APPCOMMAND_MEDIA_FAST_FORWARD: key.vKey = VKey::kForward; break;
			case APPCOMMAND_MEDIA_REWIND:       key.vKey = VKey::kRewind; break;
			case APPCOMMAND_MEDIA_CHANNEL_UP:   key.vKey = VKey::kChannelUp; break;
			case APPCOMMAND_MEDIA_CHANNEL_DOWN: key.vKey = VKey::kChannelDown; break;
			}
			if(key.vKey != -1)
			{
				onKeyDown (key);
				return (EventResult)true;
			}
		}
		break;

	case WM_INPUTLANGCHANGE :
		{
			LCID lcid = LOWORD (lParam);
			if(lcid != oldInputLanguage)
			{
				oldInputLanguage = lcid;
				SignalSource (Signals::kLocales).signal (Message (Signals::kInputLanguageChanged));
			}
		}
		break;

	case WM_ENTERSIZEMOVE :
		{
			// if the focus view is child of an IEditControlHost, it will be removed, so let the host view gain focus
			View* newFocus = unknown_cast<View> (GetViewInterfaceUpwards<IEditControlHost> (getFocusView ()));

			setFocusView (nullptr); // otherwise native text controls move around screen

			if(newFocus)
				saveFocusView (newFocus);

			onResizing (true);
		}
		return nullptr;

	case WM_EXITSIZEMOVE :
		onResizing (false);
		onFocus (FocusEvent::kSetFocus); // restore the focus view lost in WM_ENTERSIZEMOVE
		break;

	case WM_SIZING :
		{
			RECT* rect = reinterpret_cast<RECT*> (lParam);
			ASSERT (rect != nullptr)

			Rect clientRect (0, 0, rect->right - rect->left, rect->bottom - rect->top);
			DpiScale::toCoordRect (clientRect, savedDpiFactor);

			Rect constrained (clientRect);
			constrainSize (constrained);

			if(savedDpiFactor != 1.f || constrained != clientRect)
			{
				DpiScale::toPixelRect (constrained, savedDpiFactor);

				#if (0 && DEBUG)
				Debugger::printf ("WM_SIZING (%d) constrained pixel delta: %d, %d\n", wParam, constrained.getWidth () - (rect->right - rect->left), constrained.getHeight () - (rect->bottom - rect->top));
				#endif

				// adjust at the "touched" edges
				static const Vector<WPARAM> leftEdges ({ WMSZ_LEFT, WMSZ_TOPLEFT, WMSZ_BOTTOMLEFT });
				static const Vector<WPARAM> topEdges ({ WMSZ_TOP, WMSZ_TOPLEFT, WMSZ_TOPRIGHT});

				if(!leftEdges.contains (wParam))
					rect->right = rect->left + constrained.getWidth ();
				else
					rect->left = rect->right - constrained.getWidth ();

				if(!topEdges.contains (wParam))
					rect->bottom = rect->top + constrained.getHeight ();
				else
					rect->top = rect->bottom - constrained.getHeight ();

				return (EventResult)TRUE;
			}
		}
		break;

	case WM_SIZE :
		CCL_PRINTF ("%sWM_SIZE %d x %d\n", CCL_INDENT, LOWORD(lParam), HIWORD(lParam))

		if(wParam == SIZE_MAXIMIZED)
		{
			WindowEvent maximizeEvent (*this, WindowEvent::kMaximize);
			signalWindowEvent (maximizeEvent);
		}

		#if 0 && DEBUG_LOG
		else if(wParam == SIZE_MINIMIZED)
		{
			CCL_PRINTLN ("SIZE_MINIMIZED")
			DWORD xstyle = ::GetWindowLong (hwnd, GWL_EXSTYLE);
			if((xstyle & WS_EX_TOPMOST))
			{
				CCL_PRINTF ("WS_EX_TOPMOST for \"%s\"\n", MutableCString (title).str ())
				::SetWindowLong (hwnd, GWL_EXSTYLE, xstyle & ~WS_EX_TOPMOST); // remove WS_EX_TOPMOST does not work
				xstyle = ::GetWindowLong (hwnd, GWL_EXSTYLE); // WS_EX_TOPMOST still set
			}
		}
		#endif

		updateSize ();
		break;

	case WM_MOVE :
		CCL_PRINTF ("WM_MOVE %d\n", (int)hwnd)
		
		updateSize ();
		break;

	case WM_MOVING :
		{
			RECT* rect = (RECT*)lParam;
			CCL_PRINTF ("WM_MOVING: %d, %d (%d x %d)\n", rect->left, rect->top, rect->right - rect->left, rect->bottom - rect->top)

			// apply our size limits to the suggested rect (if our size changes during a move interaction, windows still passes the old size in following WM_MOVE messages)
			SizeLimit limits (getSizeLimits ());
			if(limits.isValid ())
			{
				limits.resolveConflicts ();

				// determine total non-client size
				RECT wr, cr;
				::GetWindowRect ((HWND)handle, &wr);
				::GetClientRect ((HWND)handle, &cr);
				Point ncSize ((wr.right - wr.left) - (cr.right - cr.left), (wr.bottom - wr.top) - (cr.bottom - cr.top));
			
				// translate size of given frame rect to client size
				Point size (rect->right - rect->left, rect->bottom - rect->top);
				size -= ncSize;

				// apply size limits
				DpiScale::toCoordPoint (size, getContentScaleFactor ());
				limits.makeValid (size);
				DpiScale::toPixelPoint (size, getContentScaleFactor ());

				// back to frame size
				size += ncSize;
				rect->right = rect->left + size.x;
				rect->bottom = rect->top + size.y;
				return (EventResult)TRUE;
			}
		}
		break;

	case WM_GETMINMAXINFO :
		{
			MINMAXINFO* m = (MINMAXINFO*)lParam;
			DWORD wstyle = GetWindowLong (hwnd, GWL_STYLE);
			DWORD xstyle = GetWindowLong (hwnd, GWL_EXSTYLE);

			getSizeLimits ();
			Rect minSize (0, 0, sizeLimits.minWidth, sizeLimits.minHeight);
			Rect maxSize (0, 0, sizeLimits.maxWidth, sizeLimits.maxHeight);
			Win32::AdjustWindowSize (minSize, style, wstyle, xstyle, hasVisibleMenuBar (), getContentScaleFactor ());

			// also apply the delta from AdjustWindowSize to maxSize
			if(sizeLimits.maxWidth < kMaxCoord)
				maxSize.right += minSize.right - sizeLimits.minWidth;
			if(sizeLimits.maxHeight < kMaxCoord)
				maxSize.bottom += minSize.bottom - sizeLimits.minHeight;

			DpiScale::toPixelRect (minSize, getContentScaleFactor ());
			DpiScale::toPixelRect (maxSize, getContentScaleFactor ());

			m->ptMinTrackSize.x = minSize.getWidth ();
			m->ptMinTrackSize.y = minSize.getHeight ();
			m->ptMaxTrackSize.x = maxSize.getWidth ();
			m->ptMaxTrackSize.y = maxSize.getHeight ();
			CCL_PRINTF ("WM_GETMINMAXINFO min %d, %d, max %d, %d\n", sizeLimits.minWidth, sizeLimits.minHeight, sizeLimits.maxWidth, sizeLimits.maxHeight)

			if(getStyle ().isCustomStyle (Styles::kWindowAppearanceCustomFrame))
			{
				// we may have to adjust the maximimized rect for borderless windows (otherwise it would hide the taskbar)

				// MSDN: For systems with multiple monitors, the ptMaxSize and ptMaxPosition members describe the maximized size
				// and position of the window on the primary monitor, even if the window ultimately maximizes onto a secondary monitor.
				// In that case, the window manager adjusts these values to compensate for differences between the primary monitor
				// and the monitor that displays the window.

				// only adjust if window is on the primary monitor
				const Win32::ScreenInformation& screen = Win32::gScreens.screenForWindowHandle (handle);
				if(Win32::gScreens.isPrimaryScreen (screen))
				{
					Rect monitorRect (screen.pixelRect);
					Rect workRect (screen.pixelWorkArea);

					m->ptMaxPosition.x = workRect.left - monitorRect.left;
					m->ptMaxPosition.y = workRect.top - monitorRect.top;
					m->ptMaxSize.x = ccl_abs (workRect.getWidth ());
					m->ptMaxSize.y = ccl_abs (workRect.getHeight ());
				}
			}
		}
		return nullptr;

	case WM_MENUCOMMAND :
		{
			Menu* menu = WindowsPopupMenu::fromSystemMenu ((HANDLE)lParam);
			int idx = (int)wParam;
			MenuItem* item = menu ? menu->at (idx) : nullptr;
			if(item)
				item->select ();
		}
		return nullptr;

	case WM_INITMENU :
		{
			MenuBar* menuBar = WindowsMenuBar::fromSystemMenu ((HMENU)wParam);
			if(menuBar)
				menuBar->init ();
		}
		return nullptr;

	case WM_ENTERMENULOOP :
	case WM_EXITMENULOOP :
		inMenuLoop = e.msg == WM_ENTERMENULOOP;
		return nullptr;

	case WM_ACTIVATE :
		CCL_PRINTF ("WM_ACTIVATE (%s) %s %s\n", wParam ? "true" : "false", myClass ().getPersistentName (), MutableCString (getTitle ()).str ())
		onActivate (wParam != WA_INACTIVE);
		Win32::EnforceWindowOrder (); // after Window::onActivate, which changes the z-order via DesktopManager::onActivateWindow 
		return nullptr;
	
	case WM_ACTIVATEAPP :
		CCL_PRINTF ("WM_ACTIVATEAPP (%s) %s\n", wParam ? "true" : "false", MutableCString (getTitle ()).str ())
		GUI.onAppStateChanged (wParam ? IApplication::kAppActivated : IApplication::kAppDeactivated);
		break;

	case WM_MOUSEACTIVATE :
		CCL_PRINTF ("WM_MOUSEACTIVATE %s %s\n", myClass ().getPersistentName (), MutableCString (getTitle ()).str ())
		onActivate (true);

		if(ccl_cast<ChildWindow> (this))
		{
			// set platform focus to ChildWindow (so it can forward key events to an IPlugInView via ChildWindowDelegate, when no foreign child window takes focus)
			// but don't steal focus from a foreign plug-in child window (ChildWindow will still receive key events via mangling in WindowsUserInterface::nextEvent)
			HWND focusWnd = ::GetFocus ();
			if(!::IsChild (hwnd, focusWnd))
				::SetFocus (hwnd);
		}
		break;
	
	case WM_MDIACTIVATE :
		onActivate (hwnd == (HWND)lParam);
		break;

	case WM_SHOWWINDOW :
		CCL_PRINTLN ("WM_SHOWWINDOW")
		if(lParam != 0 && (GetWindowLong (hwnd, GWL_STYLE) & WS_CHILD) == 0)
		{
			// pass message to child windows (needed for video)
			struct MessageRelay
			{
				static BOOL CALLBACK EnumChildProc(HWND hwnd, LPARAM lParam)
				{
					SystemEvent* e = (SystemEvent*) lParam;
					SendMessage (hwnd, WM_SHOWWINDOW, (WPARAM)e->wParam, (LPARAM)e->lParam);
					return TRUE;
				}
			};			
			EnumChildWindows (hwnd, MessageRelay::EnumChildProc,(LPARAM)&e);
		}
		if(lParam == SW_PARENTOPENING && wParam == TRUE)
		{
			// The owner window is being restored. We must manually restore our maximized state
			if(isMaximized ())
				maximize (true);
		}
		break;

	case WM_CLOSE :
		if(onClose ())
			::DestroyWindow (hwnd);
		return nullptr;

	case WM_DESTROY :
		::RevokeDragDrop (hwnd); // release IDropTarget
		::SetWindowLongPtr (hwnd, GWLP_USERDATA, 0);

		// From MSDN: When a window that previously returned providers has been destroyed, notify UI Automation.
		if(AccessibilityManager::isEnabled () && accessibilityProvider)
		{
			accessibilityProvider->disconnect ();
			safe_release (accessibilityProvider);
			::UiaReturnRawElementProvider (hwnd, 0, 0, nullptr);
		}

		// reenable parent dialog of progress window (see makeNativePopupWindow())
		if(style.isCustomStyle (Styles::kWindowBehaviorProgressDialog))
		{
			HWND hwndParent = ::GetParent (hwnd);
			if(hwndParent && !::IsWindowEnabled (hwndParent))
			{
				::EnableWindow (hwndParent, TRUE);
				::SetFocus (hwndParent);
			}
		}
		
		inDestroyEvent = true;
		onDestroy ();
		release (); // Window object is detroyed here!
		return nullptr;

	case WM_WINDOWPOSCHANGED :
		{
			CCL_PRINTLN ("WM_WINDOWPOSCHANGED")
			static int reordering = 0;
			if(reordering++ == 0)
				Win32::EnforceWindowOrder ();
			reordering--;

			if(!hasBeenDrawn () && hasLayeredRenderTarget ())
				invalidate (); // trigger initial update, seems to be required at soome point for a layered window (would also work in WM_SHOWWINDOW, but not for dialogs)
		}	break;

	case WM_DPICHANGED :
		{
			UINT dpiX = LOWORD (e.wParam);
			float dpiFactor = DpiScale::getFactor (dpiX);
			const RECT* newRect = reinterpret_cast<const RECT*> (e.lParam);
			ASSERT (newRect != nullptr)
			Rect newPixelRect;
			Win32::GdiInterop::fromSystemRect (newPixelRect, *newRect);
			onDpiChanged (dpiFactor, newPixelRect);
		}
		return nullptr;

	case WM_DISPLAYCHANGE :
	case WM_SETTINGCHANGE :
		Win32::gScreens.displayChanged ();
		break;

	case WM_COPYDATA :
		return EventResult(LRESULT(Win32::HandleCopyData (GUI.getApplication (), static_cast<COPYDATASTRUCT*> (e.lParam))));

	case WM_GETOBJECT :
		// TODO: how to best handle child windows?
		ASSERT (inDestroyEvent == false)
		if(DWORD(LPARAM(e.lParam)) == UiaRootObjectId)
		{
			if(AccessibilityManager::isEnabled () && ccl_cast<ChildWindow> (this) == nullptr && hwnd == handle)
			{
				IRawElementProviderSimple* elementProvider = nullptr;
				if(AccessibilityProvider* accessibilityProvider = getAccessibilityProvider ())
					elementProvider = Win32::UIAutomationElementProvider::toPlatformProvider (accessibilityProvider);

				if(elementProvider)
					return EventResult(::UiaReturnRawElementProvider (hwnd, WPARAM(e.wParam), LPARAM(e.lParam), elementProvider));
			}
			return nullptr;
		}
	}

	e.notHandled = true;
	return (EventResult)-1;
}

//************************************************************************************************
// CCLWindowProc
//************************************************************************************************

LRESULT CALLBACK CCLWindowProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CCL::Window* window = (CCL::Window*)::GetWindowLongPtr (hwnd, GWLP_USERDATA);

	if(msg == WM_NCCREATE || msg == WM_CREATE)
	{
		CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
		window = (CCL::Window*)cs->lpCreateParams;
		
		::SetWindowLongPtr (hwnd, GWLP_USERDATA, (LONG_PTR)window);

		if(msg == WM_NCCREATE)
		{
			Win32::gDpiInfo.enableNonClientDpiScaling (hwnd);
			return 1;
		}
		else // WM_CREATE
		{
			StyleRef style = window->getStyle ();
			if(style.isCustomStyle (Styles::kWindowAppearanceCustomFrame) && style.isCustomStyle (Styles::kWindowAppearanceRoundedCorners))
			{
				// Supported starting with Windows 11
				DWM_WINDOW_CORNER_PREFERENCE preference = DWMWCP_ROUND;
				::DwmSetWindowAttribute (hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &preference, sizeof(preference));
			}
			return 0;
		}
	}

	if(window) switch(msg)
	{
	case WM_NCHITTEST :
		if(window->getStyle ().isCustomStyle (Styles::kWindowAppearanceCustomFrame) && 
			window->getStyle ().isCustomStyle (Styles::kWindowBehaviorSizable))
		{
			Point p (GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)); // in screen coordinates

			RECT windowRect {};
			::GetWindowRect (hwnd, &windowRect);
			Rect size;
			Win32::GdiInterop::fromSystemRect (size, windowRect);

			enum { kEdge = 4, kCornerBottom = 12 };

			Rect growRect (size);
			growRect.left = growRect.right - kCornerBottom;
			growRect.top = growRect.bottom - kCornerBottom;
			if(growRect.pointInside (p))
				return HTBOTTOMRIGHT;

			growRect = size;
			growRect.right = growRect.left + kCornerBottom;
			growRect.top = growRect.bottom - kCornerBottom;
			if(growRect.pointInside (p))
				return HTBOTTOMLEFT;

			if(p.y <= size.top + kEdge)
			{
				if(p.x <= size.left + kEdge)
					return HTTOPLEFT;
				else if(p.x >= size.right - kEdge)
					return HTTOPRIGHT;
				else
					return HTTOP;
			}
			else if(p.x <= size.left + kEdge)
				return HTLEFT;
			else if(p.x >= size.right - kEdge)
				return HTRIGHT;
			else if(p.y >= size.bottom - kEdge)
				return HTBOTTOM;
		}
		break;

	case WM_NCLBUTTONDOWN :
		if(window->getStyle ().isCustomStyle (Styles::kWindowAppearanceCustomFrame) && 
			window->getStyle ().isCustomStyle (Styles::kWindowBehaviorSizable))
		{
			int edge = 0;
			switch (wParam)
			{
			case HTBOTTOMRIGHT: ::SetCursor (LoadCursor (nullptr, IDC_SIZENWSE)); edge = WMSZ_BOTTOMRIGHT; break;
			case HTBOTTOMLEFT:  ::SetCursor (LoadCursor (nullptr, IDC_SIZENESW)); edge = WMSZ_BOTTOMLEFT; break;
			case HTTOPRIGHT:    ::SetCursor (LoadCursor (nullptr, IDC_SIZENESW)); edge = WMSZ_TOPRIGHT; break;
			case HTTOPLEFT:     ::SetCursor (LoadCursor (nullptr, IDC_SIZENWSE)); edge = WMSZ_TOPLEFT; break;
			case HTLEFT:        ::SetCursor (LoadCursor (nullptr, IDC_SIZEWE)); edge = WMSZ_LEFT; break;
			case HTRIGHT:       ::SetCursor (LoadCursor (nullptr, IDC_SIZEWE)); edge = WMSZ_RIGHT; break;
			case HTTOP:         ::SetCursor (LoadCursor (nullptr, IDC_SIZENS)); edge = WMSZ_TOP; break;
			case HTBOTTOM:      ::SetCursor (LoadCursor (nullptr, IDC_SIZENS)); edge = WMSZ_BOTTOM; break;
			default:
				break;
			}

			POINT p;
			::GetCursorPos (&p);
			LPARAM lParam = p.x | (p.y << 16);
			::SendMessage (hwnd, WM_SYSCOMMAND, SC_SIZE | edge, lParam);
			return 0;
		}
		break;
		
	case WM_NCMOUSEMOVE :
		switch(wParam)
		{
		case HTBOTTOMRIGHT: ::SetCursor (LoadCursor (nullptr, IDC_SIZENWSE)); break;
		case HTBOTTOMLEFT:  ::SetCursor (LoadCursor (nullptr, IDC_SIZENESW)); break;
		case HTTOPRIGHT:    ::SetCursor (LoadCursor (nullptr, IDC_SIZENESW)); break;
		case HTTOPLEFT:     ::SetCursor (LoadCursor (nullptr, IDC_SIZENWSE)); break;
		case HTLEFT:        
		case HTRIGHT:       ::SetCursor (LoadCursor (nullptr, IDC_SIZEWE)); break;
		case HTTOP:         
		case HTBOTTOM:      ::SetCursor (LoadCursor (nullptr, IDC_SIZENS)); break;
		default:
			break;
		}
		return 0;
		
	case WM_ENDSESSION :
		if(wParam == TRUE || (lParam & ENDSESSION_CRITICAL) || (lParam & ENDSESSION_LOGOFF))
		{
			GUI.onAppStateChanged (IApplication::kAppTerminates);

			if(IApplication* application = GUI.getApplication ())
				application->requestQuit ();
		}
		return 0;

	case WM_NCACTIVATE :
		CCL_PRINTF ("WM_NCACTIVATE (%s) %s\n", wParam ? "true" : "false", MutableCString (window->getTitle ()).str ())

		if(!window->getStyle ().isCustomStyle (Styles::kWindowAppearanceCustomFrame))  // fall thru for customframes
		{
			if(wParam == FALSE)
			{
				// avoid deactivation flicker
				if(ccl_cast<PopupSelectorWindow> (window))
					break;

				if(!GUI.isApplicationActive ())
					break;
				
				// keep activation state
				if(Desktop.getStackDepth (kDialogLayer) == 0 || Desktop.isPopupActive ())
					return ::DefWindowProc (hwnd, msg, 1, lParam);
			}
			break;
		}
		// fallthrough intended

	default :
		CCL::SystemEvent e (hwnd, msg, (void*)wParam, (void*)lParam);
		LRESULT result = (LRESULT)window->handleEvent (e);
		if(e.wasHandled ())
			return result;
	}

	return ::DefWindowProc (hwnd, msg, wParam, lParam);
}

//************************************************************************************************
// CCLWindowProc
//************************************************************************************************

LRESULT CALLBACK CCLMessageWindowProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CCL::SystemEventHandler* handler = (CCL::SystemEventHandler*)::GetWindowLongPtr (hwnd, GWLP_USERDATA);

	if(msg == WM_CREATE)
	{
		CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
		handler = (CCL::SystemEventHandler*)cs->lpCreateParams;
		::SetWindowLongPtr (hwnd, GWLP_USERDATA, (LONG_PTR)handler);
	}

	if(handler)
	{
		SystemEvent e (hwnd, msg, (void*)wParam, (void*)lParam);
		EventResult result = handler->handleEvent (e);
		if(e.wasHandled ())
			return (LRESULT)result;
	}

	return ::DefWindowProc (hwnd, msg, wParam, lParam);
}
