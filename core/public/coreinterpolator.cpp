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
// Filename    : core/public/coreinterpolator.cpp
// Description : Interpolator class
//
//************************************************************************************************

#include "core/public/coreinterpolator.h"
#include "core/public/corestringbuffer.h"
#include "core/public/corevector.h"
#include "core/public/coreprimitives.h"

#include <math.h>

#ifndef CORE_INTERPOLATOR_REGISTRY_ENABLED
#define CORE_INTERPOLATOR_REGISTRY_ENABLED 1
#endif

namespace Core {

//************************************************************************************************
// InterpolatorClass
//************************************************************************************************

struct InterpolatorClass
{
	CStringPtr name;
	InterpolatorFactory::CreateFunc createFunc;

	InterpolatorClass (CStringPtr name = nullptr, InterpolatorFactory::CreateFunc createFunc = nullptr)
	: name (name), 
	  createFunc (createFunc)
	{}
};
	
//************************************************************************************************
// InterpolatorClassList
//************************************************************************************************

class InterpolatorClassList: public Vector<InterpolatorClass>
{
public:
	InterpolatorClassList ()
	{
		add (InterpolatorClass ("linear", InterpolatorFactory::createInterpolator<LinearInterpolator>));
		add (InterpolatorClass ("linrev", InterpolatorFactory::createInterpolator<LinearReverseInterpolator>));
		add (InterpolatorClass ("zoom", InterpolatorFactory::createInterpolator<ZoomInterpolator>));
	}

	static InterpolatorClassList& instance ()
	{
		static InterpolatorClassList theInstance;
		return theInstance;
	}
};

} // namespace Core

using namespace Core;

//************************************************************************************************
// InterpolatorFactory
//************************************************************************************************

Interpolator* InterpolatorFactory::create (CStringPtr _name)
{
	ConstString name (_name);
	if(!name.isEmpty ())
		VectorForEach (InterpolatorClassList::instance (), InterpolatorClass, c)
			if(name == c.name)
			{
				Interpolator* interpolator = c.createFunc ();
				interpolator->setName (c.name);
				return interpolator;
			}
		EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void InterpolatorFactory::add (CStringPtr name, CreateFunc createFunc)
{
#if CORE_INTERPOLATOR_REGISTRY_ENABLED
	InterpolatorClassList::instance ().add (InterpolatorClass (name, createFunc));
#endif
}

//************************************************************************************************
// LinearInterpolator
//************************************************************************************************

LinearInterpolator::LinearInterpolator (float minRange, float maxRange)
: Interpolator (minRange, maxRange)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

float LinearInterpolator::normalizedToRange (float normalized) const
{ 
	if((maxRange - minRange) <= 0) 
		return 0.f;

	normalized = ((0 > normalized) ? 0 : ((1.f > normalized) ? normalized : 1.f));
	return normalized * (maxRange - minRange) + minRange; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float LinearInterpolator::rangeToNormalized (float value) const
{ 
	if((maxRange - minRange) <= 0) 
		return 0.f;

	value = ((minRange > value) ? minRange : ((maxRange > value) ? value : maxRange));
	return (value - minRange) / (maxRange - minRange); 
}

//************************************************************************************************
// LinearReverseInterpolator
//************************************************************************************************

LinearReverseInterpolator::LinearReverseInterpolator (float minRange, float maxRange)
: Interpolator (minRange, maxRange)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

float LinearReverseInterpolator::normalizedToRange (float normalized) const
{ 
	if((maxRange - minRange) <= 0) 
		return 0.f;

	normalized = ((0 > normalized) ? 0 : ((1.f > normalized) ? normalized : 1.f));
	return maxRange - normalized * (maxRange - minRange); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float LinearReverseInterpolator::rangeToNormalized (float value) const
{ 
	if((maxRange - minRange) <= 0) 
		return 0.f;

	value = ((minRange > value) ? minRange : ((maxRange > value) ? value : maxRange));
	return (maxRange - value) / (maxRange - minRange); 
}

//************************************************************************************************
// SegmentInterpolator
//************************************************************************************************

SegmentInterpolator::SegmentInterpolator (int numPoints, const BreakPoint points[])
: Interpolator (points[0].range, points[numPoints - 1].range),
  breakPoints (points),
  numPoints (numPoints)
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SegmentInterpolator::setRange (float minRange, float maxRange, float midRange)
{
	// nothing here, range can't be changed!
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float SegmentInterpolator::normalizedToRange (float normalized) const
{ 
	if((maxRange - minRange) <= 0) 
		return 0.f;

	normalized = ((0 > normalized) ? 0 : ((1.f > normalized) ? normalized : 1.f));
	
	int maxPoint = numPoints - 1;
	for(int i = 0; i < maxPoint; i++)
	{
		if(normalized <= breakPoints[i + 1].normalized)
		{
			return	(normalized - breakPoints[i].normalized) / (breakPoints[i + 1].normalized - breakPoints[i].normalized) * 
					(breakPoints[i + 1].range - breakPoints[i].range) + breakPoints[i].range; 
		}
	}

	return maxRange; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float SegmentInterpolator::rangeToNormalized (float value) const
{ 
	if((maxRange - minRange) <= 0) 
		return 0.f;

	value = ((minRange > value) ? minRange : ((maxRange > value) ? value : maxRange));

	int maxPoint = numPoints - 1;
	for(int i = 0; i < maxPoint; i++)
	{
		if(value <= breakPoints[i + 1].range)
		{
			return	(value - breakPoints[i].range) / (breakPoints[i + 1].range - breakPoints[i].range)  *
					(breakPoints[i + 1].normalized - breakPoints[i].normalized) + breakPoints[i].normalized;
		}
	}

	return 1.f; 
}

//************************************************************************************************
// ZoomInterpolator
//************************************************************************************************

ZoomInterpolator::ZoomInterpolator (float minRange, float maxRange, float midRange)
: Interpolator (minRange, maxRange, midRange),
  midRangeInv (1.f / midRange)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ZoomInterpolator::setRange (float minRange, float maxRange, float midRange)
{
	Interpolator::setRange (minRange, maxRange, midRange);
	midRangeInv = 1.f / midRange;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float ZoomInterpolator::normalizedToRange (float normalized) const
{ 
	if((maxRange - minRange) <= 0) 
		return 0.f;

	normalized = ((0 > normalized) ? 0 : ((1.f > normalized) ? normalized : 1.f));

	return powf (normalized, midRangeInv) * (maxRange - minRange) + minRange;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float ZoomInterpolator::rangeToNormalized (float value) const
{ 
	if((maxRange - minRange) <= 0) 
		return 0.f;

	value = ((minRange > value) ? minRange : ((maxRange > value) ? value : maxRange));

	return powf ((value - minRange) / (maxRange - minRange), midRange);
}
