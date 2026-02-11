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
// Filename    : testreporter.h
// Description : Console logging test reporter
//
//************************************************************************************************

#ifndef _ccl_testreporter_h
#define _ccl_testreporter_h

#include "testresultprinter.h"

#include "ccl/base/collections/objectarray.h"
#include "ccl/extras/tools/testrunner.h"

namespace CCL {

//************************************************************************************************
// TestReporter
//************************************************************************************************

class TestReporter: public Object,
					public ITestReporter
{
public:
	TestReporter ();


	// ITestReporter
	void beginTestRun (int numTests, StringRef filter) override;
	void endTestRun () override;
	void addResult (TestResult* testResult) override;
	bool allTestsPassed () const override;

	CLASS_INTERFACE (ITestReporter, Object)

private:
	double totalDuration; // seconds

	ObjectArray failedTests;
	ObjectArray passedTests;

	TestResultPrinter printer;
};

} // namespace CCL

#endif // _ccl_testreporter_h
