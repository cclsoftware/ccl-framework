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
// Filename    : testrunner.cpp
// Description : Unit Test Runner
//
//************************************************************************************************

#include "testrunner.h"
#include "testcollectionregistry.h"

#include "ccl/public/systemservices.h"

using namespace CCL;

//************************************************************************************************
// TestRunner
//************************************************************************************************

void TestRunner::addTestReporter (ITestReporter* reporter)
{
	reporters.add (reporter);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TestRunner::run (StringRef filter) const
{
	auto& registry = TestCollectionRegistry::instance ();

	Vector<TestDescription> tests;
	registry.collectTests (tests, filter);

	beginTestRun (tests.count (), filter);

	for(auto& test : tests)
	{
		AutoPtr<TestResult> result = runInternal (test.suite, test.testIndex);
		report (result);
	}

	endTestRun ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TestRunner::beginTestRun (int numTests, StringRef filter) const
{
	for(auto* reporter : reporters)
		reporter->beginTestRun (numTests, filter);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TestRunner::endTestRun () const
{
	for(auto* reporter : reporters)
		reporter->endTestRun ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TestRunner::report (TestResult* result) const
{
	for(auto* reporter : reporters)
		reporter->addResult (result);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TestResult* TestRunner::runInternal (ITestSuite* suite, int index) const
{
	TestResult* testResult = NEW TestResult ();

	double begin = System::GetProfileTime ();
	suite->runTest (index, testResult);
	double end = System::GetProfileTime ();

	testResult->setSuiteName (suite->getName ());
	testResult->setTestName (suite->getTestName (index));
	testResult->setDuration (end - begin);

	return testResult;
}
