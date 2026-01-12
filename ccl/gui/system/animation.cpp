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
// Filename    : ccl/gui/system/animation.cpp
// Description : Animation
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/system/animation.h"
#include "ccl/gui/graphics/graphicshelper.h"

#include "ccl/base/message.h"
#include "ccl/base/math/mathcurve.h"
#include "ccl/public/math/mathprimitives.h"

#include "ccl/public/systemservices.h"

namespace CCL {

//************************************************************************************************
// CubicBezierTimingFunction - see http://cubic-bezier.com
//************************************************************************************************

// Bezier curves use a parametric representation:
// x(t) = bezierX (t)
// y(t) = bezierY (t)
// To get y(x) it is needed to solve t(x) = inv_bezierX (t) numerically as the used polynomials are not 
// uniquely bijective / invertible.

class CubicBezierTimingFunction: public Object,
								 public ITimingFunction
{
public:
	DECLARE_CLASS (CubicBezierTimingFunction, Object)

	CubicBezierTimingFunction (double c1x = 0., double c1y = 0., double c2x = 1., double c2y = 1.);
	CubicBezierTimingFunction (const AnimationControlPoints& values);

	// ITimingFunction
	tresult CCL_API setType (AnimationTimingType type) override;
	tresult CCL_API setControlPoints (const AnimationControlPoints& values) override;
	tresult CCL_API getControlPoints (AnimationControlPoints& values) const override;
	double CCL_API getTime (double t) const override;

	CLASS_INTERFACE (ITimingFunction, Object)

protected:
	AnimationControlPoints controlPoints;
	Math::CubicBezierCurve bezierX;
	Math::CubicBezierCurve bezierY;
	Math::CubicCurve derivateX;

	INLINE double solve (double tIn, double epsilon) const
	{
		#if 1
		Math::CurveApproacher approacher (bezierX, derivateX, 0., 1.);
		double t = approacher.getX (tIn, epsilon);
		#else
		double t = bezierX.getX (tIn);
		#endif
		return bezierY.getY (t);
	}
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// CubicBezierTimingFunction
//************************************************************************************************

DEFINE_CLASS (CubicBezierTimingFunction, Object)
DEFINE_CLASS_UID (CubicBezierTimingFunction, 0xf1ce1691, 0xa991, 0x4ea3, 0xbe, 0x7c, 0xc6, 0x3e, 0xd6, 0x78, 0x51, 0x0)

//////////////////////////////////////////////////////////////////////////////////////////////////

CubicBezierTimingFunction::CubicBezierTimingFunction (double c1x, double c1y, double c2x, double c2y)
{
	AnimationControlPoints values (c1x, c1y, c2x, c2y);
	setControlPoints (values);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CubicBezierTimingFunction::CubicBezierTimingFunction (const AnimationControlPoints& values)
{
	setControlPoints (values);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CubicBezierTimingFunction::setType (AnimationTimingType type)
{
	const ITimingFunction* function = Animation::getStandardTimingFunction (type);
	ASSERT (function != nullptr)
	if(function == nullptr)
		return kResultInvalidArgument;

	AnimationControlPoints values = {0};
	function->getControlPoints (values);
	return setControlPoints (values);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CubicBezierTimingFunction::setControlPoints (const AnimationControlPoints& values)
{
	controlPoints = values;
	bezierX.assign (0., values.c1x, values.c2x, 1.);
	bezierY.assign (0., values.c1y, values.c2y, 1.);
	bezierX.getDerivative (derivateX);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CubicBezierTimingFunction::getControlPoints (AnimationControlPoints& values) const
{
	values = controlPoints;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

double CCL_API CubicBezierTimingFunction::getTime (double t) const
{
	return solve (t, 1e-3);
}

//************************************************************************************************
// AnimationClock
//************************************************************************************************

DEFINE_CLASS_HIDDEN (AnimationClock, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

AnimationClock::AnimationClock ()
: systemTime (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

AnimationClock::~AnimationClock ()
{
	ASSERT (animations.isEmpty ())
}

//////////////////////////////////////////////////////////////////////////////////////////////////

double AnimationClock::getSystemTime () const
{
	return systemTime;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AnimationClock::addAnimation (Animation* animation)
{
	ASSERT (animation->isRunning ())
	animations.add (animation);
	if(!isTimerEnabled ())
	{
		startTimer ();
		systemTime = System::GetProfileTime ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AnimationClock::removeAnimation (Animation* animation)
{
	ASSERT (animation->isRunning ())
	animations.remove (animation);
	if(animations.isEmpty ())
		stopTimer ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AnimationClock::onIdleTimer ()
{
	AnimationClockScope scope (this);

	systemTime = System::GetProfileTime ();

	// Note: animations remove themselves when finished!
	ListForEachObject (animations, Animation, a)
		a->animate ();
	EndFor
}

//************************************************************************************************
// Animation
//************************************************************************************************

const ITimingFunction* Animation::getStandardTimingFunction (AnimationTimingType which)
{
	// see CSS <timing-function> "cubic-bezier()"
	// http://msdn.microsoft.com/en-us/library/ie/hh772288%28v=vs.85%29.aspx

	static CubicBezierTimingFunction easeIn (0.42, 0, 1, 1);
	static CubicBezierTimingFunction easeOut (0, 0, 0.58, 1);
	static CubicBezierTimingFunction easeInOut (0.42, 0, 0.58, 1);

	switch(which)
	{
	case kTimingEaseIn : return &easeIn;
	case kTimingEaseOut : return &easeOut;
	case kTimingEaseInOut : return &easeInOut;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_ABSTRACT_HIDDEN (Animation, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

Animation::Animation ()
: duration (0),
  targetProperty (nullptr, nullptr),
  state (kStopped),
  startTime (0.),
  repeatCount (1),
  options (0),
  timingType (kTimingLinear),
  resetMode (kResetBackwards)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Animation::~Animation ()
{
	if(isRunning ())
		stop (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Animation::getDescription (AnimationDescription& description) const
{
	description.duration = getDuration ();
	description.timingType = getTimingType ();
	description.controlPoints = getControlPoints ();
	description.repeatCount = getRepeatCount ();
	description.options = getOptions ();
	description.resetMode = getResetMode ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Animation::setDescription (const AnimationDescription& description)
{
	setDuration (description.duration);
	setTimingType (description.timingType);
	setControlPoints (description.controlPoints);
	setRepeatCount (description.repeatCount);
	setOptions (description.options);
	setResetMode (description.resetMode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAnimationCompletionHandler* Animation::getCompletionHandler () const
{
	return completionHandler;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Animation::setCompletionHandler (IAnimationCompletionHandler* handler)
{
	completionHandler = handler;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Animation::start ()
{
	ASSERT (!isRunning ())
	if(isRunning ())
		return;

	state = kRunning;

	ASSERT (clock != nullptr)
	if(clock)
	{
		clock->addAnimation (this);
		startTime = clock->getSystemTime ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Animation::stop (bool destroyed)
{
	ASSERT (isRunning ())
	if(!isRunning ())
		return;

	ASSERT (clock != nullptr)
	if(clock)
		clock->removeAnimation (this);

	if(destroyed == false)
		resetValue ();

	state = kStopped;
	startTime = 0.;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Animation::isRunning () const
{
	return state > kStopped;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

double Animation::getNormalizedValue (double position) const
{
	double normalizedValue = 0.;
	switch(timingType)
	{
	case kTimingToggle :
		normalizedValue = position < .5 ? 0. : 1.;
		break;
	
	case kTimingLinear :
		normalizedValue = position;
		break;

	case kTimingCubicBezier :
		normalizedValue = CubicBezierTimingFunction (controlPoints).getTime (position);
		break;

	default :
		if(const ITimingFunction* function = getStandardTimingFunction (timingType))
			normalizedValue = function->getTime (position);
		#if DEBUG
		else
			CCL_DEBUGGER ("Unknown timing type!\n")
		#endif
		break;
	}
	return normalizedValue;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Animation::getPosition (double& position, int64& repeatIndex, double relativeTime) const
{
	if(ccl_equals (duration, 0., 1e-13))
	{
		position = 1.;
		return false; // animation needs to be stopped
	}

	repeatIndex = (int64)(relativeTime / duration); // how many times the animation repeated
	position = fmod (relativeTime, duration) / duration; // relative position inside current repetition

	// in auto-reverse mode, go backwards with every other repetition
	if(isAutoReverse () && repeatIndex % 2)
		position = 1. - position;

	// check if repeat count is reached
	if(repeatCount != kRepeatForever)
	{
		if(isAutoReverse ())
			repeatIndex /= 2;

		if(repeatIndex >= repeatCount)
			return false; // animation needs to be stopped
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Animation::animate ()
{
	ASSERT (state == kRunning)
	ASSERT (clock != nullptr)

	double relativeTime = clock->getSystemTime () - startTime;

	double position = 0.;
	int64 repeatIndex = 0;
	if(getPosition (position, repeatIndex, relativeTime) == false)
	{
		stop ();
		finish ();
		return;
	}

	double normalizedValue = getNormalizedValue (position);

	CCL_PRINTF ("animate time = %.3lf position = %.3lf normalized value = %.3lf\n", relativeTime, position, normalizedValue)

	targetProperty.set (getValue (normalizedValue));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

double Animation::getTotalRunningTime () const
{
	if(repeatCount == kRepeatForever)
		return -1.;

	double totalTime = duration;
	if(isAutoReverse ())
		totalTime *= 2.;
	totalTime *= repeatCount;
	return totalTime;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Variant Animation::getValueAtTime (double relativeTime) const
{
	double position = 0.;
	int64 repeatIndex = 0;
	if(getPosition (position, repeatIndex, relativeTime) == false)
		return getFinalValue (); // already stopped

	double normalizedValue = getNormalizedValue (position);
	return getValue (normalizedValue);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Variant Animation::getFirstValue () const
{
	return getValue (0.);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Variant Animation::getFinalValue () const
{
	return getValue (1.);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Animation::resetValue ()
{
	AnimationClockScope scope (clock);

	Variant value (resetMode == kResetBackwards ? getFirstValue () : getFinalValue ());
	value.setUserValue (short(resetMode));
	targetProperty.set (value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Animation::finish ()
{
	SharedPtr<Object> holder (this);

	if(completionHandler)
		completionHandler->onAnimationFinished ();

	AnimationManager::instance ().onAnimationFinished (this);
}

//************************************************************************************************
// BasicAnimation
//************************************************************************************************

bool BasicAnimation::canAnimate (VariantRef value)
{
	if(IUIValue* v = IUIValue::toValue (value))
		switch(v->getType ())
		{
		case IUIValue::kPoint :
		case IUIValue::kRect :
		case IUIValue::kColor :
		case IUIValue::kColorF :
		case IUIValue::kPointF :
		case IUIValue::kRectF :
		case IUIValue::kPointF3D :
		case IUIValue::kPointF4D :
			return true;
		}

	return value.isNumeric ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (BasicAnimation, Animation)
DEFINE_CLASS_UID (BasicAnimation, 0xe6b9650e, 0x874e, 0x4d1f, 0x9b, 0x8e, 0x13, 0x8b, 0x15, 0x4f, 0x8a, 0xaa)

//////////////////////////////////////////////////////////////////////////////////////////////////

IUIValue::Type BasicAnimation::getValueType () const
{
	IUIValue* value = IUIValue::toValue (startValue);
	return value ? value->getType () : IUIValue::kNil;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API BasicAnimation::setStartValue (VariantRef value)
{
	ASSERT (canAnimate (value))
	if(!canAnimate (value))
		return kResultInvalidArgument;

	startValue = value;
	startValue.share ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API BasicAnimation::setEndValue (VariantRef value)
{
	ASSERT (canAnimate (value))
	if(!canAnimate (value))
		return kResultInvalidArgument;

	endValue = value; 
	endValue.share ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API BasicAnimation::setDescription (const AnimationDescription& description)
{
	SuperClass::setDescription (description); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API BasicAnimation::setCompletionHandler (IAnimationCompletionHandler* handler)
{
	SuperClass::setCompletionHandler (handler); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Variant BasicAnimation::getValue (double normalizedValue) const
{
	if(startValue.isNumeric ())
		return interpolate (startValue, endValue, normalizedValue);
	else
	{
		static UIValue object;
		object.reset ();

		IUIValue* start = IUIValue::toValue (startValue); 
		IUIValue* end = IUIValue::toValue (endValue); 
		ASSERT (start && end)
		if(start && end)
		{
			ASSERT (start->getType () == end->getType ())
			switch(start->getType ())
			{
			case IUIValue::kPoint :
				{
					Point startPoint, endPoint, p;
					start->toPoint (startPoint);
					end->toPoint (endPoint);
					p.x = (Coord)interpolate (startPoint.x, endPoint.x, normalizedValue);
					p.y = (Coord)interpolate (startPoint.y, endPoint.y, normalizedValue);
					object.fromPoint (p);
				}
				break;

			case IUIValue::kPointF :
				{
					PointF startPoint, endPoint, p;
					start->toPointF (startPoint);
					end->toPointF (endPoint);
					p.x = (CoordF)interpolate (startPoint.x, endPoint.x, normalizedValue);
					p.y = (CoordF)interpolate (startPoint.y, endPoint.y, normalizedValue);
					object.fromPointF (p);
				}
				break;

			case IUIValue::kPointF3D :
				{
					PointF3D startPoint, endPoint, p;
					start->toPointF3D (startPoint);
					end->toPointF3D (endPoint);
					p.x = (CoordF)interpolate (startPoint.x, endPoint.x, normalizedValue);
					p.y = (CoordF)interpolate (startPoint.y, endPoint.y, normalizedValue);
					p.z = (CoordF)interpolate (startPoint.z, endPoint.z, normalizedValue);
					object.fromPointF3D (p);
				}
				break;

			case IUIValue::kPointF4D :
				{
					PointF4D startPoint, endPoint, p;
					start->toPointF4D (startPoint);
					end->toPointF4D (endPoint);
					p.x = (CoordF)interpolate (startPoint.x, endPoint.x, normalizedValue);
					p.y = (CoordF)interpolate (startPoint.y, endPoint.y, normalizedValue);
					p.z = (CoordF)interpolate (startPoint.z, endPoint.z, normalizedValue);
					p.w = (CoordF)interpolate (startPoint.w, endPoint.w, normalizedValue);
					object.fromPointF4D (p);
				}
				break;

			case IUIValue::kRect :
				{
					Rect startRect, endRect, r;
					start->toRect (startRect);
					end->toRect (endRect);
					r.left = (Coord)interpolate (startRect.left, endRect.left, normalizedValue);
					r.top = (Coord)interpolate (startRect.top, endRect.top, normalizedValue);
					r.right = (Coord)interpolate (startRect.right, endRect.right, normalizedValue);
					r.bottom = (Coord)interpolate (startRect.bottom, endRect.bottom, normalizedValue);
					object.fromRect (r);
				}
				break;

			case IUIValue::kRectF :
				{
					RectF startRect, endRect, r;
					start->toRectF (startRect);
					end->toRectF (endRect);
					r.left = (CoordF)interpolate (startRect.left, endRect.left, normalizedValue);
					r.top = (CoordF)interpolate (startRect.top, endRect.top, normalizedValue);
					r.right = (CoordF)interpolate (startRect.right, endRect.right, normalizedValue);
					r.bottom = (CoordF)interpolate (startRect.bottom, endRect.bottom, normalizedValue);
					object.fromRectF (r);
				}
				break;

			case IUIValue::kColor :
				{
					Color startColor, endColor, c;
					start->toColor (startColor);
					end->toColor (endColor);
					c = Color::linearGradient (startColor, endColor, (float)normalizedValue);
					object.fromColor (c);
				}
				break;

			case IUIValue::kColorF :
				{
					ColorF startColor, endColor, c;
					start->toColorF (startColor);
					end->toColorF (endColor);
					c = ColorF::linearGradient (startColor, endColor, (float)normalizedValue);
					object.fromColorF (c);
				}
				break;
			}
		}

		return static_cast<IUIValue*> (&object);
	}
}

//************************************************************************************************
// TransformAnimation
//************************************************************************************************

DEFINE_CLASS (TransformAnimation, Animation)
DEFINE_CLASS_UID (TransformAnimation, 0x5c52b447, 0xdfe, 0x4574, 0x9c, 0xec, 0x64, 0x51, 0xf6, 0xc7, 0x62, 0x36)

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TransformAnimation::setDescription (const AnimationDescription& description)
{
	SuperClass::setDescription (description);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TransformAnimation::setCompletionHandler (IAnimationCompletionHandler* handler)
{
	SuperClass::setCompletionHandler (handler); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult TransformAnimation::addOperation (MatrixOpType type, double startValue, double endValue)
{
	return operations.add (MatrixOp (type, startValue, endValue)) ? kResultOk : kResultOutOfMemory;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API TransformAnimation::addTranslationX (double startValue, double endValue)
{
	return addOperation (kTranslateX, startValue, endValue);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API TransformAnimation::addTranslationY (double startValue, double endValue)
{
	return addOperation (kTranslateY, startValue, endValue);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API TransformAnimation::addScalingX (double startValue, double endValue)
{
	return addOperation (kScaleX, startValue, endValue);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API TransformAnimation::addScalingY (double startValue, double endValue)
{
	return addOperation (kScaleY, startValue, endValue);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API TransformAnimation::addRotation (double startAngle, double endAngle)
{
	return addOperation (kRotate, startAngle, endAngle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API TransformAnimation::addSkewingX (double startAngle, double endAngle)
{
	return addOperation (kSkewX, startAngle, endAngle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API TransformAnimation::addSkewingY (double startAngle, double endAngle)
{
	return addOperation (kSkewY, startAngle, endAngle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TransformAnimation::getStartTransform (Transform& t) const
{
	getTransformForValue (t, 0.);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TransformAnimation::getEndTransform (Transform& t) const
{
	getTransformForValue (t, 1.);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TransformAnimation::getTransformForValue (Transform& t, double normalizedValue) const
{
	for(int i = operations.count () - 1; i >= 0; --i)
	{
		const MatrixOp& op = operations[i];
		double currentValue = interpolate (op.startValue, op.endValue, normalizedValue);
		switch(op.type)
		{
		case kTranslateX :
			t.translate ((float)currentValue, 0.f);
			break;
		case kTranslateY :
			t.translate (0.f, (float)currentValue);
			break;
		case kScaleX :
			t.scale ((float)currentValue, 1.f);
			break;
		case kScaleY :
			t.scale (1.f, (float)currentValue);
			break;
		case kRotate :
			t.rotate ((float)Math::degreesToRad (currentValue));
			break;
		case kSkewX :
			t.skewX ((float)Math::degreesToRad (currentValue));
			break;
		case kSkewY :
			t.skewY ((float)Math::degreesToRad (currentValue));
			break;
		case kNoOp :
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Variant TransformAnimation::getValue (double normalizedValue) const
{
	Transform t;
	getTransformForValue (t, normalizedValue);

	static UIValue object;
	object.fromTransform (t);
	return static_cast<IUIValue*> (&object);
}

//************************************************************************************************
// AnimationManager::TargetItem
//************************************************************************************************

AnimationManager::TargetItem::TargetItem (IObject* target)
: target (target),
  subject (UnknownPtr<ISubject> (target))
{
	ASSERT (target && subject)
	if(subject)
		subject->addObserver (this);

	animations.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AnimationManager::TargetItem::~TargetItem ()
{
	if(subject)
		subject->removeObserver (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IObject* AnimationManager::TargetItem::getTarget () const
{
	return target;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AnimationManager::TargetItem::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kDestroyed)
	{
		ASSERT (subject && subject == this->subject)
		AnimationManager::instance ().onTargetDestroyed (target);
		// ATTENTION: 'this' is dead after onTargetDestroyed()!
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AnimationManager::TargetItem::add (Animation* a)
{
	a->retain ();
	animations.add (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AnimationManager::TargetItem::remove (Animation* a)
{
	return animations.remove (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Animation* AnimationManager::TargetItem::find (StringID propertyId) const
{
	ArrayForEach (animations, Animation, a)
		if(a->getTargetProperty ().getID () == propertyId)
			return a;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AnimationManager::TargetItem::isEmpty () const
{
	return animations.isEmpty ();
}

//************************************************************************************************
// AnimationManager
//************************************************************************************************

DEFINE_SINGLETON_CLASS (AnimationManager, Object)
DEFINE_CLASS_UID (AnimationManager, 0x85196530, 0x58e2, 0x4ec1, 0x95, 0x45, 0x1, 0x1a, 0x6, 0x15, 0xee, 0xb6)
DEFINE_SINGLETON (AnimationManager)
static const CString kAnimationFinished ("animationFinished");

//////////////////////////////////////////////////////////////////////////////////////////////////

AnimationManager::AnimationManager ()
{
	sharedClocks.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AnimationManager::~AnimationManager ()
{
	cancelSignals ();

	ASSERT (handlers.isEmpty ())

#if DEBUG
	for(int i = 0; i < kBucketCount; i++)
	{
		TargetItemBucket& bucket = buckets[i];
		if(!bucket.isEmpty ())
			CCL_DEBUGGER ("AnimationManager bucket not empty!!!")
	}
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AnimationClock* AnimationManager::getSharedClock (StringID clockName)
{
	ASSERT (!clockName.isEmpty ())
	if(clockName.isEmpty ())
		return nullptr;

	ArrayForEach (sharedClocks, AnimationClock, clock)
		if(clock->getName () == clockName)
			return clock;
	EndFor

	AnimationClock* clock = NEW AnimationClock;
	clock->setName (clockName);
	sharedClocks.add (clock);
	return clock;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AnimationManager::onAnimationFinished (Animation* animation)
{
	(NEW Message (kAnimationFinished, ccl_as_unknown (animation)))->post (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AnimationManager::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kAnimationFinished)
	{
		Animation* animation = unknown_cast<Animation> (msg[0].asUnknown ());
		ASSERT (animation)
		IObject* target = animation->getTargetProperty ().getHolder ();

		// cleanup
		if(TargetItem* item = lookup (target))
			if(item->remove (animation))
			{
				animation->release ();
				if(item->isEmpty ())
				{
					getBucket (item->getTarget ()).remove (item);
					item->release ();
				}
			}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AnimationManager::registerHandler (IAnimationHandler* handler)
{
	ASSERT (handler && !handlers.contains (handler))
	if(!handler)
		return kResultInvalidPointer;
	handlers.add (handler);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AnimationManager::unregisterHandler (IAnimationHandler* handler)
{
	ASSERT (handler && handlers.contains (handler))
	if(!handler)
		return kResultInvalidPointer;
	return handlers.remove (handler) ? kResultOk : kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AnimationManager::addAnimation (IObject* _target, StringID _propertyId, const IAnimation* _prototype)
{	
	// resolve target
	Property targetProperty (_target, _propertyId); 
	IObject* target = targetProperty.getHolder ();
	StringID propertyId = targetProperty.getID ();
	const Animation* prototype = Animation::cast<Animation> (_prototype);	

	SOFT_ASSERT (target && !propertyId.isEmpty () && prototype, "AnimationManager::addAnimation")
	if(!target || propertyId.isEmpty () || !prototype)
		return kResultInvalidArgument;
		
	// try handlers first...
	VectorForEach (handlers, IAnimationHandler*, handler)
		if(handler->addAnimation (target, propertyId, prototype) == kResultOk)
			return kResultOk;
	EndFor

	AutoPtr<Animation> animation = (Animation*)prototype->clone ();
	animation->setTargetProperty (targetProperty);

	if(animation->getClock () == nullptr) // create a local clock if no shared clock is used
	{
		AutoPtr<AnimationClock> clock = NEW AnimationClock;
		animation->setClock (clock);
	}

	add (animation, target);
	animation->start ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AnimationManager::add (Animation* animation, IObject* target)
{
	TargetItem* item = lookup (target);
	if(item == nullptr)
		getBucket (target).append (item = NEW TargetItem (target));

	// remove existing animation for this property
	Animation* existing = item->find (animation->getTargetProperty ().getID ());
	if(existing)
	{
		item->remove (existing);
		existing->release ();
	}

	item->add (animation);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AnimationManager::removeAnimation (IObject* _target, StringID _propertyId)
{
	// resolve target
	Property targetProperty (_target, _propertyId);
	IObject* target = targetProperty.getHolder ();
	StringID propertyId = targetProperty.getID ();
	
	ASSERT (target && !propertyId.isEmpty ())
	if(!target || propertyId.isEmpty ())
		return kResultInvalidArgument;

	// try handlers first...
	VectorForEach (handlers, IAnimationHandler*, handler)
		if(handler->removeAnimation (target, propertyId) == kResultOk)
			return kResultOk;
	EndFor

	bool removed = false;
	if(TargetItem* item = lookup (target))
		if(Animation* a = item->find (propertyId))
		{
			item->remove (a);
			if(a->isRunning ())
				a->stop ();
			a->release ();

			if(item->isEmpty ())
			{
				getBucket (item->getTarget ()).remove (item);
				item->release ();
			}
			removed = true;
		}
	return removed ? kResultOk : kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AnimationManager::onTargetDestroyed (IObject* target)
{
	TargetItem* item = lookup (target);
	if(item)
	{
		getBucket (item->getTarget ()).remove (item);
		item->release ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AnimationManager::TargetItemBucket& AnimationManager::getBucket (IObject* target) const
{
	return const_cast<TargetItemBucket&> (buckets[ccl_hash_pointer (target, kBucketCount)]);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AnimationManager::TargetItem* AnimationManager::lookup (IObject* target) const
{
	ListForEach (getBucket (target), TargetItem*, item)
		if(item->getTarget () == target)
			return item;
	EndFor
	return nullptr;
}

//************************************************************************************************
// StartAnimationAction
//************************************************************************************************

DEFINE_CLASS_HIDDEN (StartAnimationAction, TriggerAction)

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API StartAnimationAction::execute (IObject* target)
{
	ASSERT (target != nullptr)
	ASSERT (prototype != nullptr)
	if(prototype == nullptr || target == nullptr)
		return;

	StringID propertyId = prototype->getTargetProperty ().getID ();
	AnimationManager::instance ().addAnimation (target, propertyId, prototype);
}

//************************************************************************************************
// StopAnimationAction
//************************************************************************************************

DEFINE_CLASS_HIDDEN (StopAnimationAction, TriggerAction)

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API StopAnimationAction::execute (IObject* target)
{
	ASSERT (!propertyId.isEmpty ())
	AnimationManager::instance ().removeAnimation (target, propertyId);
}

//************************************************************************************************
// AnimationCompletionHandlerList
//************************************************************************************************

void AnimationCompletionHandlerList::addCompletionHandler (IAnimationCompletionHandler* handler)
{
	add (handler, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AnimationCompletionHandlerList::onAnimationFinished ()
{
	ForEachUnknown (*this, u)
	UnknownPtr<IAnimationCompletionHandler> handler (u);
	if(handler)
		handler->onAnimationFinished ();
	EndFor
}
