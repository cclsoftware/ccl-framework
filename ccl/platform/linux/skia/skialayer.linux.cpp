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

#include "ccl/platform/shared/skia/skiadevice.h"

#include "ccl/gui/graphics/imaging/offscreen.h"
#include "ccl/gui/system/animation.h"

namespace CCL {
namespace Linux {

//************************************************************************************************
// SkiaLayer
//************************************************************************************************

class SkiaLayer: public NativeGraphicsLayer,
				 public WaylandObject
{
public:
	DECLARE_CLASS (SkiaLayer, NativeGraphicsLayer)
	
	SkiaLayer ();
	~SkiaLayer ();
	
	PROPERTY_BOOL (frameRequested, FrameRequested)

	// NativeGraphicsLayer
	tresult CCL_API construct (IUnknown* content, RectRef bounds = Rect (), int mode = 0, float contentScaleFactor = 1.f) override;
	tresult CCL_API setContent (IUnknown* content) override;
	tresult CCL_API placeAbove (IGraphicsLayer* layer, IGraphicsLayer* sibling) override;
	tresult CCL_API placeBelow (IGraphicsLayer* layer, IGraphicsLayer* sibling) override;
	void CCL_API setSize (Coord width, Coord height) override;
	void CCL_API setContentScaleFactor (float factor) override;
	void CCL_API setUpdateNeeded () override;
	void CCL_API setUpdateNeeded (RectRef rect) override;
	void CCL_API suspendTiling (tbool suspend, const Rect* visibleRect) override;
	tresult CCL_API flush () override;
	void CCL_API setOffset (PointRef offset) override;
	void CCL_API setOffsetX (float offsetX) override;
	void CCL_API setOffsetY (float offsetY) override;
	void CCL_API setMode (int mode) override;
	void CCL_API setOpacity (float opacity) override;
	void CCL_API setTransform (TransformRef transform) override;
	tresult CCL_API addSublayer (IGraphicsLayer* layer) override;
	tresult CCL_API removeSublayer (IGraphicsLayer* layer) override;
	tresult CCL_API addAnimation (StringID propertyId, const IAnimation* animation) override;
	tresult CCL_API removeAnimation (StringID propertyId) override;
	tbool CCL_API getPresentationProperty (Variant& value, StringID propertyId) const override;
	void CCL_API setBackColor (const Color& color) override;
	
	// WaylandObject
	void onCompositorDisconnected () override;
	void onCompositorConnected () override;
	
protected:
	SharedPtr<IUnknown> content;
	AutoPtr<LinuxLayerRenderTarget> renderTarget;
	SubSurface<>* subSurface;
	
	AutoPtr<Bitmap> contentBitmap;
	Color backColor;
	
	int mode;
	
	Rect size;
	Rect contentRect;
	Rect dirtyRect;
	Point offset;
	float contentScaleFactor;
	float opacity;
	SkMatrix transformMatrix;
	bool needCanvasUpdate;
	
	tresult show ();
	void hide ();
	virtual Surface* getSurface ();
	virtual Surface* getParentSurface ();
	void drawContent ();
	virtual Rect getClipRect ();
	void checkSize ();
	void updatePosition ();
	void destroySurface ();
	void requestCanvasUpdate ();
	virtual void requestFrame (bool deep = false);
	void hidePendingSublayers ();
};

//************************************************************************************************
// SkiaRootLayer
//************************************************************************************************

class SkiaRootLayer: public SkiaLayer
{
public:
    DECLARE_CLASS (SkiaRootLayer, SkiaLayer)

	SkiaRootLayer ();
	~SkiaRootLayer ();
	
    // SkiaLayer
	tresult CCL_API construct (IUnknown* content, RectRef bounds = Rect (), int mode = 0, float contentScaleFactor = 1.f) override;
	tresult CCL_API setContent (IUnknown* content) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	
protected:
	Window* window;

	// SkiaLayer
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
		return NEW SkiaLayer;
	if(classID == ClassID::TiledLayer)
		return NEW SkiaLayer; // not yet implemented
	return nullptr;
}

//************************************************************************************************
// SkiaLayer
//************************************************************************************************

DEFINE_CLASS_HIDDEN (SkiaLayer, NativeGraphicsLayer)

//////////////////////////////////////////////////////////////////////////////////////////////////

SkiaLayer::SkiaLayer ()
: contentScaleFactor (1.f),
  subSurface (nullptr),
  opacity (1.f),
  needCanvasUpdate (false),
  backColor (Colors::kTransparentBlack),
  frameRequested (false)
{
	setDeferredRemoval (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkiaLayer::~SkiaLayer ()
{
	hide ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkiaLayer::destroySurface ()
{
	renderTarget.release ();
	subSurface->destroySurface ();
	delete subSurface;
	subSurface = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Surface* SkiaLayer::getSurface ()
{
	return subSurface;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Surface* SkiaLayer::getParentSurface ()
{
	SkiaLayer* parentLayer = unknown_cast<SkiaLayer> (getParentLayer ());
	if(parentLayer == nullptr)
		return nullptr;
	
	return parentLayer->getSurface ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaLayer::construct (IUnknown* newContent, RectRef bounds, int newMode, float factor)
{
	mode = newMode;
	content = newContent;
	contentScaleFactor = factor;
	contentRect.setSize (bounds.getSize ());
	offset = bounds.getLeftTop ();
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult SkiaLayer::show ()
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

			for(SkiaLayer* layer : iterate_as<SkiaLayer> (sublayers))
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

void SkiaLayer::hide ()
{
	WaylandClient::instance ().unregisterObject (*this);
	
	for(SkiaLayer* layer : iterate_as<SkiaLayer> (sublayers))
		layer->hide ();

	renderTarget.release ();
	
	if(subSurface)
		destroySurface ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkiaLayer::onCompositorDisconnected ()
{
	hide ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkiaLayer::onCompositorConnected ()
{
	show ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaLayer::setContent (IUnknown* newContent)
{
	content = newContent;
	UnknownPtr<IGraphicsLayerContent> layerContent (content);
	if(layerContent != nullptr)
		setUpdateNeeded ();
	else
	{
		dirtyRect = contentRect.getSize ();
		requestCanvasUpdate ();
	}

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaLayer::placeAbove (IGraphicsLayer* _layer, IGraphicsLayer* _sibling)
{
	tresult result = SuperClass::placeAbove (_layer, _sibling);
	if(result == kResultOk)
	{
		SkiaLayer* layer = unknown_cast<SkiaLayer> (_layer);
		SkiaLayer* sibling = unknown_cast<SkiaLayer> (_sibling);
		if(layer == nullptr || sibling == nullptr)
			return kResultInvalidArgument;

		if(layer->subSurface && sibling->getSurface ())
			layer->subSurface->placeAbove (*sibling->getSurface ());
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaLayer::placeBelow (IGraphicsLayer* _layer, IGraphicsLayer* _sibling)
{
	tresult result = SuperClass::placeBelow (_layer, _sibling);
	if(result == kResultOk)
	{
		SkiaLayer* layer = unknown_cast<SkiaLayer> (_layer);
		SkiaLayer* sibling = unknown_cast<SkiaLayer> (_sibling);
		if(layer == nullptr || sibling == nullptr)
			return kResultInvalidArgument;

		if(layer->subSurface && sibling->getSurface ())
			layer->subSurface->placeBelow (*sibling->getSurface ());
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SkiaLayer::setSize (Coord width, Coord height)
{
	if(width == contentRect.getWidth () && height == contentRect.getHeight ())
		return;
	
	contentRect.setSize (Point (width, height));
	checkSize ();
	for(SkiaLayer* layer : iterate_as<SkiaLayer> (sublayers))
		layer->checkSize ();
	requestCanvasUpdate ();
	
	if(unknown_cast<Bitmap> (content) == nullptr)
		contentBitmap.release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SkiaLayer::setContentScaleFactor (float factor)
{
	if(factor == contentScaleFactor)
		return;
	contentScaleFactor = factor;
	
	if(renderTarget)
		renderTarget->setContentScaleFactor (factor);

	if(unknown_cast<Bitmap> (content) == nullptr)
		contentBitmap.release ();

	setUpdateNeeded ();

	for(SkiaLayer* layer : iterate_as<SkiaLayer> (sublayers))
		layer->setContentScaleFactor (contentScaleFactor);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SkiaLayer::setUpdateNeeded ()
{
	setUpdateNeeded (contentRect.getSize ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SkiaLayer::setUpdateNeeded (RectRef rect)
{
	if(!unknown_cast<Image> (content))
		dirtyRect.join (rect);
	requestFrame ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkiaLayer::requestCanvasUpdate ()
{
	needCanvasUpdate = true;
	requestFrame ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkiaLayer::requestFrame (bool deep)
{
	if(deep && !sublayers.isEmpty ())
	{
		for(SkiaLayer* layer : iterate_as<SkiaLayer> (sublayers))
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
	SkiaLayer* parentLayer = unknown_cast<SkiaLayer> (getParentLayer ());
	if(parentLayer)
		parentLayer->requestFrame ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SkiaLayer::suspendTiling (tbool suspend, const Rect* visibleRect)
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkiaLayer::hidePendingSublayers ()
{
	ForEach (removedSublayers, SkiaLayer, subLayer)
		subLayer->hide ();
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaLayer::flush ()
{
	hidePendingSublayers ();
	removePendingSublayers ();

	frameRequested = false;

	for(SkiaLayer* layer : iterate_as<SkiaLayer> (sublayers))
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

void CCL_API SkiaLayer::setOffset (PointRef newOffset)
{
	offset = newOffset;
	updatePosition ();
	requestCanvasUpdate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SkiaLayer::setOffsetX (float offsetX)
{
	offset.x = offsetX;
	updatePosition ();
	requestCanvasUpdate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SkiaLayer::setOffsetY (float offsetY)
{
	offset.y = offsetY;
	updatePosition ();
	requestCanvasUpdate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SkiaLayer::setMode (int newMode)
{
	mode = newMode;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SkiaLayer::setOpacity (float newOpacity)
{
	if(opacity != newOpacity)
	{
		opacity = newOpacity;
		requestCanvasUpdate ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SkiaLayer::setTransform (TransformRef t)
{
	transformMatrix = SkMatrix::MakeAll (t.a0, t.b0, t.t0, t.a1, t.b1, t.t1, 0, 0, 1);
	requestCanvasUpdate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaLayer::addSublayer (IGraphicsLayer* layer)
{
	tresult result = SuperClass::addSublayer (layer);

	SkiaLayer* skiaLayer = unknown_cast<SkiaLayer> (layer);

	if(skiaLayer != nullptr)
	{
		result = skiaLayer->show ();

		if(skiaLayer->subSurface != nullptr)
		{
			if(subSurface != nullptr)
				skiaLayer->subSurface->placeAbove (*subSurface);
			for(SkiaLayer* sibling : iterate_as<SkiaLayer> (sublayers))
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

tresult CCL_API SkiaLayer::removeSublayer (IGraphicsLayer* layer)
{
	tresult result = SuperClass::removeSublayer (layer);
	requestCanvasUpdate ();
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
	
tresult CCL_API SkiaLayer::addAnimation (StringID propertyId, const IAnimation* newAnimation)
{
	tresult result = kResultInvalidArgument;

	const Animation* animation = Animation::cast<Animation> (newAnimation);
	if(!animation)
		return result;
	
	AutoPtr<Animation> animationCopy = ccl_cast<Animation> (animation->clone ());
	if(animationCopy)
		result = AnimationManager::instance ().addAnimation (this, propertyId, animationCopy);
	
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaLayer::removeAnimation (StringID propertyId)
{
	tresult result = AnimationManager::instance ().removeAnimation (this, propertyId);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SkiaLayer::getPresentationProperty (Variant& value, StringID propertyId) const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SkiaLayer::setBackColor (const Color& color)
{
	backColor = color;
	requestCanvasUpdate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkiaLayer::drawContent ()
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

Rect SkiaLayer::getClipRect ()
{
	SkiaLayer* parentLayer = unknown_cast<SkiaLayer> (getParentLayer ());
	if(parentLayer == nullptr)
		return contentRect.getSize ();

	Rect clipRect;
	if(get_flag<int> (mode, kClipToBounds))
	{
		clipRect = contentRect;
		clipRect.offset (offset.x, offset.y);
		clipRect.bound (parentLayer->getClipRect ());
		clipRect.offset (-offset.x, -offset.y);
	}
	else
		clipRect = parentLayer->getClipRect ();
	return clipRect;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkiaLayer::checkSize ()
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

void SkiaLayer::updatePosition ()
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

DEFINE_CLASS_HIDDEN (SkiaRootLayer, SkiaLayer)

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
