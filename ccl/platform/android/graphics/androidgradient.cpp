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
// Filename    : ccl/platform/android/graphics/androidgradient.cpp
// Description : Android Gradient
//
//************************************************************************************************

#include "androidgradient.h"
#include "frameworkgraphics.h"

using namespace CCL;
using namespace CCL::Android;

//************************************************************************************************
// AndroidGradient
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (AndroidGradient, NativeGradient)

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidGradient::initStops (const Stop stops[], int stopCount)
{
	ASSERT (stopCount <= kMaxStopCount)
	stopCount = ccl_min (stopCount, kMaxStopCount);

	colors.setCount (stopCount);
	positions.setCount (stopCount);

	for(int i = 0; i < stopCount; i++)
	{
		colors[i] = FrameworkGraphics::toJavaColor (stops[i].color);
		positions[i] = stops[i].position;
	}
}

//************************************************************************************************
// AndroidLinearGradient
//************************************************************************************************

DEFINE_CLASS_HIDDEN (AndroidLinearGradient, AndroidGradient)

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AndroidLinearGradient::construct (PointFRef startPoint, PointFRef endPoint, const Stop stops[], int stopCount,
												  IGradient* other)
{
	ASSERT (other == nullptr) // copying gradient stops not implemented
	
	this->startPoint = startPoint;
	this->endPoint = endPoint;

	initStops (stops, stopCount);

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

jobject AndroidLinearGradient::getPaint ()
{
	JniAccessor jni;
	if(!paint.isValid ())
		paint.assign (jni, FrameworkGraphicsFactoryClass.createLinearGradientPaint (*gGraphicsFactory,
								startPoint.x, startPoint.y, endPoint.x, endPoint.y, 
								JniIntArray (jni, colors), JniFloatArray (jni, positions)));
	return paint;
}

//************************************************************************************************
// AndroidRadialGradient
//************************************************************************************************

DEFINE_CLASS_HIDDEN (AndroidRadialGradient, AndroidGradient)

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AndroidRadialGradient::construct (PointFRef center, float radius, const Stop stops[], int stopCount,
												  IGradient* other)
{
	ASSERT (other == nullptr) // copying gradient stops not implemented
	
	this->center = center;
	this->radius = radius > 0.f ? radius : 1.f;

	initStops (stops, stopCount);

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

jobject AndroidRadialGradient::getPaint ()
{
	JniAccessor jni;
	if(!paint.isValid ())
		paint.assign (jni, FrameworkGraphicsFactoryClass.createRadialGradientPaint (*gGraphicsFactory,
								center.x, center.y, radius, 
								JniIntArray (jni, colors), JniFloatArray (jni, positions)));
	return paint;
}
