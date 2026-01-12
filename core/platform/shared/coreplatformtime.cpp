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
// Filename    : core/platform/shared/coreplatformtime.cpp
// Description : Timing Functions platform implementation base
//
//************************************************************************************************

#include "core/platform/corefeatures.h"

#if CORE_TIME_IMPLEMENTATION == CORE_PLATFORM_IMPLEMENTATION
#include CORE_PLATFORM_IMPLEMENTATION_HEADER (coretime)
#elif CORE_TIME_IMPLEMENTATION == CORE_FEATURE_IMPLEMENTATION_POSIX
#include "core/platform/shared/posix/coretime.posix.h"
#elif CORE_TIME_IMPLEMENTATION == CORE_FEATURE_IMPLEMENTATION_CMSIS
#include "core/platform/shared/cmsis/coretime.cmsis.h"
#endif

using namespace Core;
using namespace Platform;

//************************************************************************************************
// Timing Functions
//************************************************************************************************

double SystemClock::Helper::convertTimeToSeconds (abs_time t)
{
	static double factor = getTimeToSecondsFactor ();
	return static_cast<double> (t * factor);
}
