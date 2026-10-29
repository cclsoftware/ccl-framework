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
// Filename    : ccl/platform/linux/skia/skialayer.linux.cpp
// Description : Graphics Layer for Skia content
//
//************************************************************************************************

#define DEBUG_LOG 0
#define DEBUG_DRAW_LAYERS (0 && DEBUG)

#include "ccl/platform/linux/skia/skialayer.linux.h"
#include "ccl/platform/linux/skia/skiarendertarget.linux.h"
#include "ccl/platform/linux/gui/window.linux.h"
#include "ccl/platform/linux/wayland/subsurface.h"

#include "ccl/platform/shared/skia/skialayer.h"
#include "ccl/platform/shared/skia/skiadevice.h"

#include "ccl/gui/graphics/imaging/offscreen.h"

namespace CCL {
namespace Linux {

//************************************************************************************************
// LinuxSkiaLayer
//************************************************************************************************

class LinuxSkiaLayer: public SkiaLayer,
				 	  public WaylandObject
{
public:
	DECLARE_CLASS (LinuxSkiaLayer, SkiaLayer)
	
	LinuxSkiaLayer ();
	~LinuxSkiaLayer ();
	
	PROPERTY_BOOL (frameRequested, FrameRequested)

	// SkiaLayer
	tresult CCL_API placeAbove (IGraphicsLayer* layer, IGraphicsLayer* sibling) override;
	tresult CCL_API placeBelow (IGraphicsLayer* layer, IGraphicsLayer* sibling) override;
	void CCL_API setContentScaleFactor (float factor) override;
	tresult CCL_API flush () override;
	tresult CCL_API addSublayer (IGraphicsLayer* layer) override;
	tresult CCL_API removeSublayer (IGraphicsLayer* layer) override;

	// WaylandObject
	void onCompositorDisconnected () override;
	void onCompositorConnected () override;
	
protected:
	AutoPtr<LinuxLayerRenderTarget> renderTarget;
	SubSurface<>* subSurface;
	
	tresult show ();
	void hide ();
	virtual Surface* getSurface ();
	virtual Surface* getParentSurface ();
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

class SkiaRootLayer: public LinuxSkiaLayer
{
public:
    DECLARE_CLASS (SkiaRootLayer, LinuxSkiaLayer)

	SkiaRootLayer ();
	~SkiaRootLayer ();
	
    // SkiaLayer
	tresult CCL_API construct (IUnknown* content, RectRef bounds = Rect (), int mode = 0, float contentScaleFactor = 1.f) override;
	tresult CCL_API setContent (IUnknown* content) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	
protected:
	Window* window;

	// LinuxSkiaLayer
	Surface* getParentSurface () override;
	Surface* getSurface () override;
	Rect getClipRect () override;
	void requestFrame (bool deep = false) override;
};

} // namespace Linux
} // namespace CCL

using namespace CCL;
using namespace Linux;

//************************************************************************************************
// SkiaLayerFactory
//************************************************************************************************

IGraphicsLayer* SkiaLayerFactory::createLayer (UIDRef classID)
{
	if(classID == ClassID::RootLayer)
		return NEW SkiaRootLayer;
	if(classID == ClassID::GraphicsLayer)
		return NEW LinuxSkiaLayer;
	if(classID == ClassID::TiledLayer)
		return NEW LinuxSkiaLayer; // not yet implemented
	return nullptr;
}

//************************************************************************************************
// LinuxSkiaLayer
//************************************************************************************************

DEFINE_CLASS_HIDDEN (LinuxSkiaLayer, SkiaLayer)

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxSkiaLayer::LinuxSkiaLayer ()
: subSurface (nullptr),
  frameRequested (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxSkiaLayer::~LinuxSkiaLayer ()
{
	hide ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxSkiaLayer::destroySurface ()
{
	renderTarget.release ();
	subSurface->destroySurface ();
	delete subSurface;
	subSurface = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Surface* LinuxSkiaLayer::getSurface ()
{
	return subSurface;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Surface* LinuxSkiaLayer::getParentSurface ()
{
	LinuxSkiaLayer* parentLayer = unknown_cast<LinuxSkiaLayer> (getParentLayer ());
	if(parentLayer == nullptr)
		return nullptr;
	
	return parentLayer->getSurface ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult LinuxSkiaLayer::show ()
{
	Surface* parentSurface = getParentSurface ();
	if(parentSurface != nullptr)
	{
		if(subSurface == nullptr)
			subSurface = NEW SubSurface (*parentSurface);

		// if parent wayland surface is not available yet, defer construction
		if(!renderTarget.isValid () && parentSurface->getWaylandSurface () != nullptr)
		{
			subSurface->createSurface ();
			subSurface->setSynchronous (true);
				
			subSurface->commit ();

			renderTarget = LinuxLayerRenderTarget::create (subSurface->getWaylandSurface (), *this);

			renderTarget->setContentScaleFactor (contentScaleFactor);

			for(LinuxSkiaLayer* layer : iterate_as<LinuxSkiaLayer> (sublayers))
			{
				if(layer->getSurface () != nullptr && !layer->renderTarget.isValid ())
					layer->show ();
			}
		}

		// Set content first, resize afterwards.
		// Resizing triggers presentation, and some compositors won't show the surface if the first frame doesn't have any content.
		setContent (content);

		checkSize ();
		requestCanvasUpdate ();

		WaylandClient::instance ().registerObject (*this);
	}

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxSkiaLayer::hide ()
{
	WaylandClient::instance ().unregisterObject (*this);
	
	for(LinuxSkiaLayer* layer : iterate_as<LinuxSkiaLayer> (sublayers))
		layer->hide ();

	renderTarget.release ();
	
	if(subSurface)
		destroySurface ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxSkiaLayer::onCompositorDisconnected ()
{
	hide ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxSkiaLayer::onCompositorConnected ()
{
	show ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API LinuxSkiaLayer::placeAbove (IGraphicsLayer* _layer, IGraphicsLayer* _sibling)
{
	tresult result = SuperClass::placeAbove (_layer, _sibling);
	if(result == kResultOk)
	{
		LinuxSkiaLayer* layer = unknown_cast<LinuxSkiaLayer> (_layer);
		LinuxSkiaLayer* sibling = unknown_cast<LinuxSkiaLayer> (_sibling);
		if(layer == nullptr || sibling == nullptr)
			return kResultInvalidArgument;

		if(layer->subSurface && sibling->getSurface ())
			layer->subSurface->placeAbove (*sibling->getSurface ());
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API LinuxSkiaLayer::placeBelow (IGraphicsLayer* _layer, IGraphicsLayer* _sibling)
{
	tresult result = SuperClass::placeBelow (_layer, _sibling);
	if(result == kResultOk)
	{
		LinuxSkiaLayer* layer = unknown_cast<LinuxSkiaLayer> (_layer);
		LinuxSkiaLayer* sibling = unknown_cast<LinuxSkiaLayer> (_sibling);
		if(layer == nullptr || sibling == nullptr)
			return kResultInvalidArgument;

		if(layer->subSurface && sibling->getSurface ())
			layer->subSurface->placeBelow (*sibling->getSurface ());
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API LinuxSkiaLayer::setContentScaleFactor (float factor)
{
	if(factor == contentScaleFactor)
		return;
	
	if(renderTarget)
		renderTarget->setContentScaleFactor (factor);

	SuperClass::setContentScaleFactor (factor);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxSkiaLayer::requestFrame (bool deep)
{
	if(deep && !sublayers.isEmpty ())
	{
		for(LinuxSkiaLayer* layer : iterate_as<LinuxSkiaLayer> (sublayers))
			layer->requestFrame (true);
		return;
	}

	if(getSurface () != nullptr && !renderTarget.isValid ())
	{
		Surface* parentSurface = getParentSurface ();
		if(parentSurface && parentSurface->getWaylandSurface () != nullptr)
			show ();
	}

	if(renderTarget)
		frameRequested = true;
	LinuxSkiaLayer* parentLayer = unknown_cast<LinuxSkiaLayer> (getParentLayer ());
	if(parentLayer)
		parentLayer->requestFrame ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxSkiaLayer::hidePendingSublayers ()
{
	ForEach (removedSublayers, LinuxSkiaLayer, subLayer)
		subLayer->hide ();
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API LinuxSkiaLayer::flush ()
{
	hidePendingSublayers ();
	removePendingSublayers ();

	frameRequested = false;

	for(LinuxSkiaLayer* layer : iterate_as<LinuxSkiaLayer> (sublayers))
	{
		if(layer->frameRequested)
			layer->flush ();
	}
		
	if(getSurface () != nullptr && !renderTarget.isValid ())
		show ();

	if(contentRect.isEmpty ())
		return kResultFailed;

	if(!renderTarget.isValid ())
		return kResultFailed;

	checkSize ();

	if(!contentRect.isEmpty ())
	{
		drawContent ();
		renderTarget->onPresent ();
	}

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API LinuxSkiaLayer::addSublayer (IGraphicsLayer* layer)
{
	tresult result = SuperClass::addSublayer (layer);

	LinuxSkiaLayer* skiaLayer = unknown_cast<LinuxSkiaLayer> (layer);

	if(skiaLayer != nullptr)
	{
		result = skiaLayer->show ();

		if(skiaLayer->subSurface != nullptr)
		{
			if(subSurface != nullptr)
				skiaLayer->subSurface->placeAbove (*subSurface);
			for(LinuxSkiaLayer* sibling : iterate_as<LinuxSkiaLayer> (sublayers))
			{
				if(sibling == skiaLayer)
					continue;
				if(sibling->subSurface != nullptr)
					skiaLayer->subSurface->placeAbove (*sibling->subSurface);
			}
		}
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API LinuxSkiaLayer::removeSublayer (IGraphicsLayer* layer)
{
	tresult result = SuperClass::removeSublayer (layer);
	requestCanvasUpdate ();
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxSkiaLayer::drawContent ()
{
	if(dirtyRect.isEmpty () && !needCanvasUpdate)
		return;
	
	if(!renderTarget.isValid () || renderTarget->getSkiaRenderTarget () == nullptr)
		return;

	if(!dirtyRect.isEmpty ())
	{
		if(Bitmap* bitmap = unknown_cast<Bitmap> (content))
		{
			if(contentBitmap != bitmap)
				contentBitmap = return_shared (bitmap);
		}
		else
		{
			if(contentBitmap == nullptr)
			{
				dirtyRect = contentRect.getSize ();
				contentBitmap = NEW Bitmap (contentRect.getWidth (), contentRect.getHeight (), get_flag<int> (mode, kIgnoreAlpha) ? Bitmap::kRGB : Bitmap::kRGBAlpha, contentScaleFactor);
			}
			BitmapGraphicsDevice graphics (contentBitmap);
			if(!graphics.isNullDevice ())
			{
				if(get_flag<int> (mode, kIgnoreAlpha))
					graphics.fillRect (dirtyRect, SolidBrush (Colors::kBlack));
				else
					graphics.clearRect (dirtyRect);
				graphics.addClip (dirtyRect);
				
				UnknownPtr<IGraphicsLayerContent> layerContent (content);
				if(layerContent)
				{
					IGraphicsLayerContent::LayerHint layerHint = layerContent->getLayerHint ();
					if(layerHint == kGraphicsContentEmpty)
						return;
					layerContent->drawLayer (graphics, UpdateRgn (dirtyRect));
				}
			}
		}
	}
	
	if(contentBitmap != nullptr)
	{
		renderTarget->onRender ();
		
		SkiaScopedGraphicsDevice nativeDevice (*renderTarget->getSkiaRenderTarget (), *renderTarget->asUnknown ());
		
		GraphicsDevice graphics;
		graphics.setNativeDevice (&nativeDevice);

		Rect clipRect (getClipRect ());

		Rect updateRect = contentRect.getSize ();
		graphics.clearRect (updateRect);
		graphics.fillRect (updateRect, SolidBrush (backColor));
		
		nativeDevice.getCanvas ()->save ();
		
		nativeDevice.getCanvas ()->translate (-renderTarget->getSize ().left, -renderTarget->getSize ().top);

		#if DEBUG_DRAW_LAYERS
		graphics.drawRect (clipRect, Pen (Colors::kRed));
		#else
		nativeDevice.addClip (clipRect);
		#endif
		if(opacity < 1)
		{
			SkPaint alpha;
			alpha.setAlphaf (opacity);
			nativeDevice.getCanvas ()->saveLayer (nullptr, &alpha);
		}
		
		if(!get_flag<int> (mode, kClipToBounds))
			nativeDevice.getCanvas ()->translate (offset.x, offset.y);

		nativeDevice.getCanvas ()->concat (transformMatrix);

		graphics.drawImage (contentBitmap, updateRect, updateRect);
		#if DEBUG_DRAW_LAYERS
		graphics.drawRect (updateRect, Pen (Colors::kGreen));
		#endif

		nativeDevice.getCanvas ()->restore ();

		#if DEBUG_DRAW_LAYERS
		graphics.drawRect (renderTarget->getSize ().getSize (), Pen (Colors::kYellow));
		#endif
	}
	
	dirtyRect.setReallyEmpty ();
	needCanvasUpdate = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxSkiaLayer::checkSize ()
{
	if(!renderTarget.isValid ())
		return;
	
	size = getClipRect ();
	if(size != renderTarget->getSize ())
	{
		if(contentRect.isEmpty ())
			subSurface->setPosition ({0, 0});
		else
			updatePosition ();
		
		renderTarget->resize (size);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxSkiaLayer::updatePosition ()
{
	if(subSurface)
	{
		if(get_flag<int> (mode, kClipToBounds))
			subSurface->setPosition (size.getLeftTop () + offset);
		else
			subSurface->setPosition (size.getLeftTop ());
	}
}

//************************************************************************************************
// SkiaRootLayer
//************************************************************************************************

DEFINE_CLASS_HIDDEN (SkiaRootLayer, LinuxSkiaLayer)

//////////////////////////////////////////////////////////////////////////////////////////////////

SkiaRootLayer::SkiaRootLayer ()
: window (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkiaRootLayer::~SkiaRootLayer ()
{
	if(window)
		window->removeObserver (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaRootLayer::setContent (IUnknown* content)
{
	if(window)
		window->removeObserver (this);
	window = unknown_cast<Window> (content);
	if(window)
		window->addObserver (this);
		
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

void CCL_API SkiaRootLayer::notify (ISubject* subject, MessageRef msg)
{
	if(subject == window)
	{
		if(msg == IWindow::kSystemWindowChanged)
			requestFrame (true);
	}
	SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Surface* SkiaRootLayer::getParentSurface ()
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
	
Surface* SkiaRootLayer::getSurface ()
{
	ASSERT (window)
	if(!window)
		return nullptr;
	
	return LinuxWindow::cast (window);
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
