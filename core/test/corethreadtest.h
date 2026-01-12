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
// Filename    : core/test/corethreadtest.h
// Description : Core Thread Tests
//
//************************************************************************************************

#ifndef _corethreadtest_h
#define _corethreadtest_h

#include "coretestbase.h"

namespace Core {
namespace Test {

//************************************************************************************************
// ThreadTest
//************************************************************************************************

class ThreadTest: public TestBase
{
public:
	// TestBase
	CStringPtr getName () const;
	bool run (ITestContext& testContext);
};

//************************************************************************************************
// LockTest
//************************************************************************************************

class LockTest: public TestBase
{
public:
	// TestBase
	CStringPtr getName () const;
	bool run (ITestContext& testContext);
};

//************************************************************************************************
// SignalTest
//************************************************************************************************

class SignalTest: public TestBase
{
public:
	// TestBase
	CStringPtr getName () const;
	bool run (ITestContext& testContext);
};

} // namespace Test
} // namespace Core

#endif // _corethreadtest_h
