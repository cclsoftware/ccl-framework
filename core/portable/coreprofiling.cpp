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
// Filename    : core/portable/coreprofiling.cpp
// Description : Profiling Utilities
//
//************************************************************************************************

#include "core/portable/coreprofiling.h"

using namespace Core;
using namespace Portable;
using namespace Threads;

//************************************************************************************************
// ProfilingData
//************************************************************************************************

ProfilingData::ProfilingData ()
: numberOfCounters (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ProfilingData::getNumberOfCounters () const
{
	return numberOfCounters;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ProfilingData::addCounter (CStringPtr label)
{
	int index = numberOfCounters;
	if(index < kMaxProfilingCounters)
	{
		numberOfCounters++;
		setCounterLabel (index, label);
		return index;
	}
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ProfilingData::setCounterLabel (int counter, CStringPtr label)
{
	if(counter >= 0 && counter < numberOfCounters)
		counters[counter].label = label;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr ProfilingData::getCounterLabel (int counter) const
{
	if(counter >= 0 && counter < numberOfCounters)
		return counters[counter].label;
	return "";
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ProfilingData::setField (int counter, Key key, uint32 value)
{
	if(counter < 0 || counter >= numberOfCounters)
		return false;
		
	switch(key)
	{
	case kCPUUsage:
		counters[counter].cpuUsage = value;
		return true;
		
	case kAvgInterval:
		counters[counter].avgInterval = value;
		return true;
		
	case IProfilingData::kMinInterval:
		counters[counter].minInterval = value;
		return true;
		
	case IProfilingData::kMaxInterval:
		counters[counter].maxInterval = value;
		return true;
	
	default:
		return false;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ProfilingData::getField (uint32& value, int counter, Key key) const
{
	if(counter < 0 || counter >= numberOfCounters)
		return false;
		
	switch(key)
	{
	case kCPUUsage:
		value = counters[counter].cpuUsage;
		return true;
		
	case kAvgInterval:
		value = counters[counter].avgInterval;
		return true;
		
	case kMinInterval:
		value = counters[counter].minInterval;
		return true;
		
	case kMaxInterval:
		value = counters[counter].maxInterval;
		return true;
		
	default:
		return false;
	}
}

//************************************************************************************************
// LockFreePerformanceProfiler
//************************************************************************************************

LockFreePerformanceProfiler::LockFreePerformanceProfiler ()
: numberOfCounters (0),
  startTime (0)
{
	reset ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LockFreePerformanceProfiler::setup (int numberOfCounters)
{
	ASSERT (numberOfCounters <= kMaxProfilingCounters)
	this->numberOfCounters = numberOfCounters;
	reset ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LockFreePerformanceProfiler::reset ()
{
	::memset (counters, 0, numberOfCounters * sizeof(Counter));
	for(int i = 0; i < numberOfCounters; ++i)
		counters[i].minCount = ~0;
	startTime = HighPerformanceClock::getCount ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LockFreePerformanceProfiler::beginInterval (int counter)
{
	if(counter >= 0 && counter < kMaxProfilingCounters)
		counters[counter].startTime = HighPerformanceClock::getCount ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LockFreePerformanceProfiler::endInterval (int counter)
{
	if(counter < 0 || counter >= kMaxProfilingCounters)
		return;
		
	abs_time now = HighPerformanceClock::getCount ();

	uint64 count = now - counters[counter].startTime;
	if(count < counters[counter].minCount)
		counters[counter].minCount = count;
	if(count > counters[counter].maxCount)
		counters[counter].maxCount = count;
	counters[counter].totalCount += count;
	counters[counter].iterationCount++;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LockFreePerformanceProfiler::getProfilingData (IProfilingData& data) const
{
	abs_time now = HighPerformanceClock::getCount ();
	uint64 frequency = HighPerformanceClock::getFrequency ();
	uint64 totalCount = now - startTime;

	for(int i = 0; i < numberOfCounters && i < data.getNumberOfCounters (); i++)
	{
		uint32 minInterval = static_cast<uint32> (1000 * 1000 * counters[i].minCount / frequency);
		data.setField (i, IProfilingData::kMinInterval, minInterval);
		
		uint32 maxInterval = static_cast<uint32> (1000 * 1000 * counters[i].maxCount / frequency);
		data.setField (i, IProfilingData::kMaxInterval, maxInterval);
		
		uint64 avgTicks = counters[i].totalCount / counters[i].iterationCount;
		uint32 avgInterval = static_cast<uint32> (1000 * 1000 * avgTicks / frequency);
		data.setField (i, IProfilingData::kAvgInterval, avgInterval);

		uint32 cpuUsage = static_cast<uint32> (100 * counters[i].totalCount / totalCount);
		data.setField (i, IProfilingData::kCPUUsage, cpuUsage);
	}
}

//************************************************************************************************
// PerformanceProfiler
//************************************************************************************************

void PerformanceProfiler::endInterval (int counter)
{
	ScopedLock lock (mutex);
	LockFreePerformanceProfiler::endInterval (counter);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PerformanceProfiler::getProfilingData (IProfilingData& data) const
{
	ScopedLock lock (mutex);
	LockFreePerformanceProfiler::getProfilingData (data);
}
