//************************************************************************************************
//
// CCL Test Runner
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
// Filename    : testreporter.cpp
// Description : Console logging test reporter
//
//************************************************************************************************

#include "testreporter.h"

using namespace CCL;

//************************************************************************************************
// TestReporter
//************************************************************************************************

TestReporter::TestReporter ()
: totalDuration (0.)
{
	passedTests.objectCleanup ();
	failedTests.objectCleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TestReporter::beginTestRun (int numTests, StringRef filter)
{
	totalDuration = 0.;
	printer.printSetup (numTests, filter);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TestReporter::endTestRun ()
{
	printer.printSummary (passedTests.count (), failedTests.count (), totalDuration);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TestReporter::addResult (TestResult* testResult)
{
	totalDuration += testResult->getDuration ();

	printer.printTestResult (*testResult);

	if(testResult->hasPassed ())
	{
		passedTests.add (return_shared (testResult));
		return;
	}

	printer.printAssertionResults (*testResult);
	failedTests.add (return_shared (testResult));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TestReporter::allTestsPassed () const
{
	return failedTests.isEmpty ();
}
