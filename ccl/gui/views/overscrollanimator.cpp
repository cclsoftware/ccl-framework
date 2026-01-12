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
// Filename    : ccl/gui/views/overscrollanimator.cpp
// Description : Animator and MouseHandler - Overscroll behavior for IOverScrollAnimatable(s)
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/views/mousehandler.h"
#include "ccl/gui/touch/touchhandler.h"
#include "ccl/gui/views/overscrollanimator.h"
#include "ccl/gui/system/mousecursor.h"
#include "ccl/gui/gui.h"

#include "ccl/public/systemservices.h"
#include "ccl/app/params.h"

#include <math.h>

#include "ccl/public/gui/framework/themeelements.h"

using namespace CCL;


//************************************************************************************************
// Helper functions
//************************************************************************************************

struct OverScrollHelper
{
	static bool calculateAverageVelocity (float& pointsPerSecond, float pDelta, int64 elapsedTime);
	static float getBoostedDelta (float delta, float pps, int maxScrollRange);
	static float getBoundedVelocity (float velocity, float maxPPS);
};

//////////////////////////////////////////////////////////////////////////////////////////////////

bool OverScrollHelper::calculateAverageVelocity (float& pps, float pDelta, int64 elapsedTime)
{
	int sign = (int)ccl_sign (pDelta);
	if(sign != 0)
	{
		float currentPPS = ccl_abs (pDelta) / elapsedTime * 1000;
	
		pps = (0.6f * currentPPS) + (0.4f * ccl_abs (pps));
		pps *= sign;
		
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float OverScrollHelper::getBoostedDelta (float delta, float pps, int maxScrollRange)
{
	float rangeFactor = maxScrollRange / 100;
	float curveFactor = ccl_min (1000.f, ccl_abs(pps)) / 1000.f;
	float boostFactor = rangeFactor * (powf (curveFactor, rangeFactor / 2)) * 2; // extreme boost for large values
	return delta + (delta * boostFactor);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float OverScrollHelper::getBoundedVelocity (float velocity, float maxPPS)
{
	velocity = ccl_bound (velocity, -maxPPS, maxPPS);
	
	float tempBoundVelocity = ccl_bound (velocity, -15.f, 15.f);
	
	tempBoundVelocity *= (ccl_sign (tempBoundVelocity) < 0) ? -1 : 1;
	
	tempBoundVelocity = -ccl_max (10.f, tempBoundVelocity) + 25;
	
	float factor = (tempBoundVelocity / 15) * 5;
	velocity *= factor;
	
	velocity = ccl_bound (velocity, -maxPPS, maxPPS);
	
	return velocity;
}


//************************************************************************************************
// OverScrollAnimator::OverScrollHandler
//************************************************************************************************
class OverScrollAnimator::OverScrollHandler: public PeriodicMouseHandler
{
public:
	DECLARE_CLASS (OverScrollAnimator::OverScrollHandler, PeriodicMouseHandler)
	
	OverScrollHandler (View* view = nullptr, OverScrollAnimator* animator = nullptr, ClickAction* clickAction = nullptr);
	
	~OverScrollHandler ();
	
	void setBoostRange (int boostRangeV, int boostRangeH);
	
	// PeriodicMouseHandler
	void onBegin () override;
	void onRelease (bool canceled) override;
	bool onMove (int moveFlags) override;
	bool onPeriodic () override;
	
protected:
	IParameter* vOverScrollParam;
	IParameter* vScrollParam;
	IParameter* hOverScrollParam;
	IParameter* hScrollParam;
	OverScrollAnimator* animator;
	MouseCursor* oldCursor;
	bool wasShiftPressedState;
	Point startValue;
	Point previousValue;
	Point previousWhere;
	float vPointsPerSecond;
	float hPointsPerSecond;
	float scrollDeltaH;
	float scrollDeltaV;
	int boostRangeV;
	int boostRangeH;
	int64 previousTime;
	int minimalPPS;
	int direction;
	bool isClick;
	int boostRange;
	
	AutoPtr<ClickAction> clickAction;
	
	void initialize ();
	PointF getRollOutVelocity () const;
	void updateStartValues ();
};

//************************************************************************************************
// OverScrollAnimator::OverScrollHandler
//************************************************************************************************

DEFINE_CLASS_HIDDEN (OverScrollAnimator::OverScrollHandler, MouseHandler)

//////////////////////////////////////////////////////////////////////////////////////////////////

OverScrollAnimator::OverScrollHandler::OverScrollHandler (View* view, OverScrollAnimator* animator, ClickAction* clickAction)
: PeriodicMouseHandler (view),
  oldCursor (nullptr),
  wasShiftPressedState (false),
  animator (animator),
  vPointsPerSecond (0.f),
  hPointsPerSecond (0.f),
  scrollDeltaH (0.f),
  scrollDeltaV (0.f),
  previousTime (0),
  vOverScrollParam (nullptr),
  vScrollParam (nullptr),
  hOverScrollParam (nullptr),
  hScrollParam (nullptr),
  minimalPPS (1),
  direction (animator->getDirection ()),
  isClick (true),
  clickAction (clickAction),
  boostRangeV (100),
  boostRangeH (100)
{
	initialize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

OverScrollAnimator::OverScrollHandler::~OverScrollHandler ()
{
	tooltipPopup.reserve (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OverScrollAnimator::OverScrollHandler::initialize ()
{
	if(direction == Styles::kVertical)
	{
		vOverScrollParam = animator->getOverScrollParameter (true);
		vScrollParam = animator->getScrollParameter (true);
		minimalPPS = animator->getSnapSize (true);
	}
	else if(direction == Styles::kHorizontal)
	{
		hOverScrollParam = animator->getOverScrollParameter (false);
		hScrollParam = animator->getScrollParameter (false);
		minimalPPS = animator->getSnapSize (false);
	}
	else
	{
		direction = (Styles::kVertical|Styles::kHorizontal);
		vOverScrollParam = animator->getOverScrollParameter (true);
		vScrollParam = animator->getScrollParameter (true);
		hOverScrollParam = animator->getOverScrollParameter (false);
		hScrollParam = animator->getScrollParameter (false);
		minimalPPS = ccl_min (animator->getSnapSize (false), animator->getSnapSize (true));
	}
	
	checkKeys (true);
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

void OverScrollAnimator::OverScrollHandler::setBoostRange (int _boostRangeV, int _boostRangeH)
{
	boostRangeV = _boostRangeV;
	boostRangeH = _boostRangeH;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OverScrollAnimator::OverScrollHandler::onBegin ()
{
	if(vScrollParam && (direction == Styles::kVertical))
		vScrollParam->beginEdit ();
	
	if(hScrollParam && (direction == Styles::kHorizontal))
		hScrollParam->beginEdit ();

	startValue.y = vOverScrollParam ? vOverScrollParam->getValue ().asInt () : 0;
	startValue.x = hOverScrollParam ? hOverScrollParam->getValue ().asInt () : 0;
	wasShiftPressedState = (current.keys.getModifiers () == KeyState::kShift);
	
	oldCursor = GUI.getCursor ();
	AutoPtr<MouseCursor> newCursor = MouseCursor::createCursor (ThemeElements::kSizeVerticalCursor);
	GUI.setCursor (newCursor);

	previousTime = System::GetSystemTicks ();
	previousWhere = current.where;
	previousValue = startValue;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OverScrollAnimator::OverScrollHandler::onRelease (bool canceled)
{
	GUI.setCursor (oldCursor);
	
	tooltipPopup.reserve (false);

	bool isPotentialClick = false;
	bool sameX = ccl_equals (first.where.x , current.where.x, 2);
	bool sameY = ccl_equals (first.where.x , current.where.x, 2);

	static const double kInterpretAsClickDuration = 0.25;
	if(sameY && sameY && (current.eventTime - first.eventTime) < kInterpretAsClickDuration)
		isPotentialClick = true;
	
	if((isClick || isPotentialClick) && clickAction)
	{
		clickAction->execute ();
		
		if(vScrollParam && (direction == Styles::kVertical))
			vScrollParam->endEdit ();
		
		if(hScrollParam && (direction == Styles::kHorizontal))
			hScrollParam->endEdit ();
	}
	else
		animator->startScrollAnimation (getRollOutVelocity ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool OverScrollAnimator::OverScrollHandler::onMove (int moveFlags)
{
	if(moveFlags & kPeriodicMove)	// filter all periodic moves
	{
		PeriodicMouseHandler::onMove (moveFlags);
		return true;
	}
	else if(moveFlags & kMouseMoved)
	{
		if(isClick)
		{
			// stop potential click step animations of previous click events
			animator->stopAnimation (true);
			animator->stopAnimation (false);
		}
		
		isClick = false;
		
		bool isShiftPressed = (current.keys.getModifiers () & KeyState::kShift) != 0;
		if(isShiftPressed != wasShiftPressedState)
			updateStartValues ();

		//calculate the average velocity in points per seconds
		int64 currentTime = System::GetSystemTicks ();
		int64 elapsedTime = (currentTime - previousTime);
		if(elapsedTime > 0)
		{
			scrollDeltaH = (previousWhere.x - current.where.x);
			scrollDeltaV = (previousWhere.y - current.where.y);
			
			float adjustedDeltaH = isShiftPressed ? (scrollDeltaH * 0.2f) : (scrollDeltaH * 0.8f);
			float adjustedDeltaV = isShiftPressed ? (scrollDeltaV * 0.2f) : (scrollDeltaV * 0.8f);
			
			previousValue.x = ccl_to_int (previousValue.x + adjustedDeltaH);
			previousValue.y = ccl_to_int (previousValue.y + adjustedDeltaV);

			animator->setScrollValue (false, previousValue.x);
			animator->setScrollValue (true, previousValue.y);
						
			OverScrollHelper::calculateAverageVelocity (hPointsPerSecond, scrollDeltaH, elapsedTime);
			OverScrollHelper::calculateAverageVelocity (vPointsPerSecond, scrollDeltaV, elapsedTime);
		
			previousWhere = current.where;
			previousTime = currentTime;
		
			//CCL_PRINTF ("elapsedTimeGap: %d, deltaY: %f, deltaX: %f, ppsV: %f, ppsH: %f \n", elapsedTime, scrollDeltaV, scrollDeltaH, vPointsPerSecond, hPointsPerSecond);
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool OverScrollAnimator::OverScrollHandler::onPeriodic ()
{
	// when the user stops the movement, we want a fast decay of the roll-out velocity
	static const float kFastDecayFactor = 0.5f;
	vPointsPerSecond *= kFastDecayFactor;
	hPointsPerSecond *= kFastDecayFactor;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PointF OverScrollAnimator::OverScrollHandler::getRollOutVelocity () const
{
	float boostedDeltaH = OverScrollHelper::getBoostedDelta (OverScrollHelper::getBoundedVelocity (scrollDeltaH, 300.f), hPointsPerSecond, boostRangeH);
	float boostedDeltaV = OverScrollHelper::getBoostedDelta (OverScrollHelper::getBoundedVelocity (scrollDeltaV, 300.f), vPointsPerSecond, boostRangeV);
	
	PointF accumulatedDelta;
	
	bool shiftPressed = (current.keys.getModifiers () & KeyState::kShift) != 0;
	accumulatedDelta.x = (boostedDeltaH * (shiftPressed ? 0.1f : 0.34f));
	accumulatedDelta.y = (boostedDeltaV * (shiftPressed ? 0.1f : 0.34f));

	// avoid zero velocity
	accumulatedDelta.x += (ccl_sign (boostedDeltaH) < 0) ? -1 : 1;
	accumulatedDelta.y += (ccl_sign (boostedDeltaV) < 0) ? -1 : 1;

	accumulatedDelta.x = ccl_bound<float> (-accumulatedDelta.x, -boostRangeH, boostRangeH);
	accumulatedDelta.y = ccl_bound<float> (-accumulatedDelta.y, -boostRangeV, boostRangeV);
	
	return accumulatedDelta;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OverScrollAnimator::OverScrollHandler::updateStartValues ()
{
	startValue.y = vOverScrollParam ? vOverScrollParam->getValue ().asInt () : 0;
	startValue.x = hOverScrollParam ? hOverScrollParam->getValue ().asInt () : 0;
	first = current;
	wasShiftPressedState = !wasShiftPressedState;
}

//************************************************************************************************
// OverScrollAnimationCompletionHandler
//************************************************************************************************
	
class OverScrollAnimator::OverScrollAnimationCompletionHandler: public Object,
											  public IAnimationCompletionHandler
{
public:
	OverScrollAnimationCompletionHandler (OverScrollAnimator* animator, bool verticalDirection, int endValue, float bounceVelocity = 0.f)
	: animator (animator),
	  endValue (endValue),
	  bounceVelocity (bounceVelocity),
	  isVertical (verticalDirection)
	{
	}

	void CCL_API onAnimationFinished () override
	{
		bool pendingAnimation = !ccl_equals (bounceVelocity, 0.f, 0.00001f);
		animator->stopAnimation (isVertical, endValue, pendingAnimation);
		
		if(pendingAnimation)
		{
			if(animator->needsBounceAnimation (isVertical))
				animator->triggerBounceBackAnimation (isVertical, bounceVelocity);
			else
				animator->triggerBounceOutAnimation (isVertical, bounceVelocity);
		}
	}
	
	CLASS_INTERFACE (IAnimationCompletionHandler, Object)
	
private:
	SharedPtr<OverScrollAnimator> animator;
	int endValue;
	float bounceVelocity;
	bool isVertical;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// Tags
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum OverScrollAnimatorParams
	{
		kHOverScroll = 1000,
		kVOverScroll
	};
}

//************************************************************************************************
// IOverScrollAnimatable
//************************************************************************************************

DEFINE_IID_ (IOverScrollAnimatable, 0x129BF9A7, 0xC5CB, 0x46D2, 0xA2, 0xEC, 0x15, 0x00, 0x92, 0xC4, 0xF5, 0x71)

//************************************************************************************************
// OverScrollAnimator
//************************************************************************************************

DEFINE_STRINGID_MEMBER_ (OverScrollAnimator, kVScrollPos, "fullVScrollPosition")
DEFINE_STRINGID_MEMBER_ (OverScrollAnimator, kHScrollPos, "fullHScrollPosition")

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (OverScrollAnimator, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

OverScrollAnimator::OverScrollAnimator (IOverScrollAnimatable* scrollable, int direction)
: animationRunning (false),
  wrapAround (false),
  scrollable (scrollable),
  vScrollParam (nullptr),
  hScrollParam (nullptr),
  direction (direction),
  pendingSteps (0),
  stepsStartValue (-1),
  initialized (false)
{
	ASSERT (scrollable) // mandatory
	
	if(scrollable)
		initialize (scrollable);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

OverScrollAnimator::~OverScrollAnimator ()
{
	stopAnimation (true);
	stopAnimation (false);

	share_and_observe_unknown<IParameter> (this, vScrollParam, nullptr);
	share_and_observe_unknown<IParameter> (this, hScrollParam, nullptr);
	
	cancelSignals ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OverScrollAnimator::initialize (IOverScrollAnimatable* scrollable)
{
	scrollable->getOverScrollMargins (overScrollMargins);
	scrollable->getSnapSize (snapSize);
	scrollable->getScrollRange (scrollRange);
	wrapAround = scrollable->isWrapAround ();
	
	share_and_observe_unknown<IParameter> (this, vScrollParam, scrollable->getScrollParameter (true));
	share_and_observe_unknown<IParameter> (this, hScrollParam, scrollable->getScrollParameter (false));

	setupOverScrollParameter ();
	
	initialized = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API OverScrollAnimator::paramChanged (IParameter* param)
{
	switch(param->getTag ())
	{
	case Tag::kVOverScroll:
		vScrollParam->setNormalized (overScrollPosToNormalized (true));
		scrollable->onOverScroll (true, param->getValue ());
		return true;
		
	case Tag::kHOverScroll:
		hScrollParam->setNormalized (overScrollPosToNormalized (false));
		scrollable->onOverScroll (false, param->getValue ());
		return true;
		
	default:
		return false;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////
	
void CCL_API OverScrollAnimator::paramEdit (IParameter* param, tbool begin)
{
	// IParamObserver
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Coord OverScrollAnimator::getOverScrollBoundsMin (bool verticalDirection) const
{
	if(isWrapAround ())
		return 0;
		
	return verticalDirection ? overScrollMargins.top : overScrollMargins.left;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Coord OverScrollAnimator::getOverScrollBoundsMax (bool verticalDirection) const
{
	Coord overScrollMargin = 0;
	if(isWrapAround () == false)
		overScrollMargin = verticalDirection ? overScrollMargins.bottom : overScrollMargins.right;
	
	if(IParameter* param = getOverScrollParameter (verticalDirection))
		return param->getMax ().asInt () - overScrollMargin;

	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int OverScrollAnimator::getScrollParamValueRange (bool verticalDirection) const
{
	if(IParameter* param = getScrollParameter (verticalDirection))
		return param->getMax ().asInt () - param->getMin ().asInt ();
	
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* OverScrollAnimator::getOverScrollParameter (bool verticalDirection) const
{
	return verticalDirection ? vOverScrollParam : hOverScrollParam;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* OverScrollAnimator::getScrollParameter (bool verticalDirection) const
{
	return verticalDirection ? vScrollParam : hScrollParam;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Coord OverScrollAnimator::getSnapSize (bool verticalDirection) const
{
	Coord snap = verticalDirection ? snapSize.y : snapSize.x;
	return (snap > 0) ? snap : 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int OverScrollAnimator::getScrollRange (bool verticalDirection) const
{
	Coord range = verticalDirection ? scrollRange.y : scrollRange.x;
	return (range > 0) ? range : 0;
}
				
//////////////////////////////////////////////////////////////////////////////////////////////////
				
void OverScrollAnimator::setupOverScrollParameter ()
{
	if(vScrollParam)
	{
		vOverScrollParam = NEW IntParam (0, 100, "vOverScrollParam");
		vOverScrollParam->connect (this, Tag::kVOverScroll);
		updateOverScrollRange (true, vOverScrollParam);
		maximalPointsPerSecond = vOverScrollParam->getPrecision ();
	}
	if(hScrollParam)
	{
		hOverScrollParam = NEW IntParam (0, 100, "hOverScrollParam");
		hOverScrollParam->connect (this, Tag::kHOverScroll);
		updateOverScrollRange (false, hOverScrollParam);
		maximalPointsPerSecond = hOverScrollParam->getPrecision ();
	}
	
	if (vScrollParam && hScrollParam)
		maximalPointsPerSecond = ccl_max (vOverScrollParam->getPrecision (), hOverScrollParam->getPrecision ());

	
	bool forceValueUpdate = true;  // in case the scrollParameter is set to 0 the overScrollParameter would stay in a ("outOfBounds" / "overscroll") position
	updateOverScrollPosition (true, forceValueUpdate);
	updateOverScrollPosition (false, forceValueUpdate);
}
				
//////////////////////////////////////////////////////////////////////////////////////////////////

void OverScrollAnimator::updateOverScrollRange (bool verticalDirection, IParameter* overScrollParam)
{
	int bothOverScrollMargins = 0;
	if(isWrapAround ())
		bothOverScrollMargins = getSnapSize (verticalDirection);
	else
		bothOverScrollMargins = verticalDirection ? (overScrollMargins.top + overScrollMargins.bottom) : (overScrollMargins.left + overScrollMargins.right);
	
	overScrollParam->setMin (0);
	overScrollParam->setMax (getScrollRange (verticalDirection) + bothOverScrollMargins);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MouseHandler* OverScrollAnimator::createMouseHandler (View* view, ClickAction* clickAction)
{
	if(pendingSteps == 0)
	{
		stopAnimation (true);
		stopAnimation (false);
	}

	auto* handler = NEW OverScrollHandler (view, this, clickAction);
	handler->setBoostRange (getScrollParamValueRange (true), getScrollParamValueRange (false));
	return handler;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITouchHandler* OverScrollAnimator::createTouchHandler (View* view, ClickAction* clickAction)
{
	if(pendingSteps == 0)
	{
		stopAnimation (true);
		stopAnimation (false);
	}
	
	AutoPtr<OverScrollHandler> handler = NEW OverScrollHandler (view, this, clickAction);
	handler->setBoostRange (ccl_to_int (getScrollParamValueRange (true) * 1.5f), ccl_to_int (getScrollParamValueRange (false) * 1.5f));
	return NEW TouchMouseHandler (handler, handler->getView ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OverScrollAnimator::startScrollAnimation (PointFRef velocity)
{
	pendingSteps = 0;
	
	if(canScroll (Styles::kVertical))
	{
		if(needsBounceAnimation (true))
			triggerBounceBackAnimation (true);
		else
			triggerRollOutAnimation (true, velocity.y);
	}
	
	if(canScroll (Styles::kHorizontal))
	{
		if(needsBounceAnimation (false))
			triggerBounceBackAnimation (false);
		else
			triggerRollOutAnimation (false, velocity.x);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool OverScrollAnimator::needsBounceAnimation (bool verticalDirection) const
{
	if(isWrapAround ())
		return false;
		
	if(IParameter* overScrollParam = getOverScrollParameter (verticalDirection))
	{
		int startValue = overScrollParam->getValue ();
		if(startValue < getOverScrollBoundsMin (verticalDirection))
			return true;
		else if(startValue > getOverScrollBoundsMax (verticalDirection))
			return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OverScrollAnimator::triggerRollOutAnimation (bool verticalDirection, float velocity)
{
	IParameter* scrollParam = getOverScrollParameter (verticalDirection);
	int scrollParamMin = getOverScrollBoundsMin (verticalDirection);
	int scrollParamMax = getOverScrollBoundsMax (verticalDirection);

	ASSERT (scrollParam)
	
	// calculate animation values
	velocity = OverScrollHelper::getBoundedVelocity (velocity, maximalPointsPerSecond);
	int distance = getRollOutDistance (velocity);
	int startValue = scrollParam ? scrollParam->getValue ().asInt () : 0;
	int endValue = startValue - distance + (getSnapSize (verticalDirection) / 2);
	int snappedEndValue = (endValue / getSnapSize (verticalDirection)) * getSnapSize (verticalDirection);
	
	float endSlope = 0.f; // slope of the animation when reaching the endValue
	if(isWrapAround () == false)
	{
		/**	When the roll-out animation goes beyond the scroll bounds, a bounce-out animation takes over
			and the initial roll-out phase needs to be stopped.
			We cannot observe the scroll value to go passed this point and abruptly stop the animation,
			because it would already be too late. But luckily we don't have to, because we know the exact point (the scroll border)
			and can calculate the real duration and end velocity for the roll-out animation.
			These are the set-up values for this (first roll-out phase) animation.
			(the new end-velocity is also the start-bounceOutVelocity for the subsequent bounce-out animation) */
		
		endSlope = getSlopeWhenLeavingScrollRange (verticalDirection, startValue, snappedEndValue); // endSlope [0,1] - can be used to determine the endVelocity (bounceOutVelocity) in points per second
		snappedEndValue = ccl_bound (snappedEndValue, scrollParamMin, scrollParamMax); // bound snappedEndValue to scrollRange
	}
	
	float bounceOutVelocity = endSlope * velocity;
	if(distance == 0)
		distance = 1;
	
	float newDistanceFactor = (snappedEndValue - startValue) / (float)distance;
	distance = ccl_abs (snappedEndValue - startValue);
	
	if(distance > 0)
	{
		double duration = getAnimationDuration (distance, velocity * newDistanceFactor);

		// setup animation
		BasicAnimation rollOutAnimation;
		rollOutAnimation.setTimingType (kTimingCubicBezier);
		rollOutAnimation.setControlPoints (getEaseOutPoints (endSlope));
		rollOutAnimation.setDuration (duration);
		rollOutAnimation.setRepeatCount (1);
		rollOutAnimation.setStartValue (startValue);
		rollOutAnimation.setEndValue (snappedEndValue);
		rollOutAnimation.setCompletionHandler (AutoPtr<OverScrollAnimationCompletionHandler> (NEW OverScrollAnimationCompletionHandler (this, verticalDirection, snappedEndValue, bounceOutVelocity)));

		//CCL_PRINTF ("RollOutAnimation: startValue: %d, snappedEndValue: %d, velocity: %f, duration: %f, stopVelocity: %f \n", startValue, snappedEndValue, distance, velocity, duration, bounceOutVelocity);

		AnimationManager::instance ().addAnimation (this, verticalDirection ? kVScrollPos : kHScrollPos, rollOutAnimation.asInterface ());
		animationRunning = true;
	}
	else
	{
		setScrollValue (verticalDirection, snappedEndValue);
	
		if(IParameter* scrollParam = getScrollParameter (verticalDirection))
		{
			scrollParam->performUpdate ();
			scrollParam->endEdit ();
		}
		
	}
}
											   
//////////////////////////////////////////////////////////////////////////////////////////////////

AnimationControlPoints OverScrollAnimator::getEaseOutPoints (float slope) const
{
	// get ease out control points with end slope [0,1]
	double c2y = ((1 - ccl_bound (slope, 0.f, 1.f)) * 0.42) + 0.58;
	AnimationControlPoints values = {0, 0, 0.58, c2y};
	return values;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OverScrollAnimator::triggerBounceOutAnimation (bool verticalDirection, float velocity)
{
	IParameter* scrollParam = getOverScrollParameter (verticalDirection);

	ASSERT (scrollParam)
	
	// calculate animation values
	int distance = getBounceOutDistance (verticalDirection, velocity);
	double duration = getAnimationDuration (distance, velocity);
	int startValue = scrollParam ? scrollParam->getValue ().asInt () : 0;
	int endValue = startValue - distance;
	
	// setup animation
	BasicAnimation bounceOutAnimation;
	bounceOutAnimation.setRepeatCount (1);
	bounceOutAnimation.setTimingType (kTimingEaseOut);
	bounceOutAnimation.setStartValue (startValue);
	bounceOutAnimation.setDuration (duration);
	bounceOutAnimation.setEndValue (endValue);
	bounceOutAnimation.setCompletionHandler (AutoPtr<OverScrollAnimationCompletionHandler> (NEW OverScrollAnimationCompletionHandler (this, verticalDirection, endValue, velocity)));

	//CCL_PRINTF ("BounceOutAnimation: startValue: %d, endValue: %d, stopVelocity: %f, duration: %f \n", startValue, endValue, 0.f, duration);

	AnimationManager::instance ().addAnimation (this, verticalDirection ? kVScrollPos : kHScrollPos, bounceOutAnimation.asInterface ());
	animationRunning = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OverScrollAnimator::triggerBounceBackAnimation (bool verticalDirection, float velocity)
{
	IParameter* scrollParam = getOverScrollParameter (verticalDirection);
	int scrollParamMin = getOverScrollBoundsMin (verticalDirection);
	int scrollParamMax = getOverScrollBoundsMax (verticalDirection);
	
	ASSERT (scrollParam)
	
	// calculate animation values
	int startValue = scrollParam ? scrollParam->getValue ().asInt () : 0;
	int endValue = (startValue < scrollParamMin) ? scrollParamMin : scrollParamMax;
	int distance = ccl_abs (startValue - endValue);
	double duration = getAnimationDuration (distance, velocity);
	
	// setup animation with fixed duration
	BasicAnimation bounceBackAnimation;
	bounceBackAnimation.setRepeatCount (1);
	bounceBackAnimation.setTimingType (kTimingEaseInOut);
	bounceBackAnimation.setStartValue (startValue);
	bounceBackAnimation.setDuration (duration);
	bounceBackAnimation.setEndValue (endValue);
	bounceBackAnimation.setCompletionHandler (AutoPtr<OverScrollAnimationCompletionHandler> (NEW OverScrollAnimationCompletionHandler (this, verticalDirection, endValue)));
	
	//CCL_PRINTF ("BounceBackAnimation: startValue: %d, endValue: %d, stopVelocity: %f, duration: %f \n", startValue, endValue, 0.f, duration);

	AnimationManager::instance ().addAnimation (this, verticalDirection ? kVScrollPos : kHScrollPos, bounceBackAnimation.asInterface ());
	animationRunning = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float OverScrollAnimator::getSlopeWhenLeavingScrollRange (bool verticalDirection, int startValue, int endValue) const
{
	float normalizedVelocityFactor = 0.f;
	int scrollParamMin = getOverScrollBoundsMin (verticalDirection);
	int scrollParamMax = getOverScrollBoundsMax (verticalDirection);
	
	if(endValue > scrollParamMax)
	{
		int desiredRange = (endValue - startValue);
		int availableRange = (scrollParamMax - startValue);
		normalizedVelocityFactor = 1 - (availableRange / (float)desiredRange);
	}
	else if(endValue < scrollParamMin)
	{
		int desiredRange = (startValue - endValue);
		int availableRange = (startValue - scrollParamMin);
		normalizedVelocityFactor = 1 - (availableRange / (float)desiredRange);
	}
	
	return normalizedVelocityFactor;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OverScrollAnimator::stopAnimation (bool verticalDirection, int endValue, bool pendingAnimation)
{
	if(!animationRunning)
		return;

	if(AnimationManager::instance ().removeAnimation (this, verticalDirection ? kVScrollPos : kHScrollPos) == kResultOk)
	{
		animationRunning = false;

		if(endValue != -1)
		{
			pendingSteps = 0;
			setScrollValue (verticalDirection, endValue);
		}
		
		if(pendingAnimation == false)
		{
			if(IParameter* scrollParam = getScrollParameter (verticalDirection))
			{
				scrollParam->performUpdate ();
				scrollParam->endEdit ();
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int OverScrollAnimator::getRollOutDistance (float velocity) const
{
	return (int)ccl_sign (velocity) * ccl_to_int ((velocity / 10) * (velocity / 10));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int OverScrollAnimator::getBounceOutDistance (bool verticalDirection, float velocity) const
{
	int maxBounceOut = getOverScrollBoundsMin (verticalDirection);
	return ccl_bound (getRollOutDistance (velocity), -maxBounceOut, maxBounceOut);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

double OverScrollAnimator::getAnimationDuration (int distance, float velocity) const
{
	ASSERT (velocity != 0.f)
	
	if(velocity == 0.f)
		return 1.0;
	
	return ccl_abs (distance) / ccl_abs (2 * velocity);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OverScrollAnimator::setScrollValue (bool verticalDirection, int value)
{
	if(IParameter* overScrollParam = getOverScrollParameter (verticalDirection))
	{
		if(isWrapAround ())
		{
			int scrollParamMax = getOverScrollBoundsMax (verticalDirection);
	
			while(value < 0)
				value += scrollParamMax;
	 
			value = value % scrollParamMax;
		}

		if(canScroll (verticalDirection ? Styles::kVertical : Styles::kHorizontal))
			overScrollParam->setValue (value, true);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool OverScrollAnimator::onMouseWheel (const MouseWheelEvent& event)
{
	if(pendingSteps != 0)
		return false;
	
	if(event.isContinuous ())
		return onMouseWheelContinuous (event);
	else
		return onMouseWheelDiscrete (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool OverScrollAnimator::prepareScrollToAnimation (int& endValue, int step, bool verticalDirection)
{
	if(IParameter* valueParam = getScrollParameter (verticalDirection))
	{
		int currentValue = stepsStartValue;
		if(pendingSteps == 0)
			stepsStartValue = currentValue = valueParam->getValue ().asInt ();
		
		int targetValue = currentValue + step + pendingSteps;
		targetValue = ccl_bound (targetValue, valueParam->getMin ().asInt (), valueParam->getMax ().asInt ());
		
		if(targetValue == currentValue)
		{
			valueParam->setValue (targetValue, true);
			pendingSteps = 0;
			return false;
		}
		double normalizedTargetValue = (targetValue - valueParam->getMin ().asInt ()) / (double) getScrollParamValueRange (verticalDirection);
		endValue = (normalizedTargetValue * getScrollRange (verticalDirection)) + getOverScrollBoundsMin (verticalDirection);
		
		//check unreasonable endValue
		if(IParameter* scrollParam = getOverScrollParameter (verticalDirection))
		{
			int startValue = scrollParam->getValue ().asInt ();
			int maxValue = scrollParam->getMax ().asInt ();
			int midValue = maxValue / 2;
			
			int distance = ccl_abs (startValue - endValue);
			if(distance > midValue)
				endValue += (midValue > endValue) ? maxValue : -maxValue;
		}
		
		pendingSteps += step;
		return true;
	}
	
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OverScrollAnimator::increment ()
{
	int endValue;
	bool verticalDirection = canScroll (Styles::kVertical);

	if(prepareScrollToAnimation (endValue, 1, verticalDirection))
		triggerScrollToAnimation (verticalDirection, endValue);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OverScrollAnimator::decrement ()
{
	int endValue;
	bool verticalDirection = canScroll (Styles::kVertical);

	if(prepareScrollToAnimation (endValue, -1, verticalDirection))
		triggerScrollToAnimation (verticalDirection, endValue);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OverScrollAnimator::triggerScrollToAnimation (bool verticalDirection, int endValue)
{
	IParameter* scrollParam = getOverScrollParameter (verticalDirection);
	
	if(pendingSteps != 0)
		stopAnimation (verticalDirection);
	
	// calculate animation values
	int startValue = scrollParam ? scrollParam->getValue ().asInt () : 0;
	
	int distance = ccl_abs (startValue - endValue);
	Coord velocityPPS = getSnapSize (verticalDirection);
	double duration = getAnimationDuration (distance, velocityPPS + (velocityPPS * 0.1f * ccl_abs (pendingSteps)));

	// setup animation with fixed duration
	BasicAnimation scrollToAnimation;
	scrollToAnimation.setRepeatCount (1);
	scrollToAnimation.setTimingType ((pendingSteps != 0) ? kTimingEaseOut : kTimingEaseInOut);
	scrollToAnimation.setStartValue (startValue);
	scrollToAnimation.setDuration (duration);
	scrollToAnimation.setEndValue (endValue);
	scrollToAnimation.setCompletionHandler (AutoPtr<OverScrollAnimationCompletionHandler> (NEW OverScrollAnimationCompletionHandler (this, verticalDirection, endValue)));
	
	//CCL_PRINTF ("ScrollToAnimation: startValue: %d, endValue: %d, stopVelocity: %f, duration: %f \n", startValue, endValue, 0.f, duration);
	
	AnimationManager::instance ().addAnimation (this, verticalDirection ? kVScrollPos : kHScrollPos, scrollToAnimation.asInterface ());
	animationRunning = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool OverScrollAnimator::onMouseWheelDiscrete (const MouseWheelEvent& event)
{
	IParameter* param = event.isVertical () ? vScrollParam : hScrollParam;
	if(!param)
		param = vScrollParam ? vScrollParam : hScrollParam;
		
	if(param)
	{
		// stop possible bounceBack animation
		stopAnimation (direction);
	
		param->beginEdit ();

		if(event.delta < 0)
		{
			if((param->getValue () == param->getMax ()) && isWrapAround ())
				param->setValue (param->getMin (), true);
			else
				param->increment ();
		}
		else
		{
			if((param->getValue () == param->getMin ()) && isWrapAround ())
				param->setValue (param->getMax (), true);
			else
				param->decrement ();
		}
		param->endEdit ();
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool OverScrollAnimator::onMouseWheelContinuous (const MouseWheelEvent& event)
{
	static PointF pointsPerSecond;
	static PointF accumulatedDelta;
	static int signX = 0;
	static int signY = 0;
	static bool rollOutTriggered = false;
	static bool directionChange = false;
	
	cancelSignals ();
	
	bool isShiftPressed = (event.keys.getModifiers () & KeyState::kShift) != 0;
	
	float scrollDeltaX = event.isAxisInverted () ? -event.deltaX : event.deltaX;
	float scrollDeltaY = event.isAxisInverted () ? -event.deltaY : event.deltaY;
	
	int scrollSignX = (int)ccl_sign (scrollDeltaX);
	int scrollSignY = (int)ccl_sign (scrollDeltaY);
	
	// deltaFactor simulates additional friction when scrolling deliberately out of bounds
	if(!event.isRollOutPhase ())
	{
		scrollDeltaX *= needsBounceAnimation (false) ? 0.2f : 1.f;
		scrollDeltaY *= needsBounceAnimation (true) ? 0.2f : 1.f;
	}
	
	int elapsedTime = getTimeSinceLastEvent ();

	if(direction == (Styles::kVertical|Styles::kHorizontal))
		directionChange = ((scrollDeltaY * scrollSignY) > (scrollDeltaX * scrollSignX)) ? (scrollSignY != signY) : (scrollSignX != signX);
	if(direction == Styles::kVertical)
		directionChange = (scrollSignY != signY);
	else
	 	directionChange = (scrollSignX != signX);
	 
	signX = scrollSignX;
	signY = scrollSignY;
	
	// reset values on potential first MouseWheelEvent or direction change
	if(elapsedTime > 1000 || directionChange)
	{
		pointsPerSecond.x = 0.f;
		pointsPerSecond.y = 0.f;
		accumulatedDelta.x = 0.f;
		accumulatedDelta.y = 0.f;
		elapsedTime = 10;
	}
	
	float boostedDeltaH = OverScrollHelper::getBoostedDelta (scrollDeltaX, pointsPerSecond.x, getScrollParamValueRange (false));
	float boostedDeltaV = OverScrollHelper::getBoostedDelta (scrollDeltaY, pointsPerSecond.y, getScrollParamValueRange (true));

	accumulatedDelta.x += (boostedDeltaH * (isShiftPressed ? 0.1f : 0.34f));
	accumulatedDelta.y += (boostedDeltaV * (isShiftPressed ? 0.1f : 0.34f));
	
	if(event.isRollOutPhase ())
	{
		if(rollOutTriggered == false)
		{
			// ignore rollout events - trigger scroll animation
			float velocityBoundH = getScrollParamValueRange (false);
			float velocityBoundV = getScrollParamValueRange (true);
			accumulatedDelta.x = ccl_bound (-accumulatedDelta.x, -velocityBoundH, velocityBoundH);
			accumulatedDelta.y = ccl_bound (-accumulatedDelta.y, -velocityBoundV, velocityBoundV);
			startScrollAnimation (accumulatedDelta);
		}
		accumulatedDelta.x = 0.f;
		accumulatedDelta.y = 0.f;
		pointsPerSecond.x = 0.f;
		pointsPerSecond.y = 0.f;
		rollOutTriggered = true;
		return true;
	}
	else
	{
		rollOutTriggered = false;
		stopAnimation (event.isVertical ());

		OverScrollHelper::calculateAverageVelocity (pointsPerSecond.x, scrollDeltaX, elapsedTime);
		OverScrollHelper::calculateAverageVelocity (pointsPerSecond.y, scrollDeltaY, elapsedTime);
	
		CCL_PRINTF ("current value: %d,  %f, %f \n", vOverScrollParam->getValue ().asInt (), accumulatedDelta.y, boostedDeltaV);
	
		if(hOverScrollParam && hScrollParam)
		{
			if(ccl_abs (accumulatedDelta.x) > 1)
			{
				setScrollValue (false, ccl_to_int (hOverScrollParam->getValue ().asInt () + accumulatedDelta.x));
				
				accumulatedDelta.x = 0;
			}
			
			if(elapsedTime > 0)
				scrollToSnappedPosition (false, (int)elapsedTime);
		}
		
		if(vOverScrollParam && vScrollParam)
		{
			if(ccl_abs (accumulatedDelta.y) > 1)
			{
				int newValue = ccl_to_int (vOverScrollParam->getValue ().asInt () + accumulatedDelta.y);
				
				CCL_PRINTF ("new Value: %d, \n", newValue);

				if(newValue > (200 * snapSize.y))
					accumulatedDelta.y = 0;
					
				setScrollValue (true, newValue);
				
				accumulatedDelta.y = 0;
			}
			
			if(elapsedTime > 0)
				scrollToSnappedPosition (true, (int)elapsedTime);
		}
			
		return true;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int OverScrollAnimator::getTimeSinceLastEvent () const
{
	static int64 lastContinuousScrollEventTime = 0;
	int64 currentContinuousScrollEventTime = System::GetSystemTicks ();
	int timeBetweenScrollEvents = (int)(currentContinuousScrollEventTime - lastContinuousScrollEventTime);
	lastContinuousScrollEventTime = currentContinuousScrollEventTime;
	return ccl_max (1, timeBetweenScrollEvents);
}

/////////////////////////////////////////////////////////////////////////////

void OverScrollAnimator::scrollToSnappedPosition (bool verticalDirection, int delay)
{
	int pauseBeforeScrollTrigger = ccl_max (50, delay * 2);
			
	// after last scroll event - wait - and simulate rollout events to snapped position
	if(needsBounceAnimation (verticalDirection))
	{
		if(verticalDirection)
			(NEW Message ("triggerVerticalBounceBackAnimation"))->post (this, pauseBeforeScrollTrigger);
		else
			(NEW Message ("triggerHorizontalBounceBackAnimation"))->post (this, pauseBeforeScrollTrigger);
	}
	else if(IParameter* scrollParam = getOverScrollParameter (verticalDirection))
	{
		int repeats = 0;
		int step = 0;
		
		int startValue = scrollParam->getValue ();
		int snappedEndValue = ((startValue + (getSnapSize (verticalDirection) / 2)) / getSnapSize (verticalDirection)) * getSnapSize (verticalDirection);
		if(snappedEndValue > startValue)
		{
			repeats = snappedEndValue - startValue;
			step = 1;
		}
		else if(snappedEndValue < startValue)
		{
			repeats = startValue - snappedEndValue;
			step = -1;
		}
		
		if(repeats > 0)
			(NEW Message ("scrollToNextSnappedValue", verticalDirection, step, repeats, delay))->post (this, pauseBeforeScrollTrigger);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OverScrollAnimator::scrollToNextSnappedValue (MessageRef msg)
{
	bool verticalDirection = msg[0].asBool ();
	int step = msg[1].asInt ();
	int repeats = msg[2].asInt () - 1;
	int timeToNextEvent = ccl_to_int (msg[3].asInt () * 1.1); // slow down with 1.1 (maxFactor in 10th iteration ~ 2.6)
	
	if(IParameter* overScrollParam = getOverScrollParameter (verticalDirection))
	{
		setScrollValue (verticalDirection, overScrollParam->getValue ().asInt () + step);
		if(repeats > 0)
		{
			(NEW Message ("scrollToNextSnappedValue", verticalDirection, step, repeats, timeToNextEvent))->post (this, timeToNextEvent);
		}
		else if(IParameter* scrollParam = getScrollParameter (verticalDirection))
		{
			scrollParam->performUpdate ();
			scrollParam->endEdit ();
		}
	}
		
}

///////////////////////////////////////////////////////////////////////////

int OverScrollAnimator::normalizedToOverScrollPos (bool verticalDirection)
{
 	float normalizedValue = verticalDirection ? vScrollParam->getNormalized () : hScrollParam->getNormalized ();
 
	Coord firstOverScrollMargin = 0;
	if(isWrapAround () == false)
		firstOverScrollMargin = verticalDirection ? overScrollMargins.top : overScrollMargins.left;
		
	return int(normalizedValue * getScrollRange (verticalDirection) + firstOverScrollMargin);
}

///////////////////////////////////////////////////////////////////////////

float OverScrollAnimator::overScrollPosToNormalized (bool verticalDirection)
{
	int scrollPos = verticalDirection ? vOverScrollParam->getValue ().asInt () : hOverScrollParam->getValue ().asInt ();

	if(isWrapAround () == false)
	{
		Coord scrollParamMin = verticalDirection ? overScrollMargins.top : overScrollMargins.left;
		scrollPos -= scrollParamMin;
		scrollPos = ccl_bound (scrollPos, 0, getOverScrollBoundsMax (verticalDirection) - scrollParamMin);
	}
	
	int value = (scrollPos + (getSnapSize (verticalDirection) / 2)) / getSnapSize (verticalDirection);
	
	if(IParameter* scrollParam = verticalDirection ? vScrollParam : hScrollParam)
	{
		int valueRange = scrollParam->getMax ().asInt () - scrollParam->getMin ().asInt ();
		if(valueRange > 0)
			return (value % (valueRange + 1) / (float)valueRange);
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Coord OverScrollAnimator::getOverScrollPosition (bool verticalDirection)
{
	if(verticalDirection)
		return vOverScrollParam ? vOverScrollParam->getValue ().asInt () : 0;
	else
		return hOverScrollParam ? hOverScrollParam->getValue ().asInt () : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OverScrollAnimator::updateOverScrollPosition (bool verticalDirection, bool forceValueUpdate)
{
	IParameter* overScrollParam = getOverScrollParameter (verticalDirection);
	IParameter* scrollParam = verticalDirection ? vScrollParam : hScrollParam;
	
	if(scrollParam && overScrollParam)
	{
		if(scrollParam->getValue () != scrollParam->getValuePlain (overScrollPosToNormalized (verticalDirection)) || forceValueUpdate)
		{
			stopAnimation (verticalDirection);
			overScrollParam->setValue (normalizedToOverScrollPos (verticalDirection), true);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API OverScrollAnimator::setProperty (MemberID propertyId, const Variant& var)
{
	auto isAnimationReset = [] (VariantRef v) { return v.getUserValue () == IAnimation::kResetBackwards; };

	if((propertyId == kVScrollPos) || (propertyId == kHScrollPos)) // from animation manager
	{
		if(!isAnimationReset (var)) // ignore reset to startValue
			setScrollValue ((propertyId == kVScrollPos), var.asInt ());
	}
	
	return SuperClass::setProperty (propertyId, var);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API OverScrollAnimator::notify (ISubject* subject, MessageRef msg)
{
	if(isEqualUnknown (subject, vScrollParam) && (msg == kChanged))
	{
		updateOverScrollPosition (true);
	}
	else if(isEqualUnknown (subject, hScrollParam) && (msg == kChanged))
	{
		updateOverScrollPosition (false);
	}
	else if(msg == "triggerVerticalBounceBackAnimation")
		triggerBounceBackAnimation (true);
	else if(msg == "triggerHorizontalBounceBackAnimation")
		triggerBounceBackAnimation (false);
	else if(msg == "scrollToNextSnappedValue")
		scrollToNextSnappedValue (msg);
	else
		SuperClass::notify (subject, msg);
}
