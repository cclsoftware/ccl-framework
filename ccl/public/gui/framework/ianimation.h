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
// Filename    : ccl/public/gui/framework/ianimation.h
// Description : Animation Interfaces
//
//************************************************************************************************

#ifndef _ccl_ianimation_h
#define _ccl_ianimation_h

#include "ccl/public/base/iunknown.h"

#include "ccl/meta/generated/cpp/gui-constants-generated.h"

namespace CCL {

interface IObject;
interface IAnimationCompletionHandler;
struct AnimationDescription;

//////////////////////////////////////////////////////////////////////////////////////////////////
/** 
\ingroup gui
*/
namespace ClassID
{
	DEFINE_CID (BasicAnimation, 0xe6b9650e, 0x874e, 0x4d1f, 0x9b, 0x8e, 0x13, 0x8b, 0x15, 0x4f, 0x8a, 0xaa)
	DEFINE_CID (TransformAnimation, 0x5c52b447, 0xdfe, 0x4574, 0x9c, 0xec, 0x64, 0x51, 0xf6, 0xc7, 0x62, 0x36)
	DEFINE_CID (AnimationManager, 0x85196530, 0x58e2, 0x4ec1, 0x95, 0x45, 0x1, 0x1a, 0x6, 0x15, 0xee, 0xb6)
	DEFINE_CID (CubicBezierTimingFunction, 0xf1ce1691, 0xa991, 0x4ea3, 0xbe, 0x7c, 0xc6, 0x3e, 0xd6, 0x78, 0x51, 0x0)
}

//////////////////////////////////////////////////////////////////////////////////////////////////
/* AnimationTimingType
\ingroup gui
*/
//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_ENUM (AnimationTimingType)
{
	kTimingLinear = kAnimationTimingTypeLinear,
	kTimingToggle = kAnimationTimingTypeToggle,
	kTimingEaseIn = kAnimationTimingTypeEaseIn,
	kTimingEaseOut = kAnimationTimingTypeEaseOut,
	kTimingEaseInOut = kAnimationTimingTypeEaseInOut,
	kTimingCubicBezier = kAnimationTimingTypeCubicBezier
};

//************************************************************************************************
/* AnimationControlPoints
\ingroup gui
*/
//************************************************************************************************

struct AnimationControlPoints
{
	union
	{
		struct { double c1x, c1y, c2x, c2y; }; ///< control points for cubic bezier timing
	};	

	AnimationControlPoints (double c1x = 0., double c1y = 0., double c2x = 0., double c2y = 0.)
	: c1x (c1x), c1y (c1y), c2x (c2x), c2y (c2y)
	{}
};

//************************************************************************************************
/** Animation interface
\ingroup gui
*/
//************************************************************************************************

interface IAnimation: IUnknown
{
	enum Options
	{
		kAutoReverse = 1<<0	 ///< animation goes backward and forward
	};
	
	enum RepeatCount
	{
		kRepeatForever = 0xFFFF	///< animation repeats endlessly
	};

	/** Animation will reset the property to the start (or end) value. 
		Mode can be retrieved from Variant::getUserValue() in IObject::setProperty(). */
	enum ResetMode
	{
		kResetBackwards = 1,	///< animation resets to start value (default)
		kResetForwards			///< animation resets to end value
	};

	virtual void CCL_API setDescription (const AnimationDescription& description) = 0;

	virtual void CCL_API setCompletionHandler (IAnimationCompletionHandler* handler) = 0;

	DECLARE_IID (IAnimation)
};

DEFINE_IID (IAnimation, 0x7055bf75, 0xc0e2, 0x49fd, 0xb3, 0x4f, 0xe9, 0x48, 0xf5, 0xa9, 0x95, 0x0)

//************************************************************************************************
// AnimationDescription
//************************************************************************************************

struct AnimationDescription
{
	double duration;
	AnimationTimingType timingType;
	AnimationControlPoints controlPoints;
	int repeatCount;
	int options;
	int resetMode;

	AnimationDescription ()
	: duration (0.),
	  timingType (kTimingLinear),
	  repeatCount (1),
	  options (0),
	  resetMode (IAnimation::kResetBackwards)
	{}
};

//************************************************************************************************
// IAnimationCompletionHandler
//************************************************************************************************

interface IAnimationCompletionHandler: IUnknown
{
	virtual void CCL_API onAnimationFinished () = 0;

	DECLARE_IID (IAnimationCompletionHandler)
};

DEFINE_IID (IAnimationCompletionHandler, 0x8b4cac33, 0x4b3a, 0x4f88, 0x85, 0xa4, 0xf4, 0x13, 0x12, 0x57, 0xb2, 0xa7)

//************************************************************************************************
// IBasicAnimation
/** Animates a single scalar value, rectangle, point, or color (IUIValue). */
//************************************************************************************************

interface IBasicAnimation: IAnimation
{
	virtual tresult CCL_API setStartValue (VariantRef value) = 0;

	virtual tresult CCL_API setEndValue (VariantRef value) = 0;

	DECLARE_IID (IBasicAnimation)
};

DEFINE_IID (IBasicAnimation, 0xf79c8922, 0x7e25, 0x433a, 0x9c, 0x66, 0x73, 0xf9, 0x1d, 0xba, 0x88, 0xe9)

//************************************************************************************************
// ITransformAnimation
/** Animates a 2D transformation matrix (IUIValue). */
//************************************************************************************************

interface ITransformAnimation: IAnimation
{
	/** Add translation on X-axis. */
	virtual tresult CCL_API addTranslationX (double startValue, double endValue) = 0;

	/** Add translation on Y-axis. */
	virtual tresult CCL_API addTranslationY (double startValue, double endValue) = 0;

	/** Add scaling on X-axis. */
	virtual tresult CCL_API addScalingX (double startValue, double endValue) = 0;

	/** Add scaling on Y-axis. */
	virtual tresult CCL_API addScalingY (double startValue, double endValue) = 0;

	/** Add rotation from start to end angle, both are in degrees. */
	virtual tresult CCL_API addRotation (double startAngle, double endAngle) = 0;

	/** Add skewing from start to end angle on X-axis, both are in degrees. */
	virtual tresult CCL_API addSkewingX (double startAngle, double endAngle) = 0;

	/** Add skewing from start to end angle on Y-axis, both are in degrees. */
	virtual tresult CCL_API addSkewingY (double startAngle, double endAngle) = 0;

	DECLARE_IID (ITransformAnimation)
};

DEFINE_IID (ITransformAnimation, 0xf79c8922, 0x7e25, 0x433a, 0x9c, 0x66, 0x73, 0xf9, 0x1d, 0xba, 0x88, 0xe9)

//************************************************************************************************
// ITimingFunction
//************************************************************************************************

interface ITimingFunction: IUnknown
{
	/** Initialize with predefined type. */
	virtual tresult CCL_API setType (AnimationTimingType type) = 0;
		
	/** Set control points for cubic bezier timing function. */
	virtual tresult CCL_API setControlPoints (const AnimationControlPoints& values) = 0;

	/** Get control points for cubic bezier timing function. */
	virtual tresult CCL_API getControlPoints (AnimationControlPoints& values) const = 0;

	/** Map input time to output time, both normalized between [0,1]. */
	virtual double CCL_API getTime (double t) const = 0;

	DECLARE_IID (ITimingFunction)
};

DEFINE_IID (ITimingFunction, 0x2262d86d, 0xd949, 0x4663, 0x84, 0x61, 0xa1, 0xe5, 0xea, 0xcd, 0xbe, 0xe7)

//************************************************************************************************
// IAnimationHandler
//************************************************************************************************

interface IAnimationHandler: IUnknown
{
	/** Add animation for given target property. Animation object will be copied. */
	virtual tresult CCL_API addAnimation (IObject* target, StringID propertyId, const IAnimation* prototype) = 0;

	/** Remove animation for given target property. */
	virtual tresult CCL_API removeAnimation (IObject* target, StringID propertyId) = 0;

	DECLARE_IID (IAnimationHandler)
};

DEFINE_IID (IAnimationHandler, 0xd97b8842, 0x3b81, 0x4fa0, 0x9d, 0x1f, 0x2e, 0xd1, 0x37, 0x64, 0x48, 0x88)

//************************************************************************************************
// IAnimationManager
//************************************************************************************************

interface IAnimationManager: IAnimationHandler
{
	/** Register animation handler. */
	virtual tresult CCL_API registerHandler (IAnimationHandler* handler) = 0;

	/** Unregister animation handler. */
	virtual tresult CCL_API unregisterHandler (IAnimationHandler* handler) = 0;

	DECLARE_IID (IAnimationManager)
};

DEFINE_IID (IAnimationManager, 0x4c11a676, 0xef5a, 0x4105, 0x89, 0x7d, 0x38, 0x92, 0xb3, 0xff, 0xb7, 0xff)

} // namespace CCL

#endif // _ccl_ianimation_h
