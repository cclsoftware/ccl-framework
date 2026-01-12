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
// Filename    : core/platform/lk/corethread.lk.cpp
// Description : Little Kernel Thread Primitives
//
//************************************************************************************************

#include "coretime.lk.h"

// Little Kernel
#include "platform.h"

using namespace Core;
using namespace Platform;

//************************************************************************************************
// SystemClock
//************************************************************************************************

abs_time SystemClock::getTime ()
{
	 return current_time (); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uint64 SystemClock::getFrequency ()
{
	return 1000;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

double SystemClock::getTimeToSecondsFactor ()
{
	return 1000;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

double SystemClock::toSeconds (abs_time t)
{
	return t / getTimeToSecondsFactor ();
};

//////////////////////////////////////////////////////////////////////////////////////////////////

abs_time SystemClock::toMilliseconds (abs_time t)
{
	return t;
}; 

//////////////////////////////////////////////////////////////////////////////////////////////////

abs_time SystemClock::toMicroseconds (abs_time t)
{
	return t * 1000;
};

//************************************************************************************************
// HighPerformanceClock
//************************************************************************************************

abs_time HighPerformanceClock::getCount ()
{
	return current_time_hires ();
};

//////////////////////////////////////////////////////////////////////////////////////////////////

uint64 HighPerformanceClock::getFrequency ()
{
	return 1000000;
};
