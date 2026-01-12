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
// Filename    : core/platform/shared/coreplatformtime.h
// Description : Timing Functions platform implementation base
//
//************************************************************************************************

#ifndef _coreplatformtime_h
#define _coreplatformtime_h

#include "core/public/coretypes.h"

namespace Core {
namespace Platform {

//************************************************************************************************
// Timing Functions
//************************************************************************************************

namespace SystemClock
{
	abs_time getTime ();
	uint64 getFrequency ();
	double getTimeToSecondsFactor ();

	double toSeconds (abs_time t);
	abs_time toMilliseconds (abs_time t);
	abs_time toMicroseconds (abs_time t);

	namespace Helper
	{
		double convertTimeToSeconds (abs_time t);
		abs_time convertTimeToMilliseconds (abs_time t);
		abs_time convertTimeToMicroseconds (abs_time t);
	}
}

//************************************************************************************************
// High Performance Timing Functions (microsecond precision or better)
//************************************************************************************************

namespace HighPerformanceClock
{
	abs_time getCount ();
	uint64 getFrequency ();
}

//************************************************************************************************
// Timing Functions implementation
//************************************************************************************************

inline abs_time SystemClock::Helper::convertTimeToMilliseconds (abs_time t)
{
	return static_cast<abs_time> (convertTimeToSeconds (t) * 1000.);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline abs_time SystemClock::Helper::convertTimeToMicroseconds (abs_time t)
{
	return static_cast<abs_time> (convertTimeToSeconds (t) * 1000000.);
}

} // namespace Platform
} // namespace Core

#endif // _coreplatformtime_h
