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
// Filename    : ccl/gui/views/viewanimation.cpp
// Description : View Animation
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/views/viewanimation.h"

#include "ccl/gui/views/view.h"
#include "ccl/gui/windows/window.h"
#include "ccl/gui/system/animation.h"
#include "ccl/gui/graphics/nativegraphics.h"
#include "ccl/gui/graphics/graphicshelper.h"
#include "ccl/gui/graphics/imaging/bitmap.h"

#include "ccl/public/gui/graphics/dpiscale.h"

namespace CCL {

//************************************************************************************************
// AnimatorBase::LayerAdder
//************************************************************************************************

class AnimatorBase::LayerAdder: public Object,
								public IAnimationCompletionHandler
{
public:
	LayerAdder (IGraphicsLayer* parentLayer, IGraphicsLayer* subLayer, PointRef offset)
	: parentLayer (parentLayer),
	  subLayer (subLayer),
	  offset (offset)
	{
		if(parentLayer && subLayer)
			nextSibling = parentLayer->getNextSibling (subLayer);
	}

	static AutoPtr<IAnimationCompletionHandler> create (IGraphicsLayer* parentLayer, IGraphicsLayer* subLayer, PointRef offset)
	{
		AutoPtr<IAnimationCompletionHandler> handler = NEW LayerAdder (parentLayer, subLayer, offset);
		return handler;
	}
	
	// IAnimationCompletionHandler
	void CCL_API onAnimationFinished () override
	{
		if(subLayer)
		{
			// remove from current parent first
			if(IGraphicsLayer* currentParent = subLayer->getParentLayer ())
				currentParent->removeSublayer (subLayer);

			subLayer->setTransform (Transform ());
			subLayer->setOffset (offset);

			if(parentLayer)
			{
				parentLayer->addSublayer (subLayer);
				if(nextSibling)
					parentLayer->placeBelow (subLayer, nextSibling);
			}
		}
	}
	
	CLASS_INTERFACE (IAnimationCompletionHandler, Object)
	
protected:
	SharedPtr<IGraphicsLayer> parentLayer;
	SharedPtr<IGraphicsLayer> subLayer;
	SharedPtr<IGraphicsLayer> nextSibling;
	Point offset;
};

//************************************************************************************************
// AnimatorBase::LayerRemover
//************************************************************************************************

class AnimatorBase::LayerRemover: public Object,
								  public IAnimationCompletionHandler
{
public:
	LayerRemover (IGraphicsLayer* parentLayer, IGraphicsLayer* subLayer)
	: parentLayer (parentLayer),
	  subLayer (subLayer)
	{}

	~LayerRemover ()
	{
		CCL_PRINTLN ("LayerRemover dtor")
	}

	static AutoPtr<IAnimationCompletionHandler> create (IGraphicsLayer* parentLayer, IGraphicsLayer* subLayer)
	{
		AutoPtr<IAnimationCompletionHandler> handler = NEW LayerRemover (parentLayer, subLayer);
		return handler;
	}

	// IAnimationCompletionHandler
	void CCL_API onAnimationFinished () override
	{
		CCL_PRINTLN ("Animation finished")

		if(parentLayer && subLayer)
			parentLayer->removeSublayer (subLayer);
	}

	CLASS_INTERFACE (IAnimationCompletionHandler, Object)

protected:
	SharedPtr<IGraphicsLayer> parentLayer;
	SharedPtr<IGraphicsLayer> subLayer;
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// ViewScreenCapture
//************************************************************************************************

DEFINE_CLASS (ViewScreenCapture, Object)
DEFINE_CLASS_UID (ViewScreenCapture, 0xe0c3509e, 0x2e5c, 0x4d75, 0xae, 0xc9, 0x29, 0x33, 0x65, 0x48, 0x96, 0xb)

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* CCL_API ViewScreenCapture::takeScreenshot (IView* _view, const Rect* inRect, int options)
{
	View* view = unknown_cast<View> (_view);
	ASSERT (view)
	if(!view)
		return nullptr;

	Rect r;
	if(inRect)
		r = *inRect;
	else
		view->getVisibleClient (r);
	if(r.isEmpty ())
		return nullptr;

	Window* window = view->getWindow ();
	float scaleFactor = window ? window->getContentScaleFactor () : 1.f;

	if(!DpiScale::isIntAligned (scaleFactor))
	{
		// copy one pixel more if size is not aligned on physical pixels
		PixelPointF pixelSizeF (Point (r.getWidth (), r.getHeight ()), scaleFactor);
		if(!DpiScale::isIntAligned (pixelSizeF.x))
			r.right++;
		if(!DpiScale::isIntAligned (pixelSizeF.y))
			r.bottom++;
	}

	bool opaque = view->getStyle ().isOpaque ();
	bool platformMode = (options & kPlatformMode) != 0;
	Bitmap* bitmap = NEW Bitmap (r.getWidth (), r.getHeight (), opaque ? Bitmap::kRGB : Bitmap::kRGBAlpha, scaleFactor);

	auto renderTransparentView = [view](RectRef r, Bitmap* bitmap)
	{
		BitmapGraphicsDevice device (bitmap);
		device.clearRect (r);
		view->renderTo (device, UpdateRgn (r));
	};

	if(window)
	{
		Point offset;
		view->clientToWindow (offset);
		r.offset (offset);

		if(platformMode == true)
		{
			NativeBitmap* nativeBitmap = NativeGraphicsEngine::instance ().createScreenshotFromWindow (window);
			if(nativeBitmap)
			{
				Bitmap windowBitmap (nativeBitmap); // takes ownership!
				BitmapGraphicsDevice device (bitmap);
				device.drawImage (&windowBitmap, r, Rect (0, 0, r.getWidth (), r.getHeight ()));
			}
			else // failed :-(
				safe_release (bitmap);
		}
		else
		{
			if(opaque)
			{
				BitmapGraphicsDevice device (bitmap);
				window->renderTo (device, UpdateRgn (r), Point (-r.left, -r.top));
			}
			else
				renderTransparentView (r, bitmap);
		}
	}
	else // view is not attached to a window
	{		
		ASSERT (platformMode == false)

		if(opaque)
		{
			BitmapGraphicsDevice device (bitmap);
			device.fillRect (r, view->getVisualStyle ().getBackBrush ()); // clear background
			view->renderTo (device, UpdateRgn (r));
		}
		else
			renderTransparentView (r, bitmap);
	}

	return bitmap;
}

//************************************************************************************************
// ViewAnimationHandler
//************************************************************************************************

bool ViewAnimationHandler::isLayerProperty (StringID propertyId)
{
	return propertyId == IGraphicsLayer::kOpacity;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewAnimationHandler::registerSelf (bool state)
{
	AnimationManager& animationManager = AnimationManager::instance ();
	if(state)
		animationManager.registerHandler (this);
	else
		animationManager.unregisterHandler (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ViewAnimationHandler::addAnimation (IObject* target, StringID propertyId, const IAnimation* prototype)
{
	// delegate to graphics layer associated with view
	if(View* view = unknown_cast<View> (target))
		if(IGraphicsLayer* layer = view->getGraphicsLayer ())
			if(isLayerProperty (propertyId))
				return layer->addAnimation (propertyId, prototype);

	return kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ViewAnimationHandler::removeAnimation (IObject* target, StringID propertyId)
{
	// delegate to graphics layer associated with view
	if(View* view = unknown_cast<View> (target))
		if(IGraphicsLayer* layer = view->getGraphicsLayer ())
			if(isLayerProperty (propertyId))
				return layer->removeAnimation (propertyId);

	return kResultFalse;
}

//************************************************************************************************
// AnimatorBase
//************************************************************************************************

AnimatorBase::AnimatorBase (View* parentView, float contentScaleFactor)
: parentView (parentView),
  contentScaleFactor (contentScaleFactor),
  duration (.25),
  timingType (kTimingEaseInOut),
  parentLayer (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphicsLayer* AnimatorBase::addClippingLayer (IGraphicsLayer* parentLayer, PointRef offset, Coord width, Coord height)
{
	IGraphicsLayer* clippingLayer = NativeGraphicsEngine::instance ().createGraphicsLayer (ClassID::GraphicsLayer);
	clippingLayer->construct (nullptr, Rect (offset.x, offset.y, Point (width, height)), IGraphicsLayer::kClipToBounds, contentScaleFactor);
	parentLayer->addSublayer (clippingLayer);
	return clippingLayer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphicsLayer* AnimatorBase::createLayerForImage (IImage* image, int mode)
{
	AutoPtr<IGraphicsLayer> layer;
	if(image != nullptr)
		if(layer = NativeGraphicsEngine::instance ().createGraphicsLayer (ClassID::GraphicsLayer))
			layer->construct (image, Rect (0, 0, image->getWidth (), image->getHeight ()), mode, contentScaleFactor);
	return layer.detach ();
}

//************************************************************************************************
// ViewAnimator
//************************************************************************************************

BEGIN_STYLEDEF (ViewAnimator::transitionTypes)
	{"none",	Styles::kTransitionNone},
	{"fade",	Styles::kTransitionFade},
	{"movein",	Styles::kTransitionMoveIn},
	{"moveout",	Styles::kTransitionMoveOut},
	{"conceal",	Styles::kTransitionConceal},
	{"reveal",	Styles::kTransitionReveal},
	{"push",	Styles::kTransitionPush},
	{"pushleft",Styles::kTransitionPushLeft},
	{"pushup",	Styles::kTransitionPushUp},
	{"pushdown",Styles::kTransitionPushDown},
	{"fall",	Styles::kTransitionFall},
	{"lift",	Styles::kTransitionLift},
	{"rise",	Styles::kTransitionRise},
	{"sink",	Styles::kTransitionSink},
	{"zoom",	Styles::kTransitionZoomIn},
	{"zoomin",	Styles::kTransitionZoomIn},
	{"zoomout",	Styles::kTransitionZoomOut},
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

/*static*/ ViewAnimator* ViewAnimator::create (View* parentView, TransitionType transitionType)
{
	if(transitionType != Styles::kTransitionNone)
		if(Window* window = parentView->getWindow ()) // view must be attached!
			if(NativeGraphicsEngine::instance ().hasGraphicsLayers ())
			{
				float contentScaleFactor = window->getContentScaleFactor ();
				return NEW ViewAnimator (parentView, transitionType, contentScaleFactor);
			}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/*static*/ TransitionType ViewAnimator::getInverseTransition (TransitionType transitionType)
{
	switch(transitionType)
	{
	case Styles::kTransitionMoveIn: return Styles::kTransitionMoveOut;
	case Styles::kTransitionMoveOut: return Styles::kTransitionMoveIn;
	case Styles::kTransitionConceal: return Styles::kTransitionReveal;
	case Styles::kTransitionReveal: return Styles::kTransitionConceal;
	case Styles::kTransitionPush: return Styles::kTransitionPushLeft;
	case Styles::kTransitionPushLeft: return Styles::kTransitionPush;
	case Styles::kTransitionPushUp: return Styles::kTransitionPushDown;
	case Styles::kTransitionPushDown: return Styles::kTransitionPushUp;
	case Styles::kTransitionFall: return Styles::kTransitionLift;
	case Styles::kTransitionLift: return Styles::kTransitionFall;
	case Styles::kTransitionRise: return Styles::kTransitionSink;
	case Styles::kTransitionSink: return Styles::kTransitionRise;
	case Styles::kTransitionZoomIn: return Styles::kTransitionZoomOut;
	case Styles::kTransitionZoomOut: return Styles::kTransitionZoomIn;
	default: return transitionType;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_ABSTRACT_HIDDEN (ViewAnimator, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

ViewAnimator::ViewAnimator (View* parentView, TransitionType transitionType, float contentScaleFactor)
: AnimatorBase (parentView, contentScaleFactor),
  transitionType (transitionType),
  fromLayerParent (nullptr),
  ignoreAlpha (true)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewAnimator::snipFromView (View* fromView, const Rect* rect)
{
#if 0 // does not work yet
	if(transitionType == Styles::kTransitionZoomOut)
		if(IGraphicsLayer* layer = fromView->getGraphicsLayer ())
		{
			// borrow existing backing layer for animation
			fromLayer = layer;
			fromLayerSize = rect ? rect->getSize () : fromView->getSize ().getSize ();
			fromLayerParent = fromLayer->getParentLayer ();
			if(fromLayerParent)
			{
				// remove fromLayer immediately (before its gets resized by it's view)
				fromLayerParent->removeSublayer (fromLayer);
				fromView->setLayerBackingEnabled (false);
			}
			return;
		}
#endif
	
	if(fromView->getStyle ().isTransparent ())
		setIgnoreAlpha (false);
	
	if(fromImage = ViewScreenCapture ().takeScreenshot (fromView, rect))
		fromLayerSize (fromImage->getWidth (), fromImage->getHeight ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ViewAnimator::isFromLayerAnimationOnly () const
{
	if(transitionType == Styles::kTransitionFade ||
	   transitionType == Styles::kTransitionReveal ||
	   transitionType == Styles::kTransitionMoveOut ||
	   transitionType == Styles::kTransitionZoomOut ||
	   transitionType == Styles::kTransitionSink ||
	   transitionType == Styles::kTransitionLift)
		return true;
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewAnimator::snipToView (View* toView, const Rect* rect)
{
	if(isFromLayerAnimationOnly ()) // toView is on screen already
		return;

	if(transitionType == Styles::kTransitionZoomIn)
		if(IGraphicsLayer* layer = toView->getGraphicsLayer ())
		{
			// borrow existing backing layer for animation
			toLayer = layer;
			toView->getParentLayer (toLayerOffset);
			return;
		}

	if(toView->getStyle ().isTransparent ())
		setIgnoreAlpha (false);

	toImage = ViewScreenCapture ().takeScreenshot (toView, rect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ViewAnimator::isPrepared () const
{
	return parentLayer != nullptr && clippingLayer != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ViewAnimator::prepare ()
{
	ASSERT (isPrepared () == false)
	if(isPrepared ())
		return true;

	Point parentOffset;
	parentLayer = parentView->getParentLayer (parentOffset);
	ASSERT (parentLayer != nullptr)
	if(parentLayer == nullptr)
		return false;

	if(!fromLayer && fromImage)
		fromLayerSize (fromImage->getWidth (), fromImage->getHeight ());

	Coord imageWidth = fromLayerSize.x;
	Coord imageHeight = fromLayerSize.y;

	clippingLayer = addClippingLayer (parentLayer, parentOffset, imageWidth, imageHeight);
	//parentLayer->flush (); causes flicker!
	
	if(toImage == nullptr && isFromLayerAnimationOnly () == false)
		return false;
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ViewAnimator::makeTransition ()
{
	if(!isPrepared ())
		if(!prepare ())
			return false;

	AutoPtr<AnimationCompletionHandlerList> completionHandler (NEW AnimationCompletionHandlerList);
	completionHandler->addCompletionHandler (LayerRemover::create (parentLayer, clippingLayer));
	
	if(externalHandler)
		completionHandler->addCompletionHandler (externalHandler);
	
	if(fromLayer) // ...fromLayer only exists for kTransitionZoomOut (disabled for now - #if 0)
	{
		if(fromLayerParent)
			completionHandler->addCompletionHandler (LayerAdder::create (fromLayerParent, fromLayer, Point ()));
	}
	else
		fromLayer = createLayerForImage (fromImage, isIgnoreAlpha () ? IGraphicsLayer::kIgnoreAlpha : 0);
	
	if(fromLayer == nullptr)
		return false;
	
	clippingLayer->addSublayer (fromLayer);
	
	AutoPtr<IGraphicsLayer> toLayer;
	if(this->toLayer) // ...this->toLayer only exists for kTransitionZoomIn
	{
		toLayer.share (this->toLayer);
		if(IGraphicsLayer* oldParentLayer = toLayer->getParentLayer ())
		{
			// remove toLayer from it's parent and schedule re-adding it when animation is completed
			oldParentLayer->removeSublayer (toLayer);
			completionHandler->addCompletionHandler (LayerAdder::create (oldParentLayer, toLayer, toLayerOffset));
		}
	}
	else
		toLayer = createLayerForImage (toImage, isIgnoreAlpha () ? IGraphicsLayer::kIgnoreAlpha : 0);
	
	parentLayer->flush ();

	AnimationDescription description;
	description.timingType = timingType;
	description.duration = duration;

	if(transitionType == Styles::kTransitionFall)
	{
		CCL_PRINTLN ("Preparing fall transition from top...")
		
		Coord imageHeight = fromImage->getHeight ();
		
		toLayer->setOffset (Point (0, -imageHeight));
		clippingLayer->addSublayer (toLayer);
		
		TransformAnimation fallIn;
		fallIn.setDescription (description);
		fallIn.addTranslationY (0, imageHeight);
		fallIn.setCompletionHandler (completionHandler);

		toLayer->addAnimation (IGraphicsLayer::kTransform, fallIn.asInterface ());
	}
	else if(transitionType == Styles::kTransitionLift)
	{
		CCL_PRINTLN ("Preparing lift transition...")

		Coord imageHeight = fromImage->getHeight ();

		TransformAnimation moveOut;
		moveOut.setDescription (description);
		moveOut.addTranslationY (0., -imageHeight);
		moveOut.setCompletionHandler (completionHandler);

		fromLayer->addAnimation (IGraphicsLayer::kTransform, moveOut.asInterface ());
	}
	else if(transitionType == Styles::kTransitionSink)
	{
		CCL_PRINTLN ("Preparing sink transition...")

		Coord imageHeight = fromImage->getHeight ();

		TransformAnimation moveOut;
		moveOut.setDescription (description);
		moveOut.addTranslationY (0., imageHeight);
		moveOut.setCompletionHandler (completionHandler);

		fromLayer->addAnimation (IGraphicsLayer::kTransform, moveOut.asInterface ());
	}
	else if(transitionType == Styles::kTransitionRise)
	{
		CCL_PRINTLN ("Preparing rise transition from bottom...")
		
		Coord imageHeight = fromImage->getHeight ();
		
		toLayer->setOffset (Point (0, imageHeight));
		clippingLayer->addSublayer (toLayer);
		
		TransformAnimation riseIn;
		riseIn.setDescription (description);
		riseIn.addTranslationY (0, -imageHeight);
		riseIn.setCompletionHandler (completionHandler);

		toLayer->addAnimation (IGraphicsLayer::kTransform, riseIn.asInterface ());
	}
	else if(transitionType == Styles::kTransitionPush || transitionType == Styles::kTransitionPushLeft)
	{
		CCL_PRINTLN ("Preparing push transition...")
		Coord imageWidth = fromImage->getWidth ();
		Coord translation = transitionType == Styles::kTransitionPush ? imageWidth : -imageWidth;

		toLayer->setOffset (Point (-translation, 0));
		clippingLayer->addSublayer (toLayer);

		TransformAnimation moveIn;
		moveIn.setDescription (description);
		moveIn.addTranslationX (0, translation);

		TransformAnimation moveOut;
		moveOut.setDescription (description);
		moveOut.addTranslationX (0, translation);
		moveOut.setCompletionHandler (completionHandler);
		
		toLayer->addAnimation (IGraphicsLayer::kTransform, moveIn.asInterface ());
		fromLayer->addAnimation (IGraphicsLayer::kTransform, moveOut.asInterface ());
	}
	else if(transitionType == Styles::kTransitionPushUp || transitionType == Styles::kTransitionPushDown)
	{
		CCL_PRINTLN ("Preparing push transition...")
		Coord imageHeight = fromImage->getHeight ();
		Coord translation = transitionType == Styles::kTransitionPushUp ? -imageHeight : imageHeight;
		
		toLayer->setOffset (Point (0, -translation));
		clippingLayer->addSublayer (toLayer);
		
		TransformAnimation moveIn;
		moveIn.setDescription (description);
		moveIn.addTranslationY (0, translation);
		
		TransformAnimation moveOut;
		moveOut.setDescription (description);
		moveOut.addTranslationY (0, translation);
		moveOut.setCompletionHandler (completionHandler);
		
		toLayer->addAnimation (IGraphicsLayer::kTransform, moveIn.asInterface ());
		fromLayer->addAnimation (IGraphicsLayer::kTransform, moveOut.asInterface ());
	}
	else if(transitionType == Styles::kTransitionMoveIn)
	{
		CCL_PRINTLN ("Preparing move in (conceal) transition from left...")

		Coord imageWidth = fromImage->getWidth ();

		toLayer->setOffset (Point (-imageWidth, 0));
		clippingLayer->addSublayer (toLayer);

		TransformAnimation moveIn;
		moveIn.setDescription (description);
		moveIn.addTranslationX (0, imageWidth);
		moveIn.setCompletionHandler (completionHandler);
		
		toLayer->addAnimation (IGraphicsLayer::kTransform, moveIn.asInterface ());
	}
	else if(transitionType == Styles::kTransitionMoveOut)
	{
		CCL_PRINTLN ("Preparing move out (reveal) transition from right...")

		Coord imageWidth = fromImage->getWidth ();

		TransformAnimation moveOut;
		moveOut.setDescription (description);
		moveOut.addTranslationX (0., -imageWidth);
		moveOut.setCompletionHandler (completionHandler);

		fromLayer->addAnimation (IGraphicsLayer::kTransform, moveOut.asInterface ());
	}
	else if(transitionType == Styles::kTransitionConceal)
	{
		CCL_PRINTLN ("Preparing conceal (move in) transition from right...")

		Coord imageWidth = fromImage->getWidth ();

		toLayer->setOffset (Point (imageWidth, 0));
		clippingLayer->addSublayer (toLayer);

		TransformAnimation moveIn;
		moveIn.setDescription (description);
		moveIn.addTranslationX (0, -imageWidth);
		moveIn.setCompletionHandler (completionHandler);
		
		toLayer->addAnimation (IGraphicsLayer::kTransform, moveIn.asInterface ());
	}
	else if(transitionType == Styles::kTransitionReveal)
	{
		CCL_PRINTLN ("Preparing reveal (move out) transition from left...")

		Coord imageWidth = fromImage->getWidth ();

		TransformAnimation moveOut;
		moveOut.setDescription (description);
		moveOut.addTranslationX (0., imageWidth);
		moveOut.setCompletionHandler (completionHandler);

		fromLayer->addAnimation (IGraphicsLayer::kTransform, moveOut.asInterface ());
	}
	else if(transitionType == Styles::kTransitionZoomIn)
	{
		clippingLayer->addSublayer (toLayer);

		TransformAnimation zoomIn;
		zoomIn.setDescription (description);

		if(!fromRect.isEmpty ())
		{
			toLayer->setOffset (fromRect.getLeftTop ());

			Coord imageWidth = fromImage->getWidth ();
			Coord imageHeight = fromImage->getHeight ();

			zoomIn.addScalingX ((double)fromRect.getWidth ()/(double)imageWidth, 1.);
			zoomIn.addScalingY ((double)fromRect.getHeight ()/(double)imageHeight, 1.);

			zoomIn.addTranslationX (0, -fromRect.left);
			zoomIn.addTranslationY (0, -fromRect.top);
		}
		else
		{
			zoomIn.addScalingX (0., 1.);
			zoomIn.addScalingY (0., 1.);
		}

		zoomIn.setCompletionHandler (completionHandler);

		BasicAnimation fadeIn;
		fadeIn.setDescription (description);
		fadeIn.setStartValue (0.2f);
		fadeIn.setEndValue (1.f);

		toLayer->addAnimation (IGraphicsLayer::kOpacity, fadeIn.asInterface ());
		toLayer->addAnimation (IGraphicsLayer::kTransform, zoomIn.asInterface ());
	}
	else if(transitionType == Styles::kTransitionZoomOut)
	{
		TransformAnimation zoomOut;
		zoomOut.setDescription (description);
		
		if(!fromRect.isEmpty ())
		{
			// interpreting fromRect as toRect			
			Coord imageWidth = fromLayerSize.x;
			Coord imageHeight = fromLayerSize.y;

			zoomOut.addScalingX (1., (double)fromRect.getWidth ()/(double)imageWidth);
			zoomOut.addScalingY (1., (double)fromRect.getHeight ()/(double)imageHeight);
			
			zoomOut.addTranslationX (0, fromRect.left);
			zoomOut.addTranslationY (0, fromRect.top);
		}
		else
		{
			zoomOut.addScalingX (1., 0.);
			zoomOut.addScalingY (1., 0.);
		}
		
		zoomOut.setCompletionHandler (completionHandler);
		
		BasicAnimation fadeOut;
		fadeOut.setDescription (description);
		fadeOut.setStartValue (1.f);
		fadeOut.setEndValue (0.2f);
		
		fromLayer->addAnimation (IGraphicsLayer::kOpacity, fadeOut.asInterface ());
		fromLayer->addAnimation (IGraphicsLayer::kTransform, zoomOut.asInterface ());
	}
	else
	{
		ASSERT (transitionType == Styles::kTransitionFade)
		CCL_PRINTLN ("Preparing fade transition...")
	
		BasicAnimation fadeOut;
		fadeOut.setDescription (description);
		fadeOut.setStartValue (1.f);
		fadeOut.setEndValue (0.f);
		fadeOut.setCompletionHandler (completionHandler);

		fromLayer->addAnimation (IGraphicsLayer::kOpacity, fadeOut.asInterface ());
	}

	parentLayer->flush ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ViewAnimator::setTransitionProperty (StringID propertyId, VariantRef value)
{
	if(propertyId == kDuration)
	{
		duration = value;
		return true;
	}
	else if(propertyId == kTimingType)
	{
		timingType = value;
		return true;
	}
	else if(propertyId == kFromRect)
	{
		if(IUIValue* uiValue = IUIValue::toValue (value))
			uiValue->toRect (fromRect);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ViewAnimator::setProperty (MemberID propertyId, const Variant& var)
{
	return setTransitionProperty (propertyId, var);
}

//************************************************************************************************
// LayoutAnimator::ViewItem
//************************************************************************************************

struct LayoutAnimator::ViewItem: public Object
{
	ObservedPtr<View> view;
	AutoPtr<IImage> fromImage;
	AutoPtr<IImage> toImage;
	IGraphicsLayer* fromLayer;
	IGraphicsLayer* toLayer;
	Rect fromRect;
	Rect toRect;
	
	ViewItem ()
	: fromLayer (nullptr), toLayer (nullptr)
	{}
};

//************************************************************************************************
// LayoutAnimator
//************************************************************************************************

LayoutAnimator* LayoutAnimator::create (View* parentView)
{
	if(Window* window = parentView->getWindow ()) // view must be attached
		if(NativeGraphicsEngine::instance ().hasGraphicsLayers ())
			return NEW LayoutAnimator (parentView, window->getContentScaleFactor ());
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LayoutAnimator::LayoutAnimator (View* parentView, float contentScaleFactor)
: AnimatorBase (parentView, contentScaleFactor)
{
	items.objectCleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LayoutAnimator::snipOldViews ()
{
	ForEachViewFast (*parentView, view)
		ViewItem* item = NEW ViewItem;
		item->view = view;
		item->fromRect = view->getSize ();
		item->fromImage = ViewScreenCapture ().takeScreenshot (view);
		if(item->fromImage)
			item->fromLayer = createLayerForImage (item->fromImage);
		items.add (item);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LayoutAnimator::snipNewViews ()
{
	ForEach (items, ViewItem, item)
		if(item->view && item->view->isAttached ())
		{
			item->toRect = item->view->getSize ();
			item->toImage = ViewScreenCapture ().takeScreenshot (item->view);
			if(item->toImage)
				item->toLayer = createLayerForImage (item->toImage);
		}
		else
		{
			// todo: view disappeared
		}
	EndFor
	
	// todo: check for views without items (appeared)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LayoutAnimator::makeTransition ()
{
	Point parentOffset;
	parentLayer = parentView->getParentLayer (parentOffset);
	ASSERT (parentLayer != nullptr)
	if(parentLayer == nullptr)
		return false;
		
	clippingLayer = addClippingLayer (parentLayer, parentOffset, parentView->getWidth (), parentView->getHeight ());
	parentLayer->flush ();

	AutoPtr<AnimationCompletionHandlerList> completionHandler (NEW AnimationCompletionHandlerList);

	AnimationDescription description;
	description.timingType = timingType;
	description.duration = duration;

	ForEach (items, ViewItem, item)
		if(item->fromRect == item->toRect)
			continue;
			
		double scaleX = item->toRect.getWidth () / ccl_max (1, item->fromRect.getWidth ());
		double scaleY = item->toRect.getHeight () / ccl_max (1, item->fromRect.getHeight ());

		// apply transform and fade animations to old and new layer
		auto makeAnimations = [&] (IGraphicsLayer* layer, float fadeStart, float fadeEnd)
		{
			if(!layer)
				return;

			TransformAnimation transform;
			transform.setDescription (description);
		
			layer->setOffset (item->fromRect.getLeftTop ());
			clippingLayer->addSublayer (layer);
			
			// grow / shrink from old to new size (same for old and new)
			if(scaleX != 1)
				transform.addScalingX (1, scaleX);
			if(scaleY != 1)
				transform.addScalingY (1, scaleY);
			
			// move fro old to new position
			transform.addTranslationX (item->fromRect.left, item->toRect.left);
			transform.addTranslationY (item->fromRect.top, item->toRect.top);

			// fade in / out
			BasicAnimation fade;
			fade.setDescription (description);
			fade.setStartValue (fadeStart);
			fade.setEndValue (fadeEnd);
	
			if(completionHandler.isValid ())
			{
				completionHandler->addCompletionHandler (LayerRemover::create (parentLayer, clippingLayer));
				transform.setCompletionHandler (completionHandler);
				completionHandler = nullptr; // only once
			}

			layer->addAnimation (IGraphicsLayer::kOpacity, fade.asInterface ());
			layer->addAnimation (IGraphicsLayer::kTransform, transform.asInterface ());
		};
	
		makeAnimations (item->fromLayer, 1.f, 0.2f);
		makeAnimations (item->toLayer, 0.2f, 1.f);
	EndFor
		
	parentLayer->flush ();
	return true;
}
