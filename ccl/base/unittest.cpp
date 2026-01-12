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
// Filename    : ccl/base/unittest.cpp
// Description : Unit Test Suite
//
//************************************************************************************************

#include "unittest.h"

using namespace CCL;

//************************************************************************************************
// TestSuite
//************************************************************************************************

DEFINE_CLASS_HIDDEN (TestSuite, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

TestSuite::TestSuite (StringRef name)
: name (name)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TestSuite::addTest (Test* test)
{
	tests.add (test);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API TestSuite::getName () const
{
	return name;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int TestSuite::countTests () const
{
	return tests.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef TestSuite::getTestName (int index) const
{
	Test* test = tests.at (index);
	if(test != nullptr)
		return test->getName ();
	return CCLSTR ("");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult TestSuite::runTest (int index, ITestContext* context)
{
	Test* test = tests.at (index);

	if(test != nullptr)
	{
		test->setTestContext (context);
		test->setUp ();
		test->testBody ();
		test->tearDown ();
		return kResultOk;
	}

	return kResultInvalidArgument;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult TestSuite::setUp ()
{
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult TestSuite::tearDown ()
{
	return kResultOk;
}

//************************************************************************************************
// TestRegistry
//************************************************************************************************

DEFINE_SINGLETON (TestRegistry)

//////////////////////////////////////////////////////////////////////////////////////////////////

void TestRegistry::registerTestFactory (ITestFactory* testFactory)
{
	testFactories.add (testFactory);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TestRegistry::createTestSuites (Vector<AutoPtr<ITestSuite>>& testSuites) const
{
	for(auto& testFactory : testFactories)
	{
		Test* test = testFactory->createTest ();

		StringRef suiteName = testFactory->getSuiteName ();

		auto matchesSuiteName = [&suiteName] (const AutoPtr<ITestSuite>& testSuite) -> bool
		{
			return testSuite->getName () == suiteName;
		};

		TestSuite* suite = nullptr;
		if(AutoPtr<ITestSuite>* result = testSuites.findIf (matchesSuiteName))
			suite = unknown_cast<TestSuite> (*result);
		else
		{
			testSuites.add (NEW TestSuite (suiteName));
			suite = unknown_cast<TestSuite> (testSuites.last ());
		}

		if(suite != nullptr)
			suite->addTest (test);
	}
}

//************************************************************************************************
// TestCollection
//************************************************************************************************

void TestCollection::populateFrom (const TestRegistry& registry)
{
	registry.createTestSuites (suites);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API TestCollection::countSuites () const
{
	return suites.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITestSuite* CCL_API TestCollection::getSuite (int index) const
{
	return suites.at (index);
}
