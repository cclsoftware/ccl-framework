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
// Filename    : ccl/platform/win/gui/transparentwindow.win.cpp
// Description : Transparent Window
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/win/gui/transparentwindow.win.h"

#include "ccl/gui/windows/window.h"
#include "ccl/gui/graphics/nativegraphics.h"
#include "ccl/base/collections/objectarray.h"

#include "ccl/platform/win/interfaces/iwin32graphics.h"
#include "ccl/platform/win/gui/windowhelper.h"
#include "ccl/platform/win/gui/windowclasses.h"
#include "ccl/platform/win/gui/screenscaling.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

static void ManageTransparentWindow (WindowsTransparentWindow& window, const char* where);

static const Win32::ScreenInformation& GetScreenForTransparentWindow (WindowsTransparentWindow& window)
{
	void* handle = nullptr;
	if(window.getParentWindow ())
		handle = window.getParentWindow ()->getSystemWindow ();
	if(!handle)
		handle = window.getNativeWindow ();

	return Win32::gScreens.screenForWindowHandle (handle);
}

//************************************************************************************************
// TransparentWindow
//************************************************************************************************

TransparentWindow* TransparentWindow::create (Window* parentWindow, int options, StringRef title)
{
	return NEW WindowsTransparentWindow (parentWindow, options, title);
}

//************************************************************************************************
// WindowsTransparentWindow
//************************************************************************************************

WindowsTransparentWindow::WindowsTransparentWindow (Window* parentWindow, int options, StringRef title)
: TransparentWindow (parentWindow, options, title),
  nativeWindow (nullptr)
{
	DWORD wstyle = WS_POPUP;
	DWORD xstyle = WS_EX_LAYERED;

	// find top-level parent in case it's a child window...
	HWND hwndParent = parentWindow ? (HWND)parentWindow->getSystemWindow () : nullptr;
	hwndParent = Win32::FindTopLevelWindow (hwndParent);

	nativeWindow = ::CreateWindowEx (xstyle, Win32::kTransparentWindowClass, StringChars (title), wstyle,
									 CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
									 hwndParent, nullptr, g_hMainInstance,
									 nullptr);
	ASSERT (nativeWindow != NULL)
	::SetWindowLongPtr (nativeWindow, GWLP_USERDATA, (LONG_PTR)this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowsTransparentWindow::~WindowsTransparentWindow ()
{
	::DestroyWindow (nativeWindow);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsTransparentWindow::show ()
{
	::ShowWindow (nativeWindow, SW_SHOWNA);

	ManageTransparentWindow (*this, "show");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsTransparentWindow::hide ()
{
	::ShowWindow (nativeWindow, SW_HIDE);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WindowsTransparentWindow::isVisible () const
{
	return ::IsWindowVisible (nativeWindow) == TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsTransparentWindow::update (RectRef _size, Bitmap& _bitmap, PointRef _offset, float opacity)
{
	CCL_PRINTF ("TransparentWindow::update: x = %d y = %d width = %d height = %d\n",
				 _size.left, _size.top, _size.getWidth (), _size.getHeight ())

	// window rect (in pixels)
	Rect windowSize (_size);
	const Win32::ScreenInformation& screen = GetScreenForTransparentWindow (*this);
	screen.toPixelRect (windowSize);

	NativeBitmap* nativeBitmap = _bitmap.getNativeBitmap ();
	ASSERT (nativeBitmap != nullptr)

	if(Bitmap::isHighResolutionScaling (screen.scaleFactor))
		if(MultiResolutionBitmap* multiBitmap = ccl_cast<MultiResolutionBitmap> (&_bitmap))
			if(multiBitmap->getNativeBitmap2x ())
				nativeBitmap = multiBitmap->getNativeBitmap2x ();

	// source size and offset in native bitmap (in pixels)
	Point sourceSize (_size.getSize ());
	DpiScale::toPixelPoint (sourceSize, nativeBitmap->getContentScaleFactor ());

	Point offset (_offset);
	DpiScale::toPixelPoint (offset, nativeBitmap->getContentScaleFactor ());

	// to copy pixels, the bitmap source rect must have the same size in pixels as the window
	// if the sizes don't match, draw the bitmap into a temporary offscreen first (stretching)
	AutoPtr<NativeBitmap> offscreen;
	if(windowSize.getSize () != sourceSize)
	{
		Rect offscreenSize (0, 0, windowSize.getWidth (), windowSize.getHeight ());
		if(offscreen = NativeGraphicsEngine::instance ().createOffscreen (offscreenSize.getWidth (), offscreenSize.getHeight (), Bitmap::kRGBAlpha, false, nullptr)) // (don't take scale factor from parentWindow)
		{
			AutoPtr<NativeGraphicsDevice> offscreenDevice (NativeGraphicsEngine::instance ().createBitmapDevice (offscreen));

			Rect sourceRect (offset.x, offset.y, sourceSize);
			nativeBitmap->draw (*offscreenDevice, sourceRect, offscreenSize);

			nativeBitmap = offscreen;
			offset (0, 0); // drawn at origin of offscreen, no more offset when copying to window (below)
		}
	}

	POINT screenPos = {windowSize.left, windowSize.top};
	SIZE screenSize = {windowSize.getWidth (), windowSize.getHeight ()};

	UnknownPtr<Win32::IWin32Bitmap> gdiBitmap = nativeBitmap->asUnknown ();
	ASSERT (gdiBitmap != nullptr)
	if(!gdiBitmap)
		return;

	HDC hdcScreen = nullptr;//::GetDC (0);
	HDC hdcBitmap = ::CreateCompatibleDC (hdcScreen);
	HGDIOBJ oldBitmap = ::SelectObject (hdcBitmap, gdiBitmap->getHBITMAP ());
	POINT bitmapOffset = {offset.x, offset.y};

	BLENDFUNCTION blendFunc = {0};
	blendFunc.BlendOp = AC_SRC_OVER;
	blendFunc.BlendFlags = 0;
	blendFunc.SourceConstantAlpha = (BYTE)ccl_bound<float> (opacity * 255.f, 0, 255);
	blendFunc.AlphaFormat = gdiBitmap->isAlphaPixelFormat () ? AC_SRC_ALPHA : 0;

	BOOL result = ::UpdateLayeredWindow (nativeWindow,
										 hdcScreen,
										 &screenPos,
										 &screenSize,
										 hdcBitmap,
										 &bitmapOffset,
										 RGB (0, 0, 0),
										 &blendFunc,
										 ULW_ALPHA);

	//ASSERT (result == TRUE)
	#if DEBUG_LOG
	if(result == FALSE)
	{
		int error = ::GetLastError ();
		CCL_PRINTF ("UpdateLayeredWindow failed with error = %d!!!\n", error)
	}
	#endif

	::SelectObject (hdcBitmap, oldBitmap);
	::DeleteDC (hdcBitmap);
	//::ReleaseDC (0, hdcScreen);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsTransparentWindow::move (PointRef _position)
{
	CCL_PRINTF ("TransparentWindow::move: x = %d y = %d\n", _position.x, _position.y)

	Point position (_position);
	const Win32::ScreenInformation& screen = GetScreenForTransparentWindow (*this);
	screen.toPixelPoint (position);

	POINT screenPos = {position.x, position.y};
	BOOL result = ::UpdateLayeredWindow (nativeWindow,
										 NULL,
										 &screenPos,
										 nullptr,
										 NULL,
										 nullptr,
										 RGB (0, 0, 0),
										 nullptr,
										 0);

	//ASSERT (result == TRUE)
	#if DEBUG_LOG
	if(result == FALSE)
	{
		int error = ::GetLastError ();
		CCL_PRINTF ("UpdateLayeredWindow failed with error = %d!!!\n", error)
	}
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// CCLTransparentWindowProc
//////////////////////////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK CCLTransparentWindowProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	WindowsTransparentWindow* window = (WindowsTransparentWindow*)::GetWindowLongPtr (hwnd, GWLP_USERDATA);

	switch(msg)
	{
	case WM_NCHITTEST :
		return HTTRANSPARENT;

	case WM_ACTIVATEAPP :
		if(window && wParam && ::IsWindowVisible (hwnd))
			ManageTransparentWindow (*window, "ActivateApp");
		break;
	}
	return ::DefWindowProc (hwnd, msg, wParam, lParam);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static void InsertWindowAfter (HWND hwndThis, HWND hwndAfter)
{
	#if 0//DEBUG
	char className[128] = {0};
	::GetClassNameA (hwndAfter, className, 127);
	Debugger::println (className);
	#endif

	::SetWindowPos (hwndThis, hwndAfter, 0, 0, 0, 0, SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE/*|SWP_NOOWNERZORDER*/);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static HWND GetWindowAfter (HWND hwndThis)
{
	return ::GetNextWindow (hwndThis, GW_HWNDNEXT);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static void ManageTransparentWindow (WindowsTransparentWindow& window, const char* where)
{
	if(window.isKeepOnTop ())
		return;

	#if 0//DEBUG
	Debugger::printf ("ManageTransparentWindow() %s\n", where);
	#endif

	HWND hwndThis = window.getNativeWindow ();
	HWND hwndParent = ::GetParent (hwndThis);
	HWND hwndAfter = nullptr;

	WindowsTransparentWindow* reference = nullptr;
	if(window.getParentWindow ())
		if(WindowsTransparentWindow* first = static_cast<WindowsTransparentWindow*> (window.getParentWindow ()->getFirstTransparentWindow ()))
			if(first && first != &window)
				reference = first;

	if(reference)
	{
		HWND hwndReference = reference->getNativeWindow ();

		#if 1
		// check relation of reference window to parent
		if(GetWindowAfter (hwndReference) != hwndParent)
		{
			InsertWindowAfter (hwndReference, hwndParent);
			#if 0//DEBUG
			Debugger::println ("### Fixed transparent window reference ###");
			#endif
		}
		#endif

		hwndAfter = hwndReference;
	}

	if(hwndAfter == nullptr)
		hwndAfter = GetWindowAfter (hwndParent);

	InsertWindowAfter (hwndThis, hwndAfter);
}

