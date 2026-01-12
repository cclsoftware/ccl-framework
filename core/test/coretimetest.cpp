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
// Filename    : core/test/coretimetest.cpp
// Description : Core Time Tests
//
//************************************************************************************************

#include "coretimetest.h"

#include "core/system/coretime.h"
#include "core/system/corethread.h"

using namespace Core;
using namespace Test;

//************************************************************************************************
// TimeTest
//************************************************************************************************

CORE_REGISTER_TEST (TimeTest)

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr TimeTest::getName () const
{
	return "Core Time";
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TimeTest::run (ITestContext& testContext)
{
	abs_time firstTime = SystemClock::getTime ();

	double seconds = SystemClock::toSeconds (firstTime);
	if(seconds < 0)
	{
		CORE_TEST_FAILED ("System clock time is negative.")
		return false;
	}

	abs_time lastTime = SystemClock::getTime ();
	if(firstTime > lastTime)
	{
		CORE_TEST_FAILED ("System clock is not steady.")
		return false;
	}

	Threads::CurrentThread::sleep (2000);
	double secondsAfterWaiting = SystemClock::getSeconds ();
	if(secondsAfterWaiting <= seconds)
	{
		char message[STRING_STACK_SPACE_MAX];
		snprintf (message, STRING_STACK_SPACE_MAX, "Time before waiting for two seconds: %f", seconds);
		CORE_TEST_MESSAGE (message)
		snprintf (message, STRING_STACK_SPACE_MAX, "Time after waiting for two seconds: %f", secondsAfterWaiting);
		CORE_TEST_MESSAGE (message)
		CORE_TEST_FAILED ("SystemClock::getSeconds did not increment after waiting for two seconds.")
		return false;
	}

	return true;
}
