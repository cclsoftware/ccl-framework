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
// Filename    : core/test/coretreesettest.h
// Description : Core TreeSet Test
//
//************************************************************************************************

#ifndef _coretreesettest_h
#define _coretreesettest_h

#include "coretestbase.h"

namespace Core {
namespace Test {

//************************************************************************************************
// TreeSetTest
//************************************************************************************************

class TreeSetTest: public TestBase
{
public:
	// TestBase
	CStringPtr getName () const;
	bool run (ITestContext& testContext);
};

} // namespace Test
} // namespace Core

#endif // _coretreesettest_h
