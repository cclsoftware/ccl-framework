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
// Filename    : core/test/coretestbase.cpp
// Description : Test base class
//
//************************************************************************************************

#include "coretestbase.h"

#include "core/public/corestringbuffer.h"

using namespace Core;
using namespace Test;

//************************************************************************************************
// TestBase
//************************************************************************************************

bool TestBase::run (ITestContext& testContext)
{
	setUp (testContext);
	testBody (testContext);
	tearDown (testContext);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr TestBase::getName () const
{
	return "";
}

//************************************************************************************************
// TestRegistry
//************************************************************************************************

DEFINE_STATIC_SINGLETON (TestRegistry)

//////////////////////////////////////////////////////////////////////////////////////////////////

void TestRegistry::runAllTests (ITestContext& testContext)
{
	for(int i = 0; i < tests.count (); ++i)
	{
		CString128 msg ("Running Test: ");
		msg.append (tests[i]->getName ());
		CORE_TEST_MESSAGE (msg.str ())
		tests[i]->run (testContext);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Vector<TestBase*>& TestRegistry::getTests () const
{
	return tests;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TestRegistry::addTest (TestBase* test)
{
	tests.add (test);
}
