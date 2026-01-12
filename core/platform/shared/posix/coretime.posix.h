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
// Filename    : core/platform/shared/posix/coretime.posix.h
// Description : POSIX Timing Functions
//
//************************************************************************************************

#ifndef _coretime_posix_h
#define _coretime_posix_h

#include "core/platform/shared/coreplatformtime.h"

#include <time.h>

namespace Core {
namespace Platform {

static const long kSec2Nano = 1000000000L;

//************************************************************************************************
// Timing Functions
//************************************************************************************************

inline abs_time SystemClock::getTime ()
{
	timespec now;
	clock_gettime (CLOCK_MONOTONIC, &now);
	return static_cast<int64> (now.tv_sec) * kSec2Nano + now.tv_nsec;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline uint64 SystemClock::getFrequency ()
{
	return kSec2Nano;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline double SystemClock::getTimeToSecondsFactor ()
{
    return 1. / static_cast<double> (kSec2Nano);
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

#endif // _coretime_posix_h
