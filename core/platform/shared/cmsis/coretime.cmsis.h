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
// Filename    : core/platform/shared/cmsis/coretime.cmsis.h
// Description : CMSIS Timing Functions
//
//************************************************************************************************

#ifndef _coretime_cmsis_h
#define _coretime_cmsis_h

#include "core/platform/corefeatures.h"
#include "core/platform/shared/coreplatformtime.h"

#include <cmsis_os2.h>

namespace Core {
namespace Platform {

static const long kSec2Micro = 1000000L;
static const long kSec2Milli = 1000L;

//************************************************************************************************
// Timing Functions
//************************************************************************************************

inline abs_time SystemClock::getTime ()
{
	return osKernelGetTickCount ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline uint64 SystemClock::getFrequency ()
{
    return osKernelGetTickFreq ();
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
	return t * kSec2Milli / osKernelGetTickFreq ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline abs_time SystemClock::toMicroseconds (abs_time t)
{
	return t * kSec2Micro / osKernelGetTickFreq ();
}

} // namespace Platform
} // namespace Core

#endif // _coretime_cmsis_h
