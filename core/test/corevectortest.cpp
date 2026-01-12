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
// Filename    : core/test/corevectortest.cpp
// Description : Core Vector Tests
//
//************************************************************************************************

#include "corevectortest.h"

#include "core/public/corevector.h"

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
// VectorTest
//************************************************************************************************

CORE_REGISTER_TEST (VectorTest)

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr VectorTest::getName () const
{
	return "Core Vector";
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool VectorTest::run (ITestContext& testContext)
{
	// Functor based API.
	{
		Vector<int> vector; // CUT - class under test
		if(!vector.isEmpty ())
			CORE_TEST_FAILED ("Vector not empty.")

		vector.add (1);
		vector.add (2);
		vector.add (3);
		vector.add (4);

		if(vector.count () != 4)
		CORE_TEST_FAILED ("Vector test data not added.")

		/*
		 * Test: findIf ()
		 */
		int* item = vector.findIf (EvenNumberFunctor ());
		if(item == nullptr)
			CORE_TEST_FAILED ("Could not lookup test item")
		if(*item != 2)
			CORE_TEST_FAILED ("Retrieved wrong item")

		/*
		 * Test: removeIf ()
		 */
		int removedCount = vector.removeIf (EvenNumberFunctor ());
		if(removedCount != 2)
			CORE_TEST_FAILED ("Failed to remove elements")
		if(vector.count () != 2)
			CORE_TEST_FAILED ("Failed to remove elements")
		if(!vector.contains (1))
			CORE_TEST_FAILED ("Removed wrong element")
		if(!vector.contains (3))
			CORE_TEST_FAILED ("Removed wrong element")
	}

	// Predicate function based API
	{
		Vector<int> vector; // CUT - class under test
		if(!vector.isEmpty ())
		CORE_TEST_FAILED ("Vector not empty.")

		vector.add (1);
		vector.add (2);
		vector.add (3);
		vector.add (4);

		if(vector.count () != 4)
		CORE_TEST_FAILED ("Vector test data not added.")

		/*
		 * Test: findIf ()
		 */
		int* item = vector.findIf (EvenNumberPredicate);
		if(item == nullptr)
			CORE_TEST_FAILED ("Could not lookup test item")
		if(*item != 2)
			CORE_TEST_FAILED ("Retrieved wrong item")

		/*
		 * Test: removeIf ()
		 */
		int removedCount = vector.removeIf (EvenNumberPredicate);
		if(removedCount != 2)
			CORE_TEST_FAILED ("Failed to remove elements")
		if(vector.count () != 2)
			CORE_TEST_FAILED ("Failed to remove elements")
		if(!vector.contains (1))
			CORE_TEST_FAILED ("Removed wrong element")
		if(!vector.contains (3))
			CORE_TEST_FAILED ("Removed wrong element")
	}

	return true;
}
