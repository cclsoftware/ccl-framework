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
// Filename    : core/test/corestringtest.h
// Description : Core String Tests
//
//************************************************************************************************

#ifndef _corestringtest_h
#define _corestringtest_h

#include "coretestbase.h"

namespace Core {
namespace Test {

//************************************************************************************************
// StringTest
//************************************************************************************************

class StringTest: public TestBase
{
public:
	// TestBase
	CStringPtr getName () const;
	bool run (ITestContext& testContext);

private:
	bool testAppendInteger (ITestContext& testContext);
	bool testTokenizer (ITestContext& testContext);
	bool testTokenizerInplace (ITestContext& testContext);
	bool testTokenizerWithEmptyTokens (ITestContext& testContext);
	bool testTokenizerInplaceWithEmptyTokens (ITestContext& testContext);
};

} // namespace Test
} // namespace Core

#endif // _corestringtest_h
