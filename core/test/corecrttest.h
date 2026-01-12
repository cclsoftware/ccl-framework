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
// Filename    : core/test/corecrttest.h
// Description : Core CRT Tests
//
//************************************************************************************************

#ifndef _corecrttest_h
#define _corecrttetst_h

#include "coretestbase.h"

namespace Core {
namespace Test {

//************************************************************************************************
// CoreCrtTest
//************************************************************************************************

class CoreCrtTest: public TestBase
{
public:
	// TestBase
	CStringPtr getName () const;
	bool run (ITestContext& testContext);

protected:
	bool uidSscanfTest (const char *string, const char *format);
	bool macSscanfTest (const char *string, const char *format);
};

} // namespace Test
} // namespace Core

#endif // _corecrttest_h
