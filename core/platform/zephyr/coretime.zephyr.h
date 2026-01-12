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
// Filename    : core/platform/zephyr/coretime.zephyr.h
// Description : Zephyr Timing Functions
//
//************************************************************************************************

#ifndef _coretime_zephyr_h
#define _coretime_zephyr_h

#include "core/platform/shared/coreplatformtime.h"

#include "corezephyr.h"

namespace Core {
namespace Platform {

//************************************************************************************************
// Timing Functions
//************************************************************************************************

inline abs_time SystemClock::getTime ()
{
	return k_uptime_get ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline double SystemClock::getTimeToSecondsFactor ()
{
	return 1. / MSEC_PER_SEC;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline double SystemClock::toSeconds (abs_time t)
{
	return Helper::convertTimeToSeconds (t);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline abs_time SystemClock::toMilliseconds (abs_time t)
{
	return getTime ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline abs_time SystemClock::toMicroseconds (abs_time t)
{
	return getTime () * 1000;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline uint64 SystemClock::getFrequency ()
{
	return MSEC_PER_SEC;
}

//************************************************************************************************
// High Performance Timing Functions
//************************************************************************************************

inline abs_time HighPerformanceClock::getCount ()
{
	return k_cycle_get_64 ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline uint64 HighPerformanceClock::getFrequency ()
{
	return sys_clock_hw_cycles_per_sec ();
}

} // namespace Platform
} // namespace Core

#endif // _coretime_zephyr_h
 
