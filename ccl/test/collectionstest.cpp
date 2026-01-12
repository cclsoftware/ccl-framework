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
// Filename    : collectionstest.cpp
// Description : Unit tests for collection classes
//
//************************************************************************************************

#include "ccl/base/unittest.h"

#include "ccl/base/object.h"
#include "ccl/base/collections/objectarray.h"
#include "ccl/base/collections/objectlist.h"
#include "ccl/base/collections/linkablelist.h"

using namespace Core;
using namespace CCL;

//************************************************************************************************
// TestObject
//************************************************************************************************

class TestObject: public Linkable
{
public:
	DECLARE_CLASS (TestObject, Linkable)

	PROPERTY_VARIABLE (int, value, Value)

	TestObject (int value = 0)
	: value (value),
	  Linkable ()
	{}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (TestObject, Linkable)

//************************************************************************************************
// CollectionsTest
//************************************************************************************************

template <class T>
class CollectionsTest: public Test {};

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_T (CollectionsTest, TestObjectArray)
{
	// Test: removeIf<T, Lambda>
	{
		auto hasEvenValue = [&] (const TestObject& obj) -> bool
		{
			return obj.getValue () % 2 == 0;
		};
		
		TypeParam objects;
		objects.objectCleanup (true);
		objects.add (NEW TestObject (1));
		objects.add (NEW TestObject (2));
		objects.add (NEW TestObject (3));
		objects.add (NEW TestObject (4));
		CCL_TEST_ASSERT_EQUAL (4, objects.count ());
		
		// Run method under test
		CCL_TEST_ASSERT_EQUAL (2, objects.template removeIf<TestObject> (hasEvenValue));
		
		// Verify
		CCL_TEST_ASSERT_EQUAL (2, objects.count ());
		TestObject* obj = ccl_cast<TestObject> (objects.at (0));
		CCL_TEST_ASSERT_EQUAL (1, obj->getValue ());
		obj = ccl_cast<TestObject> (objects.at (1));
		CCL_TEST_ASSERT_EQUAL (3, obj->getValue ());
	}
	
	// Test: removeIf<Lambda>
	{
		auto hasEvenValue = [&] (const Object* obj) -> bool
		{
			if(const TestObject* t = ccl_cast<TestObject> (obj))
				return t->getValue () % 2 == 0;
			return false;
		};
		
		TypeParam objects;
		objects.objectCleanup (true);
		objects.add (NEW TestObject (1));
		objects.add (NEW TestObject (2));
		objects.add (NEW TestObject (3));
		objects.add (NEW TestObject (4));
		CCL_TEST_ASSERT_EQUAL (4, objects.count ());
		
		// Run method under test
		CCL_TEST_ASSERT_EQUAL (2, objects.removeIf (hasEvenValue));
		
		// Verify
		CCL_TEST_ASSERT_EQUAL (2, objects.count ());
		TestObject* obj = ccl_cast<TestObject> (objects.at (0));
		CCL_TEST_ASSERT_EQUAL (1, obj->getValue ());
		obj = ccl_cast<TestObject> (objects.at (1));
		CCL_TEST_ASSERT_EQUAL (3, obj->getValue ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_T_ADD (CollectionsTest, TestObjectArray, ObjectArray)
CCL_TEST_T_ADD (CollectionsTest, TestObjectArray, ObjectList)
CCL_TEST_T_ADD (CollectionsTest, TestObjectArray, LinkableList)
