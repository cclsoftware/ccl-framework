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
// Filename    : core/test/coreallocatortest.h
// Description : Core Allocator Tests
//
//************************************************************************************************

#ifndef _coreallocatortest_h
#define _coreallocatortest_h

#include "coretestbase.h"

namespace Core {
namespace Test {

//************************************************************************************************
// AllocatorTest
//************************************************************************************************

class AllocatorTest: public TestBase
{
public:
	// TestBase
	CStringPtr getName () const override;
	bool run (ITestContext& testContext) override;
};

} // namespace Test
} // namespace Core

#endif // _coreallocatortest_h
