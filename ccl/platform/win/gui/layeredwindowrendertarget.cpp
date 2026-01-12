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
// Filename    : ccl/platform/win/gui/layeredwindowrendertarget.cpp
// Description : Layered window render target (WS_EX_LAYERED)
//
//************************************************************************************************

#include "ccl/platform/win/gui/layeredwindowrendertarget.h"
#include "ccl/platform/win/gui/screenscaling.h"

#include "ccl/gui/windows/window.h"

#include "ccl/public/base/debug.h"

using namespace CCL;
using namespace Win32;

//************************************************************************************************
// LayeredWindowRenderTarget
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (LayeredWindowRenderTarget, NativeWindowRenderTarget)

//////////////////////////////////////////////////////////////////////////////////////////////////

LayeredWindowRenderTarget::LayeredWindowRenderTarget (Window& window)
: NativeWindowRenderTarget (window),
  offscreen (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

LayeredWindowRenderTarget::~LayeredWindowRenderTarget ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

float LayeredWindowRenderTarget::getContentScaleFactor () const
{
	return window.getContentScaleFactor ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LayeredWindowRenderTarget::shouldCollectUpdates ()
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMutableRegion* LayeredWindowRenderTarget::getUpdateRegion ()
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LayeredWindowRenderTarget::onRender ()
{
	HWND hwnd = (HWND)window.getSystemWindow ();
	ASSERT (hwnd)

	Rect windowSize (window.getSize ());
 	const Win32::ScreenInformation& screen = Win32::gScreens.screenForWindowHandle (hwnd);
	screen.toPixelRect (windowSize);

	if(!offscreen)
	{
		offscreen = NEW Offscreen (windowSize.getWidth (), windowSize.getHeight (), Offscreen::kRGBAlpha, false, &window);
		CCL_PRINTF ("LayeredWindowRenderTarget: offscreen %d x %d pixel\n", windowSize.getWidth (), windowSize.getHeight ())
	}

	Win32::GdiClipRegion updateRegion (hwnd); // copy update region before BeginPaint()!

	PAINTSTRUCT ps;
	::BeginPaint (hwnd, &ps);

	CCL_PROFILE_START (drawToOffscreen)
	Win32::GdiClipRegion::RectList rectList (updateRegion);
	rectList.adjustToCoords (getContentScaleFactor ());
	render (rectList);
	CCL_PROFILE_STOP (drawToOffscreen)

	::EndPaint (hwnd, &ps);

	// update layered window with offscreen
	NativeBitmap* nativeBitmap = offscreen->getNativeBitmap ();
	ASSERT (nativeBitmap)

	POINT screenPos = {windowSize.left, windowSize.top};
	SIZE screenSize = {windowSize.getWidth (), windowSize.getHeight ()};

	UnknownPtr<Win32::IWin32Bitmap> gdiBitmap = ccl_as_unknown (nativeBitmap);
	ASSERT (gdiBitmap)
	if(!gdiBitmap)
		return;

	HDC hdcScreen = nullptr;
	HDC hdcBitmap = ::CreateCompatibleDC (hdcScreen);
	HGDIOBJ oldBitmap = ::SelectObject (hdcBitmap, gdiBitmap->getHBITMAP ());
	POINT bitmapOffset = {0, 0};

	BLENDFUNCTION blendFunc = {0};
	blendFunc.BlendOp = AC_SRC_OVER;
	blendFunc.BlendFlags = 0;
	blendFunc.SourceConstantAlpha = (BYTE)ccl_bound<float> (window.getOpacity () * 255.f, 0, 255);
	blendFunc.AlphaFormat = AC_SRC_ALPHA;

	BOOL result = ::UpdateLayeredWindow (hwnd, hdcScreen, &screenPos, &screenSize,
										 hdcBitmap, &bitmapOffset, RGB (0, 0, 0), &blendFunc, ULW_ALPHA);

	CCL_PRINTF ("UpdateLayeredWindow %d (%d, %d,   %d x %d%s)\n", result, screenPos.x, screenPos.y, screenSize.cx, screenSize.cy, result ? "" : MutableCString ("  FAILED: Error ").appendInteger (::GetLastError ()).str ())

	::SelectObject (hdcBitmap, oldBitmap);
	::DeleteDC (hdcBitmap);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LayeredWindowRenderTarget::render (const Win32::GdiClipRegion::RectList& rectList)
{
	BitmapGraphicsDevice graphicsDevice (offscreen);
	GraphicsDevice* oldDevice = window.setGraphicsDevice (&graphicsDevice);

	for(int i = 0; i < rectList.rectCount; i++)
	{
		RectRef rect = rectList.rects [i];
		graphicsDevice.saveState ();
		graphicsDevice.addClip (rect);

		graphicsDevice.clearRect (rect); // clear background

		window.draw (UpdateRgn (rect));
		graphicsDevice.restoreState ();
	}

	window.setGraphicsDevice (oldDevice);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LayeredWindowRenderTarget::onSize ()
{
	if(offscreen)
	{
		PixelPoint windowSize (Point (window.getWidth (), window.getHeight ()), getContentScaleFactor ());
		ASSERT (windowSize.x > 0 && windowSize.y > 0)

		if(offscreen->getSize () != windowSize)
			offscreen.release ();
	}
	window.invalidate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LayeredWindowRenderTarget::onScroll (RectRef inRect, PointRef inDelta)
{
	if(offscreen)
	{
		Rect rect (inRect);
		Point delta (inDelta);

		float scaleFactor = getContentScaleFactor ();
		bool fractionalScaling = DpiScale::isIntAligned (scaleFactor) == false;
		if(fractionalScaling)
		{
			PixelRectF rectF (rect, scaleFactor);
			PixelPointF deltaF (delta, scaleFactor);
			if(rectF.isPixelAligned () == false || deltaF.isPixelAligned () == false)
			{
				// cannot scroll fractional pixels
				Rect r (rect);
				r.offset (delta);
				r.join (rect);
				window.invalidate (r);
				return;
			}
			rect = rectFToInt (rectF);
			delta = pointFToInt (deltaF);
		}
		else
		{
			DpiScale::toPixelRect (rect, scaleFactor);
			DpiScale::toPixelPoint (delta, scaleFactor);
		}

		// note: offscreen bitmap has window scaling factor, arguments are interpreted as pixels
		offscreen->scrollPixelRect (rect, delta);

		// invalidate areas
		window.finishScroll (inRect, inDelta);
	}
}
