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
// Filename    : bitsettest.cpp
// Description : Unit tests for BitSet and IDSet
//
//************************************************************************************************

#include "ccl/base/unittest.h"
#include "core/public/corebitset.h"

using namespace Core;
using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (BitSetTest, TestInitialSize)
{
	BitSet bs;
	CCL_TEST_ASSERT (bs.getSize () == 0);
	BitSet bs2 (10);
	CCL_TEST_ASSERT (bs2.getSize () == 10);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (BitSetTest, TestInitialValues)
{
	BitSet bs (10);
	for(int i = 0; i < bs.getSize (); i++)
	{
		CCL_TEST_ASSERT (bs.getBit (i) == false);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (BitSetTest, TestSetBit)
{
	BitSet bs (1);
	bs.setBit (0, true);
	CCL_TEST_ASSERT (bs.getBit (0) == true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (BitSetTest, TestClearBit)
{
	BitSet bs (1);
	bs.setBit (0, true);
	bs.setBit (0, false);
	CCL_TEST_ASSERT (bs.getBit (0) == false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (BitSetTest, TestToggleBit)
{
	BitSet bs (1);
	bs.toggleBit (0);
	CCL_TEST_ASSERT (bs.getBit (0) == true);
	bs.toggleBit (0);
	CCL_TEST_ASSERT (bs.getBit (0) == false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (BitSetTest, TestSetAllBits)
{
	BitSet bs (10);
	bs.setAllBits (true);
	for(int i = 0; i < bs.getSize (); i++)
	{
		CCL_TEST_ASSERT (bs.getBit (i) == true);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (BitSetTest, TestClearAllBits)
{
	BitSet bs (10);
	bs.setBit (5, true);
	bs.setAllBits (false);
	for(int i = 0; i < bs.getSize (); i++)
	{
		CCL_TEST_ASSERT (bs.getBit (i) == false);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (BitSetTest, TestIsEmpty)
{
	BitSet bs (10);
	bs.setBit (3, true);
	bs.setAllBits (false);
	CCL_TEST_ASSERT (bs.countBits (true) == 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (BitSetTest, TestCountSetBits)
{
	BitSet bs (10);
	bs.setBit (3, true);
	CCL_TEST_ASSERT (bs.countBits (true) == 1);
	bs.setBit (5, true);
	CCL_TEST_ASSERT (bs.countBits (true) == 2);
	bs.resize (123);
	bs.setAllBits (true);
	CCL_TEST_ASSERT (bs.countBits (true) == 123);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (BitSetTest, TestGetFirstSet)
{
	BitSet bs;
	CCL_TEST_ASSERT (bs.findFirst (true) < 0);
	bs.resize (10);
	CCL_TEST_ASSERT (bs.findFirst (true) < 0);
	bs.setBit (5, true);
	bs.setBit (8, true);
	CCL_TEST_ASSERT (bs.findFirst (true) == 5);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (BitSetTest, TestGetFirstNotSet)
{
	BitSet bs;
	CCL_TEST_ASSERT (bs.findFirst (false) < 0);
	bs.resize (10);
	bs.setAllBits (true);
	CCL_TEST_ASSERT (bs.findFirst (false) < 0);
	bs.setBit (6, false);
	bs.setBit (9, false);
	CCL_TEST_ASSERT (bs.findFirst (false) == 6);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (BitSetTest, TestResize)
{
	BitSet bs;
	CCL_TEST_ASSERT (bs.getSize () == 0);
	bs.setBit (32, true);
	bs.resize (100);
	CCL_TEST_ASSERT (bs.getSize () == 100);
	CCL_TEST_ASSERT (bs.getBit (32) == true);
	CCL_TEST_ASSERT (bs.countBits (true) == 1);
	bs.resize (10);
	CCL_TEST_ASSERT (bs.getSize () == 10);
	CCL_TEST_ASSERT (bs.getBit (32) == false);
	CCL_TEST_ASSERT (bs.countBits (true) == 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (BitSetTest, TestAssignmentOperator)
{
	BitSet bs1;
	bs1.setBit (5, true);
	bs1.setBit (12, true);
	BitSet bs2 = bs1;
	CCL_TEST_ASSERT (bs1.getSize () == bs2.getSize ());
	for(int i = 0; i < bs1.getSize (); i++)
	{
		CCL_TEST_ASSERT (bs1.getBit (i) == bs2.getBit (i));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (BitSetTest, TestEqualityOperator)
{
	BitSet bs1;
	BitSet bs2;
	
	bs1.setBit (5, true);
	bs2.setBit (5, true);
	bs1.toggleBit (10);
	bs2.toggleBit (10);
	CCL_TEST_ASSERT (bs1 == bs2);
	
	bs1.toggleBit (8);
	CCL_TEST_ASSERT (bs1 != bs2);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (BitSetTest, TestIDSet)
{
	IDSet ids (1, -1);
	Vector<short> idList;
	
	CCL_TEST_ASSERT (ids.newID () == 1);
	int i;
	for(i = 0; i < 100; i++)
		idList.add ((short)ids.newID ());
	for(i = 70; i > 40; i-=2)
	{
		ids.releaseID (idList.at (i));
		idList.removeAt (i);
	}
	for(i = 0; i < 20; i++)
		idList.add ((short)ids.newID ());

	Vector<short> idList2;
	for(i = 0; i < idList.count (); i++)
	{
		CCL_TEST_ASSERT (idList2.contains (idList[i]) == false);
		idList2.add (idList[i]);
	}
}

