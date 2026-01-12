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
// Filename    : core/test/coretresettest.cpp
// Description : Core TreeSet Tests
//
//************************************************************************************************

#include "coretreesettest.h"

#include "core/public/coretreeset.h"

using namespace Core;
using namespace Test;

//************************************************************************************************
// TreeSetTest
//************************************************************************************************

CORE_REGISTER_TEST (TreeSetTest)

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr TreeSetTest::getName () const
{
	return "Core TreeSet";
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TreeSetTest::run (ITestContext& testContext)
{
	// setup
	bool succeded = true;
	TreeSet<int> treeSet;
	if(!treeSet.isEmpty ())
	{
		CORE_TEST_FAILED ("New set not empty.")
		succeded = false;
	}
	
	treeSet.add (1);
	treeSet.add (2);
	treeSet.add (3);
	treeSet.add (5);
	treeSet.add (4);

	if(treeSet.count () != 5)
	{
		CORE_TEST_FAILED ("Set failed to count elements properly.")
		succeded = false;
	}

	// test contains
	if(treeSet.contains (6))
	{
		CORE_TEST_FAILED ("Set contains wrong element after add.")
		succeded = false;
	}
	if(!treeSet.contains (3))
	{
		CORE_TEST_FAILED ("Set does not contain added element.")
		succeded = false;
	}
	if(!treeSet.add (6))
	{
		CORE_TEST_FAILED ("Set claims not to add new element.")
		succeded = false;
	}
	if(treeSet.add (3))
	{
		CORE_TEST_FAILED ("Set claims to add existing element as new.")
		succeded = false;
	}
	if(!treeSet.contains (3))
	{
		CORE_TEST_FAILED ("Set removed existing element when trying to add it again.")
		succeded = false;
	}

	if(treeSet.count () != 6)
	{
		CORE_TEST_FAILED ("Set counts elements incorrectly while adding.")
		succeded = false;
	}

	// test remove
	treeSet.remove (3);
	if(treeSet.contains (3))
	{
		CORE_TEST_FAILED ("Set failed to remove element.")
		succeded = false;
	}
	if(!treeSet.contains (2))
	{
		CORE_TEST_FAILED ("Set removed unrelated element.")
		succeded = false;
	}
	if(!treeSet.remove (2))
	{
		CORE_TEST_FAILED ("Set claims not to remove existing element.")
		succeded = false;
	}
	if(treeSet.remove (2)) // same element again
	{
		CORE_TEST_FAILED ("Set claims to remove already removed element.")
		succeded = false;
	}
	if(treeSet.contains (3))
	{
		CORE_TEST_FAILED ("Set adds nonexistant element instead when trying to add it.")
		succeded = false;
	}
	treeSet.remove (6);
	treeSet.remove (5);
	treeSet.remove (4);
	if(treeSet.count () != 1)
	{
		CORE_TEST_FAILED ("Set counts elements incorrectly while removing.")
		succeded = false;
	}

	// test removing last element
	if(!treeSet.remove (1))
	{
		CORE_TEST_FAILED ("Set claims to fail removing last element.")
		succeded = false;
	}
	if(treeSet.remove (1))
	{
		CORE_TEST_FAILED ("Set claims to remove element when empty.")
		succeded = false;
	}
	if(treeSet.contains (1))
	{
		CORE_TEST_FAILED ("Set fails to remove last element.")
		succeded = false;
	}
	if(!treeSet.isEmpty ())
	{
		CORE_TEST_FAILED ("Set claims not empty after removal of last element.")
		succeded = false;
	}
	if(treeSet.count () != 0)
	{
		CORE_TEST_FAILED ("Set counts incorrectly when removing last element.")
		succeded = false;
	}


	// test remove all
	treeSet.removeAll ();
	if(!treeSet.isEmpty ())
	{
		CORE_TEST_FAILED ("Set is not empty after removeAll ().")
		succeded = false;
	}
	if(treeSet.count () != 0)
	{
		CORE_TEST_FAILED ("Set counts incorrectly when removing all elements.")
		succeded = false;
	}
	if(treeSet.contains (1))
	{
		CORE_TEST_FAILED ("Set contains element after removeAll ().")
		succeded = false;
	}

	// test removing tree node with a child
	treeSet.add (2);
	treeSet.add (1);
	treeSet.add (3);
	treeSet.add (4);

	if(!treeSet.remove (2))
	{
		CORE_TEST_FAILED ("TreeSet removing intermediate node claims failed.")
		succeded = false;
	}
	if(treeSet.contains (2))
	{
		CORE_TEST_FAILED ("TreeSet removing intermediate node failed.")
		succeded = false;
	}

	// test removing node with red sibling
	treeSet.removeAll ();
	treeSet.add (1);
	treeSet.add (2);
	treeSet.add (3);
	treeSet.add (4);
	treeSet.add (5);
	treeSet.add (6);

	if(!treeSet.remove (1))
	{
		CORE_TEST_FAILED ("TreeSet removing node with red sibling claims failed.")
		succeded = false;
	}
	if(treeSet.contains (1))
	{
		CORE_TEST_FAILED ("TreeSet removing node with red sibling failed.")
		succeded = false;
	}

	// test removing red close Nephew node
	treeSet.removeAll ();
	treeSet.add (1);
	treeSet.add (2);
	treeSet.add (3);
	treeSet.add (4);
	treeSet.add (10);
	treeSet.add (9);
	treeSet.add (8);
	treeSet.add (7);
	treeSet.add (6);
	treeSet.add (5);

	if(!treeSet.remove (1))
	{
		CORE_TEST_FAILED ("TreeSet removing node with red close nephew claims failed.")
		succeded = false;
	}
	if(treeSet.contains (1))
	{
		CORE_TEST_FAILED ("TreeSet removing node with red close nephew failed.")
		succeded = false;
	}

	// test copy constructor
	treeSet.removeAll ();
	treeSet.add (1);
	treeSet.add (2);
	treeSet.add (3);

	TreeSet<int> treeSetCopy (treeSet);
	
	if(!treeSetCopy.contains (1) || !treeSetCopy.contains (2) || !treeSetCopy.contains (3))
	{
		CORE_TEST_FAILED ("Set copy failed to copy elements.")
		succeded = false;
	}

	if(!treeSet.contains (1) || !treeSet.contains (2) || !treeSet.contains (3))
	{
		CORE_TEST_FAILED ("Set copy removed element from original.")
		succeded = false;
	}

	if(treeSetCopy.count () != 3)
	{
		CORE_TEST_FAILED ("Set copy failed copying count.")
		succeded = false;
	}

	treeSet.remove (3);
	if(!treeSetCopy.contains (3))
	{
		CORE_TEST_FAILED ("Set copy created set dependent on original.")
		succeded = false;
	}

	treeSetCopy.remove (2);
	if(!treeSet.contains (2))
	{
		CORE_TEST_FAILED ("Set copy made original dendent on copy.")
		succeded = false;
	}

	// test iterator access
	treeSet.removeAll ();
	treeSet.add (1);
	treeSet.add (2);
	treeSet.add (4);
	treeSet.add (8);

	int sum = 0;
	int iterationCount = 0;
	
	for(int i : treeSet) 
	{
		if(
			iterationCount == 0 && i != 1 ||
			iterationCount == 1 && i != 2 ||
			iterationCount == 2 && i != 4 ||
			iterationCount == 3 && i != 8
			)
		{
			CORE_TEST_FAILED ("Iteration is not ordered ascending.")
			succeded = false;
		}

		sum += i;
		iterationCount++;
	}
	if(sum != 15)
	{
		CORE_TEST_FAILED ("Iterator didn't hit all elements.")
		succeded = false;
	}

	treeSet.removeAll ();

	return succeded;
}
