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
// Filename    : testresult.cpp
// Description : Unit Test Result
//
//************************************************************************************************

#include "testresult.h"

using namespace CCL;

//************************************************************************************************
// AssertionResult
//************************************************************************************************

AssertionResult::AssertionResult (StringRef expression, StringRef fileName, int lineNumber)
{
	info.expression = expression;
	info.fileName = fileName;
	info.lineNumber = lineNumber;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AssertionResult::addMessage (StringRef message)
{
	info.message.append (message);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const AssertionInfo& AssertionResult::getInfo () const
{
	return info;
}

//************************************************************************************************
// TestResult
//************************************************************************************************

const Vector<AutoPtr<AssertionResult>>& TestResult::getAssertionResults () const
{
	return assertionResults;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAssertionResult& CCL_API TestResult::addPass (StringRef expression, StringRef fileName, int lineNumber)
{
	AssertionResult* result = NEW AssertionPassed (expression, fileName, lineNumber);
	assertionResults.add (result);
	return *result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAssertionResult& CCL_API TestResult::addFailure (StringRef expression, StringRef fileName, int lineNumber)
{
	AssertionResult* result = NEW AssertionFailed (expression, fileName, lineNumber);
	assertionResults.add (result);
	return *result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TestResult::hasFailed () const
{
	for(auto& result : assertionResults)
		if(result->hasFailed ())
			return true;

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TestResult::hasPassed () const
{
	return !hasFailed ();
}
