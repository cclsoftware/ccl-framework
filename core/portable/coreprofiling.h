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
// Filename    : core/portable/coreprofiling.h
// Description : Profiling Utilities
//
//************************************************************************************************

#ifndef _coreprofiling_h
#define _coreprofiling_h

#include "core/public/coreprofiler.h"

#include "core/system/corethread.h"
#include "core/system/coretime.h"
#include "core/system/coredebug.h"

namespace Core {
namespace Portable {

const int kMaxProfilingCounters = 16;

//************************************************************************************************
// Simple profiling macros for measuring execution times of single intervals
// CORE_PROFILE_XXXX macros can be enabled by defining CORE_PROFILE as 1
//************************************************************************************************

#if !defined (CORE_PROFILE)
#define CORE_PROFILE 0
#endif

#if CORE_PROFILE

#define CORE_PROFILE_START(id) uint64 _startCount_##id = HighPerformanceClock::getCount ();
#define CORE_PROFILE_STOP(id, label) uint64 _delta_##id = 1000 * 1000 * (HighPerformanceClock::getCount () - _startCount_##id) / HighPerformanceClock::getFrequency (); DebugPrintf ("%s %d �s\n", label, _delta_##id);

#else

#define CORE_PROFILE_START(id)
#define CORE_PROFILE_STOP(id, label)

#endif

//************************************************************************************************
// ProfilingData
//************************************************************************************************

class ProfilingData: public IProfilingData
{
public:
	ProfilingData ();

	// IProfilingData
	int getNumberOfCounters () const override;
	int addCounter (CStringPtr label) override;
	void setCounterLabel (int counter, CStringPtr label) override;
	CStringPtr getCounterLabel (int counter) const override;
	bool setField (int counter, Key key, uint32 value) override;
	bool getField (uint32& value, int counter, Key key) const override;

private:
	static const int kMaxLabelLength = 16;

	struct Counter
	{
		CStringBuffer<kMaxLabelLength> label;
		uint32 cpuUsage;
		uint32 avgInterval;
		uint32 minInterval;
		uint32 maxInterval;

		Counter ()
		: cpuUsage (0),
		  avgInterval (0),
		  minInterval (0),
		  maxInterval (0)
		{}
	};

	int numberOfCounters;
	Counter counters[kMaxProfilingCounters];
};

//************************************************************************************************
// LockFreePerformanceProfiler
/** Not thread-safe! */
//************************************************************************************************

class LockFreePerformanceProfiler: public IProfiler
{
public:
	LockFreePerformanceProfiler ();

	// IProfiler
	void setup (int numberOfCounters) override;
	void reset () override;
	void beginInterval (int counter) override;
	void endInterval (int counter) override;
	void getProfilingData (IProfilingData& data) const override;

protected:
	struct Counter
	{
		abs_time startTime;
		uint64 totalCount;
		uint64 minCount;
		uint64 maxCount;
		uint64 iterationCount;
	};

	int numberOfCounters;
	Counter counters[kMaxProfilingCounters];
	mutable Threads::Lock mutex;
	
	abs_time startTime;
};

//************************************************************************************************
// PerformanceProfiler
/** Thread-safe performance profiler. */
//************************************************************************************************

class PerformanceProfiler: public LockFreePerformanceProfiler
{
public:
	// LockFreePerformanceProfiler
	void endInterval (int counter) override;
	void getProfilingData (IProfilingData& data) const override;

private:
	mutable Threads::Lock mutex;
};

} // namespace Portable
} // namespace Core

#endif // _coreprofiling_h
