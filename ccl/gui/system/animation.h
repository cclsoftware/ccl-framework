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
// Filename    : ccl/gui/system/animation.h
// Description : Animation
//
//************************************************************************************************

#ifndef _ccl_animation_h
#define _ccl_animation_h

#include "ccl/base/trigger.h"
#include "ccl/base/singleton.h"
#include "ccl/base/collections/objectarray.h"
#include "ccl/base/collections/objectlist.h"

#include "ccl/public/collections/unknownlist.h"

#include "ccl/public/gui/graphics/iuivalue.h"
#include "ccl/public/gui/framework/ianimation.h"
#include "ccl/public/gui/framework/idleclient.h"

namespace CCL {

struct Transform;
class Animation;

//************************************************************************************************
// AnimationClock
/** Note: Animations can be grouped together by sharing the same clock. */
// LATER TODO: 
// - Add public interface IAnimationClock with IAnimationClockHandler
// - Rename to AnimationGroup?
//************************************************************************************************

class AnimationClock: public Object,
					  public IdleClient
{
public:
	DECLARE_CLASS (AnimationClock, Object)

	AnimationClock ();
	~AnimationClock ();

	PROPERTY_MUTABLE_CSTRING (name, Name)

	double getSystemTime () const;

	void addAnimation (Animation* animation);
	void removeAnimation (Animation* animation);

	virtual void onAnimate (bool begin) {}

	CLASS_INTERFACE (ITimerTask, Object)

protected:
	double systemTime;
	ObjectList animations;

	// IdleClient
	void onIdleTimer () override;
};

//************************************************************************************************
// AnimationClockScope
//************************************************************************************************

struct AnimationClockScope
{
	AnimationClock* clock;
	AnimationClockScope (AnimationClock* clock): clock (clock) 
	{ if(clock) clock->onAnimate (true); }
	~AnimationClockScope ()
	{ if(clock) clock->onAnimate (false); }
};

//************************************************************************************************
// Animation
/** Note: Animation objects are used as protoypes, the copy ctor has to be implemented correctly! */
//************************************************************************************************

class Animation: public Object,
				 public IAnimation
{
public:
	DECLARE_CLASS_ABSTRACT (Animation, Object)

	Animation ();
	~Animation ();

	IAnimation* asInterface () { return this; }

	template <class T> static T* cast (const IAnimation* animation)
	{
		return unknown_cast<T> (const_cast<IAnimation*> (animation));
	}

	static const ITimingFunction* getStandardTimingFunction (AnimationTimingType which);

	PROPERTY_OBJECT (Property, targetProperty, TargetProperty)
	PROPERTY_SHARED_AUTO (AnimationClock, clock, Clock)
	PROPERTY_VARIABLE (double, duration, Duration) ///< duration in seconds
	PROPERTY_VARIABLE (AnimationTimingType, timingType, TimingType)
	PROPERTY_OBJECT (AnimationControlPoints, controlPoints, ControlPoints)
	PROPERTY_VARIABLE (int, repeatCount, RepeatCount)
	PROPERTY_VARIABLE (int, options, Options)
	PROPERTY_FLAG (options, kAutoReverse, isAutoReverse)
	PROPERTY_VARIABLE (int, resetMode, ResetMode)
		
	void getDescription (AnimationDescription& description) const;
	void CCL_API setDescription (const AnimationDescription& description) override; ///< [IAnimation]

	IAnimationCompletionHandler* getCompletionHandler () const;
	void CCL_API setCompletionHandler (IAnimationCompletionHandler* handler) override; ///< [IAnimation]

	void start ();
	void stop (bool destroyed = false);
	bool isRunning () const;
	void animate (); ///< called by animation clock

	double getTotalRunningTime () const;
	Variant getValueAtTime (double relativeTime) const;
	Variant getFirstValue () const;
	Variant getFinalValue () const;

	CLASS_INTERFACE (IAnimation, Object)

protected:
	enum State { kStopped, kRunning };
	State state;
	double startTime;
	SharedPtr<IAnimationCompletionHandler> completionHandler;

	bool getPosition (double& position, int64& repeatIndex, double relativeTime) const;
	double getNormalizedValue (double position) const;
	void resetValue ();
	void finish ();

	virtual Variant getValue (double normalizedValue) const = 0;

	static INLINE double interpolate (double startValue, double endValue, double normalizedValue)
	{
		return startValue + (endValue - startValue) * normalizedValue;
	}
};

//************************************************************************************************
// BasicAnimation
//************************************************************************************************

class BasicAnimation: public Animation,
					  public IBasicAnimation
{
public:
	DECLARE_CLASS (BasicAnimation, Animation)

	IUIValue::Type getValueType () const;

	VariantRef getStartValue () const { return startValue; }
	VariantRef getEndValue () const { return endValue; }

	// IBasicAnimation
	tresult CCL_API setStartValue (VariantRef value) override;
	tresult CCL_API setEndValue (VariantRef value) override;
	void CCL_API setDescription (const AnimationDescription& description) override;
	void CCL_API setCompletionHandler (IAnimationCompletionHandler* handler) override;

	CLASS_INTERFACE (IBasicAnimation, Animation)

protected:
	Variant startValue;
	Variant endValue;

	static bool canAnimate (VariantRef value);

	// Animation
	Variant getValue (double normalizedValue) const override;
};

//************************************************************************************************
// TransformAnimation
//************************************************************************************************

class TransformAnimation: public Animation,
						  public ITransformAnimation
{
public:
	DECLARE_CLASS (TransformAnimation, Animation)

	enum MatrixOpType
	{
		kNoOp = 0,
		kTranslateX,
		kTranslateY,
		kScaleX,
		kScaleY,
		kRotate,
		kSkewX,
		kSkewY
	};

	struct MatrixOp
	{
		MatrixOpType type;
		double startValue;
		double endValue;

		MatrixOp (MatrixOpType type = kNoOp, double startValue = 0., double endValue = 0.)
		: type (type), startValue (startValue), endValue (endValue) 
		{}
	};

	static const int kMaxMatrixOpCount = 10;
	typedef Core::FixedSizeVector<MatrixOp, kMaxMatrixOpCount> MatrixOpList;

	const MatrixOpList& getOperations () const { return operations; }

	void getStartTransform (Transform& t) const;
	void getEndTransform (Transform& t) const;

	// ITransformAnimation
	tresult CCL_API addTranslationX (double startValue, double endValue) override;
	tresult CCL_API addTranslationY (double startValue, double endValue) override;
	tresult CCL_API addScalingX (double startValue, double endValue) override;
	tresult CCL_API addScalingY (double startValue, double endValue) override;
	tresult CCL_API addRotation (double startAngle, double endAngle) override;
	tresult CCL_API addSkewingX (double startAngle, double endAngle) override;
	tresult CCL_API addSkewingY (double startAngle, double endAngle) override;
	void CCL_API setDescription (const AnimationDescription& description) override;
	void CCL_API setCompletionHandler (IAnimationCompletionHandler* handler) override;

	CLASS_INTERFACE (ITransformAnimation, Animation)

protected:
	MatrixOpList operations;

	tresult addOperation (MatrixOpType type, double startValue, double endValue);
	void getTransformForValue (Transform& t, double normalizedValue) const;

	// Animation
	Variant getValue (double normalizedValue) const override;
};

//************************************************************************************************
// AnimationManager
//************************************************************************************************

class AnimationManager: public Object,
						public IAnimationManager,
						public Singleton<AnimationManager>
{
public:
	DECLARE_CLASS (AnimationManager, Object)

	AnimationManager ();
	~AnimationManager ();

	AnimationClock* getSharedClock (StringID clockName);

	void onAnimationFinished (Animation* animation);

	// IAnimationManager
	tresult CCL_API addAnimation (IObject* target, StringID propertyId, const IAnimation* prototype) override;
	tresult CCL_API removeAnimation (IObject* target, StringID propertyId) override;
	tresult CCL_API registerHandler (IAnimationHandler* handler) override;
	tresult CCL_API unregisterHandler (IAnimationHandler* handler) override;

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	CLASS_INTERFACE2 (IAnimationHandler, IAnimationManager, Object)

protected:
	class TargetItem: public Object
	{
	public:
		TargetItem (IObject* target);
		~TargetItem ();

		IObject* getTarget () const;
		
		void add (Animation* a);
		bool remove (Animation* a);
		Animation* find (StringID propertyId) const;
		bool isEmpty () const;

		// Object
		void CCL_API notify (ISubject* subject, MessageRef msg) override;

	protected:
		IObject* target;
		ISubject* subject;
		ObjectArray animations;
	};

	ObjectArray sharedClocks;
	Vector<IAnimationHandler*> handlers;

	enum Constants { kBucketCount = 512 };
	typedef LinkedList<TargetItem*> TargetItemBucket;
	TargetItemBucket buckets[kBucketCount];

	TargetItemBucket& getBucket (IObject* target) const;
	TargetItem* lookup (IObject* target) const;

	void add (Animation* animation, IObject* target);
	void onTargetDestroyed (IObject* target);
};

//************************************************************************************************
// StartAnimationAction
//************************************************************************************************

class StartAnimationAction: public TriggerAction
{
public:
	DECLARE_CLASS (StartAnimationAction, TriggerAction)

	PROPERTY_SHARED_AUTO (Animation, prototype, Prototype)

	// TriggerAction
	void CCL_API execute (IObject* target) override;
};

//************************************************************************************************
// StopAnimationAction
//************************************************************************************************

class StopAnimationAction: public TriggerAction
{
public:
	DECLARE_CLASS (StopAnimationAction, TriggerAction)

	PROPERTY_MUTABLE_CSTRING (propertyId, PropertyID)

	// TriggerAction
	void CCL_API execute (IObject* target) override;
};

//************************************************************************************************
// AnimationCompletionHandlerList
//************************************************************************************************

class AnimationCompletionHandlerList: public UnknownList,
									  public IAnimationCompletionHandler
{
public:
	void addCompletionHandler (IAnimationCompletionHandler* handler); ///< shares handler

	// IAnimationCompletionHandler
	void CCL_API onAnimationFinished () override;
	
	CLASS_INTERFACE (IAnimationCompletionHandler, UnknownList)
};

} // namespace CCL

#endif // _ccl_animation_h
