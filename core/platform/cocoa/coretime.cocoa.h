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
// Filename    : core/platform/cocoa/coretime.cocoa.h
// Description : Cocoa Timing Functions
//
//************************************************************************************************

#ifndef _coreplatformtime_cocoa_h
#define _coreplatformtime_cocoa_h

#include "core/platform/shared/coreplatformtime.h"

#include <mach/mach_time.h>

namespace Core {
namespace Platform {

//************************************************************************************************
// Timing Functions
//************************************************************************************************

inline abs_time SystemClock::getTime ()
{
	return mach_absolute_time ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline uint64 SystemClock::getFrequency ()
{
	return static_cast<uint64> (1. / getTimeToSecondsFactor ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline double SystemClock::getTimeToSecondsFactor ()
{
	mach_timebase_info_data_t machClockInfo;
	mach_timebase_info (&machClockInfo);
	double factor = (double)machClockInfo.numer / (double)machClockInfo.denom;
	factor *= 1e-9;
	return factor;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline double SystemClock::toSeconds (abs_time t)
{
    return Helper::convertTimeToSeconds (t);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline abs_time SystemClock::toMilliseconds (abs_time t)
{
    return Helper::convertTimeToMilliseconds (t);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline abs_time SystemClock::toMicroseconds (abs_time t)
{
    return Helper::convertTimeToMicroseconds (t);
}

//************************************************************************************************
// High Performance Timing Functions
//************************************************************************************************

inline abs_time HighPerformanceClock::getCount ()
{
	return SystemClock::getTime ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline uint64 HighPerformanceClock::getFrequency ()
{
	return SystemClock::getFrequency ();
}

} // namespace Platform
} // namespace Core

#endif // _coretime_cocoa_h
