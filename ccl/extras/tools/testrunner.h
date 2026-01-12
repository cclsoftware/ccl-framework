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
// Filename    : testrunner.h
// Description : Unit Test Runner
//
//************************************************************************************************

#ifndef _ccl_testrunner_h
#define _ccl_testrunner_h

#include "ccl/public/text/cclstring.h"
#include "testresult.h"

namespace CCL {

//************************************************************************************************
// ITestReporter
//************************************************************************************************

interface ITestReporter: IUnknown
{
	virtual void beginTestRun (int numTests, StringRef filter) = 0;

	virtual void endTestRun () = 0;

	virtual void addResult (TestResult* testResult) = 0;

	virtual bool allTestsPassed () const = 0;
};

//************************************************************************************************
// TestRunner
//************************************************************************************************

class TestRunner: public Object
{
public:
	/**
	 * Added reporters are owned by the caller.
	 */
	void addTestReporter (ITestReporter* reporter);

	/**
	 * Filter by test suite or test name using wildcards. Suite and test name separated by underscore
	 * e.g.:
	 * - "nameOfMyTestSuite*"
	 * - "*nameOfMyTest"
	 * - "nameOfMyTestSuite_nameOf*"
	 */
	void run (StringRef filter = "*") const;

private:
	Vector<ITestReporter*> reporters;

	void beginTestRun (int numTests, StringRef filter) const;
	void endTestRun () const;
	void report (TestResult* testResult) const;
	TestResult* runInternal (ITestSuite* suite, int index) const;
};

} // namespace CCL

#endif // _ccl_testrunner_h
