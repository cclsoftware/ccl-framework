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
// Filename    : testresult.h
// Description : Unit Test Results
//
//************************************************************************************************

#ifndef _ccl_testresult_h
#define _ccl_testresult_h

#include "ccl/base/object.h"
#include "ccl/public/text/cclstring.h"
#include "ccl/public/collections/vector.h"

#include "ccl/public/base/iunittest.h"

namespace CCL {

//************************************************************************************************
// AssertionInfo
//************************************************************************************************

struct AssertionInfo
{
	String expression;
	String fileName;
	int lineNumber = 0;
	String message;
};

//************************************************************************************************
// AssertionResult
//************************************************************************************************

class AssertionResult: public Object,
					   public IAssertionResult
{
public:
	AssertionResult (StringRef expression, StringRef fileName, int lineNumber);

	const AssertionInfo& getInfo () const;
	virtual bool hasPassed () const = 0;

	virtual bool hasFailed () const { return !hasPassed (); }

	// IAssertionResult
	tresult CCL_API addMessage (StringRef message) override;

	CLASS_INTERFACE (IAssertionResult, Object)

private:
	AssertionInfo info;
};

//************************************************************************************************
// AssertionFailed
//************************************************************************************************

class AssertionFailed: public AssertionResult
{
public:
	using AssertionResult::AssertionResult;

	// AssertionResult
	bool hasPassed () const override { return false; }
};

//************************************************************************************************
// AssertionPassed
//************************************************************************************************

class AssertionPassed: public AssertionResult
{
public:
	using AssertionResult::AssertionResult;

	// AssertionResult
	bool hasPassed () const override { return true; }
};

//************************************************************************************************
// TestResult
//************************************************************************************************

class TestResult: public Object,
				  public ITestContext
{
public:
	const Vector<AutoPtr<AssertionResult>>& getAssertionResults () const;

	bool hasFailed () const;
	bool hasPassed () const;

	PROPERTY_STRING (testName, TestName)
	PROPERTY_STRING (suiteName, SuiteName)
	PROPERTY_VARIABLE (double, duration, Duration)

	// ITestContext
	IAssertionResult& CCL_API addPass (StringRef expression, StringRef fileName, int lineNumber) override;
	IAssertionResult& CCL_API addFailure (StringRef expression, StringRef fileName, int lineNumber) override;

	CLASS_INTERFACE (ITestContext, Object)

private:
	Vector<AutoPtr<AssertionResult>> assertionResults;
};

} // namespace CCL

#endif // _ccl_testresult_h
