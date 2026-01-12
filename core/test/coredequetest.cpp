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
// Filename    : core/test/coredequetest.cpp
// Description : Core Deque Tests
//
//************************************************************************************************

#include "coredequetest.h"

#include "core/public/coredeque.h"

using namespace Core;
using namespace Test;

//************************************************************************************************
// DequeTest
//************************************************************************************************

CORE_REGISTER_TEST (DequeTest)

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr DequeTest::getName () const
{
	return "Core Deque";
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DequeTest::run (ITestContext& testContext)
{
	Deque<int> deq; // CUT - class under test

	// Test constructor
	if(!deq.isEmpty ())
	{
		CORE_TEST_FAILED ("Deque not empty.")
	}
	if(deq.count () != 0)
	{
		CORE_TEST_FAILED ("Deque not empty.")
	}

	// Test addFront (), count ()
	deq.addFront (2);
	if(deq.count () != 1 || deq.peekFront () != 2)
	{
		CORE_TEST_FAILED ("Deque::addFront () failed.")
	}
	deq.addFront (1);
	if(deq.count () != 2 || deq.peekFront () != 1)
	{
		CORE_TEST_FAILED ("Deque::addFront () failed.")
	}

	// Test popFront (), count ()
	int result = deq.popFront ();
	if(result != 1 || deq.count () != 1)
	{
		CORE_TEST_FAILED ("Deque::popFront () failed.")
	}
	result = deq.popFront ();
	if(result != 2 || deq.count () != 0)
	{
		CORE_TEST_FAILED ("Deque::popFront () failed.")
	}

	// Test addBack (), count ()
	deq.addBack (1);
	if(deq.count () != 1 || deq.peekBack () != 1)
	{
		CORE_TEST_FAILED ("Deque::addBack () failed.")
	}
	deq.addBack (2);
	if(deq.count () != 2 || deq.peekBack () != 2)
	{
		CORE_TEST_FAILED ("Deque::addBack () failed.")
	}

	// Test popBack (), count ()
	result = deq.popBack ();
	if(result != 2 || deq.count () != 1)
	{
		CORE_TEST_FAILED ("Deque::popBack() failed.")
	}
	result = deq.popBack ();
	if(result != 1 || deq.count () != 0)
	{
		CORE_TEST_FAILED ("Deque::popBack() failed.")
	}

	return true;
}


//************************************************************************************************
// FixedDequeTest
//************************************************************************************************

CORE_REGISTER_TEST (FixedDequeTest)

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr FixedDequeTest::getName () const
{
	return "Core FixedDeque";
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FixedDequeTest::run (ITestContext& testContext)
{

	FixedDeque<int> deq; // CUT - class under test

	// Test constructor
	if(!deq.isEmpty ())
	{
		CORE_TEST_FAILED ("FixedDeque not empty.")
	}
	if(deq.count () != 0)
	{
		CORE_TEST_FAILED ("FixedDeque not empty.")
	}
	if(deq.getCapacity () != 0)
	{
		CORE_TEST_FAILED ("FixedDeque wrong capacity.")
	}

	// Use pre-allocated buffer.
	const int slots = 4;
	int buffer[slots] = { 0, 0, 0, 0 };

	// Initialize
	int memorySize = sizeof(int) * slots;
	deq.initialize (buffer, memorySize);
	if(!deq.isEmpty ())
	{
		CORE_TEST_FAILED ("FixedDeque empty.")
	}
	if(deq.count () != 0)
	{
		CORE_TEST_FAILED ("FixedDeque wrong count.")
	}
	if(deq.getCapacity () != 4)
	{
		CORE_TEST_FAILED ("FixedDeque wrong capacity.")
	}

	bool success = false;
	int result = -1;

	// Test addFront (), count ()
	success = deq.addFront (2);
	if(!success || deq.count () != 1)
	{
		CORE_TEST_FAILED ("FixedDeque::addFront () failed.")
	}
	success = deq.addFront (1);
	if(!success || deq.count () != 2)
	{
		CORE_TEST_FAILED ("FixedDeque::addFront () failed.")
	}
	success = deq.peekFront (result);
	if(!success || result != 1)
	{
		CORE_TEST_FAILED ("FixedDeque::peekFront () failed.")
	}

	// Test popFront (), count ()
	success = deq.popFront (result);
	if(!success || result != 1 || deq.count () != 1)
	{
		CORE_TEST_FAILED ("FixedDeque::popFront () failed.")
	}
	success = deq.popFront (result);
	if(!success || result != 2 || deq.count () != 0)
	{
		CORE_TEST_FAILED ("FixedDeque::popFront () failed.")
	}

	// Test addBack (), count ()
	success = deq.addBack (1);
	if(!success || deq.count () != 1)
	{
		CORE_TEST_FAILED ("FixedDeque::addBack () failed.")
	}
	success = deq.addBack (2);
	if(!success || deq.count () != 2)
	{
		CORE_TEST_FAILED ("FixedDeque::addBack () failed.")
	}
	success = deq.peekBack (result);
	if(!success || result != 2)
	{
		CORE_TEST_FAILED ("FixedDeque::peekBack () failed.")
	}

	// Test popBack (), count ()
	success = deq.popBack (result);
	if(!success || result != 2 || deq.count () != 1)
	{
		CORE_TEST_FAILED ("FixedDeque::popBack() failed.")
	}
	success = deq.popBack (result);
	if(!success || result != 1 || deq.count () != 0)
	{
		CORE_TEST_FAILED ("FixedDeque::popBack() failed.")
	}

	// Limit breach, fill up container to capacity.
	for(int i = deq.count(); i < deq.getCapacity (); i++)
		deq.addFront (i);

	if(deq.count () != deq.getCapacity ())
	{
		CORE_TEST_FAILED ("FixedDeque capacity not reached")
	}
	success = deq.addBack (1);
	if(success)
	{
		CORE_TEST_FAILED ("FixedDeque::addBack() did not reject limit exceeding element.")
	}
	success = deq.addFront (1);
	if(success)
	{
		CORE_TEST_FAILED ("FixedDeque::addFront() did not reject limit exceeding element.")
	}

	return true;
}
