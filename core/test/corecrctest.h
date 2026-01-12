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
// Filename    : core/test/corecrctest.h
// Description : Core CRC Tests
//
//************************************************************************************************

#ifndef _corecrctest_h
#define _corecrctetst_h

#include "coretestbase.h"

#include "core/public/corestringbuffer.h"

namespace Core {
namespace Test {

//************************************************************************************************
// CrcTest
//************************************************************************************************

class CrcTest: public TestBase
{
public:
	// TestBase
	CStringPtr getName () const;
	bool run (ITestContext& testContext);

private:
	template <typename CrcType, typename CrcAlgorithm>
	bool testCrc (ITestContext& testContext, CStringPtr algorithmName, CStringPtr input, CrcType expectedResult)
	{
		CrcAlgorithm crc;
		crc.update (input, ConstString (input).length ());
		if(crc.get () != expectedResult)
		{
			CORE_TEST_FAILED (algorithmName);
			return false;
		}
		return true;
	}

	bool testCrc8 (ITestContext& testContext);
	bool testCrc8Variations (ITestContext& testContext);
	bool testCrc16Variations (ITestContext& testContext);
	bool testCrc32Variations (ITestContext& testContext);
	bool testCrc64 (ITestContext& testContext);

	bool testCrc8Loop (ITestContext& testContext);
	bool testCrc32Loop (ITestContext& testContext);
};

} // namespace Test
} // namespace Core

#endif // _corecrctest_h
