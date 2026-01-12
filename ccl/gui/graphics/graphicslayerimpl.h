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
// Filename    : ccl/gui/graphics/graphicslayerimpl.h
// Description : Graphics Layer implementation helper
//
//************************************************************************************************

#ifndef _ccl_graphicslayerimpl_h
#define _ccl_graphicslayerimpl_h

#include "ccl/gui/graphics/nativegraphics.h"

#include "ccl/gui/system/animation.h"

#include "ccl/public/gui/framework/itimer.h"

namespace CCL {

class GraphicsLayer;

//************************************************************************************************
// GraphicsLayerEngine
//************************************************************************************************

class GraphicsLayerEngine: public Object,
						   public ITimerTask
{
public:
	DECLARE_CLASS_ABSTRACT (GraphicsLayerEngine, Object)

	GraphicsLayerEngine ();
	~GraphicsLayerEngine ();

	void addRootLayer (GraphicsLayer* rootLayer);
	void removeRootLayer (GraphicsLayer* rootLayer);

	PROPERTY_BOOL (flushNeeded, FlushNeeded)
	virtual void flush (bool force = false) = 0;
	virtual double getNextEstimatedFrameTime () const = 0;

	void addAnimation (GraphicsLayer* layer, const Animation* animation, int propertyIntId);
	bool removeAnimation (GraphicsLayer* layer, int propertyIntId);
	void removeAnimations (GraphicsLayer* layer);
	void removeAnimations ();

	CLASS_INTERFACE (ITimerTask, Object)

protected:
	ObjectList rootLayers;
	ObjectList activeAnimations;

	struct AnimationEntry: Object
	{
		GraphicsLayer* layer;
		int propertyIntId;
		AutoPtr<Animation> animation;
		double startTime;
		double endTime;

		AnimationEntry ()
		: layer (nullptr),
		  propertyIntId (0),
		  startTime (0),
		  endTime (-1.)
		{}				
	};

	void updateAnimations ();
	AnimationEntry* findAnimation (GraphicsLayer* layer, int propertyIntId) const;
	void removeAnimation (AnimationEntry* e);

	// ITimerTask
	void CCL_API onTimer (ITimer* timer) override;
};

//************************************************************************************************
// GraphicsLayerChange
//************************************************************************************************

struct GraphicsLayerChange
{
	enum Type { kBegin, kUpdate, kEnd };

	Type type;
	int propertyIntId;
	Variant value;

	GraphicsLayerChange (Type type, int propertyIntId, VariantRef value)
	: type (type),
	  propertyIntId (propertyIntId),
	  value (value)
	{}
};

//************************************************************************************************
// GraphicsLayer
//************************************************************************************************

class GraphicsLayer: public NativeGraphicsLayer
{
public:
	DECLARE_CLASS_ABSTRACT (GraphicsLayer, NativeGraphicsLayer)

	GraphicsLayer (GraphicsLayerEngine& engine);
	~GraphicsLayer ();

	virtual Rect getBounds () const;
	bool getVisibleClient (Rect& r) const;

	// called by GraphicsLayerEngine:
	virtual void updateAll (bool& updateDone);
	virtual void presentationChanged (const GraphicsLayerChange& change);

	// NativeGraphicsLayer
	void CCL_API setOffset (PointRef offset) override;
	void CCL_API setOffsetX (float offsetX) override;
	void CCL_API setOffsetY (float offsetY) override;
	void CCL_API setOpacity (float opacity) override;
	void CCL_API setTransform (TransformRef transform) override;
	void CCL_API setContentScaleFactor (float factor) override;
	void CCL_API setUpdateNeeded () override;
	void CCL_API setUpdateNeeded (RectRef rect) override;
	tresult CCL_API addSublayer (IGraphicsLayer* layer) override;
	tresult CCL_API removeSublayer (IGraphicsLayer* layer) override;
	tresult CCL_API placeAbove (IGraphicsLayer* layer, IGraphicsLayer* sibling) override;
	tresult CCL_API placeBelow (IGraphicsLayer* layer, IGraphicsLayer* sibling) override;
	tresult CCL_API addAnimation (StringID propertyId, const IAnimation* animation) override;
	tresult CCL_API removeAnimation (StringID propertyId) override;
	tbool CCL_API getPresentationProperty (Variant& value, StringID propertyId) const override;
	tresult CCL_API flush () override;
	void CCL_API suspendTiling (tbool suspend, const Rect* visibleRect) override;

protected:
	enum AnimationFlags
	{
		kAnimateNone = 0,
		kAnimateOffsetX = 1<<0,
		kAnimateOffsetY = 1<<1,
		kAnimateOffset = kAnimateOffsetX|kAnimateOffsetY,
		kAnimateOpacity = 1<<2,
		kAnimateTransform = 1<<3
	};

	struct State
	{
		float offsetX;
		float offsetY;
		float opacity;
		Transform transform;

		State ()
		: offsetX (0.f),
		  offsetY (0.f),
		  opacity (1.f)
		{}

		Point getOffset () const
		{
			return Point (ccl_to_int (offsetX), ccl_to_int (offsetY));
		}
	};

	GraphicsLayerEngine& engine;
	Point size;
	Rect dirtyRect;
	State modelState;
	State presentationState;
	int animationFlags;
	float contentScaleFactor;

	bool isAnimated (int id) const	{ return (animationFlags & id) != 0; }
	bool isUpdateNeeded () const	{ return !dirtyRect.isEmpty (); }
	void setFlushNeeded ()			{ engine.setFlushNeeded (true); }

	virtual void applyProperty (int id) = 0;
	virtual bool applyAnimation (int id, const Animation* animation) = 0;
	virtual void attachSublayer (IGraphicsLayer* layer, bool state, IGraphicsLayer* sibling = nullptr, bool below = false) = 0;
	virtual void updateContent () = 0;

	static int toIntId (StringID propertyId);
	static bool setValue (State& state, int id, VariantRef value);
	static bool getValue (Variant& value, const State& state, int id);
};

} // namespace CCL

#endif // _ccl_graphicslayerimpl_h
