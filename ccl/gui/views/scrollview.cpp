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
// Filename    : ccl/gui/controls/scrollview.cpp
// Description : Scroll View
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/views/scrollview.h"
#include "ccl/gui/views/viewaccessibility.h"
#include "ccl/gui/controls/scrollbar.h"

#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/touch/touchinput.h"
#include "ccl/gui/touch/touchhandler.h"
#include "ccl/gui/theme/renderer/scrollbarrenderer.h"

#include "ccl/gui/system/animation.h"

#include "ccl/app/params.h"
#include "ccl/base/storage/configuration.h"

#include "ccl/public/math/mathprimitives.h"
#include "ccl/public/gui/framework/guievent.h"
#include "ccl/public/gui/iparamobserver.h"
#include "ccl/public/gui/iviewstate.h"
#include "ccl/public/storage/iattributelist.h"
#include "ccl/public/system/isignalhandler.h"
#include "ccl/public/systemservices.h"

namespace CCL {

//************************************************************************************************
// ScrollViewMouseHandler
//************************************************************************************************

class ScrollViewMouseHandler: public MouseHandler
{
public:
	ScrollViewMouseHandler (ScrollView* scrollView)
	: MouseHandler (scrollView)
	{}

	// MouseHandler
	void onBegin () override
	{   
		ScrollView* scrollView = (ScrollView*)view;
		scrollView->getPosition (startPos);
		scrollView->setManipulation (true);

		scrollView->signal (Message (ScrollView::kOnScrollBegin));
	} 

	void onRelease (bool) override
	{ 
		ScrollView* scrollView = (ScrollView*)view;

		if(scrollView->getStyle ().isCustomStyle (Styles::kScrollViewBehaviorScrollByPage|Styles::kScrollViewBehaviorSnapToViews))
		{
			// finally scroll animated to snapped position
			Point pos;
			scrollView->getPosition (pos);
			Point snappedPos (pos);
			
			Point direction (pos - startPos);
			Point pageSize;
			scrollView->getScrollByPageSize (pageSize);
			
			// move mouse at least kMinimalPagingMovement points before snapping to next page
			if((pageSize.x + pageSize.y) > 200)
			{
				Point minimalMouseMovement (ScrollView::kMinimalPagingMovement, ScrollView::kMinimalPagingMovement);
				
				if((ccl_sign (direction.x) * direction.x) < minimalMouseMovement.x)
					direction.x = 0;
				if((ccl_sign (direction.y) * direction.y) < minimalMouseMovement.y)
					direction.y = 0;
			}
			
			scrollView->snapTargetPos (snappedPos, direction);

			Point scrollRange (scrollView->getScrollRange ());
			if(scrollRange.x <= 0)
				snappedPos.x = pos.x;
			if(scrollRange.y <= 0)
				snappedPos.y = pos.y;

			scrollView->scrollTo (snappedPos, 0.5f, 2000.f);
		}

		scrollView->setManipulation (false);

		scrollView->signal (Message (ScrollView::kOnScrollEnd));
	}

	bool onMove (int moveFlags) override
	{
		ScrollView* scrollView = (ScrollView*)view;

		Point dist (current.where - first.where);
		Point scrollRange (scrollView->getScrollRange ());
		if(scrollRange.x <= 0)
			dist.x = 0;
		if(scrollRange.y <= 0)
			dist.y = 0;

		Point pos (startPos + dist);
		scrollView->scrollTo (pos);
	
		scrollView->getPosition (pos);
		scrollView->signal (Message (ScrollView::kOnScrollUpdate, pos.x, pos.y));

		return true;
	}

private:
	Point startPos;
};

//************************************************************************************************
// ScrollManipulator
//************************************************************************************************

ScrollManipulator::ScrollManipulator (ScrollView* scrollView)
: scrollView (scrollView),
  duration (-1),
  velocity (0),
  direction (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollManipulator::begin (PointRef where, bool force)
{
	if(!force && scrollView->isManipulating ())
		return;

	View* target = scrollView->getTarget ();
	first = where;
	smoothedPos (float (where.x), float (where.y));

	bool wasAnimating = scrollView->isAnimatingX () || scrollView->isAnimatingY ();
	scrollView->getPosition (initialTargetPos); // gets current animated position of the target layer
	scrollView->setManipulation (true);

	if(wasAnimating)
	{
		scrollView->stopAnimations ();
		scrollView->scrollTo (initialTargetPos);
	}
	else
		scrollView->setScrolling (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollManipulator::move (Point current)
{
	#if 1
	// smooth position
	static const float lowPassFactor = 0.66f;
	smoothedPos.x = lowPassFactor * smoothedPos.x + (1.f - lowPassFactor) * float(current.x);
	smoothedPos.y = lowPassFactor * smoothedPos.y + (1.f - lowPassFactor) * float(current.y);
	Point pos ((Coord)ccl_round<0> (smoothedPos.x), (Coord)ccl_round<0> (smoothedPos.y));
	#else
	Point pos (current);
	#endif

	Rect clipRect;
	scrollView->getClipViewRect (clipRect);

	Point delta (pos - first);
	if(!scrollView->canScrollH ())
		delta.x = 0;
	if(!scrollView->canScrollV ())
		delta.y = 0;

	// lock to one direction
	if(direction == 0)
	{
		const Coord lockTolerance = 20;
		Coord x = ccl_abs (delta.x);
		Coord y = ccl_abs (delta.y);

		if(x > y + lockTolerance)
			direction = Styles::kHorizontal;
		else if(y > x + lockTolerance)
			direction = Styles::kVertical;
	}

	if(!scrollView->canScrollOmniDirectional ())
	{
		if(direction == Styles::kHorizontal)
			delta.y = 0;
		else if(direction == Styles::kVertical)
			delta.x = 0;
	}

	View* target = scrollView->getTarget ();
	Rect targetRect (target->getSize ());
	Point p (initialTargetPos + delta);
			
	Point scrollRange = clipRect.getSize() - targetRect.getSize ();
			
	Point pBound (p);
	if(scrollRange.x < 0)
		pBound.x = ccl_bound<Coord> (p.x, scrollRange.x, 0);
	if(scrollRange.y < 0)
		pBound.y = ccl_bound<Coord> (p.y, scrollRange.y, 0);
			
	Point over (p - pBound);
	over *= 0.3f;
	p = pBound + over;
			
	scrollView->scrollTo (p);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollManipulator::end (float velocityX, float velocityY)
{
	if(scrollView->isScrollByPageEnabled ())
	{
		if(ccl_abs (velocityX) > 100.f)
			velocityX *= 10;
		if(ccl_abs (velocityY) > 100.f)
			velocityY *= 10;
	}
	
	float velocityFactor = scrollView->getVisualStyle ().getMetric ("velocityFactor", 2.f);
	velocityX *= velocityFactor;
	velocityY *= velocityFactor;
	
	// lock inertial scrolling to one direction
	if(direction == 0)
	{
		if(ccl_abs (velocityX) > ccl_abs (velocityY))
			direction = Styles::kHorizontal;
		else
			direction = Styles::kVertical;
	}
	
	if(!scrollView->canScrollOmniDirectional ())
	{
		if(direction == Styles::kHorizontal)
			velocityY = 0;
		else if(direction == Styles::kVertical)
			velocityX = 0;
	}
	
	velocity = sqrtf (powf (velocityX, 2.f) + powf (velocityY, 2.f));
	
	Point delta ((Coord)velocityX, (Coord)velocityY);
	
	if(scrollView->isScrollByPageEnabled ())
	{
		// force full page delta when velocitiy exceeds a minimum
		Rect clipRect;
		scrollView->getClipViewRect (clipRect);
		
		if(velocityX > 10.f)
			ccl_lower_limit (delta.x, Coord(clipRect.getWidth () * 0.9f));
		else if(velocityX < -10.f)
			ccl_upper_limit (delta.x, Coord (clipRect.getWidth () * -0.9f));
		
		if(velocityY > 10.f)
			ccl_lower_limit (delta.y, Coord(clipRect.getHeight () * 0.9f));
		else if(velocityY < -10.f)
			ccl_upper_limit (delta.y, Coord (clipRect.getHeight () * -0.9f));
	}
	end (delta);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollManipulator::end (PointRef delta)
{
	scrollView->setManipulation (false);
	
	View* target = scrollView->getTarget ();
	Point oldPos (target->getSize ().getLeftTop ());
	Point limitedDelta (delta);

	Rect clipRect;
	Point clipSize = scrollView->getClipViewRect (clipRect).getSize ();

	if(scrollView->isScrollByPageEnabled ())
	{
		// limit inertial delta to one page
		ccl_upper_limit (limitedDelta.x,  clipSize.x);
		ccl_lower_limit (limitedDelta.x, -clipSize.x);
		ccl_upper_limit (limitedDelta.y,  clipSize.y);
		ccl_lower_limit (limitedDelta.y, -clipSize.y);
	}

	Point p (oldPos + limitedDelta);

	if(scrollView->isScrollByPageEnabled ())
	{
		// limit target pos to one page from initial pos
		ccl_upper_limit (p.x,  initialTargetPos.x + clipSize.x);
		ccl_lower_limit (p.x,  initialTargetPos.x - clipSize.x);
		ccl_upper_limit (p.y,  initialTargetPos.y + clipSize.y);
		ccl_lower_limit (p.y,  initialTargetPos.y - clipSize.y);
	}

	if(scrollView->isScrollByPageEnabled () || scrollView->getStyle ().isCustomStyle (Styles::kScrollViewBehaviorSnapToViews))
		scrollView->snapTargetPos (p, Point ());

	if(!scrollView->canScrollH ())
		p.x = oldPos.x;
	if(!scrollView->canScrollV ())
		p.y = oldPos.y;

	if(scrollView->isScrollByPageEnabled ())
	{
		const double kPageFlipDuration = 0.3;
		duration = kPageFlipDuration;
	}
	else
		duration = scrollView->getVisualStyle ().getMetric ("inertialDuration", 3.0);

	// do inertial scrolling only when fast enough or we have to snap
	if(velocity >= 20.f
		|| scrollView->isScrollByPageEnabled ()
		|| scrollView->getStyle ().isCustomStyle (Styles::kScrollViewBehaviorSnapToViews))
		scrollView->scrollTo (p, duration, velocity);

	scrollView->setScrolling (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollManipulator::push (PointRef delta)
{
	View* target = scrollView->getTarget ();
	Point finalPos (target->getSize ().getLeftTop ()); // anticipated target position (animation might not be there yet)

	begin (Point ()); // initialTargetPos is now the current (animated) position

	Point remainingDelta (finalPos - initialTargetPos);
	Point newDelta (delta);

	if(scrollView->isScrollByPageEnabled ())
	{
		Point pageSize;
		scrollView->getScrollByPageSize (pageSize);
		newDelta.x = -pageSize.x * ccl_sign (delta.x);
		newDelta.y = -pageSize.y * ccl_sign (delta.y);
	}

	 // add remaining delta from old animation if same direction
	if(newDelta.x * remainingDelta.x > 0)
		newDelta.x += remainingDelta.x;
	if(newDelta.y * remainingDelta.y > 0)
		newDelta.y += remainingDelta.y;

	Point scrollRange (scrollView->getScrollRange ());
	if(scrollRange.x <= 0)
		newDelta.x = 0;
	if(scrollRange.y <= 0)
		newDelta.y = 0;

	if(newDelta.isNull ())
		return;

	duration = 0.4;
	velocity = 50.;

	end (newDelta);
}

//************************************************************************************************
// ScrollViewSwipeHandler
//************************************************************************************************

class ScrollViewSwipeHandler: public TouchHandler
{
public:
    ScrollViewSwipeHandler (ScrollView* scrollView, bool boostPriority)
    : TouchHandler (scrollView),
	  scrollManipulator (scrollView),
	  hasGestureMoved (false)
    {
		if(boostPriority)
		{
			addRequiredGesture (GestureEvent::kSwipe|GestureEvent::kExclusiveTouch, GestureEvent::kPriorityHighest + 1);
			addRequiredGesture (GestureEvent::kLongPress, GestureEvent::kPriorityHighest + 1);
			addRequiredGesture (GestureEvent::kSingleTap, GestureEvent::kPriorityHighest + 1);
			addRequiredGesture (GestureEvent::kDoubleTap, GestureEvent::kPriorityHighest + 1);
			addRequiredGesture (GestureEvent::kRotate,    GestureEvent::kPriorityHighest + 1);

			if(!view->getStyle ().isCustomStyle (Styles::kScrollViewBehaviorAllowZoomGesture))
				addRequiredGesture (GestureEvent::kZoom,  GestureEvent::kPriorityHighest + 1);
		}
		else
		{
			addRequiredGesture (GestureEvent::kSwipe|GestureEvent::kHorizontal, scrollView->canScrollH () ? GestureEvent::kPriorityHigh : GestureEvent::kPriorityLow);
			addRequiredGesture (GestureEvent::kSwipe|GestureEvent::kVertical,   scrollView->canScrollV () ? GestureEvent::kPriorityHigh : GestureEvent::kPriorityLow);
		}
	}

	ScrollView* getScrollView () const { return static_cast<ScrollView*> (view); }

	void beginInternal (PointRef where, bool force = false)
	{
		hasGestureMoved = false;

		Point p (where);
		view->windowToClient (p);
		scrollManipulator.begin (p, force);
	}

	void onBegin (const TouchEvent& event) override
	{
		if(const TouchInfo* touch = event.touches.getTouchInfoByID (event.touchID))
			beginInternal (touch->where);
	}

	tbool CCL_API onGesture (const GestureEvent& event) override
	{
		ScrollView* scrollView = getScrollView ();
		int gestureType = event.getType ();

		if(gestureType == GestureEvent::kSwipe && event.getState () == GestureEvent::kPossible)
		{
			// stop animation immediately on touchBegan (before a gesture has been detected)
			if(scrollView->isAnimatingX () || scrollView->isAnimatingY ())
			{
				// if layers are used, the target view is already at the animation end position: move it to the current position (animated presentation property)
				Point currentPos;
				scrollView->getPosition (currentPos);
				scrollView->scrollTo (currentPos);

				getScrollView ()->stopAnimations ();
			}
			return true;
		}

		if(gestureType == GestureEvent::kSingleTap || gestureType == GestureEvent::kDoubleTap)
		{
			// manipulation might have started in onBegin, there will be no final kEnd event
			if(scrollView->isManipulating ())
				scrollView->setManipulation (false);
			return true;
		}

		switch(event.getState ())
		{
		case GestureEvent::kBegin:
			beginInternal (event.where, true);
			break;

		case GestureEvent::kChanged:
			{
				hasGestureMoved = true;

				Point current = event.where;
				view->windowToClient (current);
				scrollManipulator.move (current);
			}
			break;

		case GestureEvent::kEnd:
			{
				float velocityX = hasGestureMoved ? event.amountX : 0;
				float velocityY = hasGestureMoved ? event.amountY : 0;
				scrollManipulator.end (velocityX, velocityY);
			}
			break;

		case GestureEvent::kFailed:
			if(scrollView->isManipulating ())
				scrollView->setManipulation (false);

			scrollView->setScrolling (false);
			break;
		}
		return true;
	}

	void onRelease (const TouchEvent& event, bool canceled) override
	{
		getScrollView ()->setManipulation (false);
	}
	
	tbool CCL_API allowsCompetingGesture (int gestureType) override
	{
		return view->getStyle ().isCustomStyle (Styles::kScrollViewBehaviorAllowZoomGesture) && gestureType == GestureEvent::kZoom;
	}

	tbool CCL_API addTouch (const TouchEvent& event) override
	{
		if(view->getStyle ().isCustomStyle (Styles::kScrollViewBehaviorAllowZoomGesture))
			return false;

		return true; // swallow other touches
	}

private:
	ScrollManipulator scrollManipulator;
	bool hasGestureMoved;
};

//************************************************************************************************
// ScrollAnimationCompletionHandler
//************************************************************************************************
	
class ScrollAnimationCompletionHandler: public Object,
										public IdleClient,
										public IAnimationCompletionHandler
{
public:
	ScrollAnimationCompletionHandler (ScrollView* scrollView, int animatingFlag)
	: scrollView (scrollView), animatingFlag (animatingFlag), targetPos (kMaxCoord, kMaxCoord)
	{
		scrollView->privateFlags |= animatingFlag;

		if(scrollView->getStyle ().isCustomStyle (Styles::kScrollViewBehaviorNotifications))
			startTimer ();
	}
	
	PROPERTY_OBJECT (Point, targetPos, TargetPos)

	void CCL_API onAnimationFinished () override
	{
		scrollView->privateFlags &= ~animatingFlag;

		if(!scrollView->isAnimatingX () && !scrollView->isAnimatingY ())
			stopTimer ();

		scrollView->setScrolling (false);
		if(animatingFlag == ScrollView::kAnimatingY)
			scrollView->stopVerticalAnimation ();
		else
			scrollView->stopHorizontalAnimation ();

		// animation resets to start value in AnimationManager::removeAnimation (stop), set targetPos afterwards
		if(targetPos.x != kMaxCoord)
			scrollView->scrollTo (targetPos);
	}
	
	void onIdleTimer () override
	{
		if(!scrollView || !scrollView->isAttached ())
			return;
	
		Point pos;
		scrollView->getPosition (pos);
		scrollView->signal (Message (ScrollView::kOnScrollUpdate, pos.x, pos.y));

		Coord xValue = abs (pos.x) / scrollView->snap.x;
		Coord yValue = abs (pos.y) / scrollView->snap.y;
		
		UnknownPtr<IParamPreviewHandler> previewHandler;
		if(previewHandler = scrollView->hParam ? scrollView->hParam->getController () : nullptr)
		{
			ParamPreviewEvent e;
			e.value = xValue;
			previewHandler->paramPreview (scrollView->hParam, e);
		}
		if(previewHandler = scrollView->vParam ? scrollView->vParam->getController () : nullptr)
		{
			ParamPreviewEvent e;
			e.value = yValue;
			previewHandler->paramPreview (scrollView->vParam, e);
		}

		// check whether scrollView is still valid, it can be null now if paramPreview stopped the animation
		if(scrollView)
		{
			if(scrollView->hParam)
				scrollView->hParam->setValue (xValue);
			else if(scrollView->vParam)
				scrollView->vParam->setValue (yValue);
		}
	}

	CLASS_INTERFACE2 (ITimerTask, IAnimationCompletionHandler, Object)
	
private:
	SharedPtr<ScrollView> scrollView;
	int animatingFlag;
};

//************************************************************************************************
// ScrollViewClipper
//************************************************************************************************

class ScrollViewClipper: public View
{
public:
	DECLARE_CLASS_ABSTRACT (ScrollViewClipper, View)

	ScrollViewClipper (const Rect& size)
	: View (size),
	  resizing (false)
	{
		sizeLimits.setUnlimited ();
		privateFlags |= kSizeLimitsValid|kExplicitSizeLimits;
	}

	void onChildSized (View* child, const Point& delta) override
	{
		if(resizing)
			return;

		ScrollView* scrollView = ccl_cast<ScrollView> (parent);
		if(scrollView && scrollView->getTarget () == child)
			scrollView->onChildSized (child, delta);
	}

	void onChildLimitsChanged (View* child) override
	{
		View::onChildLimitsChanged (child);

		ScrollView* scrollView = ccl_cast<ScrollView> (parent);
		if(scrollView && scrollView->getTarget () == child && scrollView->getStyle ().isCustomStyle (Styles::kScrollViewBehaviorTargetLimits))
			scrollView->onChildLimitsChanged (this);
	}

	void CCL_API setSize (RectRef newSize, tbool doInvalidate) override
	{
		ScopedVar<bool> scope (resizing, true);
		View::setSize (newSize, doInvalidate);

		if(auto* scrollView = ccl_cast<ScrollView> (parent))
		{
			View* target = scrollView->getTarget ();
			if(target && target->getSizeMode () == kFill)
			{
				// target has size mode kFill: pass clipView width / height depending on orientation flags
				Rect targetRect (target->getSize ());

				if(target->getStyle ().isCommonStyle (Styles::kHorizontal))
					targetRect.setWidth (getWidth ());
				else if(target->getStyle ().isCommonStyle (Styles::kVertical))
					targetRect.setHeight (getHeight ());

				target->setSize (targetRect);
			}
		}	
	}

	void CCL_API setSizeLimits (const SizeLimit& sizeLimits) override
	{} // dont pass sizeLimits further

	Color getClipBackColor () const
	{
		const IVisualStyle& vs = parent->getVisualStyle ();
		return vs.getColor ("clip.backcolor", getTheme ().getThemeColor (ThemeElements::kListViewBackColor));
	}

	LayerHint CCL_API getLayerHint () const override
	{
		if(isEmpty () && !style.isOpaque ())
			return kGraphicsContentEmpty;
		
		if(getClipBackColor ().isOpaque ())
			return kGraphicsContentOpaque;

		return kGraphicsContentHintDefault;
	}
	
	void draw (const UpdateRgn& updateRgn) override
	{
		if(style.isOpaque ())
		{
			GraphicsPort port (this);

			Color backColor = getClipBackColor ();
			port.fillRect (updateRgn.bounds, SolidBrush (backColor));
		}
		View::draw (updateRgn);
	}
	
	void CCL_API scrollClient (RectRef rect, PointRef delta) override
	{
		// the call originated from another nested scrollview: must clip to our client rect!
		Rect boundRect (rect);

		// limit old rect to our client rect
		Rect client;
		getClientRect (client);	
		boundRect.bound (client);

		// limit the scrolled rect to our client rect
		client.offset (-delta.x, -delta.y);
		boundRect.bound (client);

		View::scrollClient (boundRect, delta);
	}
		
private:
	bool resizing;
};

DEFINE_CLASS_ABSTRACT_HIDDEN (ScrollViewClipper, View)

//************************************************************************************************
// ScrollViewAccessibilityProvider
//************************************************************************************************

class ScrollViewAccessibilityProvider: public ViewAccessibilityProvider,
									   public IAccessibilityScrollProvider
{
public:
	DECLARE_CLASS_ABSTRACT (ScrollViewAccessibilityProvider, ViewAccessibilityProvider)

	ScrollViewAccessibilityProvider (ScrollView& owner);

	// IAccessibilityScrollableProvider
	tbool CCL_API canScroll (AccessibilityScrollDirection direction) const override;
	tresult CCL_API scroll (AccessibilityScrollDirection direction, AccessibilityScrollAmount amount) override;
	tresult CCL_API scrollTo (double normalizedX, double normalizedY) override;
	double CCL_API getNormalizedScrollPositionX () const override;
	double CCL_API getNormalizedScrollPositionY () const override;
	int CCL_API getPagePositionX () const override;
	int CCL_API countPagesX () const override;
	int CCL_API getPagePositionY () const override;
	int CCL_API countPagesY () const override;

	CLASS_INTERFACE (IAccessibilityScrollProvider, ViewAccessibilityProvider)

protected:
	ScrollView& getScrollView () const;
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// ScrollView
//************************************************************************************************

ScrollView* ScrollView::getScrollView (const View* targetView)
{
	ScrollView* sv = targetView ? (ScrollView*)targetView->getParent (ccl_typeid<ScrollView> ()) : nullptr;
	if(sv && sv->getTarget () == targetView)
		return sv;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_STYLEDEF (ScrollView::customStyles)
	{"autohideh",	Styles::kScrollViewBehaviorAutoHideHBar},
	{"autohidev",	Styles::kScrollViewBehaviorAutoHideVBar},
	{"autohideboth",Styles::kScrollViewBehaviorAutoHideBoth},
	{"hbuttons",	Styles::kScrollViewAppearanceHButtons},
	{"vbuttons",	Styles::kScrollViewAppearanceVButtons},
	{"autobuttonsh",Styles::kScrollViewBehaviorAutoHideHButtons},
	{"autobuttonsv",Styles::kScrollViewBehaviorAutoHideVButtons},
	{"canscrollh",	Styles::kScrollViewBehaviorCanScrollH},
	{"canscrollv",	Styles::kScrollViewBehaviorCanScrollV},
	{"extendtarget",Styles::kScrollViewBehaviorExtendTarget},
	{"noscreenscroll",Styles::kScrollViewBehaviorNoScreenScroll},
	{"layeredscroll", Styles::kScrollViewBehaviorLayeredScroll},
	{"snapviews",	  Styles::kScrollViewBehaviorSnapToViews},
	{"snapviewsdeep", Styles::kScrollViewBehaviorSnapToViewsDeep},
	{"targetlimits",  Styles::kScrollViewBehaviorTargetLimits},
	{"scrollbypage",  Styles::kScrollViewBehaviorScrollByPage},
	{"mousescroll",   Styles::kScrollViewBehaviorMouseScroll}, 
	{"scrollnotify",  Styles::kScrollViewBehaviorNotifications},
	{"notiledlayers", Styles::kScrollViewBehaviorNoTiledLayers},
	{"relativeresize", Styles::kScrollViewBehaviorRelativeResize},
	{"snappedtarget", Styles::kScrollViewBehaviorSnappedTarget},
	{"omnidirectional", Styles::kScrollViewBehaviorOmniDirectional},
	{"noswipe",			Styles::kScrollViewBehaviorNoSwipe},
	{"allowzoom",		Styles::kScrollViewBehaviorAllowZoomGesture},
	{"limittoscreen",	Styles::kScrollViewBehaviorLimitToScreen},
	{"latchwheel",		Styles::kScrollViewBehaviorLatchWheel},
	{"pagecontrol",		Styles::kScrollViewAppearancePageControl},
	{"centertarget",    Styles::kScrollViewBehaviorCenterTarget},
	{"vscrollspace",    Styles::kScrollViewBehaviorVScrollSpace},
	{"hscrollspace",	Styles::kScrollViewBehaviorHScrollSpace},
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_VISUALSTYLE_CLASS (ScrollViewStyle, VisualStyle, "ScrollViewStyle")
	ADD_VISUALSTYLE_METRIC ("scrollBarSize")		///< size of scrollbars in pixels. For example: width for a vertical bar
	ADD_VISUALSTYLE_METRIC ("buttonSize")			///< size of scroll buttons in pixels. For example: width for a vertical button
	ADD_VISUALSTYLE_METRIC ("buttonSpacing")		///< spacing between scroll buttons and scrollview edge
	ADD_VISUALSTYLE_METRIC ("borderSize")			///< size (in pixels) of an optional border
	ADD_VISUALSTYLE_METRIC ("inertialDuration")		///< duration in seconds for inertial motion after user release the scrollview (not supported on all platforms)
	ADD_VISUALSTYLE_METRIC ("snapDepth")			///< limits the view recursion depth for option "snapviewsdeep" (default: unlimited)
END_VISUALSTYLE_CLASS (ScrollViewStyle)

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (ScrollView, View)
DEFINE_CLASS_UID (ScrollView, 0x4bd10568, 0x9659, 0x4a5c, 0x91, 0xdf, 0xa6, 0x60, 0xcf, 0xee, 0x24, 0x82)

//////////////////////////////////////////////////////////////////////////////////////////////////

ScrollView::ScrollView (const Rect& size, View* target, StyleRef style, VisualStyle* visualStyle, float _zoomFactor)
: View (size, style),
  clipView (nullptr),
  target (target),
  header (nullptr),
  vBar (nullptr),
  hBar (nullptr),
  vParam (nullptr),
  hParam (nullptr),
  snap (1, 1),
  scrollBarSize (-1),
  scrollButtonSize (-1),
  scrollButtonSpacing (-1),
  borderSize (-1),
  relativeResizeRatio (-1.f),
  savedScrollPos (-1, -1),
  scrollWheelLatched (false),
  lastScrollWheelEventTime (0)
{
	#if CCL_PLATFORM_IOS
	this->style.setCustomStyle (Styles::kScrollViewBehaviorLayeredScroll);
	#endif

	zoomFactor = _zoomFactor;
	setVisualStyle (visualStyle);

	setVScrollParam (NEW ScrollParam);
	vParam->release ();
	setHScrollParam (NEW ScrollParam);
	hParam->release ();

	if(target)
	{
		savedTargetSize = target->getSize ().getSize ();
		construct ();
		checkAutoHide ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ScrollView::~ScrollView ()
{
	clipView = nullptr; // isConstructed() returns false

	setVScrollParam (nullptr);
	setHScrollParam (nullptr);
	
	cancelSignals ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Coord ScrollView::getScrollBarSize () const
{
	if(scrollBarSize == -1)
	{
		const IVisualStyle& vs = getVisualStyle ();
		scrollBarSize = vs.getMetric ("scrollBarSize", -1);
		if(scrollBarSize == -1)
		{
			scrollBarSize = getTheme ().getThemeMetric (ThemeElements::kScrollBarSize);
			if(getStyle ().isSmall ())
				scrollBarSize /= 2;
		}
	}
	return ccl_to_int (scrollBarSize * getZoomFactor ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Coord ScrollView::getScrollButtonSize () const
{
	if(scrollButtonSize == -1)
	{
		const IVisualStyle& vs = getVisualStyle ();
		scrollButtonSize = vs.getMetric ("buttonSize", -1);
		if(scrollButtonSize == -1)
			scrollButtonSize = getScrollBarSize ();
	}
	return ccl_to_int (scrollButtonSize * getZoomFactor ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Coord ScrollView::getScrollButtonSpacing () const
{
	if(scrollButtonSpacing == -1)
	{
		const IVisualStyle& vs = getVisualStyle ();
		scrollButtonSpacing = vs.getMetric ("buttonSpacing", 0);
	}
	return ccl_to_int (scrollButtonSpacing * getZoomFactor ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Coord ScrollView::getBorderSize () const
{
	if(borderSize == -1)
	{
		const IVisualStyle& vs = getVisualStyle ();
		borderSize = vs.getMetric ("borderSize", -1);
		if(borderSize == -1)
			borderSize = getTheme ().getThemeMetric (ThemeElements::kBorderSize);
	}
	return ccl_to_int (borderSize * getZoomFactor ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ScrollView::isConstructed () const
{
	return clipView != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollView::construct ()
{
	// we need an extra view to clip out the scrollbars
	Rect r;
	Rect headerRect;
	clipView = NEW ScrollViewClipper (calcClipRect (r, headerRect));
	clipView->setSizeMode (kAttachAll);

	// pass translucent flag from scrollview or target view to clip view for layer-backing (alpha mode)
	if(getStyle ().isTranslucent () || target->getStyle ().isTranslucent ())
		StyleModifier (*clipView).setCommonStyle (Styles::kTranslucent);

	addView (clipView);
	clipView->addView (target);

	if(style.isCustomStyle (Styles::kScrollViewBehaviorLayeredScroll))
	{
		if(!style.isCustomStyle (Styles::kScrollViewBehaviorNoTiledLayers))
			target->isTiledLayerMode (true);
		clipView->setLayerBackingEnabled (true);
		target->setLayerBackingEnabled (true);
	}

	if(style.isCommonStyle (Styles::kScrollViewAppearanceVScrollBar))
		addVScrollBar (Rect (r.right, r.top, r.right + getScrollBarSize (), r.bottom));

	if(style.isCommonStyle (Styles::kScrollViewAppearanceHScrollBar))
		addHScrollBar (Rect (r.left, r.bottom, r.right, r.bottom + getScrollBarSize ()));

	if(style.isCustomStyle (Styles::kScrollViewAppearanceVButtons))
	{
		getScrollButtonSize ();
		getScrollButtonSpacing ();
		addScrollButtonUp   (Rect (r.left, r.top - scrollButtonSize - scrollButtonSpacing, r.right, r.top - scrollButtonSpacing));
		addScrollButtonDown (Rect (r.left, r.bottom + scrollButtonSpacing, r.right, r.bottom + scrollButtonSize + scrollButtonSpacing));
	}
	if(style.isCustomStyle (Styles::kScrollViewAppearanceHButtons))
	{
		getScrollButtonSize ();
		getScrollButtonSpacing ();
		addScrollButtonLeft  (Rect (r.left - scrollButtonSize - scrollButtonSpacing, r.top, r.left - scrollButtonSpacing, r.bottom));
		addScrollButtonRight (Rect (r.right + scrollButtonSpacing, r.top, r.right + scrollButtonSize + scrollButtonSpacing, r.bottom));
	}
	initScrollBars ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ScrollView::construct (IView* _target)
{
	ASSERT (target == nullptr)
	if(target)
		return kResultUnexpected;

	target = unknown_cast<View> (_target);
	ASSERT (target != nullptr)
	if(!target)
		return kResultInvalidArgument;

	construct ();
	checkAutoHide ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollView::setVScrollBarStyle (VisualStyle* visualStyle)
{
	vBarStyle = visualStyle;
	if(vBar)
		vBar->setVisualStyle (visualStyle);

	ForEachView (*this, view)
	if(ScrollButton* button = ccl_cast<ScrollButton> (view))
		if(button->getStyle ().isHorizontal () == false)
			button->setVisualStyle (visualStyle);
	EndFor	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollView::setHScrollBarStyle (VisualStyle* visualStyle)
{
	hBarStyle = visualStyle;
	if(hBar)
		hBar->setVisualStyle (visualStyle);
	
	ForEachView (*this, view)
	if(ScrollButton* button = ccl_cast<ScrollButton> (view))
		if(button->getStyle ().isHorizontal () == true)
			button->setVisualStyle (visualStyle);
	EndFor	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollView::addHScrollBar (RectRef rect)
{
	StyleFlags barStyle ((style.common & Styles::kSmall) | Styles::kHorizontal);
	if(style.isCustomStyle (Styles::kScrollViewBehaviorLayeredScroll))
		barStyle.setCommonStyle (Styles::kTranslucent); // background image might be (partially) translucent (we don't know)

	if(style.isCustomStyle (Styles::kScrollViewAppearancePageControl))
		hBar = NEW PageControl (rect, hParam, barStyle);
	else
		hBar = NEW ScrollBar (rect, hParam, barStyle);
		
	hBar->setZoomFactor (getZoomFactor ());
	hBar->setName ("hbar");
	if(theme)
		hBar->setTheme (theme);
	if(hBarStyle)
		hBar->setVisualStyle (hBarStyle);
	hBar->setSizeMode (kAttachLeft|kAttachRight|kAttachBottom);

	if(style.isCustomStyle (Styles::kScrollViewBehaviorLayeredScroll))
		hBar->setLayerBackingEnabled (true);

	insertView (0, hBar); // addView would lead to problems when this gets called during onSize...

	if(vBar)
	{
		Rect r (vBar->getSize ());
		r.bottom = rect.top;
		vBar->setSize (r);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollView::addVScrollBar (RectRef rect)
{
	StyleFlags barStyle ((style.common & Styles::kSmall) | Styles::kVertical);
	if(style.isCustomStyle (Styles::kScrollViewBehaviorLayeredScroll))
		barStyle.setCommonStyle (Styles::kTranslucent);

	if(style.isCustomStyle (Styles::kScrollViewAppearancePageControl))
		vBar = NEW PageControl (rect, vParam, barStyle);
	else
		vBar = NEW ScrollBar (rect, vParam, barStyle);
		
	vBar->setZoomFactor (getZoomFactor ());
	vBar->setName ("vbar");
	if(vBarStyle)
		vBar->setVisualStyle (vBarStyle);
	if(theme)
		vBar->setTheme (theme);
	vBar->setSizeMode (kAttachTop|kAttachBottom|kAttachRight);

	if(style.isCustomStyle (Styles::kScrollViewBehaviorLayeredScroll))
		vBar->setLayerBackingEnabled (true);

	insertView (0, vBar);

	if(hBar)
	{
		Rect r (hBar->getSize ());
		r.right = rect.left;
		hBar->setSize (r);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollView::removeHScrollBar ()
{
	if(vBar)
	{
		Rect r (vBar->getSize ());
		r.bottom = hBar->getSize ().bottom;
		vBar->setSize (r);
	}
	removeView (hBar);
	hBar->release ();
	hBar = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollView::removeVScrollBar ()
{
	if(hBar)
	{
		Rect r (hBar->getSize ());
		r.right = vBar->getSize ().right;
		hBar->setSize (r);
	}
	removeView (vBar);
	vBar->release ();
	vBar = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollView::addScrollButtonUp (RectRef r)
{
	addScrollButton (r, vParam, vBarStyle, Styles::kVertical, ScrollButton::kPartButtonDown, View::kAttachLeft|View::kAttachRight|View::kAttachTop);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollView::addScrollButtonDown (RectRef r)
{
	addScrollButton (r, vParam, vBarStyle, Styles::kVertical, ScrollButton::kPartButtonUp, View::kAttachLeft|View::kAttachRight|View::kAttachBottom);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollView::addScrollButtonLeft (RectRef r)
{
	addScrollButton (r, hParam, hBarStyle, Styles::kHorizontal, ScrollButton::kPartButtonDown, View::kAttachLeft|View::kAttachTop|View::kAttachBottom);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollView::addScrollButtonRight (RectRef r)
{
	addScrollButton (r, hParam, hBarStyle, Styles::kHorizontal, ScrollButton::kPartButtonUp, View::kAttachRight|View::kAttachTop|View::kAttachBottom);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollView::addScrollButton (RectRef rect, IParameter* param, VisualStyle* visualStyle, int orientation, int partCode, int sizemode)
{
	ScrollButton* button = NEW ScrollButton (rect, param, partCode);
	button->setSizeMode (sizemode);
	button->setStyle (orientation);
	if(visualStyle)
		button->setVisualStyle (visualStyle);
	if(theme)
		button->setTheme (theme);
	insertView (0, button);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollView::removeScrollButtons (bool horizontal)
{
	ForEachView (*this, view)
		if(ScrollButton* button = ccl_cast<ScrollButton> (view))
			if(button->getStyle ().isHorizontal () == horizontal)
			{
				removeView (button);
				button->release ();
			}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollView::setStyle (StyleRef newStyle)
{
	bool needHBar = newStyle.isCommonStyle (Styles::kScrollViewAppearanceHScrollBar);
	bool needVBar = newStyle.isCommonStyle (Styles::kScrollViewAppearanceVScrollBar);
	bool hasHBar = (hBar != nullptr);
	bool hasVBar = (vBar != nullptr);
	bool hscrollChanged = needHBar != hasHBar;
	bool vscrollChanged = needVBar != hasVBar;
	
	bool needHButtons = newStyle.isCustomStyle (Styles::kScrollViewAppearanceHButtons);
	bool needVButtons = newStyle.isCustomStyle (Styles::kScrollViewAppearanceVButtons);
	bool hasHButtons  = style.isCustomStyle (Styles::kScrollViewAppearanceHButtons);
	bool hasVButtons  = style.isCustomStyle (Styles::kScrollViewAppearanceVButtons);
	bool hButtonsChanged = needHButtons != hasHButtons;
	bool vButtonsChanged = needVButtons != hasVButtons;

	bool hasLayeredScroll = style.isCustomStyle (Styles::kScrollViewBehaviorLayeredScroll);

	SuperClass::setStyle (newStyle);

	if(hscrollChanged || vscrollChanged || hButtonsChanged || vButtonsChanged)
	{
		ASSERT (clipView)
		Rect clipRect (clipView->getSize ());
		if(hscrollChanged)
		{
			if(needHBar)
			{
				Rect barRect (clipRect);
				if(style.isCustomStyle (Styles::kScrollViewBehaviorHScrollSpace))
				{
					barRect.top = barRect.bottom;
					barRect.bottom = barRect.top + getScrollBarSize ();
				}
				else
				{
					barRect.top = barRect.bottom - getScrollBarSize ();
					clipRect.bottom = barRect.top;
				}
				addHScrollBar (barRect);
			}
			else
			{
				if(!style.isCustomStyle (Styles::kScrollViewBehaviorHScrollSpace))
					clipRect.bottom = hBar->getSize ().bottom;
				removeHScrollBar ();
			}
		}

		if(vscrollChanged)
		{
			Rect headerClipRect (clipRect);
			View* headerClipView = header ? header->getParent () : nullptr;
			if(headerClipView)
				headerClipRect = headerClipView->getSize ();
			else
				headerClipRect.setHeight (0);

			if(needVBar)
			{
				Rect barRect (clipRect);
				barRect.left = barRect.right - getScrollBarSize ();				
				barRect.top = headerClipRect.bottom;
				addVScrollBar (barRect);
				clipRect.right = barRect.left;
			}
			else
			{
				clipRect.right = vBar->getSize ().right;
				headerClipRect.right = clipRect.right;
				removeVScrollBar ();
			}

			if(headerClipView)
				headerClipView->setSize (headerClipRect);
		}

		if(hButtonsChanged)
		{
			if(needHButtons)
			{
				Rect buttonRect (clipRect);
				buttonRect.setWidth (getScrollButtonSize ());
				clipRect.left = buttonRect.right + getScrollButtonSpacing ();
				addScrollButtonLeft (buttonRect);


				buttonRect.right = clipRect.right;
				buttonRect.left  = clipRect.right - getScrollButtonSize ();
				clipRect.right = buttonRect.left - getScrollButtonSpacing ();
				addScrollButtonRight (buttonRect);
			}
			else
			{
				clipRect.left  -= (getScrollButtonSize () + getScrollButtonSpacing ());
				clipRect.right += (getScrollButtonSize () + getScrollButtonSpacing ());
				removeScrollButtons (true);
			}
		}

		if(vButtonsChanged)
		{
			if(needVButtons)
			{
				Rect buttonRect (clipRect);
				buttonRect.setHeight (getScrollButtonSize ());
				clipRect.top = buttonRect.bottom + getScrollButtonSpacing ();
				addScrollButtonUp (buttonRect);

				buttonRect.bottom = clipRect.bottom;
				buttonRect.top    = clipRect.bottom - getScrollButtonSize ();
				clipRect.bottom = buttonRect.top - getScrollButtonSpacing ();
				addScrollButtonDown (buttonRect);
			}
			else
			{
				clipRect.top    -= (getScrollButtonSize () + getScrollButtonSpacing ());
				clipRect.bottom += (getScrollButtonSize () + getScrollButtonSpacing ());
				removeScrollButtons (false);
			}
		}
		clipView->setSize (clipRect);
		initScrollBars ();
	}

	if(!hasLayeredScroll && style.isCustomStyle (Styles::kScrollViewBehaviorLayeredScroll))
	{
		// (style is set after construct () when created via ccl_new)
		if(target)
		{
			if(!style.isCustomStyle (Styles::kScrollViewBehaviorNoTiledLayers))
				target->isTiledLayerMode (true);
			
			target->setLayerBackingEnabled (true);
		}
		if(clipView)
			clipView->setLayerBackingEnabled (true);
	}

	if(target && clipView)
		checkAutoHide ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollView::checkAutoHide ()
{
	ASSERT (target && clipView)
	bool autoHideH = style.isCustomStyle (Styles::kScrollViewBehaviorAutoHideHBar);
	bool autoHideV = style.isCustomStyle (Styles::kScrollViewBehaviorAutoHideVBar);
	bool autoHideHButtons = style.isCustomStyle (Styles::kScrollViewBehaviorAutoHideHButtons);
	bool autoHideVButtons = style.isCustomStyle (Styles::kScrollViewBehaviorAutoHideVButtons);

	if(autoHideH || autoHideV || autoHideHButtons || autoHideVButtons)
	{
		RectRef targetClip (clipView->getSize ());
		Coord targetW = savedTargetSize.x;
		Coord targetH = savedTargetSize.y;
		Coord clipW = targetClip.getWidth ();
		Coord clipH = targetClip.getHeight ();

		// first treat hideable scrollbars as hidden
		if(hBar && autoHideH)
			clipH += hBar->getHeight ();
		if(vBar && autoHideV)
			clipW += vBar->getWidth ();

		// treat hideable buttons as hidden
		bool hasHButtons = style.isCustomStyle (Styles::kScrollViewAppearanceHButtons);
		bool hasVButtons = style.isCustomStyle (Styles::kScrollViewAppearanceVButtons);
		if(hasHButtons)
			clipW += (getScrollButtonSize () + getScrollButtonSpacing ()) * 2;
		if(hasVButtons)
			clipH += (getScrollButtonSize () + getScrollButtonSpacing ()) * 2;

		bool needHBar = autoHideH ? targetW > clipW : (hBar != nullptr);
		bool needVBar = autoHideV ? targetH > clipH : (vBar != nullptr);
		bool needHButtons = autoHideHButtons ? targetW > clipW : hasHButtons;
		bool needVButtons = autoHideVButtons ? targetH > clipH : hasVButtons;

		// a scrollbar for one direction results in less space in the other direction
		if(autoHideV && autoHideH)
		{
			if(needVBar)
			{
				clipW -= getScrollBarSize ();
				needHBar = targetW > clipW;
			}
			else if(needHBar)
			{
				clipH -= getScrollBarSize ();
				needVBar = targetH > clipH;
			}
		}
		CCL_PRINTF ("ScrollView::checkAutoHide: SV (%d, %d), Clip (%d, %d), Target (%d, %d) %s %s\n",
			size.getWidth (), size.getHeight (), clipW, clipH, targetW, targetH,
			needVBar ? "needVBar" : "", needHBar ? "needHBar" : "");

		StyleFlags newStyle (getStyle ());
		newStyle.setCommonStyle (Styles::kScrollViewAppearanceHScrollBar, needHBar);
		newStyle.setCommonStyle (Styles::kScrollViewAppearanceVScrollBar, needVBar);
		newStyle.setCustomStyle (Styles::kScrollViewAppearanceHButtons, needHButtons);
		newStyle.setCustomStyle (Styles::kScrollViewAppearanceVButtons, needVButtons);
		if(newStyle != getStyle ())
			setStyle (newStyle);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollView::checkClientSnapSize ()
{
	if(style.isCustomStyle (Styles::kScrollViewBehaviorSnappedTarget))
	{
		Rect clipRect (clipView->getSize ());
		clipRect.bottom -= clipRect.getHeight () % snap.y;
		clipView->setSize (clipRect);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollView::setHeader (View* _header)
{
	if(header)
	{
		View* headerClipView = header->getParent ();
		ASSERT (headerClipView)
		if(headerClipView)
		{
			headerClipView->removeView (header);
			header->release ();
			removeView (headerClipView);
			headerClipView->release ();
		}
	}

	header = _header;

	Rect clipRect;
	Rect headerClipRect;
	calcClipRect (clipRect, headerClipRect);

	if(header)
	{
		// move header to current scroll position
		Coord targetPos = target ? target->getSize ().left : 0;
		Coord targetWidth = target ? target->getWidth () : 0;
		Rect r (header->getSize ());
		r.moveTo (Point (targetPos, 0));
		r.setWidth (ccl_max (targetWidth, headerClipRect.getWidth ()));
		header->setSize (r);

		View* headerClipView = NEW View (headerClipRect);
		headerClipView->setSizeMode (kAttachLeft|kAttachRight|kAttachTop);
		headerClipView->addView (header);
		addView (headerClipView);
	}
	clipView->setSize (clipRect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ScrollView::setVScrollParam (IParameter* param)
{
	if(vParam)
	{
		UnknownPtr<ISubject> (vParam)->removeObserver (this);
		vParam->release ();
	}
	vParam = param;
	if(vParam)
	{
		vParam->retain ();
		UnknownPtr<ISubject> (vParam)->addObserver (this);
	
		if(vBar)
			vBar->setParameter (vParam);
	}

	if(isConstructed ())
		initScrollBars ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ScrollView::setHScrollParam (IParameter* param)
{
	if(hParam)
	{
		UnknownPtr<ISubject> (hParam)->removeObserver (this);
		hParam->release ();
	}
	hParam = param;
	if(hParam)
	{
		hParam->retain ();
		UnknownPtr<ISubject> (hParam)->addObserver (this);

		if(hBar)
			hBar->setParameter (hParam);
	}

	if(isConstructed ())
		initScrollBars ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* CCL_API ScrollView::getVScrollParam ()
{
	return vParam;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* CCL_API ScrollView::getHScrollParam ()
{
	return hParam;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect& ScrollView::calcClipRect (Rect& targetClip, Rect& headerClip) const
{
	getClientRect (targetClip);
	if(style.isBorder ())
		targetClip.contract (getBorderSize ());

	if(style.isCommonStyle (Styles::kScrollViewAppearanceVScrollBar))
		targetClip.right -= getScrollBarSize ();
	if(style.isCommonStyle (Styles::kScrollViewAppearanceHScrollBar) || style.isCustomStyle (Styles::kScrollViewBehaviorHScrollSpace))
		targetClip.bottom -= getScrollBarSize ();

	if(style.isCustomStyle (Styles::kScrollViewAppearanceHButtons))
	{
		targetClip.left  += getScrollBarSize ();
		targetClip.right -= getScrollBarSize ();
	}
	if(style.isCustomStyle (Styles::kScrollViewAppearanceVButtons))
	{
		targetClip.top    += getScrollBarSize ();
		targetClip.bottom -= getScrollBarSize ();
	}

	if(header)
	{
		headerClip = targetClip;
		headerClip.setHeight (header->getHeight ());
		targetClip.top = headerClip.bottom;
	}
	else
		headerClip.setEmpty ();
	return targetClip;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect& CCL_API ScrollView::getScrollSize (Rect& r) const
{
	clipView->getClientRect (r);
	return r;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SizeLimit ScrollView::getClipViewLimits () const
{
	SizeLimit limits;
	limits.setUnlimited ();
	
	// 1. target limits
	if(style.isCustomStyle (Styles::kScrollViewBehaviorTargetLimits))
	{
		// maximum: not larger than target limits allow
		limits.maxWidth = target->getSizeLimits ().maxWidth;
		limits.maxHeight = target->getSizeLimits ().maxHeight;

		// also respect target's minimum limits in the non-scrolling direction
		if(!canScrollH ())
			limits.minWidth = target->getSizeLimits ().minWidth;
		if(!canScrollV ())
			limits.minHeight = target->getSizeLimits ().minHeight;
	}

	// 2. limits from visual style
	Coord maxW = getVisualStyle ().getMetric<Coord> ("clip.maxWidth", kMaxCoord);
	Coord maxH = getVisualStyle ().getMetric<Coord> ("clip.maxHeight", kMaxCoord);
	if(maxW < kMaxCoord && target->getSizeLimits ().minWidth > maxW)
		limits.minWidth = limits.maxWidth = maxW;
	if(maxH < kMaxCoord && target->getSizeLimits ().minHeight > maxH)
		limits.minHeight = limits.maxHeight = maxH;
	
	return limits;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollView::calcSizeLimits ()
{
	// smallest possible size is determined by all surrounding elements: scrollbars, header, border, etc.
	Rect clipRect;
	Rect headerClipRect;
	calcClipRect (clipRect, headerClipRect);
	Point decorSize = getSize ().getSize () - clipRect.getSize ();

	// default: as large as you want
	sizeLimits.maxWidth  = kMaxCoord;
	sizeLimits.maxHeight = kMaxCoord;

	SizeLimit clipLimits (getClipViewLimits ());

	sizeLimits.minWidth  = clipLimits.minWidth + decorSize.x;
	sizeLimits.minHeight = clipLimits.minHeight + decorSize.y;

	if(clipLimits.maxWidth >= 0 && clipLimits.maxWidth < kMaxCoord)
		sizeLimits.maxWidth = clipLimits.maxWidth + decorSize.x;

	if(clipLimits.maxHeight >= 0 && clipLimits.maxHeight < kMaxCoord)
		sizeLimits.maxHeight = clipLimits.maxHeight + decorSize.y;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollView::passDownSizeLimits ()
{
	// don't pass down further to scrollbars, clip view, etc...
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollView::limitToScreenSize (Rect& scrollViewSize)
{
	Window* window = getWindow ();
	if(window)
	{
		Point pos;
		clientToScreen (pos);
		Rect sizeOnScreen (scrollViewSize);
		sizeOnScreen.moveTo (pos);

		// find monitor: use center of top window edge (center of window might be outside screen before we try to repair)
		int monitor = Desktop.findMonitor (Point (sizeOnScreen.getCenter ().x, sizeOnScreen.top), true);
		if(monitor >= 0)
		{
			// our size + distance to outer window frame must not exceed monitor size
			Rect monitorSize;
			Desktop.getMonitorSize (monitorSize, monitor, true);

			Rect frameSize;
			window->getFrameSize (frameSize);

			Point paddingFromWindow = frameSize.getSize () - getSize().getSize ();
			Point available = monitorSize.getSize () - paddingFromWindow;

			ccl_upper_limit (scrollViewSize.right, scrollViewSize.left + available.x);
			ccl_upper_limit (scrollViewSize.bottom, scrollViewSize.top + available.y);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollView::onColorSchemeChanged (const ColorSchemeEvent& event)
{
	SuperClass::onColorSchemeChanged (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollView::draw (const UpdateRgn& updateRgn)
{
	drawBackground (updateRgn);

	View::draw (updateRgn);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollView::drawBackground (const UpdateRgn& updateRgn)
{ 
	if(style.isOpaque ())
	{
		GraphicsPort port (this);

		if(!hasVisualStyle ())
			port.fillRect (updateRgn.bounds, SolidBrush (getTheme ().getThemeColor (ThemeElements::kListViewBackColor)));
		else
		{
			IImage* background = getVisualStyle ().getBackgroundImage ();
			if(background)
			{
				Rect rect;
				getClientRect (rect);
				port.drawImage (background, Rect (0, 0, background->getWidth (), background->getHeight ()), rect);
			}
			else
				port.fillRect (updateRgn.bounds, getVisualStyle ().getBackBrush ());
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Point& CCL_API ScrollView::getSnap () const
{ 
	return snap; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ScrollView::setSnap (const Point& _snap)
{
	if(!style.isCustomStyle (Styles::kScrollViewBehaviorSnapToViews))
		snap = _snap;

	initScrollBars ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollView::snapTargetPos (Point& targetPos, PointRef direction)
{
	Point start (targetPos);
	start *= -1;
	Point offset;

	Point endPos (target->getSize ().getSize () - clipView->getSize ().getSize ());

	struct SnapFinder
	{
		Point& start;
		Point nearestLower;
		Point nearestUpper;
		Coord recursionMinSize;
		int maxRecursionDepth;

		SnapFinder (PointRef endPos, Point& start)
		: nearestUpper (endPos), start (start), recursionMinSize (kMaxCoord), maxRecursionDepth (NumericLimits::kMaxInt)
		{}

		inline void addPositionX (Coord x)
		{
			if(x < start.x)
				ccl_lower_limit (nearestLower.x, x);
			else
				ccl_upper_limit (nearestUpper.x, x);
		}

		inline void addPositionY (Coord y)
		{
			if(y < start.y)
				ccl_lower_limit (nearestLower.y, y);
			else
				ccl_upper_limit (nearestUpper.y, y);
		}

		void addChildViewsX (View& parent, Coord childOffset, int depth)
		{
			if(depth > maxRecursionDepth)
				return;

			ForEachViewFast (parent, v)
				Coord x = childOffset + v->getSize ().left;
				addPositionX (x);

				if(v->getWidth () >= recursionMinSize && v)
					addChildViewsX (*v, x, depth + 1);
			EndFor
		}

		void addChildViewsY (View& parent, Coord childOffset, int depth)
		{
			if(depth > maxRecursionDepth)
				return;

			ForEachViewFast (parent, v)
				Coord y = childOffset + v->getSize ().top;
				addPositionY (y);

				if(v->getHeight () >= recursionMinSize)
					addChildViewsY (*v, y, depth + 1);
			EndFor
		}

	} snapFinder (endPos, start);

	snapFinder.maxRecursionDepth = getVisualStyle ().getMetric ("snapDepth", NumericLimits::kMaxInt16);

	Vector<Point> snapPositions;
	if(isScrollByPageEnabled ())
	{
		// page size defaults to clipView, but can be overridden by configuration 
		Point pageSize;
		getScrollByPageSize (pageSize);
		ccl_lower_limit (pageSize.x, 10);
		ccl_lower_limit (pageSize.y, 10);
		
		// find nearest page snap positions
		Point p;
		while(true)
		{
			snapFinder.addPositionX (p.x);
			snapFinder.addPositionY (p.y);

			if(p == endPos)
				break;

			p += pageSize;
			ccl_upper_limit (p.x, endPos.x);
			ccl_upper_limit (p.y, endPos.y);
		}
	}
	else if(getStyle ().isCustomStyle (Styles::kScrollViewBehaviorSnapToViews))
	{
		// find topmost view in target with more than one child
		View* snapParent = target;
		while(!snapParent->isEmpty () && snapParent->getFirst () == snapParent->getLast ())
		{
			snapParent = snapParent->getFirst ();
			offset += snapParent->getSize ().getLeftTop ();
		}
		start -= offset;

		// use child view positions (left/top) for snapping
		ASSERT (snapParent)
		if(getStyle ().isCustomStyle (Styles::kScrollViewBehaviorSnapToViewsDeep))
			snapFinder.recursionMinSize = 100;

		snapFinder.addChildViewsX (*snapParent, 0, 0);
		snapFinder.addChildViewsY (*snapParent, 0, 0);
	}
	else
		return;

	// choose nearest position
	Point p (start);
	if(direction.x == 0)
		p.x = ((start.x - snapFinder.nearestLower.x) < (snapFinder.nearestUpper.x - start.x)) ? snapFinder.nearestLower.x : snapFinder.nearestUpper.x;
	else
		p.x = (direction.x > 0) ? snapFinder.nearestLower.x : snapFinder.nearestUpper.x;
		
	if(direction.y == 0)
		p.y = (start.y - snapFinder.nearestLower.y) < (snapFinder.nearestUpper.y - start.y) ? snapFinder.nearestLower.y : snapFinder.nearestUpper.y;
	else
		p.y = (direction.y > 0) ? snapFinder.nearestLower.y : snapFinder.nearestUpper.y;

	CCL_PRINTF ("ScrollView::Snap %d, %d\n", p.x, p.y)

	p += offset;
	p *= -1;
	targetPos = p;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static void setScrollParamRange (IParameter* param, int range, float pageSize)
{
	UnknownPtr<IScrollParameter> sParam (param);
	if(sParam)
		sParam->setRange (range, pageSize);
	else
	{
		param->setMin (0);
		param->setMax (range);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollView::initScrollBars ()
{
	if(vParam)
	{
		int range = target->getHeight () - clipView->getHeight ();
		if(range < 0)
			range = 0;
		float pageSize = target->getHeight () != 0 ? (float)clipView->getHeight () / (float)target->getHeight () : 0;

		int max = range / snap.y;
		if(max % snap.y)
			max++;
		if(max * snap.y < range) // end coord must be reachable
			max++;
		ASSERT (max * snap.y >= range)

		if(range > 0)
			ccl_lower_limit (max, 1); // must be able to scroll

		if(target->getHeight () > 0)
		{
			setScrollParamRange (vParam, max, pageSize);
			vParam->setValue (abs (target->getSize ().top) / snap.y);

			if(style.isCustomStyle (Styles::kScrollViewBehaviorNotifications))
				if(UnknownPtr<IObserver> observer = vParam->getController ())
					(NEW Message (IParameter::kRangeChanged))->post (observer);
		}
		
		// scroll immediately
//		notify (UnknownPtr<ISubject> (vParam), Message (kChanged));
	}

	if(hParam)
	{
		int range = target->getWidth () - clipView->getWidth ();
		if(range < 0)
			range = 0;
		float pageSize = target->getWidth () != 0 ? (float)clipView->getWidth () / (float)target->getWidth () : 0;

		int max = range / snap.x;
		if(max % snap.x)
			max++;
		if(max * snap.x < range) // end coord must be reachable
			max++;
		ASSERT (max * snap.x >= range)

		if(range > 0)
			ccl_lower_limit (max, 1); // must be able to scroll

		if(target->getWidth () > 0)
		{
			setScrollParamRange (hParam, max, pageSize);
			hParam->setValue (abs (target->getSize ().left) / snap.x);

			if(style.isCustomStyle (Styles::kScrollViewBehaviorNotifications))
				if(UnknownPtr<IObserver> observer = hParam->getController ())
					(NEW Message (IParameter::kRangeChanged))->post (observer);
		}
		
		// scroll immediately
//		notify (UnknownPtr<ISubject> (hParam), Message (kChanged));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ScrollBar* ScrollView::getVScrollBar ()
{
	return vBar;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ScrollBar* ScrollView::getHScrollBar ()
{
	return hBar;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollView::scrollClientToTargetRect (RectRef newTarget)
{
	Rect oldTarget (target->getSize ());
	if(oldTarget != newTarget)
	{
		if(style.isCustomStyle (Styles::kScrollViewBehaviorLayeredScroll))
			target->setSize (newTarget, false);
		else if(oldTarget.intersect (newTarget) && isAttached ())
		{
			Rect rect;
			target->getVisibleClient (rect);

			Rect visibleClipper;
			clipView->getVisibleClient (visibleClipper);

			Point delta = newTarget.getLeftTop () - oldTarget.getLeftTop ();
			Point cut = delta - (visibleClipper.getSize () - rect.getSize ());
			if(delta.x != 0)
			{
				Rect invalidRect;
				if(delta.x > 0)
				{
					rect.right -= cut.x;
					if(rect.getWidth () < cut.x)
						invalidRect (rect.right, rect.top, rect.left + cut.x, rect.bottom);
				}
				else
				{
					rect.left -= cut.x;
					if(rect.getWidth () < -cut.x)
						invalidRect (rect.right + cut.x, rect.top, rect.left, rect.bottom);
				}

				if(!invalidRect.isEmpty ())
					target->invalidate (invalidRect);
			}

			if(delta.y != 0)
			{
				Rect invalidRect;
				if(delta.y > 0)
				{
					rect.bottom -= cut.y;
					if(rect.getHeight () < cut.y)
						invalidRect (rect.left, rect.bottom, rect.right, rect.top + cut.y);
				}
				else
				{
					rect.top -= cut.y;
					if(rect.getHeight () < -cut.y)
						invalidRect (rect.left, rect.bottom + cut.y, rect.right, rect.top);
				}

				if(!invalidRect.isEmpty ())
					target->invalidate (invalidRect);
			}

			Point p;
			rect.offset (windowToClient (target->clientToWindow (p)));
				
			target->setSize (newTarget, false);

			scrollClientInternal (rect, delta);
				
			Window* window = getWindow ();
			if(window && !window->shouldCollectUpdates ())
				window->redraw ();
		}
		else
			target->setSize (newTarget);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void ScrollView::scrollClientInternal (RectRef rect, PointRef delta)
{
	if(!rect.isEmpty ())
	{
		if(style.isCustomStyle (Styles::kScrollViewBehaviorNoScreenScroll))
		{
			clipView->invalidate ();
			if(header)
				header->invalidate ();
		}
		else
			scrollClient (rect, delta);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ScrollView::notify (ISubject* subject, MessageRef msg)
{
	if(isManipulating ())
		return;

	if(UnknownPtr<IParameter> (subject) == vParam && msg == kChanged)
	{
		if(isAnimatingY ())
		{
			savePosition ();
			return;
		}

		Rect newTarget;
		target->getClientRect (newTarget);
		newTarget.offset (target->getSize ().left, -vParam->getValue ().asInt () * snap.y);
		checkPosition (newTarget);

		scrollClientToTargetRect (newTarget);

		savePosition ();
	}
	else if(UnknownPtr<IParameter> (subject) == hParam && msg == kChanged)
	{
		if(isAnimatingX ())
		{
			savePosition ();
			return;
		}
		
		Rect newTarget;
		target->getClientRect (newTarget);
		newTarget.offset (-hParam->getValue ().asInt () * snap.x, target->getSize ().top);
		checkPosition (newTarget);

		scrollClientToTargetRect (newTarget);

		syncHeader (newTarget.left);
		savePosition ();
	}
	else if(msg == IScrollParameter::kStopAnimations)
	{
		stopAnimations ();
	}
	else if(msg == IScrollParameter::kAnimationAdded || msg == IScrollParameter::kAnimationRemoved)
	{
		synchronizeAnimation (subject, msg);
	}
	else if(msg == "onContinuousWheelEnded")
	{
		Point direction (-msg[0].asInt (), -msg[1].asInt ());
		onContinuousWheelEnded (direction);
	}
	else if(msg == "checkScreenSize")
	{
		if(isAttached ())
		{
			Rect rect (getSize ());
			limitToScreenSize (rect);
			setSize (rect);
		}
		else
			(NEW Message ("checkScreenSize"))->post (this);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollView::onContinuousWheelEnded (Point& direction)
{
	Point snappedPos;
	getPosition (snappedPos);
	
	Point pageSize;
	getScrollByPageSize (pageSize);

	//Point direction (startPos - snappedPos);
	CCL_PRINTF ("ScrollView::onContinuousWheelEnded (direction: %d, currentPos %d, ", direction.x, snappedPos.x)
		
	// move mouse at least kMinimalPagingMovement points before snapping to next page
	if((pageSize.x + pageSize.y) > 200)
	{
		Point minimalWheelMovement (kMinimalPagingMovement, kMinimalPagingMovement);

		Coord currentAbsX (snappedPos.x * ccl_sign (snappedPos.x));
		currentAbsX += minimalWheelMovement.x;
		Coord remainderX (currentAbsX % pageSize.x);
		if(remainderX < (2 * minimalWheelMovement.x))
			direction.x = 0;

		Coord currentAbsY (snappedPos.y * ccl_sign (snappedPos.y));
		currentAbsY += minimalWheelMovement.y;
		Coord remainderY (currentAbsX % pageSize.y);
		if(remainderY < (2 * minimalWheelMovement.y))
			direction.y = 0;

		CString dirString ((direction.x < 0) ? "right" : (direction.x > 0) ? "left" : "0");
		CCL_PRINTF ("new direction: %s)\n", dirString.str ());
	}
	snapTargetPos (snappedPos, direction);

	CCL_PRINTF ("targetSnappedPos: %d)\n", snappedPos.x);

	scrollTo (snappedPos, 0.5f, 2000.f);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAttributeList* ScrollView::getViewState (tbool create)
{
	if(!persistenceID.isEmpty () && (hParam || vParam))
	{
		ILayoutStateProvider* provider = target ? UnknownPtr<ILayoutStateProvider> (target->getController ()) : nullptr;
		if(!provider)
			provider = GetViewInterfaceUpwards<ILayoutStateProvider> (this);
		if(provider)
			return provider->getLayoutState (persistenceID, create);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ScrollView::isScrollByPageEnabled () const
{
	if(style.isCustomStyle (Styles::kScrollViewBehaviorScrollByPage))
	   return true;
	
	bool enabled = false;
	if(!persistenceID.isEmpty ())
		Configuration::Registry::instance ().getValue (enabled, "GUI.Controls.ScrollByPage", persistenceID);
	return enabled;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PointRef ScrollView::getScrollByPageSize (Point& size) const
{
	size = clipView->getSize ().getSize ();

	if(!persistenceID.isEmpty ())
	{
		Configuration::Registry::instance ().getValue (size.x, "GUI.Controls.ScrollByPage.width", persistenceID);
		Configuration::Registry::instance ().getValue (size.y, "GUI.Controls.ScrollByPage.height", persistenceID);
	}
	return size;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollView::savePosition ()
{
	// store scroll positions
	if(IAttributeList* attribs = getViewState (true))
	{
		if(hParam)
		{
			savedScrollPos.x = hParam->getValue ().asInt ();

			// in relativeResize mode, save normalized values (restore might happen before the view has it's final size)
			if(style.isCustomStyle (Styles::kScrollViewBehaviorRelativeResize))
				attribs->setAttribute ("hn", hParam->getNormalized ());
			else
				attribs->setAttribute ("h", savedScrollPos.x);
		}
		if(vParam)
		{
			savedScrollPos.y = vParam->getValue ().asInt ();

			if(style.isCustomStyle (Styles::kScrollViewBehaviorRelativeResize))
				attribs->setAttribute ("vn", vParam->getNormalized ());
			else
				attribs->setAttribute ("v", savedScrollPos.y);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollView::restorePosition ()
{
	if(getSize ().isEmpty () == false)
	{
		if(makeVisibleRect.isEmpty () == false)
		{
			// makeVisibleRect is in target coords
			if(canScrollH ())
			{
				Coord deltaX = savedScrollPos.x * snap.x;
				if(makeVisibleRect.left < deltaX || makeVisibleRect.right > (deltaX + getWidth ()))
					savedScrollPos.x = ccl_max (0, makeVisibleRect.left / snap.x);
			}

			if(canScrollV ())
			{
				Coord deltaY = savedScrollPos.y * snap.y;
				if(makeVisibleRect.top < deltaY || makeVisibleRect.bottom > (deltaY + getHeight ()))
					savedScrollPos.y = ccl_max (0, makeVisibleRect.top / snap.y);
			}

			makeVisibleRect.setEmpty ();
		}
	}
	
	if(savedScrollPos != Point (-1, -1))
	{
		if(savedScrollPos.x == -1)
			savedScrollPos.x = 0;
		if(savedScrollPos.y == -1)
			savedScrollPos.y = 0;
		
		Rect targetClient;
		target->getClientRect (targetClient);
		targetClient.offset (-savedScrollPos.x * snap.x, -savedScrollPos.y * snap.y);
		target->setSize (targetClient);

		initScrollBars ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollView::attached (View* parent)
{
	// draw clip view background only when using layers, but not if scrollview or target is transparent (to be set before the clipView is attached via SuperClass::attached)
	if(!style.isCustomStyle (Styles::kScrollViewBehaviorLayeredScroll) || getStyle ().isTransparent () || target->getStyle ().isTransparent ())
		StyleModifier (*clipView).setCommonStyle (Styles::kTransparent);
	
	SuperClass::attached (parent);

	// reset layer backing flag if layers are not available
	if(style.isCustomStyle (Styles::kScrollViewBehaviorLayeredScroll) && !clipView->isLayerBackingEnabled ())
	{
		style.custom &= ~Styles::kScrollViewBehaviorLayeredScroll;
		// but remember that layered scroll was requested (used for wheel)
		simulateLayeredScroll (true);
	}
	
	// restore scroll positions
	if(IAttributeList* attribs = getViewState (false))
	{
		AttributeAccessor a (*attribs);
		if(style.isCustomStyle (Styles::kScrollViewBehaviorRelativeResize))
		{
			float hNorm = 0, vNorm = 0;
			a.getFloat (hNorm, "hn");
			a.getFloat (vNorm, "vn");
			savedScrollPos.x = coordFToInt (hNorm * hParam->getMax ().asInt () * snap.x);
			savedScrollPos.y = coordFToInt (vNorm * vParam->getMax ().asInt () * snap.y);
		}
		else
		{
			a.getInt (savedScrollPos.x, "h");
			a.getInt (savedScrollPos.y, "v");
		}		
		restorePosition ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollView::removed (View* parent)
{
	stopAnimations ();
	SuperClass::removed (parent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollView::checkPosition (Rect& targetRect)
{
	// *** Vertical ***
	Coord uncovered = clipView->getHeight () - targetRect.bottom;
	if(uncovered > 0)
	{
		targetRect.offset (0, uncovered);
		if(targetRect.top > 0)
		{
			Coord top = 0;
			if(style.isCustomStyle (Styles::kScrollViewBehaviorCenterTarget))
				top = (clipView->getHeight () - targetRect.getHeight ()) / 2;
	
			targetRect.moveTo (Point (targetRect.left, top));
		}
	}

	// *** Horizontal ***
	uncovered = clipView->getWidth () - targetRect.right;
	if(uncovered > 0)
	{
		targetRect.offset (uncovered);
		if(targetRect.left > 0)
		{				
			Coord left = 0;
			if(style.isCustomStyle (Styles::kScrollViewBehaviorCenterTarget))
				left = (clipView->getWidth () - targetRect.getWidth ()) / 2;
			
			targetRect.moveTo (Point (left, targetRect.top));
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollView::checkPosition ()
{
	Rect targetRect (target->getSize ());
	checkPosition (targetRect);
	target->setSize (targetRect);

	syncHeader (targetRect.left);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollView::syncHeader (Coord scrollPos)
{ 
	ASSERT (target && clipView)
	if(header)
	{
		Rect headerRect (header->getSize ());
		headerRect.left = scrollPos;
		headerRect.setWidth (ccl_max (target->getWidth (), clipView->getWidth ()));
		header->setSize (headerRect);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ScrollView::setTargetSize (const Rect& wantedSize)
{ 
	if(target)
	{
		savedTargetSize = wantedSize.getSize ();

		ScopedFlag<kResizingTarget> guard (privateFlags);

		bool fitH = (getSizeMode () & kHFitSize) != 0;
		bool fitV = (getSizeMode () & kVFitSize) != 0;
		bool extendTarget = style.isCustomStyle (Styles::kScrollViewBehaviorExtendTarget);
		bool centerTarget = style.isCustomStyle (Styles::kScrollViewBehaviorCenterTarget);

		if(!fitH && !fitV && !extendTarget)
		{
			target->setSize (wantedSize);
			savedTargetSize = wantedSize.getSize ();
			checkPosition (); // todo: check if called anyway..?
			return;
		}

		// do it twice to master autohide scrollbar changes // todo: smarter...
		for(int i = 0; i < 2; i++)
		{
			Rect scrollRect;
			getScrollSize (scrollRect);

			Coord w = savedTargetSize.x;
			Coord h = savedTargetSize.y;
			if(!fitH && !centerTarget)
				ccl_lower_limit (w, scrollRect.getWidth ());
			if(!fitV && !centerTarget)
				ccl_lower_limit (h, scrollRect.getHeight ());

			if(hasExplicitSizeLimits  ())
			{
				// respect explicit minimal size of scrollView (we want to fill the scrollRect)
				const SizeLimit& scrollLimits (getSizeLimits ());
				if(fitH)
				{
					Coord minW = ccl_max (scrollLimits.minWidth - (getWidth () - scrollRect.right), 0);
					ccl_lower_limit (w, minW);
				}
				if(fitV)
				{
					Coord minH = ccl_max (scrollLimits.minHeight - (getHeight () - scrollRect.bottom), 0);
					ccl_lower_limit (h, minH);
				}
			}

			Rect r (0, 0, w ,h);

			if(w > scrollRect.getWidth ())
			{
				Coord maxLeft = w - scrollRect.getWidth ();
				Coord left = abs (target->getSize ().left);
				r.offset (-ccl_min<Coord> (left, maxLeft));
			}
			else if(w < scrollRect.getWidth () && centerTarget)
			{
				Coord diff = scrollRect.getWidth () - w;
				r.offset (diff / 2);
			}

			if(h > scrollRect.getHeight ())
			{
				Coord maxTop = h - scrollRect.getHeight ();
				Coord top = abs (target->getSize ().top);
				r.offset (0, -ccl_min<Coord> (top, maxTop));
			}
			else if(h < scrollRect.getHeight () && centerTarget)
			{
				Coord diff = scrollRect.getHeight () - h;
				r.offset (0, diff / 2);
			}

			target->setSize (r);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollView::onSize (const Point& delta)
{
	Rect rect;
	getClientRect (rect);

	if(style.isCustomStyle (Styles::kScrollViewBehaviorRelativeResize))
	{
		if(relativeResizeRatio == -1.f)
		{
			Rect oldTargetRect (getTargetView ()->getSize ());
			if(style.isVertical ())
				relativeResizeRatio = oldTargetRect.getHeight () / (float)(rect.getHeight () - delta.y);
			else
				relativeResizeRatio = oldTargetRect.getWidth () / (float)(rect.getWidth () - delta.x);
		}
	}
	
	if(style.isBorder ())
	{
		Coord border = getBorderSize ();
		Rect h (rect);
		if(delta.x > 0)
		{
			h.right = h.right - delta.x;
			h.left = h.right - border;
			invalidate (h);
		}
		else if(delta.x < 0)
		{
			h.left = h.right - border;
			invalidate (h);
		}

		if(delta.y > 0)
		{
			rect.bottom = rect.bottom - delta.y;
			rect.top = rect.bottom - border;
			invalidate (rect);
		}
		else if(delta.y < 0)
		{
			rect.top = rect.bottom - border;
			invalidate (rect);
		}
	}

	View::onSize (delta);
	
	if(target)
	{
		if(style.isCustomStyle (Styles::kScrollViewBehaviorRelativeResize))
		{
			resizeTargetRelative (rect);
			signal (Message (kSizeChanged));
			return;
		}

		if(style.isCustomStyle (Styles::kScrollViewBehaviorExtendTarget))
			setTargetSize (savedTargetSize);
			
		restorePosition ();
		checkPosition ();
		checkAutoHide ();
		checkClientSnapSize ();
		initScrollBars ();
	}
	
	signal (Message (kSizeChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollView::resizeTargetRelative (const Rect& rect)
{
	Rect targetRect (getTargetView ()->getSize ());
	SizeLimit targetRectLimit (getTargetView ()->getSizeLimits ());
	if(style.isVertical ())
	{
		Coord newHeight = ccl_bound ((int)(rect.getHeight () * relativeResizeRatio + 0.5), targetRectLimit.minHeight, targetRectLimit.maxHeight);
		targetRect.top = -(int)((newHeight - clipView->getHeight ()) * vParam->getNormalized () + 0.5);
		targetRect.setHeight (newHeight);
	}
	else
	{
		Coord newWidth = ccl_bound ((int)(rect.getWidth () * relativeResizeRatio + 0.5), targetRectLimit.minWidth, targetRectLimit.maxWidth);
		targetRect.left = -(int)((newWidth - clipView->getWidth ()) * hParam->getNormalized () + 0.5);
		targetRect.setWidth (newWidth);
	}
	
	checkPosition (targetRect);
	savedTargetSize = targetRect.getSize ();
	target->setSize (targetRect);
	syncHeader (targetRect.left);
	
	checkAutoHide ();
	initScrollBars ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float ScrollView::getScrollSpeedV () const
{
	return 15;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float ScrollView::getScrollSpeedH () const
{
	return 15;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Point ScrollView::getScrollRange () const
{
	Point range (target->getSize ().getSize () - clipView->getSize ().getSize ());
	if(!canScrollH ())
		range.x = 0;
	if(!canScrollV ())
		range.y = 0;
	return range;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITouchHandler* ScrollView::createTouchHandler (const TouchEvent& event)
{
	if(style.isCustomStyle (Styles::kScrollViewBehaviorNoSwipe))
		return nullptr;

	if(event.touches.getTouchCount () > 0)
	{
		// check if scrolling is possible
		Point scrollRange (getScrollRange ());
		if(scrollRange.x <= 0 && scrollRange.y <= 0 && !style.isCustomStyle (Styles::kScrollViewBehaviorAllowZoomGesture))
			return nullptr;
		
		// swallow other touches while scrolling
		if(isManipulating ())
			return NEW NullTouchHandler (this);

		// don't scroll while another touch handlers is active in our area
		if(Window* window = getWindow ())
			if(window->getTouchInputState ().hasTouchHandlerInViewArea (*this))
				return nullptr;
		
		// boost priorities while animation is running (prevent subViews of target from receiving touches)
		bool boostPriority = isAnimatingX () || isAnimatingY ();
		if(boostPriority)
		{
			if(IGraphicsLayer* targetLayer = target->getGraphicsLayer ())
			{
				// but don't boost when animation has almost reached the target position (e.g. during slow ease out)
				Variant offsetX, offsetY;
				if(targetLayer->getPresentationProperty (offsetX, IGraphicsLayer::kOffsetX) &&
					targetLayer->getPresentationProperty (offsetY, IGraphicsLayer::kOffsetY))
				{
					Point distance (target->getSize ().getLeftTop () - Point (offsetX, offsetY));
					if(ccl_max (ccl_abs (distance.x), ccl_abs (distance.y)) <= 4)
						boostPriority = false;
				}
			}
		}
		
		Rect clipRect;
		getClipViewRect (clipRect);
		Point where (event.touches.getTouchInfo (0).where);
		windowToClient (where);
		clientToScreen (where);
		if(clipRect.pointInside (where))
			return NEW ScrollViewSwipeHandler (this, boostPriority);
		else
			return nullptr;
	
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MouseHandler* ScrollView::createMouseHandler (const MouseEvent& event) 
{
	if(style.isCustomStyle (Styles::kScrollViewBehaviorMouseScroll)) 
		return NEW ScrollViewMouseHandler (this); 
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ScrollView::onMouseWheel (const MouseWheelEvent& event)
{
	double scrollEventTime = System::GetProfileTime ();
	if(scrollWheelLatched)
		if((scrollEventTime - lastScrollWheelEventTime) > kScrollWheelLatchDelay)
			scrollWheelLatched = false;
	
	if(!scrollWheelLatched)
		if(View::onMouseWheel (event))
			return true;
	
	if(!scrollWheelLatched && style.isCustomStyle (Styles::kScrollViewBehaviorLatchWheel))
		scrollWheelLatched = true;
	lastScrollWheelEventTime = scrollEventTime;
	
	int sign = (int)ccl_sign (-event.delta);
	Point delta;
	
	auto isScrollable = [](IParameter* param)
	{
		return (param && param->getMin () != param->getMax ()) ? true : false;
	};
	
	auto scrollEndReached = [](IParameter* param, Coord delta)
	{
		return (delta > 0 && (param->getValue () == param->getMax ())) ||
			   (delta < 0 && (param->getValue () == param->getMin ()));
	};
	
	if(canScrollOmniDirectional () && event.isContinuous ())
	{	
		if(!isScrollable (vParam) && !isScrollable (hParam))
			return false;
		
		delta.x = Coord (-event.deltaX);
		delta.y = Coord (-event.deltaY);
	}
	else if(canScrollV () && event.isVertical ())
	{
		if(!isScrollable (vParam))
			return false;
			
		delta.x = 0;
		
		if(event.isContinuous ())
			delta.y = Coord (-event.delta);
		else
			delta.y = Coord (ceilf (-event.delta * sign) * getScrollSpeedV () * sign);
		
		if(scrollEndReached (vParam, delta.y))
			return false;
	}
	else if(canScrollH ())
	{
		if(!isScrollable (hParam))
			return false;
			
		delta.y = 0;
		
		if(event.isContinuous ())
			delta.x = Coord (-event.delta);
		else
			delta.x = Coord (ceilf (-event.delta * sign) * getScrollSpeedH () * sign);
			
		if(scrollEndReached (hParam, delta.x))
			return false;
	}
	else
		return false;
	
	auto scrollByVH = [this](PointRef delta)
	{
		setManipulation (true);
		scrollByV (delta.y);
		scrollByH (delta.x);
		setManipulation (false); // hmm, we don't know if it's over when sent via 2 finger gesture on OSX
	};
	
	if(simulateLayeredScroll ())
	{
		ScrollManipulator manipulator (this);
		manipulator.push (delta);
	}
	else
	{
		if(isScrollByPageEnabled ())
		{
			if(isAnimatingX () || isAnimatingY () || event.isRollOutPhase ())
				return true;
			
			Coord deltaAmount (sign * Coord (-event.delta));
			if(event.isContinuous () && (deltaAmount < kMinimalPagingMovement))
			{
				scrollByVH (delta);
				(NEW Message ("onContinuousWheelEnded", delta.x, delta.y))->post (this, 50);
			}
			else
			{
				Point pageSize;
				getScrollByPageSize (pageSize);
				delta.x = pageSize.x * sign;
				delta.y = pageSize.y * sign;
				ScrollManipulator manipulator (this);
				manipulator.push (delta);
			}
		}
		else
		{
			scrollByVH (delta);
		}
	}
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ScrollView::onGesture (const GestureEvent &event)
{
	return View::onGesture (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ScrollView::onDragEnter (const DragEvent& event)
{
	if(target)
	{
		Point offset;
		offset -= target->getSize ().getLeftTop ();
		offset -= clipView->getSize ().getLeftTop ();

		DragEvent e2 (event);
		e2.where.offset (offset);
		if(target->onDragEnter (e2))
			return true;
	}
	return SuperClass::onDragEnter (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ScrollView::onDrop (const DragEvent& event)
{
	if(target)
	{
		Point offset;
		offset -= target->getSize ().getLeftTop ();
		offset -= clipView->getSize ().getLeftTop ();

		DragEvent e2 (event);
		e2.where.offset (offset);
		if(target->onDrop (e2))
			return true;
	}
	return SuperClass::onDrop (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool ScrollView::makeVisible (RectRef _rect, tbool relaxed)
{
	// continue upwards if we can't scroll; todo: handle directions separately
	if(!canScrollH () && !canScrollV ())
		return SuperClass::makeVisible (_rect, relaxed);

	CCL_PRINTF ("ScrollView::makevisible (%d,%d,%d,%d)\n", _rect.left, _rect.top, _rect.right, _rect.bottom)

	// move rect to clipView coords
	Rect rect (_rect);
	rect.offset (-clipView->getSize ().left, -clipView->getSize ().top);

	Coord clipW = clipView->getWidth ();
	Coord clipH = clipView->getHeight ();

	if(clipH == 0 || clipW == 0)
	{
		makeVisibleRect = rect;
		makeVisibleRect.offset (target->getSize ().getLeftTop () * -1); // store in target coords
		return false;
	}
	
	bool scrollH = true;
	bool scrollV = true;
	if(relaxed)
	{
		// do not scroll if at least some pixels of the given rect are visible
		enum { kMinVisiblePixels = 5 };

		if(rect.top < clipH - kMinVisiblePixels && rect.bottom > kMinVisiblePixels)
			scrollV = false;

		if(rect.left < clipW - kMinVisiblePixels && rect.right > kMinVisiblePixels)
			scrollH = false;
	}

	if(scrollV)
	{
		if(rect.top <= 0 || rect.getHeight () >= clipH)
			scrollByV (rect.top);
		else if(rect.bottom > clipH)
			scrollByV (rect.bottom - clipH);

		// scroll immediately
		notify (UnknownPtr<ISubject> (vParam), Message (kChanged));
	}
	if(scrollH)
	{
		if(rect.left <= 0 || rect.getWidth () >= clipW)
			scrollByH (rect.left);
		else if(rect.right > clipW)
			scrollByH (rect.right - clipW);

		// scroll immediately
		notify (UnknownPtr<ISubject> (hParam), Message (kChanged));
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ScrollView::canScrollV () const
{
	return style.isCommonStyle (Styles::kVertical) || style.isCustomStyle (Styles::kScrollViewAppearanceVButtons|Styles::kScrollViewBehaviorCanScrollV);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ScrollView::canScrollH () const
{
	return style.isCommonStyle (Styles::kHorizontal) || style.isCustomStyle (Styles::kScrollViewAppearanceHButtons|Styles::kScrollViewBehaviorCanScrollH);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ScrollView::canScrollOmniDirectional () const
{
	return (canScrollV () && canScrollH () && style.isCustomStyle (Styles::kScrollViewBehaviorOmniDirectional));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ScrollView::scrollByV (Coord offset)
{
	if(vParam && offset && canScrollV ())
	{
		Coord targetTop = target->getSize ().top - offset;

		int value = -targetTop / snap.y;
		if(offset < 0)
		{
			if(value * snap.y > -targetTop)
				value--; // snap up
		}
		else
		{
			if(value * snap.y < -targetTop)
				value++; // snap down
		}
		vParam->setValue (value, true);
		CCL_PRINTF ("  scrollByV (%d): value %d, coord %d -> snapped %d\n", offset, value, -targetTop, value * snap.y)
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ScrollView::scrollByH (Coord offset)
{
	if(hParam && offset && canScrollH ())
	{
		Coord targetLeft = target->getSize ().left - offset;

		int value = -targetLeft / snap.x;
		if(offset < 0)
		{
			if(value * snap.x > -targetLeft)
				value--; // snap left
		}
		else
		{
			if(value * snap.x < -targetLeft)
				value++; // snap right
		}
		hParam->setValue (value, true);
		CCL_PRINTF ("  scrollByH (%d): value %d, coord %d -> snapped %d\n", offset, value, -targetLeft, value * snap.x)
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollView::scrollTo (PointRef targetPos, double duration, float velocity)
{
	Rect targetRect (target->getSize ());
	Point oldPos (targetRect.getLeftTop ());

	if(targetPos != oldPos || duration > 0) // must start animation even if position not changed (especially in the "over" case, animate back to legal position)
	{
		Rect clipRect;
		getClipViewRect (clipRect);
		Point p (targetPos);
		Point scrollRange = clipRect.getSize () - targetRect.getSize ();
		Point boundPos (p);
		if(scrollRange.x <= 0)
			boundPos.x = ccl_bound<Coord> (p.x, scrollRange.x, 0);
		if(scrollRange.y <= 0)
			boundPos.y = ccl_bound<Coord> (p.y, scrollRange.y, 0);
		
		auto getEaseOutPoints = [&](double slope)
		{
			// get ease out control points with end slope [0,1]
			double c2y = ((1.0 - ccl_bound (slope, 0.0, 1.0)) * 0.875) + 0.125;
			AnimationControlPoints values = {0, 0, 0.125, c2y};
			return values;
		};

		IGraphicsLayer* targetLayer = target->getGraphicsLayer ();	
		if(duration > 0)
		{
			AnimationTimingType timingType = kTimingEaseOut;

			bool isOverX = boundPos.x != p.x;
			bool isOverY = boundPos.y != p.y;

			// start animations
			if(targetPos.x != oldPos.x || isOverX)
			{
				// get distance factor = available distance / desired distance
				double distanceFactor = (oldPos.x - boundPos.x) / double(oldPos.x - targetPos.x);
				
				// update duration - to bounded distance
				double newDuration = duration * ccl_abs (distanceFactor);
				
				// what is the end slope (velocity) for this direction
				double endSlope = 1.0 - ccl_abs (distanceFactor);

				if(endSlope > 0.0)
					timingType = kTimingCubicBezier;
					
				p.x = boundPos.x;

				BasicAnimation animation;
				animation.setDuration (newDuration);
				animation.setControlPoints (getEaseOutPoints (endSlope));
				animation.setRepeatCount (1);
				animation.setStartValue (oldPos.x);
				animation.setEndValue (p.x);
				animation.setTimingType (timingType);
				AutoPtr<ScrollAnimationCompletionHandler> completionHandler (NEW ScrollAnimationCompletionHandler (this, kAnimatingX));
				animation.setCompletionHandler (completionHandler);

				if(targetLayer)
				{
					targetLayer->addAnimation (IGraphicsLayer::kOffsetX, animation.asInterface ());
					targetLayer->flush ();
				}
				else
				{
					completionHandler->setTargetPos (p);
					AnimationManager::instance ().addAnimation (this, IGraphicsLayer::kOffsetX, animation.asInterface ());
				}
				signalAnimation (IScrollParameter::kAnimationAdded, hParam, animation.asInterface ());
			}
			
			if(targetPos.y != oldPos.y || isOverY)
			{
				// get distance factor = available distance / desired distance
				double distanceFactor = (oldPos.y - boundPos.y) / double(oldPos.y - targetPos.y);
				
				// update duration - to bounded distance
				double newDuration = duration * ccl_abs (distanceFactor);
				
				// what is the end slope (velocity) for this direction
				double endSlope = 1.0 - ccl_abs (distanceFactor);
	
				if(endSlope > 0.0)
					timingType = kTimingCubicBezier;

				p.y = boundPos.y;

				BasicAnimation animation;
				animation.setDuration (newDuration);
				animation.setControlPoints (getEaseOutPoints (endSlope));
				animation.setRepeatCount (1);
				animation.setStartValue (oldPos.y);
				animation.setEndValue (p.y);
				animation.setTimingType (timingType);
				AutoPtr<ScrollAnimationCompletionHandler> completionHandler (NEW ScrollAnimationCompletionHandler (this, kAnimatingY));
				animation.setCompletionHandler (completionHandler);

				if(targetLayer)
				{
					targetLayer->addAnimation (IGraphicsLayer::kOffsetY, animation.asInterface ());
					targetLayer->flush ();
				}
				else
				{
					completionHandler->setTargetPos (p);
					AnimationManager::instance ().addAnimation (this, IGraphicsLayer::kOffsetY, animation.asInterface ());
				}
				signalAnimation (IScrollParameter::kAnimationAdded, vParam, animation.asInterface ());
			}
		}

		if(targetLayer || duration == 0)
		{
			p = boundPos;
			targetRect.moveTo (p);
			scrollClientToTargetRect (targetRect);

			bool hUpdate = duration == 0 && hParam->getState (IParameter::kIsEditing);
			bool vUpdate = duration == 0 && vParam->getState (IParameter::kIsEditing);
			
			hParam->setValue (ccl_round<0> (float(-p.x) / snap.x), hUpdate);
			vParam->setValue (ccl_round<0> (float(-p.y) / snap.y), vUpdate);
		}
		CCL_PRINTF ("ScrollView::scrollTo %d, %d\n", -p.x, -p.y);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ScrollView::getPosition (Point& targetPos) const
{
	targetPos = target->getSize ().getLeftTop ();

	// use the current animated position of the target layer
	if(IGraphicsLayer* targetLayer = target->getGraphicsLayer ())
	{
		Variant value;
		if(isAnimatingX () && targetLayer->getPresentationProperty (value, IGraphicsLayer::kOffsetX))
			targetPos.x = value;

		if(isAnimatingY () && targetLayer->getPresentationProperty (value, IGraphicsLayer::kOffsetY))
			targetPos.y = value;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ScrollView::stopAnimations ()
{
	if(isAnimatingY ())
		stopVerticalAnimation ();
	if(isAnimatingX ())
		stopHorizontalAnimation ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollView::stopVerticalAnimation ()
{
	SharedPtr<Object> holder (this);

	if(IGraphicsLayer* targetLayer = target->getGraphicsLayer ())
		targetLayer->removeAnimation (IGraphicsLayer::kOffsetY);
	else
		AnimationManager::instance ().removeAnimation (this, IGraphicsLayer::kOffsetY);

	if(isManipulating () == false)
		if(isAnimatingY ()) 
			signal (Message (kOnScrollEnd));

	isAnimatingY (false);
		
	signalAnimation (IScrollParameter::kAnimationRemoved, vParam, nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollView::stopHorizontalAnimation ()
{
	SharedPtr<Object> holder (this);

	if(IGraphicsLayer* targetLayer = target->getGraphicsLayer ())
		targetLayer->removeAnimation (IGraphicsLayer::kOffsetX);
	else
		AnimationManager::instance ().removeAnimation (this, IGraphicsLayer::kOffsetX);

	if(isManipulating () == false)
		if(isAnimatingX ()) 
			signal (Message (kOnScrollEnd));

	isAnimatingX (false);
		
	signalAnimation (IScrollParameter::kAnimationRemoved, hParam, nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollView::signalAnimation (StringID messageId, IParameter* param, IAnimation* animation)
{
	if(privateFlags & kSyncingAnimation)
		return;

	if(UnknownPtr<ISubject> subject = param)
		System::GetSignalHandler ().performSignal (subject, Message (messageId, asUnknown (), animation));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollView::synchronizeAnimation (ISubject* subject, MessageRef msg)
{
	if(msg[0].asUnknown () == this->asUnknown ())
		return;

	IGraphicsLayer* targetLayer = target->getGraphicsLayer ();
	UnknownPtr<IParameter> param (subject);
	if(!targetLayer || !param)
		return;

	StringID propertyId = param == hParam ? IGraphicsLayer::kOffsetX : IGraphicsLayer::kOffsetY;
	
	ScopedFlag<kSyncingAnimation> scope (privateFlags);
	if(msg == IScrollParameter::kAnimationAdded)
	{
		UnknownPtr<IAnimation> animation (msg[1]);
		if(animation)
			targetLayer->addAnimation (propertyId, animation);
	}
	else
	{
		targetLayer->removeAnimation (propertyId);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollView::setManipulation (bool begin)
{
	isManipulating (begin);

	if(hParam && canScrollH ())
	{
		hParam->setState (IParameter::kIsEditing, begin);
		
		if(begin)
			hParam->beginEdit ();
		else
			hParam->endEdit ();
	}

	if(vParam && canScrollV ())
	{
		vParam->setState (IParameter::kIsEditing, begin);
		
		if(begin)
			vParam->beginEdit ();
		else
			vParam->endEdit ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollView::setScrolling (bool begin)
{
	if(begin || (privateFlags & (kAnimatingX|kAnimatingY|kManipulating)) == 0) // supresss kOnScrollEnd while still animating
	{
		Message msg (begin ? kOnScrollBegin : kOnScrollEnd);
		signal (msg);
		if(hBar)
			hBar->signal (msg);
		if(vBar)
			vBar->signal (msg);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect& CCL_API ScrollView::getClipViewRect (Rect& bounds) const
{
	Point p;
	clipView->clientToScreen (p);
	clipView->getClientRect (bounds);
	bounds.offset (p);
	return bounds;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollView::onViewsChanged ()
{
	// ignore fitsize (SuperClass)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollView::onChildSized (View* child, const Point& delta)
{
	// check fitsize only when target view resizes (ignore scrollbars, clipper, header)
	if(child == target)
	{
		if(!isResizing () && !(privateFlags & kResizingTarget))
			savedTargetSize = child->getSize ().getSize ();
		initScrollBars ();
		syncHeader (child->getSize ().left);

		SuperClass::onChildSized (child, delta); // invalidates sizeLimits

		checkAutoHide ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollView::calcAutoSize (Rect& rect)
{
	Rect wanted;

	// try to fulfill the target's preferred size
	if(target)
		target->calcAutoSize (wanted);
	
	getClipViewLimits ().makeValid (wanted);

	Rect clipRect;
	Rect headerClipRect;
	calcClipRect (clipRect, headerClipRect);

	if(hBar && style.isCustomStyle (Styles::kScrollViewBehaviorAutoHideHBar) && !style.isCustomStyle (Styles::kScrollViewBehaviorHScrollSpace))
		clipRect.bottom += hBar->getHeight ();
	if(vBar && style.isCustomStyle (Styles::kScrollViewBehaviorAutoHideVBar))
		clipRect.right += vBar->getWidth ();

	Coord paddingX = getWidth () - clipRect.getWidth ();
	Coord paddingY = getHeight () - clipRect.getHeight ();

	rect (0, 0, wanted.getWidth () + paddingX, wanted.getHeight() + paddingY);

	if(style.isCustomStyle (Styles::kScrollViewBehaviorLimitToScreen))
	{
		if(isAttached ())
			limitToScreenSize (rect);
		else
			(NEW Message ("checkScreenSize"))->post (this);
	}
	
	if(privateFlags & kExplicitSizeLimits)
		sizeLimits.makeValid (rect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ScrollView::autoSize (tbool horizontal, tbool vertical)
{
	Rect calculated;
	calcAutoSize (calculated);

	Rect r (size);
	if(horizontal)
		r.setWidth (calculated.getWidth ());
	if(vertical)
		r.setHeight (calculated.getHeight ());

	// don't disable SizeMode, bars & clippers must follow
	
	if(target)
	{
		Rect targetSize = target->getSize ().getSize ();
		bool needVBar = (targetSize.getHeight () > r.getHeight ()) && (targetSize.getWidth () == r.getWidth ());
		if(needVBar && style.isCustomStyle (Styles::kScrollViewBehaviorVScrollSpace))
			r.setWidth (r.getWidth () + getScrollBarSize ());
	}
	
	setSize (r);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ScrollView::setProperty (MemberID propertyId, const Variant& var)
{
	auto isAnimationReset = [] (VariantRef v) { return v.getUserValue () == IAnimation::kResetBackwards; };

	if(propertyId == IGraphicsLayer::kOffsetX) // from animation manager
	{
		if(!isAnimationReset (var)) // ignore reset to startValue
			scrollTo (Point (var.asInt (), target->getSize ().top));
	}
	else if(propertyId == IGraphicsLayer::kOffsetY)
	{
		if(!isAnimationReset (var)) // ignore reset to startValue
			scrollTo (Point (target->getSize ().left, var.asInt ()));
	}
	else if(propertyId == "vpos")
	{
		if(vParam)
			vParam->setNormalized (var, true);
		return true;
	}
	else if(propertyId == "hpos")
	{
		if(hParam)
			hParam->setNormalized (var, true);
		return true;
	}
	return SuperClass::setProperty (propertyId, var);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityProvider* ScrollView::getAccessibilityProvider ()
{
	if(!accessibilityProvider)
		accessibilityProvider = NEW ScrollViewAccessibilityProvider (*this);
	return accessibilityProvider;
}

//************************************************************************************************
// ScrollViewAccessibilityProvider
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (ScrollViewAccessibilityProvider, ViewAccessibilityProvider)

//////////////////////////////////////////////////////////////////////////////////////////////////

ScrollViewAccessibilityProvider::ScrollViewAccessibilityProvider (ScrollView& owner)
: ViewAccessibilityProvider (owner)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ScrollView& ScrollViewAccessibilityProvider::getScrollView () const
{
	return static_cast<ScrollView&> (view);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ScrollViewAccessibilityProvider::canScroll (AccessibilityScrollDirection direction) const
{
	ScrollView& scrollView = getScrollView ();
	switch(direction)
	{
	case AccessibilityScrollDirection::kLeft : CCL_FALLTHROUGH
	case AccessibilityScrollDirection::kRight :
		return scrollView.canScrollH ();
	case AccessibilityScrollDirection::kUp : CCL_FALLTHROUGH
	case AccessibilityScrollDirection::kDown :
		return scrollView.canScrollV ();
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ScrollViewAccessibilityProvider::scroll (AccessibilityScrollDirection direction, AccessibilityScrollAmount amount)
{
	ScrollView& scrollView = getScrollView ();
	if(View* target = scrollView.getTarget ())
	{
		Point oldPos (target->getSize ().getLeftTop ());

		Point pageSize;
		scrollView.getScrollByPageSize (pageSize);

		Point scrollDistance;
		switch(amount)
		{
		case AccessibilityScrollAmount::kStep :
			scrollDistance.x = scrollView.getScrollSpeedH ();
			scrollDistance.y = scrollView.getScrollSpeedV ();
			break;

		case AccessibilityScrollAmount::kPage :
			scrollDistance = pageSize;
			break;
		}

		switch(direction)
		{
		case AccessibilityScrollDirection::kLeft :
			scrollView.scrollByH (-scrollDistance.x);
			scrollView.notify (UnknownPtr<ISubject> (scrollView.getHScrollParam ()), Message (kChanged));
			break;
		case AccessibilityScrollDirection::kRight :
			scrollView.scrollByH (scrollDistance.x);
			scrollView.notify (UnknownPtr<ISubject> (scrollView.getHScrollParam ()), Message (kChanged));
			break;
		case AccessibilityScrollDirection::kUp :
			scrollView.scrollByV (-scrollDistance.y);
			scrollView.notify (UnknownPtr<ISubject> (scrollView.getVScrollParam ()), Message (kChanged));
			break;
		case AccessibilityScrollDirection::kDown :
			scrollView.scrollByV (scrollDistance.y);
			scrollView.notify (UnknownPtr<ISubject> (scrollView.getVScrollParam ()), Message (kChanged));
			break;
		}

		if(target->getSize ().getLeftTop () != oldPos)
			return kResultOk;
	}

	return kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ScrollViewAccessibilityProvider::scrollTo (double horizontal, double vertical)
{
	ScrollView& scrollView = getScrollView ();
	IParameter* hScrollParam = scrollView.getHScrollParam ();
	if(hScrollParam)
		hScrollParam->setNormalized (horizontal, true);
	IParameter* vScrollParam = scrollView.getVScrollParam ();
	if(vScrollParam)
		vScrollParam->setNormalized (vertical, true);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

double CCL_API ScrollViewAccessibilityProvider::getNormalizedScrollPositionX () const
{
	ScrollView& scrollView = getScrollView ();
	IParameter* hScrollParam = scrollView.getHScrollParam ();
	if(hScrollParam)
		return hScrollParam->getNormalized ();
	return 0.;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

double CCL_API ScrollViewAccessibilityProvider::getNormalizedScrollPositionY () const
{
	ScrollView& scrollView = getScrollView ();
	IParameter* vScrollParam = scrollView.getVScrollParam ();
	if(vScrollParam)
		return vScrollParam->getNormalized ();
	return 0.;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API ScrollViewAccessibilityProvider::getPagePositionX () const
{
	ScrollView& scrollView = getScrollView ();
	Point pageSize;
	scrollView.getScrollByPageSize (pageSize);
	if(View* target = scrollView.getTarget ())
		return int(-target->getSize ().getLeftTop ().x / pageSize.x) + 1;
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API ScrollViewAccessibilityProvider::countPagesX () const
{
	ScrollView& scrollView = getScrollView ();
	Point pageSize;
	scrollView.getScrollByPageSize (pageSize);
	if(View* target = scrollView.getTarget ())
		return int(target->getSize ().getWidth () / pageSize.x);
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API ScrollViewAccessibilityProvider::getPagePositionY () const
{
	ScrollView& scrollView = getScrollView ();
	Point pageSize;
	scrollView.getScrollByPageSize (pageSize);
	if(View* target = scrollView.getTarget ())
		return int(-target->getSize ().getLeftTop ().y / pageSize.y) + 1;
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API ScrollViewAccessibilityProvider::countPagesY () const
{
	ScrollView& scrollView = getScrollView ();
	Point pageSize;
	scrollView.getScrollByPageSize (pageSize);
	if(View* target = scrollView.getTarget ())
		return int(target->getSize ().getHeight () / pageSize.y);
	return 0;
}
