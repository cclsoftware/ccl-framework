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
// Filename    : ccl/platform/shared/skia/skialayer.cpp
// Description : Graphics Layer for Skia content
//
//************************************************************************************************

#include "ccl/platform/shared/skia/skialayer.h"

#include "ccl/gui/system/animation.h"

using namespace CCL;

//************************************************************************************************
// SkiaLayer
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (SkiaLayer, NativeGraphicsLayer)

//////////////////////////////////////////////////////////////////////////////////////////////////

SkiaLayer::SkiaLayer ()
: contentScaleFactor (1.f),
  opacity (1.f),
  needCanvasUpdate (false),
  backColor (Colors::kTransparentBlack)
{
	setDeferredRemoval (true);
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

void CCL_API SkiaLayer::suspendTiling (tbool suspend, const Rect* visibleRect)
{
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
