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
// Filename    : core/test/coreatomictest.cpp
// Description : Core Atomic Tests
//
//************************************************************************************************

#include "coreatomictest.h"

#include "core/system/coreatomic.h"
#include "core/system/corethread.h"

namespace Core {
namespace Test {

//************************************************************************************************
// AtomicTestThread
//************************************************************************************************

class AtomicTestThread: public Threads::Thread
{
public:
	AtomicTestThread (int32& value)
	: Thread ("Atomic Test Thread"),
	  value (value)
	{}

	// Thread
	int threadEntry () override
	{
		for(int i = 0; i < 100000; ++i)
			AtomicAdd (value, -6);
		return true;
	}

protected:
	int32& value;
};

} // namespace Test
} // namespace Core

using namespace Core;
using namespace Test;

//************************************************************************************************
// AtomicTest
//************************************************************************************************

CORE_REGISTER_TEST (AtomicTest)

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr AtomicTest::getName () const
{
	return "Core Atomic";
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AtomicTest::run (ITestContext& testContext)
{
	bool succeeded = true;

    static int32 value;
	value = 1;

	int32 oldValue = AtomicAdd (value, 3);
	if(value != 4)
	{
		CORE_TEST_FAILED ("AtomicAdd does not add correctly.")
		succeeded = false;
	}
	if(oldValue != 1)
	{
		CORE_TEST_FAILED ("Value returned by AtomicAdd is not the original value.")
		succeeded = false;
	}

	AtomicTestThread* testThread = NEW AtomicTestThread (value);
	testThread->start ();

	for(int i = 0; i < 300003; ++i)
		AtomicAdd (value, 2);

	testThread->join (100);
	delete testThread;
	
	if(value != 10)
	{
		CORE_TEST_FAILED ("Parallel calls to AtomicAdd did not lead to the correct result.")
		succeeded = false;
	}
	
	return succeeded;
}
