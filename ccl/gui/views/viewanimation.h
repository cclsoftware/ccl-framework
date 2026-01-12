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
// Filename    : ccl/gui/views/viewanimation.h
// Description : View Animation
//
//************************************************************************************************

#ifndef _ccl_viewanimation_h
#define _ccl_viewanimation_h

#include "ccl/base/singleton.h"
#include "ccl/base/collections/objectlist.h"

#include "ccl/public/gui/framework/styleflags.h"
#include "ccl/public/gui/framework/ianimation.h"
#include "ccl/public/gui/framework/iviewanimation.h"
#include "ccl/public/gui/graphics/igraphicslayer.h"
#include "ccl/public/gui/graphics/iimage.h"

namespace CCL {

class View;

//************************************************************************************************
// ViewScreenCapture
//************************************************************************************************

class ViewScreenCapture: public Object,
						 public IViewScreenCapture
{
public:
	DECLARE_CLASS (ViewScreenCapture, Object)

	// IViewScreenCapture
	IImage* CCL_API takeScreenshot (IView* view, const Rect* rect = nullptr, int options = 0) override;

	CLASS_INTERFACE (IViewScreenCapture, Object)
};

//************************************************************************************************
// ViewAnimationHandler
//************************************************************************************************

class ViewAnimationHandler: public Object,
							public IAnimationHandler,
							public StaticSingleton<ViewAnimationHandler>
{
public:
	void registerSelf (bool state);

	// IAnimationHandler
	tresult CCL_API addAnimation (IObject* target, StringID propertyId, const IAnimation* prototype) override;
	tresult CCL_API removeAnimation (IObject* target, StringID propertyId) override;

	CLASS_INTERFACE (IAnimationHandler, Object)

protected:
	static bool isLayerProperty (StringID propertyId);
};

//************************************************************************************************
// AnimatorBase
//************************************************************************************************

class AnimatorBase: public Object
{
protected:
	PROPERTY_VARIABLE (double, duration, Duration)
	PROPERTY_VARIABLE (AnimationTimingType, timingType, TimingType)

protected:
	AnimatorBase (View* parentView, float contentScaleFactor);
	
	View* parentView;
	float contentScaleFactor;
	IGraphicsLayer* parentLayer;
	AutoPtr<IGraphicsLayer> clippingLayer;
	
	class LayerAdder;
	class LayerRemover;
	IGraphicsLayer* addClippingLayer (IGraphicsLayer* parentLayer, PointRef offset, Coord width, Coord height);
	IGraphicsLayer* createLayerForImage (IImage* image, int mode = IGraphicsLayer::kIgnoreAlpha);
};

//************************************************************************************************
// ViewAnimator
//************************************************************************************************

class ViewAnimator: public AnimatorBase,
					public IViewAnimator
{
public:
	DECLARE_CLASS_ABSTRACT (ViewAnimator, Object)

	static ViewAnimator* create (View* parentView, TransitionType transitionType);
	static TransitionType getInverseTransition (TransitionType transitionType);
	
	DECLARE_STYLEDEF (transitionTypes)

	PROPERTY_SHARED_AUTO (IImage, fromImage, FromImage)
	PROPERTY_SHARED_AUTO (IImage, toImage, ToImage)
	PROPERTY_SHARED_AUTO (IAnimationCompletionHandler, externalHandler, CompletionHandler)

	PROPERTY_BOOL (ignoreAlpha, IgnoreAlpha) // default: true (for optimization); set to false if images are not fully opaque

	PROPERTY_OBJECT (Rect, fromRect, FromRect)

	void snipFromView (View* fromView, const Rect* rect = nullptr);
	void snipToView (View* toView,  const Rect* rect = nullptr);

	bool makeTransition ();
	bool isFromLayerAnimationOnly () const;
	
	// IViewAnimator
	tbool CCL_API setTransitionProperty (StringID propertyId, VariantRef value) override;

	CLASS_INTERFACE (IViewAnimator, Object)

protected:
	ViewAnimator (View* parentView, int transitionType, float contentScaleFactor);

	TransitionType transitionType;
	AutoPtr<IGraphicsLayer> fromLayer;
	AutoPtr<IGraphicsLayer> toLayer;
	Point toLayerOffset;
	Point fromLayerSize;
	IGraphicsLayer* fromLayerParent;
	
	bool isPrepared () const;
	bool prepare ();

	// IObject
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
};

//************************************************************************************************
// LayoutAnimator
//************************************************************************************************

class LayoutAnimator: public AnimatorBase
{
public:
	static LayoutAnimator* create (View* parentView);
			
	void snipOldViews ();
	void snipNewViews ();
	bool makeTransition ();

private:
	ObjectList items;

	struct ViewItem;
	
	LayoutAnimator (View* parentView, float contentScaleFactor);
};

} // namespace CCL

#endif // _ccl_viewanimation_h
