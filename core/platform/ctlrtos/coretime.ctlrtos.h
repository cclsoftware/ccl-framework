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
// Filename    : core/platform/ctlrtos/coretime.ctlrtos.h
// Description : Crossworks Tasking Library Timing Functions
//
//************************************************************************************************

#ifndef _coretime_ctlrtos_h
#define _coretime_ctlrtos_h

#include "core/platform/shared/coreplatformtime.h"

#include <ctl_api.h>

namespace Core {
namespace Platform {

//************************************************************************************************
// Timing Functions
//************************************************************************************************

inline abs_time SystemClock::getTime ()
{
	return ctl_get_current_time ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline uint64 SystemClock::getFrequency ()
{
	return 1000;   // system time is in ms
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline double SystemClock::getTimeToSecondsFactor ()
{
	return 1.0 / 1000;   // system time is in ms
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline double SystemClock::toSeconds (abs_time t)
{
    return Helper::convertTimeToSeconds (t);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline abs_time SystemClock::toMilliseconds (abs_time t)
{
	return t;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline abs_time SystemClock::toMicroseconds (abs_time t)
{
	return t * 1000;
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

#endif // _coretime_ctlrtos_h
