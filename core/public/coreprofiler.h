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
// Filename    : core/public/coreprofiler.h
// Description : Profiling Interfaces
//
//************************************************************************************************

#ifndef _coreprofiler_h
#define _coreprofiler_h

#include "core/public/coretypes.h"

namespace Core {

//************************************************************************************************
// IProfilingData
/** Interface to store profiling data. */
//************************************************************************************************

struct IProfilingData
{
	enum Key
	{
		kCPUUsage,		//< CPU usage in percent
		kMinInterval,	//< Minimum interval in microseconds
		kMaxInterval,	//< Maximum interval in microseconds
		kAvgInterval	//< Average interval in microseconds
	};

	/** Get number of profiling counters. */
	virtual int getNumberOfCounters () const = 0;

	/** Add profiling counter with label. */
	virtual int addCounter (CStringPtr label) = 0;
	
	/** Change profiling counter label. */
	virtual void setCounterLabel (int counter, CStringPtr label) = 0;
	
	/** Get profiling counter label. */
	virtual CStringPtr getCounterLabel (int counter) const = 0;
	
	/** Set value for counter, see @Key enum (CPU usage, microsecond intervals). */
	virtual bool setField (int counter, Key key, uint32 value) = 0;
	
	/** Get value from counter, see @Key enum (CPU usage, microsecond intervals). */
	virtual bool getField (uint32& value, int counter, Key key) const = 0;
};

//************************************************************************************************
// IProfiler
/** Interface implemented by profiler. */
//************************************************************************************************

struct IProfiler
{
	/** Set up given number of counters. */
	virtual void setup (int numberOfCounters) = 0;

	/** Reset profiling state. */
	virtual void reset () = 0;

	/** Begin time interval for given counter. */
	virtual void beginInterval (int counter) = 0;

	/** End time interval for given counter. */
	virtual void endInterval (int counter) = 0;

	/** Get snapshot of profiling data. */
	virtual void getProfilingData (IProfilingData& data) const = 0;
};

//************************************************************************************************
// TimedInterval
/** Guard to profile time intervals. */
//************************************************************************************************

template<bool enabled>
class TimedInterval
{
public:
	TimedInterval (IProfiler* profiler, int counter)
	{}
};
	
template<>
class TimedInterval<true>
{
public:
	TimedInterval (IProfiler* profiler, int counter)
	: profiler (profiler),
	  counter (counter)
	{
		profiler->beginInterval (counter);
	}
	
	~TimedInterval ()
	{
		profiler->endInterval (counter);
	}

private:
	IProfiler* profiler;
	int counter;
};

} // namespace Core

#endif // _coreprofiler_h
