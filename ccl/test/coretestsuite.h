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
// Filename    : coretestsuite.h
// Description : Unit tests for Core
//
//************************************************************************************************

#ifndef _ccl_coretest_h
#define _ccl_coretest_h

#include "ccl/public/base/iunittest.h"
#include "ccl/public/base/unknown.h"
#include "ccl/public/text/cclstring.h"
#include "ccl/public/collections/vector.h"

#include "core/test/coretestcontext.h"

namespace CCL {

//************************************************************************************************
// CoreTestSuite
//************************************************************************************************

class CoreTestSuite: public Unknown,
					 public ITestSuite,
					 public Core::Test::ITestContext
{
public:
	CoreTestSuite ();
	
	// ITestSuite
	StringRef CCL_API getName () const override;
	int CCL_API countTests () const override;
	StringRef CCL_API getTestName (int index) const override;
	tresult CCL_API setUp () override;
	tresult CCL_API tearDown () override;
	tresult CCL_API runTest (int index, CCL::ITestContext* context) override;
	
	// ITestContext
	void addMessage (CStringPtr message, CStringPtr sourceFile, int lineNumber) override;
	void addFailure (CStringPtr message, CStringPtr sourceFile, int lineNumber) override;

	CLASS_INTERFACE (ITestSuite, Unknown)

protected:
	Vector<String> testNames;
	CCL::ITestContext* context;
};

} // namespace CCL

#endif // _ccl_coretest_h
