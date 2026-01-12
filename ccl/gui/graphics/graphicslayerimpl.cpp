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
// Filename    : ccl/gui/graphics/graphicslayerimpl.cpp
// Description : Graphics Layer implementation helper
//
//************************************************************************************************

#include "ccl/gui/graphics/graphicslayerimpl.h"

#include "ccl/gui/graphics/graphicshelper.h"

#include "ccl/public/gui/framework/iuserinterface.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/guiservices.h"

using namespace CCL;

//************************************************************************************************
// GraphicsLayerEngine
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (GraphicsLayerEngine, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

GraphicsLayerEngine::GraphicsLayerEngine ()
: flushNeeded (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

GraphicsLayerEngine::~GraphicsLayerEngine ()
{
	ASSERT (rootLayers.isEmpty ())
	ASSERT (activeAnimations.isEmpty ())
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GraphicsLayerEngine::addRootLayer (GraphicsLayer* rootLayer)
{
	if(rootLayers.isEmpty ())
		System::GetGUI ().addIdleTask (this);

	rootLayers.add (rootLayer);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GraphicsLayerEngine::removeRootLayer (GraphicsLayer* rootLayer)
{
	rootLayers.remove (rootLayer);

	if(rootLayers.isEmpty ())
		System::GetGUI ().removeIdleTask (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API GraphicsLayerEngine::onTimer (ITimer* timer)
{
	bool updateDone = false;
	ListForEachObject (rootLayers, GraphicsLayer, rootLayer)
		rootLayer->updateAll (updateDone);
	EndFor
	
	if(updateDone || flushNeeded)
		flush ();

	flushNeeded = false;

	// check for animation completion, etc.
	updateAnimations ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GraphicsLayerEngine::addAnimation (GraphicsLayer* layer, const Animation* animation, int propertyIntId)
{
	AnimationEntry* e = NEW AnimationEntry;
	e->layer = layer;
	e->propertyIntId = propertyIntId;
	e->animation = (Animation*)animation->clone ();	
	activeAnimations.add (e);

	double totalTime = animation->getTotalRunningTime ();
	e->startTime = getNextEstimatedFrameTime ();
	if(totalTime != -1.)
		e->endTime = e->startTime + totalTime;

	GraphicsLayerChange change (GraphicsLayerChange::kBegin, e->propertyIntId, e->animation->getFirstValue ());
	e->layer->presentationChanged (change);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GraphicsLayerEngine::removeAnimation (GraphicsLayer* layer, int propertyIntId)
{
	if(AnimationEntry* e = findAnimation (layer, propertyIntId))
	{
		GraphicsLayerChange change (GraphicsLayerChange::kEnd, e->propertyIntId, e->animation->getFinalValue ());
		e->layer->presentationChanged (change);

		removeAnimation (e);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GraphicsLayerEngine::updateAnimations ()
{
	if(!activeAnimations.isEmpty ())
	{
		double now = System::GetProfileTime ();
		ListForEachObject (activeAnimations, AnimationEntry, e)
			if(e->endTime != -1. && now >= e->endTime)
			{				
				GraphicsLayerChange change (GraphicsLayerChange::kEnd, e->propertyIntId, e->animation->getFinalValue ());
				e->layer->presentationChanged (change);

				removeAnimation (e);
			}
			else
			{
				double relativeTime = now - e->startTime;
				GraphicsLayerChange change (GraphicsLayerChange::kUpdate, e->propertyIntId, e->animation->getValueAtTime (relativeTime));
				e->layer->presentationChanged (change);
			}
		EndFor
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GraphicsLayerEngine::removeAnimations (GraphicsLayer* layer)
{
	ListForEachObject (activeAnimations, AnimationEntry, e)
		if(e->layer == layer)
			removeAnimation (e);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GraphicsLayerEngine::removeAnimations ()
{
	ForEach (activeAnimations, AnimationEntry, e)
		if(IAnimationCompletionHandler* handler = e->animation->getCompletionHandler ())
			handler->onAnimationFinished ();
		e->release ();
	EndFor
	activeAnimations.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

GraphicsLayerEngine::AnimationEntry* GraphicsLayerEngine::findAnimation (GraphicsLayer* layer, int propertyIntId) const
{
	if(!activeAnimations.isEmpty ())
		ListForEachObject (activeAnimations, AnimationEntry, e)
			if(e->layer == layer && e->propertyIntId == propertyIntId)
				return e;
		EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GraphicsLayerEngine::removeAnimation (AnimationEntry* e)
{
	activeAnimations.remove (e);

	if(IAnimationCompletionHandler* handler = e->animation->getCompletionHandler ())
		handler->onAnimationFinished ();

	e->release ();
}

//************************************************************************************************
// GraphicsLayer
//************************************************************************************************

int GraphicsLayer::toIntId (StringID propertyId)
{
	if(propertyId == kOffsetX)
		return kAnimateOffsetX;
	if(propertyId == kOffsetY)
		return kAnimateOffsetY;
	if(propertyId == kOffset)
		return kAnimateOffset;
	if(propertyId == kOpacity)
		return kAnimateOpacity;
	if(propertyId == kTransform)
		return kAnimateTransform;
	return kAnimateNone;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GraphicsLayer::setValue (State& state, int id, VariantRef value)
{
	switch(id)
	{
	case kAnimateOffsetX :
		state.offsetX = value;
		break;

	case kAnimateOffsetY :
		state.offsetY = value;
		break;

	case kAnimateOffset :
		if(IUIValue* uiValue = IUIValue::toValue (value))
		{
			PointF p = uiValue->convertToPointF ();
			state.offsetX = p.x;
			state.offsetY = p.y;
		}
		break;

	case kAnimateOpacity :
		state.opacity = value;
		break;

	case kAnimateTransform :
		if(IUIValue* uiValue = IUIValue::toValue (value))
			uiValue->toTransform (state.transform);
		break;

	default :
		CCL_DEBUGGER ("Layer property not found!\n")
		return false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GraphicsLayer::getValue (Variant& value, const State& state, int id)
{
	static UIValue uiValue;
	switch(id)
	{
	case kAnimateOffsetX :
		value = state.offsetX;
		break;

	case kAnimateOffsetY : 
		value = state.offsetY;
		break;

	case kAnimateOffset : 
		uiValue.fromPoint (state.getOffset ()); 
		value = uiValue.asUnknown ();
		break;

	case kAnimateOpacity : 
		value = state.opacity;
		break;

	case kAnimateTransform : 
		uiValue.fromTransform (state.transform);
		value = uiValue.asUnknown ();
		break;

	default :
		CCL_DEBUGGER ("Layer property not found!\n")
		return false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_ABSTRACT_HIDDEN (GraphicsLayer, NativeGraphicsLayer)

//////////////////////////////////////////////////////////////////////////////////////////////////

GraphicsLayer::GraphicsLayer (GraphicsLayerEngine& engine)
: engine (engine),
  animationFlags (0),
  contentScaleFactor (1.f)
{
	dirtyRect.setReallyEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

GraphicsLayer::~GraphicsLayer ()
{
	engine.removeAnimations (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect GraphicsLayer::getBounds () const
{
	Rect r (size);

	Point p;
	p.x = ccl_to_int (isAnimated (kAnimateOffsetX) ? presentationState.offsetX : modelState.offsetX);
	p.y = ccl_to_int (isAnimated (kAnimateOffsetY) ? presentationState.offsetY : modelState.offsetY);
	r.offset (p);

	return r;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GraphicsLayer::getVisibleClient (Rect& r) const
{
	GraphicsLayer* p = (GraphicsLayer*)parentLayer;
	if(p == nullptr) // nothing visible
	{
		r.setEmpty ();
		return false;
	}

	// copied from View::getVisibleClient():
	Rect size (getBounds ());
	Coord hoffset = -size.left, voffset = -size.top;
	r = size;
	
	while(p)
	{
		Rect psize = p->getBounds ();
		Coord pw = psize.getWidth ();
		Coord ph = psize.getHeight ();

		if(r.left < 0)
			r.left = 0;
		if(r.top < 0)
			r.top = 0;
		if(r.right > pw)
			r.right = pw;
		if(r.bottom > ph)
			r.bottom = ph;

		if(r.isEmpty ())
			return false;
		
		r.offset (psize.left, psize.top);
		hoffset -= psize.left;
		voffset -= psize.top;

		p = (GraphicsLayer*)p->parentLayer;
	}

	r.offset (hoffset, voffset);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GraphicsLayer::updateAll (bool& updateDone)
{
	if(isUpdateNeeded ())
	{
		updateContent ();
		updateDone = true;
	}

	ListForEachObject (sublayers, GraphicsLayer, layer)
		layer->updateAll (updateDone);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GraphicsLayer::presentationChanged (const GraphicsLayerChange& change)
{
	setValue (presentationState, change.propertyIntId, change.value);

	if(change.type == GraphicsLayerChange::kBegin)
	{
		animationFlags |= change.propertyIntId;
	}
	else if(change.type == GraphicsLayerChange::kEnd)
	{
		animationFlags &= ~change.propertyIntId;
		
		// apply model value when animated finished
		applyProperty (change.propertyIntId);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API GraphicsLayer::setOffset (PointRef offset)
{
	modelState.offsetX = (float)offset.x;
	modelState.offsetY = (float)offset.y;

	if(!(isAnimated (kAnimateOffsetX) || isAnimated (kAnimateOffsetY)))
	{
		applyProperty (kAnimateOffset);
		setFlushNeeded ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API GraphicsLayer::setOffsetX (float offsetX)
{
	modelState.offsetX = offsetX;

	if(!isAnimated (kAnimateOffsetX))
	{
		applyProperty (kAnimateOffsetX);
		setFlushNeeded ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API GraphicsLayer::setOffsetY (float offsetY)
{
	modelState.offsetY = offsetY;

	if(!isAnimated (kAnimateOffsetY))
	{
		applyProperty (kAnimateOffsetY);
		setFlushNeeded ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API GraphicsLayer::setOpacity (float opacity)
{
	modelState.opacity = opacity;

	if(!isAnimated (kAnimateOpacity))
	{
		applyProperty (kAnimateOpacity);
		setFlushNeeded ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API GraphicsLayer::setTransform (TransformRef transform)
{
	modelState.transform = transform;

	if(!isAnimated (kAnimateTransform))
	{
		applyProperty (kAnimateTransform);
		setFlushNeeded ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API GraphicsLayer::setContentScaleFactor (float factor)
{
	contentScaleFactor = factor;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API GraphicsLayer::setUpdateNeeded ()
{
	setUpdateNeeded (Rect (0, 0, size.x, size.y));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API GraphicsLayer::setUpdateNeeded (RectRef _rect)
{
	Rect rect (_rect);
	rect.bound (Rect (0, 0, size.x, size.y));
	dirtyRect.join (rect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GraphicsLayer::addSublayer (IGraphicsLayer* layer)
{
	tresult tr = SuperClass::addSublayer (layer);
	if(tr == kResultOk)
	{
		attachSublayer (layer, true);
		setFlushNeeded ();
	}
	return tr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GraphicsLayer::removeSublayer (IGraphicsLayer* layer)
{
	tresult tr = SuperClass::removeSublayer (layer);
	if(tr == kResultOk)
	{
		attachSublayer (layer, false);
		setFlushNeeded ();
	}
	return tr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GraphicsLayer::placeAbove (IGraphicsLayer* layer, IGraphicsLayer* sibling)
{	
	tresult tr = SuperClass::placeAbove (layer, sibling);
	if(tr == kResultOk)
	{
		attachSublayer (layer, false);
		attachSublayer (layer, true, sibling, false);
		setFlushNeeded ();
	}
	return tr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GraphicsLayer::placeBelow (IGraphicsLayer* layer, IGraphicsLayer* sibling)
{
	tresult tr = SuperClass::placeBelow (layer, sibling);
	if(tr == kResultOk)
	{
		attachSublayer (layer, false);
		attachSublayer (layer, true, sibling, true);
		setFlushNeeded ();
	}
	return tr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GraphicsLayer::addAnimation (StringID propertyId, const IAnimation* _animation)
{
	int id = toIntId (propertyId);
	const Animation* animation = Animation::cast<Animation> (_animation);
	ASSERT (id != kAnimateNone && animation != nullptr)

	if(id != kAnimateNone && animation)
		if(applyAnimation (id, animation))
		{
			engine.addAnimation (this, animation, id);
			return kResultOk;
		}

	return kResultInvalidArgument;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GraphicsLayer::removeAnimation (StringID propertyId)
{
	int id = toIntId (propertyId);
	ASSERT (id != kAnimateNone)
	if(id == kAnimateNone)
		return kResultInvalidArgument;

	return engine.removeAnimation (this, id) ? kResultOk : kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API GraphicsLayer::getPresentationProperty (Variant& value, StringID propertyId) const
{
	int id = toIntId (propertyId);
	if(id == kAnimateNone)
		return false;

	return getValue (value, isAnimated (id) ? presentationState : modelState, id);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GraphicsLayer::flush ()
{
	bool unused = false;
	updateAll (unused);

	engine.flush (true);

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API GraphicsLayer::suspendTiling (tbool suspend, const Rect* visibleRect)
{
	// nothing here
}
