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
// Filename    : core/test/corestorabletest.h
// Description : Core Storable Tests
//
//************************************************************************************************

#ifndef _corestorabletest_h
#define _corestorabletest_h

#include "coretestbase.h"

namespace Core {
namespace Test {

//************************************************************************************************
// StorableTest
//************************************************************************************************

class StorableTest: public TestBase
{
public:
	// TestBase
	CStringPtr getName () const;
	bool run (ITestContext& testContext);
};

} // namespace Test
} // namespace Core

#endif // _corestorabletest_h
