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
#include "core/system/coredebug.h"

using namespace Core;
using namespace Test;

//************************************************************************************************
// TestBase
//************************************************************************************************

bool TestBase::run (ITestContext& testContext)
{
	CORE_TEST_FAILED ("This is the base class for real tests.")
	return false;
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
		CString32 runningMessage ("Running Test: ");
		runningMessage.append (tests[i]->getName ());
		CORE_TEST_MESSAGE (runningMessage.str ());
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
