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
// Filename    : ccl/gui/views/overscrollanimator.h
// Description : Animator and MouseHandler - Overscroll behavior for IOverScrollAnimatable(s)
//
//************************************************************************************************

#ifndef _ccl_overscrollanimator_h
#define _ccl_overscrollanimator_h

#include "ccl/gui/system/animation.h"

#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/iparamobserver.h"
#include "ccl/public/gui/framework/styleflags.h"

namespace CCL {

class MouseHandler;
class ClickAction;
interface ITouchHandler;

//************************************************************************************************
// IOverScrollAnimatable
/**
	\ingroup gui */
//************************************************************************************************

interface IOverScrollAnimatable: IUnknown
{
	/** the overscroll independent scrollParameter that will be modified by the animator */
	virtual IParameter* getScrollParameter (bool verticalDirection) const = 0;
	
	/** the maximum overscroll margins to perform the overscroll animation */
	virtual void getOverScrollMargins (Rect& margins) const = 0;
	
	/** the scrollrange in points without overscroll - usually defined by the scroll-parameter */
	virtual void getScrollRange (Point& scrollRange) const = 0;
	
	/** optional snapSize - default is "no snap" (1,1) - used to animate to snapped scroll positions */
	virtual void getSnapSize (Point& snapSize) const = 0;
	
	/** optional wrapAround - default is "false" - return true if the IOverScrollAnimatables support endless scrolling */
	virtual bool isWrapAround () const = 0;
	
	/** IOverScrollAnimatables should trigger a redraw or call invalidate () here */
	virtual void onOverScroll (bool verticalDirection, Coord scrollPosition) = 0;

	DECLARE_IID (IOverScrollAnimatable)
};

//************************************************************************************************
// OverScrollAnimator
/** Enabling overscroll behavior for IOverScrollAnimatable(s),
 	default direction is "omnidirectional" (Styles::kVertical | Styles::kHorizontal).
 	You should delegate createMouseHandler calls and continuous mousewheel events to this animator.
 	The absolute scroll position can be accessed via getOverScrollPosition ().
	Please stop animations when setting the overScrollParameter from elsewhere. */
//************************************************************************************************

class OverScrollAnimator: public Object,
						  public IParamObserver
{
public:
	DECLARE_CLASS (OverScrollAnimator, Object)
	
	OverScrollAnimator (IOverScrollAnimatable* scrollable = nullptr, int direction = Styles::kVertical|Styles::kHorizontal);
	~OverScrollAnimator ();
	
	DECLARE_STRINGID_MEMBER (kVScrollPos)
	DECLARE_STRINGID_MEMBER (kHScrollPos)
	
	MouseHandler* createMouseHandler (View* view, ClickAction* clickAction = nullptr);
	ITouchHandler* createTouchHandler (View* view, ClickAction* tapAction = nullptr);
	bool onMouseWheel (const MouseWheelEvent& event);
	void stopAnimation (bool verticalDirection, int endValue = -1, bool pendingAnimation = false);
	Coord getOverScrollPosition (bool verticalDirection);
	void increment ();
	void decrement ();
	
	// IParamObserver
	virtual tbool CCL_API paramChanged (IParameter* param) override;
	virtual void CCL_API paramEdit (IParameter* param, tbool begin) override;
	
	CLASS_INTERFACE (IParamObserver, Object)
	
protected:
	bool initialized;
	bool animationRunning;
	int pendingSteps;
	int stepsStartValue;
	IOverScrollAnimatable* scrollable;
	Rect overScrollMargins;
	IParameter* vScrollParam;
	IParameter* hScrollParam;
	AutoPtr<IParameter> vOverScrollParam;
	AutoPtr<IParameter> hOverScrollParam;
	Point snapSize;
	Point scrollRange;
	bool wrapAround;
	int direction;
	
	float maximalPointsPerSecond;
	
	void initialize (IOverScrollAnimatable* scrollable);
	void updateOverScrollRange (bool verticalDirection, IParameter* scrollParam);
	int getScrollParamValueRange (bool verticalDirection) const;
	IParameter* getOverScrollParameter (bool verticalDirection) const;
	IParameter* getScrollParameter (bool verticalDirection) const;
	bool needsBounceAnimation (bool verticalDirection) const;
	void startScrollAnimation (PointFRef velocity);
	void triggerRollOutAnimation (bool verticalDirection, float velocity);
	void triggerBounceOutAnimation (bool verticalDirection, float velocity);
	void triggerBounceBackAnimation (bool verticalDirection, float velocity = 100.f);
	void triggerScrollToAnimation (bool verticalDirection, int endValue);
	void setScrollValue (bool verticalDirection, int value);
	
	bool canScroll (int direction) const;
	int getDirection () const;
	bool isWrapAround () const;
	int getSnapSize (bool verticalDirection) const;
	Coord getScrollRange (bool verticalDirection) const;
	Coord getOverScrollBoundsMin (bool verticalDirection) const;
	Coord getOverScrollBoundsMax (bool verticalDirection) const;
	
	float getSlopeWhenLeavingScrollRange (bool verticalDirection, int startValue, int endValue) const;
	AnimationControlPoints getEaseOutPoints (float slope) const;
	
	int getRollOutDistance (float velocity) const;
	int getBounceOutDistance (bool verticalDirection, float velocity) const;
	double getAnimationDuration (int distance, float velocity) const;
	int getTimeSinceLastEvent () const;
	void scrollToSnappedPosition (bool verticalDirection, int delay);
	void scrollToNextSnappedValue (MessageRef msg);
	void setupOverScrollParameter ();
	int normalizedToOverScrollPos (bool verticalDirection);
	float overScrollPosToNormalized (bool verticalDirection);
	void updateOverScrollPosition (bool verticalDirection, bool forceValueUpdate = false);
	bool onMouseWheelContinuous (const MouseWheelEvent& event);
	bool onMouseWheelDiscrete (const MouseWheelEvent& event);
	bool prepareScrollToAnimation (int& endValue, int step, bool verticalDirection);
	
	// Object
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	class OverScrollAnimationCompletionHandler;
	class OverScrollHandler;
};

inline bool OverScrollAnimator::canScroll (int _direction) const { return (direction == _direction) || (direction == (Styles::kVertical|Styles::kHorizontal)); }
inline int OverScrollAnimator::getDirection () const { return direction; }
inline bool OverScrollAnimator::isWrapAround () const { return wrapAround; }


//************************************************************************************************
// ClickAction
/** Call OverScrollAnimator createMouseHandler with an optional ClickAction
	to customize the behavior when no scrolling occured. ClickAction::make ([this](){onClickCode};) */
//************************************************************************************************

template<typename Lambda> class LambdaClickAction;

class ClickAction: public Unknown
{
public:
	virtual void execute () = 0;
	
	template<typename Lambda>
	static ClickAction* make (const Lambda& clickLambda)
	{
		return NEW LambdaClickAction<Lambda> (clickLambda);
	}
};

template<typename Lambda>
class LambdaClickAction: public ClickAction
{
public:
	LambdaClickAction (const Lambda& action) : action (action) {}
	Lambda action;
	void execute () override {action ();}
};

	
} // namespace CCL

#endif // _ccl_overscrollanimator_h
