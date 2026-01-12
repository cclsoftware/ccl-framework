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
// Filename    : core/platform/ti32/coretime.dspti32.h
// Description : TI32 DSP (OMAP-L138) Timing Functions
//
//************************************************************************************************

#ifndef _coretime_dspti32_h
#define _coretime_dspti32_h

#include "core/platform/shared/coreplatformtime.h"

extern "C" { Core::uint32 CLK_gethtime ();  }

namespace Core {
namespace Platform {

//************************************************************************************************
// Timing Functions
//************************************************************************************************

inline abs_time SystemClock::getTime ()
{
    return CLK_gethtime ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline double SystemClock::getTimeToSecondsFactor ()
{
	return 1. / getFrequency ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline double SystemClock::toSeconds (abs_time t)
{
    return Helper::convertTimeToSeconds (t);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline abs_time SystemClock::toMilliseconds (abs_time t)
{
    return 1000 * t / getFrequency ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline abs_time SystemClock::toMicroseconds (abs_time t)
{
    return 1000 * 1000 * t / getFrequency ();
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

#endif // _coretime_dspti32_h
