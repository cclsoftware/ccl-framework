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
// Filename    : testresultprinter.h
// Description : Print test results to console
//
//************************************************************************************************

#ifndef _ccl_testreportformat_h
#define _ccl_testreportformat_h

#include "ccl/extras/tools/testresult.h"

#include "ccl/public/collections/vector.h"
#include "ccl/public/system/iconsole.h"

namespace CCL {

//************************************************************************************************
// TestLogBuffer
//************************************************************************************************

class TestLogBuffer
{
public:
	enum Flags
	{
		kNone = 0,
		kIgnorePrefix = 1 << 0
	};

	PROPERTY_STRING (prefix, Prefix)

	/**
	 * Adds a new line and returns a reference in order to add text and formatting
	 * e.g. newLine ().appendFormat ("%(1) %(2)", "Hello", "World")
	 */
	String& newLine (Flags flags = Flags::kNone);

	/**
	 * Removes all lines after writing them to the provided console
	 */
	void flush (System::IConsole& console);

private:
	Vector<String> lines;
	String indent;
};

//************************************************************************************************
// TestResultPrinter
//************************************************************************************************

class TestResultPrinter
{
public:
	void printSetup (int numTestsToRun, StringRef filter);
	void printTestResult (const TestResult& testResult);
	void printSummary (int numPassedTests, int numFailedTests, double totalDuration);
	void printAssertionResults (const TestResult& testResult);

private:
	TestLogBuffer buffer;

	void printFailedAssertion (const AssertionResult& assertionResult);
	int toMilliseconds (double duration) const;
};

} // namespace CCL

#endif // _ccl_testreportformat_h
