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
// Filename    : ccl/base/performance.cpp
// Description : Performance Measurement
//
//************************************************************************************************

#include "ccl/base/performance.h"

#include "ccl/public/base/variant.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/formatter.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//************************************************************************************************
// PerformanceMeter::ExtraDurationGuard
//************************************************************************************************

PerformanceMeter::ExtraDurationGuard::ExtraDurationGuard (PerformanceMeter* meter)
: meter (meter),
  startTime (0.)
{
	if(meter)
		startTime = System::GetProfileTime ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PerformanceMeter::ExtraDurationGuard::~ExtraDurationGuard ()
{
	stop ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

double PerformanceMeter::ExtraDurationGuard::stop ()
{
	if(meter)
	{
		double endTime = System::GetProfileTime ();
		double duration = endTime - startTime;
		meter->addExtraDuration (duration);
		return duration;
		meter = nullptr;
	}
	return 0;
}


//************************************************************************************************
// PerformanceMeter
//************************************************************************************************

static const double kExtraDurationToIntScaler = 1000000.0;

//////////////////////////////////////////////////////////////////////////////////////////////////

PerformanceMeter::PerformanceMeter ()
: startTime (0.),
  extraDurationInt (0),
  minFreq (1.),
  performance (0.),
  overAccumulated (0.),
  decay (0.),
  overLoad (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PerformanceMeter::setMaxPeriod (double seconds)
{
	minFreq = 1. / seconds;
	decay = seconds * 5;

	reset (); // reset meter
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PerformanceMeter::beginPeriod ()
{
	startTime = System::GetProfileTime ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PerformanceMeter::addExtraDuration (double duration)
{
	ASSERT (duration >= 0)

	double durationScaled = duration * kExtraDurationToIntScaler;
	int durationInt = int (durationScaled);
	ASSERT (ccl_abs (durationScaled - double (durationInt)) < 1.0)
	AtomicAddInline (extraDurationInt, durationInt);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PerformanceMeter::endPeriod ()
{
	double extraDuration = extraDurationInt > 0 ? (extraDurationInt / kExtraDurationToIntScaler) : 0.0;
	double endTime = System::GetProfileTime () + extraDuration;
	ASSERT (extraDuration >= 0)
	extraDurationInt = 0; // always reset here

	double p = (endTime - startTime) * minFreq;
	ASSERT (endTime >= startTime)
	ASSERT (p >= 0)
	setPerformance (p);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PerformanceMeter::setPerformance (double p)
{
	double over = p - 1.0;
	if(over > 0.0) 
	{
		overAccumulated += over;		
		if(overAccumulated > 0.75)
			setOverLoad (true);
	}
	else
		overAccumulated = 0.0;

	if(overLoad)
		performance = 1.0;
	else
	{
		if(p < 0.0075)
			p = 0.0;

		double performanceDiff = p - performance;

		if(ccl_abs (performanceDiff) > 0.025 || p < 0.03)
		{
			performanceDiff *= decay;
			performance = ccl_max (0.0, performance + performanceDiff);
		}
		if(performance >= 1.0)
			setOverLoad (true);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

double CCL_API PerformanceMeter::getPerformance ()
{
	return ccl_min (performance, 1.0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PerformanceMeter::reset ()
{
	performance = 0.;
	overLoad = false;
	overAccumulated = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PerformanceMeter::setOverLoad (tbool state)
{
	overLoad = state != 0;
	overAccumulated = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PerformanceMeter::isOverLoad ()
{
	return overLoad;
}

//************************************************************************************************
// ProcessMemoryMeter
//************************************************************************************************

ProcessMemoryMeter::ProcessMemoryMeter ()
: memoryUsed (0.),
  memoryTotal (0.),
  lastUpdateTime (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

double CCL_API ProcessMemoryMeter::getPerformance ()
{
	int64 now = System::GetSystemTicks ();
	if(now - lastUpdateTime > 2000)
	{
		lastUpdateTime = now;

		System::MemoryInfo memoryInfo;
		System::GetSystem ().getMemoryInfo (memoryInfo);

		memoryTotal = (double)memoryInfo.processMemoryTotal;
		memoryUsed = memoryTotal - (double)memoryInfo.processMemoryAvailable;
		performance = memoryUsed / memoryTotal;
		overLoad = performance >= 0.9; // indicate overload >= 90%
	}
	return performance;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ProcessMemoryMeter::getPerformanceValue (Variant& value, IntPtr token)
{
	value = token == kMemoryUsed ? memoryUsed : memoryTotal;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ProcessMemoryMeter::printPerformanceValue (String& string, VariantRef value, IntPtr token)
{
	string = Format::ByteSize::print (value);
}
