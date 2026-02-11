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
// Filename    : testresultprinter.cpp
// Description : Print test results to console
//
//************************************************************************************************

#include "testresultprinter.h"

#include "ccl/public/base/variant.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//************************************************************************************************
// TestLogBuffer
//************************************************************************************************

String& TestLogBuffer::newLine (Flags flags)
{
	lines.add ({});

	if(!get_flag (flags, Flags::kIgnorePrefix))
		lines.last ().prepend (prefix);

	return lines.last ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TestLogBuffer::flush (System::IConsole& console)
{
	for(auto& line : lines)
		console.writeLine (line);

	lines.removeAll ();
	prefix.empty ();
}

//************************************************************************************************
// TestResultPrinter
//************************************************************************************************

void TestResultPrinter::printSetup (int numTestsToRun, StringRef filter)
{
	buffer.newLine ();
	buffer.newLine ().appendFormat ("[========] Applying filter %(1)", filter);
	buffer.newLine ().appendFormat ("[========] Running %(1) tests...", numTestsToRun);
	buffer.newLine ();
	buffer.flush (System::GetConsole ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TestResultPrinter::printTestResult (const TestResult& testResult)
{
	StringRef testSuiteName = testResult.getSuiteName ();
	StringRef testName = testResult.getTestName ();
	String prefix = testResult.hasPassed () ? "[     OK ]" : "[ FAILED ]";

	int milliseconds = toMilliseconds (testResult.getDuration ());
	buffer.newLine ().appendFormat ("%(1) %(2)::%(3) (%(4) ms)", prefix, testResult.getSuiteName (), testResult.getTestName (), milliseconds);
	buffer.flush (System::GetConsole ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TestResultPrinter::printSummary (int numPassedTests, int numFailedTests, double totalDuration)
{
	int milliseconds = toMilliseconds (totalDuration);

	buffer.newLine ();
	buffer.newLine ().appendFormat ("[========] %(1) tests ran in %(2) ms.", numPassedTests + numFailedTests, milliseconds);

	if(numPassedTests > 0)
	{
		String numerus = numPassedTests > 1 ? "tests" : "test";
		buffer.newLine ().appendFormat ("[ PASSED ] %(1) %(2).", numPassedTests, numerus);
	}

	if(numFailedTests > 0)
	{
		String numerus = numFailedTests > 1 ? "tests" : "test";
		buffer.newLine ().appendFormat ("[ FAILED ] %(1) %(2).", numFailedTests, numerus);
	}

	buffer.newLine ();
	buffer.flush (System::GetConsole ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TestResultPrinter::printAssertionResults (const TestResult& testResult)
{
	auto assertionResults = testResult.getAssertionResults ();
	if(assertionResults.isEmpty ())
		return;

	StringRef fileName = assertionResults.at (0)->getInfo ().fileName;

	buffer.setPrefix ("        -> ");
	if(!fileName.isEmpty ())
		buffer.newLine ().appendFormat ("%(1), failed at:", fileName);

	buffer.newLine ();
	for(auto& assertionResult : assertionResults)
		printFailedAssertion (*assertionResult);

	buffer.newLine (TestLogBuffer::Flags::kIgnorePrefix);
	buffer.flush (System::GetConsole ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TestResultPrinter::printFailedAssertion (const AssertionResult& assertionResult)
{
	if(assertionResult.hasPassed ())
		return;

	const AssertionInfo& info = assertionResult.getInfo ();

	if(info.lineNumber != 0)
		buffer.newLine ().appendFormat ("line %(1): %(2) failed.", info.lineNumber, info.expression);
	else
		buffer.newLine ().appendFormat ("%(1) failed.", info.expression);

	if(!info.message.isEmpty ())
		buffer.newLine ().appendFormat ("    Info: %(1)", info.message);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int TestResultPrinter::toMilliseconds (double duration) const
{
	return (int)(duration * 1000.);
}
