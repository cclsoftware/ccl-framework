//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/app/paramcurve.cpp
// Description : Parameter Curve classes
//
//************************************************************************************************

#include "ccl/app/paramcurve.h"

#include "ccl/public/math/mathprimitives.h"

#include "core/public/coreinterpolator.h"

using namespace CCL;

//************************************************************************************************
// ParamCurve
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ParamCurve, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

double CCL_API ParamCurve::getRelativeValue (double startValue, double endValue, double linearValue) const
{
	double delta = endValue - startValue;
	double v = ccl_bound<double> (linearValue + delta, 0, 1);
	//if(ccl_equals (v, endValue, 0.01))
	//	v = endValue;
	return v;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr CCL_API ParamCurve::getFactoryName () const
{
	return nullptr;
}

//************************************************************************************************
// ConcaveCurve
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ConcaveCurve, ParamCurve)

//////////////////////////////////////////////////////////////////////////////////////////////////

double CCL_API ConcaveCurve::displayToNormalized (double linearValue) const
{
	return linearValue * linearValue; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

double CCL_API ConcaveCurve::normalizedToDisplay (double curveValue) const
{
	return pow (curveValue, .5);
}

//************************************************************************************************
// ConvexCurve
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ConvexCurve, ParamCurve)

//////////////////////////////////////////////////////////////////////////////////////////////////

double CCL_API ConvexCurve::displayToNormalized (double linearValue) const
{
	linearValue = 1 - linearValue;
	return 1 - linearValue * linearValue; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

double CCL_API ConvexCurve::normalizedToDisplay (double curveValue) const
{
	return 1 - pow (1 - curveValue, .5);
}

//************************************************************************************************
// InterpolatorCurve
//************************************************************************************************

DEFINE_CLASS_HIDDEN (InterpolatorCurve, ParamCurve)

//////////////////////////////////////////////////////////////////////////////////////////////////

InterpolatorCurve::InterpolatorCurve (Core::Interpolator* _interpolator)
: interpolator (nullptr),
  normalizer (nullptr)
{
	if(_interpolator)
		setInterpolator (_interpolator);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

InterpolatorCurve::~InterpolatorCurve ()
{
	setInterpolator (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void InterpolatorCurve::setInterpolator (Core::Interpolator* _interpolator)
{
	delete interpolator;
	delete normalizer;

	interpolator = _interpolator;
	normalizer = nullptr;

	if(interpolator)
		normalizer = NEW Core::LinearInterpolator (interpolator->getMinRange (), interpolator->getMaxRange ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float InterpolatorCurve::getMinRange () const
{
	return interpolator ? interpolator->getMinRange () : 0.f;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float InterpolatorCurve::getMaxRange () const
{
	return interpolator ? interpolator->getMaxRange () : 0.f;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float InterpolatorCurve::getMidRange () const
{
	return interpolator ? interpolator->getMidRange () : 0.f;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void InterpolatorCurve::setRange (float minRange, float maxRange, float midRange)
{
	if(interpolator)
		interpolator->setRange (minRange, maxRange, midRange);
	if(normalizer)
		normalizer->setRange (interpolator ? interpolator->getMinRange () : minRange, interpolator ? interpolator->getMaxRange () : maxRange);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
	
double CCL_API InterpolatorCurve::displayToNormalized (double linearValue) const
{
	// Conversion used by NormalizedValue::set(), i.e. from display to parameter range
	// p->setNormalized (curve->getCurveValue ())

	if(interpolator)
	{
		const float value = interpolator->normalizedToRange (float (linearValue));
		return double (normalizer->rangeToNormalized (value));
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

double CCL_API InterpolatorCurve::normalizedToDisplay (double curveValue) const
{
	// Conversion used by NormalizedValue::get(), i.e. from parameter range to display
	// curve->normalizedToDisplay (p->getNormalized ())
	
	if(interpolator)
	{
		const float value = normalizer->normalizedToRange (float (curveValue));
		return double (interpolator->rangeToNormalized (value));
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

double CCL_API InterpolatorCurve::getRelativeValue (double startValue, double endValue, double linearValue) const
{
	if(interpolator)	
	{
		float startRange = interpolator->normalizedToRange ((float)startValue);
		float endRange = interpolator->normalizedToRange ((float)endValue);
		float deltaRange = endRange - startRange;
		float newRange = interpolator->normalizedToRange ((float)linearValue) + deltaRange;
		return interpolator->rangeToNormalized (newRange);
	}

	return ParamCurve::getRelativeValue (startValue, endValue, linearValue);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr CCL_API InterpolatorCurve::getFactoryName () const
{
	return interpolator ? interpolator->getName () : nullptr;
}

//************************************************************************************************
// ParamCurveFactory
//************************************************************************************************

ParamCurveFactory& ParamCurveFactory::instance ()
{
	static ParamCurveFactory theFactory;
	return theFactory;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParamCurve* ParamCurveFactory::create (StringID name)
{
	VectorForEach (classes, CurveClass, c)
		if(c.name.compare (name, false) == 0)
			return c.createFunc ();
	EndFor

	// try core interpolator factory
	if(Core::Interpolator* interpolator = Core::InterpolatorFactory::create (name))
		return NEW InterpolatorCurve (interpolator);

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ParamCurveFactory::add (StringID name, CreateFunc createFunc)
{
	classes.add (CurveClass (name, createFunc));
}
