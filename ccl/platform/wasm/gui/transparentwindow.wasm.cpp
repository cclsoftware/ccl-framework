//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/platform/wasm/gui/transparentwindow.wasm.cpp
// Description : Transparent Window
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/windows/transparentwindow.h"
#include "ccl/gui/windows/window.h"
#include "ccl/gui/graphics/nativegraphics.h"

namespace CCL {

//************************************************************************************************
// WasmTransparentWindow
//************************************************************************************************

class WasmTransparentWindow: public TransparentWindow
{
public:
	WasmTransparentWindow (Window* parentWindow, int options, StringRef title);
	~WasmTransparentWindow ();

	// TransparentWindow
	void show () override;
	void hide () override;
	bool isVisible () const override { return visible; }
	void update (RectRef size, Bitmap& bitmap, PointRef offset = Point (), float opacity = 1.f) override;
	void move (PointRef position) override {}

private:
	AutoPtr<IGraphicsLayer> layer;
	bool visible;
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// TransparentWindow
//************************************************************************************************

TransparentWindow* TransparentWindow::create (Window* parentWindow, int options, StringRef title)
{
	return NEW WasmTransparentWindow (parentWindow, options, title);
}

//************************************************************************************************
// WasmTransparentWindow
//************************************************************************************************

WasmTransparentWindow::WasmTransparentWindow (Window* parentWindow, int options, StringRef title)
: TransparentWindow (parentWindow, options, title),
  visible (false)
{
	layer = NativeGraphicsEngine::instance ().createGraphicsLayer (ClassID::GraphicsLayer);
	if(layer)
		layer->construct (0, Rect (0, 0, 100, 100), 0, parentWindow->getContentScaleFactor ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WasmTransparentWindow::~WasmTransparentWindow ()
{
	if(layer && parentWindow)
	{
		parentWindow->getGraphicsLayer ()->removeSublayer (layer);
		layer.release ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmTransparentWindow::show ()
{
	if(visible)
		return;
	if(parentWindow && layer)
	{
		Point offset;
		if(IGraphicsLayer* parentLayer = parentWindow->getParentLayer (offset))
			parentLayer->addSublayer (layer);
		visible = true;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmTransparentWindow::hide ()
{
	if(!visible)
		return;
	if(parentWindow && layer)
	{
		Point offset;
		if(IGraphicsLayer* parentLayer = parentWindow->getParentLayer (offset))
			parentLayer->removeSublayer (layer);
		visible = false;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmTransparentWindow::update (RectRef size, Bitmap& bitmap, PointRef offset, float opacity)
{
	if(layer)
	{
		layer->setSize (size.getWidth (), size.getHeight ());
		layer->setOffset (size.getLeftTop () + offset);
		layer->setOpacity (opacity);
		layer->setContent (bitmap.asUnknown ());
	}
}