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
// Filename    : ccl/platform/wasm/skia/skialayer.wasm.cpp
// Description : Graphics Layer for Skia content
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/wasm/skia/skialayer.wasm.h"

#include "ccl/platform/shared/skia/skialayer.h"
#include "ccl/platform/shared/skia/skiadevice.h"

#include "ccl/platform/shared/opengles/opengles3dsupport.h"

#include "ccl/gui/graphics/imaging/offscreen.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

namespace CCL {
namespace Wasm {

class WasmSkiaLayer;

//************************************************************************************************
// WasmSkiaLayerRenderTarget
//************************************************************************************************

class WasmSkiaLayerRenderTarget: public Object,
								 public SkiaRenderTarget
{
public:
	WasmSkiaLayerRenderTarget (WasmSkiaLayer& layer)
	: layer (layer),
	  scaleFactor (1.f)
	{}

	PROPERTY_VARIABLE (float, scaleFactor, ScaleFactor)

	bool present (SkSurface* targetSurface);

	Rect getSize () const;
	bool resize (RectRef size);

	// SkiaRenderTarget
	SkCanvas* getCanvas () override { return surface ? surface->getCanvas () : nullptr; }
	float getContentScaleFactor () const override { return scaleFactor; }
	void onSize () override {};

private:
	WasmSkiaLayer& layer;
	OpenGLESImage texture;
	sk_sp<SkImage> textureImage;
	GrBackendTexture backendTexture;
};

//************************************************************************************************
// WasmSkiaLayer
//************************************************************************************************

class WasmSkiaLayer: public SkiaLayer
{
public:
	DECLARE_CLASS (WasmSkiaLayer, SkiaLayer)
	
	WasmSkiaLayer ();
	~WasmSkiaLayer ();
	
	PROPERTY_BOOL (frameRequested, FrameRequested)
	PROPERTY_OBJECT (Point, position, Position)

	virtual SkiaWindowRenderTarget* getWindowRenderTarget ();

	// SkiaLayer
	void onFrame (SkSurface* targetSurface) override;
	void resetGPUState () override;
	tresult CCL_API flush () override;
	tresult CCL_API addSublayer (IGraphicsLayer* layer) override;
	tresult CCL_API removeSublayer (IGraphicsLayer* layer) override;
	
protected:
	WasmSkiaLayerRenderTarget renderTarget;

	tresult show ();
	void hide ();
	void drawContent ();
	void destroySurface ();
	void hidePendingSublayers ();

	// SkiaLayer
	void checkSize () override;
	void updatePosition () override;
	void requestFrame (bool deep = false) override;
};

//************************************************************************************************
// SkiaRootLayer
//************************************************************************************************

class SkiaRootLayer: public WasmSkiaLayer
{
public:
    DECLARE_CLASS (SkiaRootLayer, WasmSkiaLayer)

	SkiaRootLayer ();
	
	// SkiaLayer
	SkiaWindowRenderTarget* getWindowRenderTarget () override;
	tresult CCL_API construct (IUnknown* content, RectRef bounds = Rect (), int mode = 0, float contentScaleFactor = 1.f) override;
	tresult CCL_API setContent (IUnknown* content) override;
	
protected:
	Window* window;

	// WasmSkiaLayer
	Rect getClipRect () override;
	void updatePosition () override;
	void requestFrame (bool deep = false) override;
};

} // namespace Wasm
} // namespace CCL

using namespace CCL;
using namespace Wasm;

//************************************************************************************************
// SkiaLayerFactory
//************************************************************************************************

IGraphicsLayer* SkiaLayerFactory::createLayer (UIDRef classID)
{
	if(classID == ClassID::RootLayer)
		return NEW SkiaRootLayer;
	if(classID == ClassID::GraphicsLayer)
		return NEW WasmSkiaLayer;
	if(classID == ClassID::TiledLayer)
		return NEW WasmSkiaLayer; // not yet implemented
	return nullptr;
}

//************************************************************************************************
// WasmSkiaLayerRenderTarget
//************************************************************************************************

Rect WasmSkiaLayerRenderTarget::getSize () const
{
	if(surface)
		return Rect (0, 0, surface->width (), surface->height ());
	return Rect ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WasmSkiaLayerRenderTarget::resize (RectRef size)
{
	if(size == getSize ())
		return true;

	SkiaRenderTarget* windowRenderTarget = layer.getWindowRenderTarget ();
	if(windowRenderTarget == nullptr)
		return false;

	SkCanvas* windowCanvas = windowRenderTarget->getCanvas ();
	if(windowCanvas == nullptr)
		return false;

	textureImage.reset ();
	surface.reset ();
	texture.destroy ();

	if(size.isEmpty ())
		return true;

	texture.setSize (size.getSize ());
	if(!texture.create ())
	{
		CCL_WARN ("Failed to create a layer texture.\n", 0)
		return false;
	}
	if(!texture.generateFramebuffer ())
	{
		CCL_WARN ("Failed to create a layer framebuffer.\n", 0)
		return false;
	}

	GrGLFramebufferInfo frameBufferInfo;
	frameBufferInfo.fFBOID = texture.getFramebufferID ();
	frameBufferInfo.fFormat = GL_RGBA8_OES;

	GrBackendRenderTarget targetInfo = GrBackendRenderTargets::MakeGL (size.getWidth (), size.getHeight (), 0, 8, frameBufferInfo);

	surface = SkSurfaces::WrapBackendRenderTarget (windowCanvas->recordingContext (), targetInfo, kBottomLeft_GrSurfaceOrigin, SkColorType::kRGBA_8888_SkColorType, nullptr, nullptr);
	if(surface == nullptr)
	{
		CCL_WARN ("Failed to create a layer surface\n", 0);
		return false;
	}
	
	GrGLTextureInfo textureInfo = {0};
	textureInfo.fTarget = GL_TEXTURE_2D;
	textureInfo.fID = texture.getTextureID ();
	textureInfo.fFormat = GL_RGBA8_OES;

	backendTexture = GrBackendTextures::MakeGL (size.getWidth (), size.getHeight (), skgpu::Mipmapped::kNo, textureInfo);

	textureImage = SkImages::BorrowTextureFrom (windowCanvas->recordingContext (), backendTexture, kBottomLeft_GrSurfaceOrigin, kRGBA_8888_SkColorType, kPremul_SkAlphaType, nullptr);
	if(textureImage == nullptr)
	{
		CCL_WARN ("Failed to create a Skia image from a layer texture\n", 0);
		return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WasmSkiaLayerRenderTarget::present (SkSurface* targetSurface)
{
	if(textureImage == nullptr)
		return false;

	if(targetSurface == nullptr)
		return false;

	SkiaRenderTarget* windowRenderTarget = layer.getWindowRenderTarget ();
	if(windowRenderTarget == nullptr)
		return false;

	SkCanvas* windowCanvas = windowRenderTarget->getCanvas ();
	if(windowCanvas == nullptr)
		return false;

	SkCanvas* targetCanvas = targetSurface->getCanvas ();
	if(targetCanvas == nullptr)
		return false;
	
	Rect clipRect (layer.getClipRect ());

	Rect updateRect = layer.getContentRect ().getSize ();
	
	int saveCount = targetCanvas->save ();

	Point position = layer.getPosition ();
	CCL_PRINTF ("position: %d, %d\n", position.x, position.y)

	targetCanvas->resetMatrix ();
	targetCanvas->scale (scaleFactor, scaleFactor);
	targetCanvas->translate (-clipRect.left + position.x, -clipRect.top + position.y);

	const SkRect& skiaClipRect = SkRect::MakeLTRB (clipRect.left, clipRect.top, clipRect.right, clipRect.bottom);
	targetCanvas->clipRect (skiaClipRect, true);
	if(layer.getLayerOpacity () < 1)
	{
		SkPaint alpha;
		alpha.setAlphaf (layer.getLayerOpacity ());
		targetCanvas->saveLayer (nullptr, &alpha);
	}
	
	if(!get_flag<int> (layer.getLayerMode (), IGraphicsLayer::kClipToBounds))
		targetCanvas->translate (layer.getLayerOffset ().x, layer.getLayerOffset ().y);

	targetCanvas->concat (layer.getTransformMatrix ());

	PixelRectF srcInPixel (updateRect, scaleFactor);
	SkRect skSrc;
	SkiaDevice::toSkRect (skSrc, srcInPixel);
	PixelRectF dstInPixel (updateRect, scaleFactor);
	SkRect skDst;
	SkiaDevice::toSkRect (skDst, dstInPixel);
	
	SkCanvas::SrcRectConstraint constraint = SkCanvas::kFast_SrcRectConstraint;
	SkSamplingOptions samplingOptions (SkFilterMode::kLinear, SkMipmapMode::kLinear);
	SkPaint paint;
	targetCanvas->drawImageRect (textureImage, skSrc, skDst, samplingOptions, &paint, constraint);

	targetCanvas->restoreToCount (saveCount);

	return true;
}

//************************************************************************************************
// LinuxSkiaLayer
//************************************************************************************************

DEFINE_CLASS_HIDDEN (WasmSkiaLayer, SkiaLayer)

//////////////////////////////////////////////////////////////////////////////////////////////////

WasmSkiaLayer::WasmSkiaLayer ()
: renderTarget (*this),
  frameRequested (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

WasmSkiaLayer::~WasmSkiaLayer ()
{
	hide ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkiaWindowRenderTarget* WasmSkiaLayer::getWindowRenderTarget ()
{
	WasmSkiaLayer* parentLayer = unknown_cast<WasmSkiaLayer> (getParentLayer ());
	if(parentLayer)
		return parentLayer->getWindowRenderTarget ();
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult WasmSkiaLayer::show ()
{
	if(getParentLayer () != nullptr)
	{
		if(renderTarget.getSize ().isEmpty ())
		{
			for(WasmSkiaLayer* layer : iterate_as<WasmSkiaLayer> (sublayers))
			{
				if(!layer->renderTarget.getSize ().isEmpty ())
					layer->show ();
			}
			setContent (content);
			checkSize ();
			requestCanvasUpdate ();
		}
	}
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmSkiaLayer::hide ()
{
	for(WasmSkiaLayer* layer : iterate_as<WasmSkiaLayer> (sublayers))
		layer->hide ();

	renderTarget.resize (Rect ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmSkiaLayer::resetGPUState ()
{
	hide ();
	setUpdateNeeded ();
	for(WasmSkiaLayer* layer : iterate_as<WasmSkiaLayer> (sublayers))
		layer->resetGPUState ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmSkiaLayer::requestFrame (bool deep)
{
	if(deep && !sublayers.isEmpty ())
	{
		for(WasmSkiaLayer* layer : iterate_as<WasmSkiaLayer> (sublayers))
			layer->requestFrame (true);
		return;
	}

	frameRequested = true;
	WasmSkiaLayer* parentLayer = unknown_cast<WasmSkiaLayer> (getParentLayer ());
	if(parentLayer)
		parentLayer->requestFrame ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmSkiaLayer::hidePendingSublayers ()
{
	ForEach (removedSublayers, WasmSkiaLayer, subLayer)
		subLayer->hide ();
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WasmSkiaLayer::flush ()
{
	hidePendingSublayers ();
	removePendingSublayers ();

	for(WasmSkiaLayer* layer : iterate_as<WasmSkiaLayer> (sublayers))
	{
		if(layer->frameRequested)
			layer->flush ();
	}
		
	if(renderTarget.getSize ().isEmpty ())
		show ();

	if(contentRect.isEmpty ())
		return kResultFailed;

	checkSize ();

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmSkiaLayer::onFrame (SkSurface* targetSurface)
{	
	frameRequested = false;

	flush ();
	
	if(!contentRect.isEmpty ())
	{
		drawContent ();
		renderTarget.setScaleFactor (contentScaleFactor);
		if(renderTarget.present (targetSurface))
			needCanvasUpdate = false;
	}

	for(WasmSkiaLayer* layer : iterate_as<WasmSkiaLayer> (sublayers))
		layer->onFrame (targetSurface);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WasmSkiaLayer::addSublayer (IGraphicsLayer* layer)
{
	tresult result = SuperClass::addSublayer (layer);

	WasmSkiaLayer* skiaLayer = unknown_cast<WasmSkiaLayer> (layer);

	if(skiaLayer != nullptr)
		result = skiaLayer->show ();

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WasmSkiaLayer::removeSublayer (IGraphicsLayer* layer)
{
	tresult result = SuperClass::removeSublayer (layer);
	requestCanvasUpdate ();
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmSkiaLayer::drawContent ()
{
	SkiaWindowRenderTarget* windowRenderTarget = getWindowRenderTarget ();
	if(windowRenderTarget == nullptr)
		return;

	SkCanvas* windowCanvas = windowRenderTarget->getCanvas ();
	if(windowCanvas == nullptr)
		return;

	if(dirtyRect.isEmpty () && !needCanvasUpdate)
		return;

	if(!dirtyRect.isEmpty ())
	{
		if(renderTarget.getSize () != contentRect.getSize ())
		{
			renderTarget.resize (contentRect.getSize ());
			dirtyRect = contentRect.getSize ();
		}

		SkiaScopedGraphicsDevice nativeDevice (renderTarget, *renderTarget.asUnknown ());
		
		GraphicsDevice graphics;
		graphics.setNativeDevice (&nativeDevice);

		if(Bitmap* bitmap = unknown_cast<Bitmap> (content))
			graphics.drawImage (bitmap, contentRect.getSize (), contentRect.getSize ());
		else
		{
			UnknownPtr<IGraphicsLayerContent> layerContent (content);
			if(layerContent && get_flag<int> (mode, kIgnoreAlpha))
				graphics.fillRect (dirtyRect, SolidBrush (Colors::kBlack));
			else
				graphics.clearRect (dirtyRect);
			graphics.addClip (dirtyRect);
		
			if(layerContent)
			{
				IGraphicsLayerContent::LayerHint layerHint = layerContent->getLayerHint ();
				if(layerHint == kGraphicsContentEmpty)
					return;
				layerContent->drawLayer (graphics, UpdateRgn (dirtyRect));
			}
		}
	}

	dirtyRect.setReallyEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmSkiaLayer::checkSize ()
{
	size = getClipRect ();
	updatePosition ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmSkiaLayer::updatePosition ()
{
	if(get_flag<int> (mode, kClipToBounds))
		position = size.getLeftTop () + offset;
	else
		position = size.getLeftTop ();
	if(WasmSkiaLayer* parentLayer = unknown_cast<WasmSkiaLayer> (getParentLayer ()))
		position.offset (parentLayer->getPosition ());
}

//************************************************************************************************
// SkiaRootLayer
//************************************************************************************************

DEFINE_CLASS_HIDDEN (SkiaRootLayer, WasmSkiaLayer)

//////////////////////////////////////////////////////////////////////////////////////////////////

SkiaRootLayer::SkiaRootLayer ()
: window (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkiaWindowRenderTarget* SkiaRootLayer::getWindowRenderTarget ()
{
	if(window)
		return ccl_cast<SkiaWindowRenderTarget> (window->getRenderTarget ());
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaRootLayer::setContent (IUnknown* content)
{
	if(window)
		window->removeObserver (this);
	window = unknown_cast<Window> (content);

	ASSERT (window)

	if(window)
		contentRect = window->getSize ();

	return window ? kResultOk : kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaRootLayer::construct (IUnknown* content, RectRef bounds, int mode, float contentScaleFactor)
{
	// simple pointer, not a SharedPtr, this object is owned by the window
	tresult result = setContent (content);
	if(result != kResultOk)
		return result;

	setContentScaleFactor (contentScaleFactor);

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect SkiaRootLayer::getClipRect ()
{
	if(window)
	{
		Rect size;
		window->getFrameSize (size);
		return size.getSize ();
	}
	return SkiaLayer::getClipRect ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkiaRootLayer::requestFrame (bool deep)
{
	if(deep)
		SkiaLayer::requestFrame (deep);
	if(window)
		window->invalidate (Rect ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkiaRootLayer::updatePosition ()
{}