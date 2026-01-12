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
// Filename    : core/test/corelinkedlisttest.cpp
// Description : Core LinkedList Tests
//
//************************************************************************************************

#include "corelinkedlisttest.h"

#include "core/public/corelinkedlist.h"

using namespace Core;
using namespace Test;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Helpers
//////////////////////////////////////////////////////////////////////////////////////////////////

static DEFINE_CONTAINER_PREDICATE_OBJECT (EvenNumberPredicate, int, i)
	return *i % 2 == 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

struct EvenNumberFunctor
{
	bool operator() (const int& i) const
	{
		return i % 2 == 0;
	};
};

//************************************************************************************************
// LinkedListTest
//************************************************************************************************

CORE_REGISTER_TEST (LinkedListTest)

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr LinkedListTest::getName () const
{
	return "Core LinkedList";
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LinkedListTest::run (ITestContext& testContext)
{
	// Functor based API.
	{
		LinkedList<int> list; // CUT - class under test
		if(!list.isEmpty ())
			CORE_TEST_FAILED ("List not empty.")

		list.append (1);
		list.append (2);
		list.append (3);
		list.append (4);

		if(list.count () != 4)
			CORE_TEST_FAILED ("List test data not added.")

		/*
		 * Test: findIf ()
		 */
		const int& item = list.findIf (EvenNumberFunctor ());
		if(item != 2)
			CORE_TEST_FAILED ("Could not lookup test item")

		/*
		 * Test: removeIf ()
		 */
		int removedCount = list.removeIf (EvenNumberFunctor ());
		if(removedCount != 2)
			CORE_TEST_FAILED ("Failed to remove elements")
		if(list.count () != 2)
			CORE_TEST_FAILED ("Failed to remove elements")
		if(!list.contains (1))
			CORE_TEST_FAILED ("Removed wrong element")
		if(!list.contains (3))
			CORE_TEST_FAILED ("Removed wrong element")
	}

	// Predicate function based API.
	{
		LinkedList<int> list; // CUT - class under test
		if(!list.isEmpty ())
			CORE_TEST_FAILED ("List not empty.")

		list.append (1);
		list.append (2);
		list.append (3);
		list.append (4);

		if(list.count () != 4)
			CORE_TEST_FAILED ("List test data not added.")

		/*
		 * Test: findIf ()
		 */
		const int& item = list.findIf (EvenNumberPredicate);
		if(item != 2)
			CORE_TEST_FAILED ("Could not lookup test item")

		/*
		 * Test: removeIf ()
		 */
		int removedCount = list.removeIf (EvenNumberPredicate);
		if(removedCount != 2)
			CORE_TEST_FAILED ("Failed to remove elements")
		if(list.count () != 2)
			CORE_TEST_FAILED ("Failed to remove elements")
		if(!list.contains (1))
			CORE_TEST_FAILED ("Removed wrong element")
		if(!list.contains (3))
			CORE_TEST_FAILED ("Removed wrong element")
	}

	return true;
}
