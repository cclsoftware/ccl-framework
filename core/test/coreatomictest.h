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
// Filename    : core/test/coreatomictest.h
// Description : Core Atomic Tests
//
//************************************************************************************************

#ifndef _coreatomictest_h
#define _coreatomictest_h

#include "coretestbase.h"

namespace Core {
namespace Test {

//************************************************************************************************
// AtomicTest
//************************************************************************************************

class AtomicTest: public TestBase
{
public:
	// TestBase
	CStringPtr getName () const override;
	bool run (ITestContext& testContext) override;
};

} // namespace Test
} // namespace Core

#endif // _coreatomictest_h
