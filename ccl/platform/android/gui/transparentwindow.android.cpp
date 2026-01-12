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
// Filename    : ccl/platform/android/gui/transparentwindow.android.cpp
// Description : Transparent Window
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/windows/transparentwindow.h"

#include "ccl/gui/graphics/imaging/offscreen.h"
#include "ccl/gui/graphics/graphicsdevice.h"

#include "ccl/platform/android/gui/androidview.h"
#include "ccl/platform/android/gui/frameworkview.h"
#include "ccl/platform/android/gui/graphicslayer.android.h"
#include "ccl/platform/android/gui/window.android.h"

#include "ccl/public/gui/graphics/dpiscale.h"

namespace CCL {

//************************************************************************************************
// AndroidTransparentWindow
//************************************************************************************************

class AndroidTransparentWindow: public TransparentWindow
{
public:
	AndroidTransparentWindow (Window* parentWindow, int options, StringRef title);
	~AndroidTransparentWindow ();

	// TransparentWindow
	void show () override;
	void hide () override;
	bool isVisible () const override;
	void update (RectRef size, Bitmap& bitmap, PointRef offset = Point (), float opacity = 1.f) override;
	void move (PointRef position) override;

private:
	class Layer: public Android::AndroidGraphicsLayer
	{
	public:
		Android::JniObject& getLayerView () { return layerView; }
	};

	Layer* layer;
};

} // namespace CCL

using namespace CCL;
using namespace Android;

//************************************************************************************************
// TransparentWindow
//************************************************************************************************

TransparentWindow* TransparentWindow::create (Window* parentWindow, int options, StringRef title)
{
	return NEW AndroidTransparentWindow (parentWindow, options, title);
}

//************************************************************************************************
// AndroidTransparentWindow
//************************************************************************************************

AndroidTransparentWindow::AndroidTransparentWindow (Window* parentWindow, int options, StringRef title)
: TransparentWindow (parentWindow, options, title),
  layer (0)
{
	// create android layer, add its android view to the parent window's android view
	layer = NEW Layer;
	layer->construct (0, Rect (0, 0, 100, 100), 0, parentWindow->getContentScaleFactor ());
	layer->isSprite (true); // mark as sprite for correct z-order (maintained in java FrameworkView)

	// add layer view to the window's android view
	ViewGroup.addView (*AndroidWindow::cast (parentWindow)->getFrameworkView (), layer->getLayerView ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AndroidTransparentWindow::~AndroidTransparentWindow ()
{
    CCL_PRINTLN ("~AndroidTransparentWindow")
	if(layer && parentWindow)
	{
		// remove view
		if(FrameworkView* frameworkView = AndroidWindow::cast (parentWindow)->getFrameworkView ())
			ViewGroup.removeView (*frameworkView, layer->getLayerView ());

		layer->release ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidTransparentWindow::show ()
{
    CCL_PRINTLN ("TransparentWindow::show")
	if(layer)
		AndroidView.setVisibility (layer->getLayerView (), 0); // 0: VISIBLE
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidTransparentWindow::hide ()
{
    CCL_PRINTLN ("TransparentWindow::hide")
	if(layer)
		AndroidView.setVisibility (layer->getLayerView (), 4); // 4: INVISIBLE
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AndroidTransparentWindow::isVisible () const
{
	if(layer)
		return AndroidView.getVisibility (layer->getLayerView ()) == 0;

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidTransparentWindow::update (RectRef size, Bitmap& bitmap, PointRef offset, float opacity)
{
    CCL_PRINTF ("TransparentWindow::update: pos (%d, %d) size (%d, %d) offset (%d, %d)\n",
                size.left, size.top, size.getWidth (), size.getHeight (), offset.x, offset.y)

	// copy bitmap into offscreen
	// FIX ME (see windows implementation): check if this additional offscreen is really required, handle bitmap size vs. window size (scale factor)

	Point layerSize (size.getSize ());

	float contentScaleFactor = getContentScaleFactor ();
	if(!DpiScale::isIntAligned (contentScaleFactor))
	{
		// might need to add one pixel to compensate the truncation of the fractional part
		PixelPointF pixelSizeF (layerSize, contentScaleFactor);
		if(!DpiScale::isIntAligned (pixelSizeF.x))
			layerSize.x++;
		if(!DpiScale::isIntAligned (pixelSizeF.y))
			layerSize.y++;
	}

	AutoPtr<Offscreen> offscreen = NEW Offscreen (layerSize.x, layerSize.y, IBitmap::kRGBAlpha, false, parentWindow);
	{
		Rect dst (0, 0, layerSize.x, layerSize.y);
		Rect src (dst);
		src.offset (offset);
		ImageMode mode (opacity);
		BitmapGraphicsDevice device (offscreen);
		device.drawImage (&bitmap, src, dst, &mode);
	}

	setSavedBitmap (offscreen);
	if(layer)
	{
		Point offset;
		parentWindow->screenToClient (offset);

		layer->setContent (offscreen->asUnknown ());
		layer->setOffset (size.getLeftTop () + offset);
		layer->setSize (layerSize.x, layerSize.y);
		layer->setUpdateNeeded ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidTransparentWindow::move (PointRef position)
{
    CCL_PRINTF ("TransparentWindow::move: x = %d y = %d\n", position.x, position.y)
	if(layer)
	{
		Point offset;
		parentWindow->screenToClient (offset);

		layer->setOffset (position + offset);
	}
}
