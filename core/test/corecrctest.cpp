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
// Filename    : core/test/corecrctest.cpp
// Description : Core Crc Tests
//
//************************************************************************************************

#include "corecrctest.h"

#include "core/portable/corecrc.h"

using namespace Core;
using namespace Portable;
using namespace Test;

//************************************************************************************************
// CrcTest
//************************************************************************************************

CORE_REGISTER_TEST (CrcTest)

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr CrcTest::getName () const
{
	return "Core Crc";
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CrcTest::run (ITestContext& testContext)
{
	bool succeeded = true;

	succeeded &= testCrc8 (testContext);
	succeeded &= testCrc8Variations (testContext);
	succeeded &= testCrc16Variations (testContext);
	succeeded &= testCrc32Variations (testContext);
	succeeded &= testCrc64 (testContext);

	succeeded &= testCrc8Loop (testContext);
	succeeded &= testCrc32Loop (testContext);

	return succeeded;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CrcTest::testCrc8 (ITestContext& testContext)
{
	Crc<uint8, 0x07, 0x00, false, false, 0x00> crc;
	crc.update ("\0", 1);
	if(crc.get () != 0x00)
	{
		CORE_TEST_FAILED ("Crc8 failed for input 0");
		return false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CrcTest::testCrc8Variations (ITestContext& testContext)
{
	CString64 testString = "The quick brown fox jumps over the lazy dog";
	bool succeeded = true;

	succeeded &= testCrc<uint8, Crc<uint8, 0x2F, 0xFF, false, false, 0xFF>> (testContext, "CRC-8/AUTOSAR", testString, 0x67);
	succeeded &= testCrc<uint8, Crc<uint8, 0x07, 0x00, false, false, 0x55>> (testContext, "CRC-8/I-432-1", testString, 0x94);
	succeeded &= testCrc<uint8, Crc<uint8, 0x07, 0xFF, true, true, 0x00>> (testContext, "CRC-8/ROHC", testString, 0xBF);
	succeeded &= testCrc<uint8, Crc<uint8, 0x07, 0x00, false, false, 0x00>> (testContext, "CRC-8/SMBUS", testString, 0xC1);

	return succeeded;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CrcTest::testCrc16Variations (ITestContext& testContext)
{
	CString64 testString = "The swift tan fox leaps atop the sluggish dog";
	bool succeeded = true;

	succeeded &= testCrc<uint16, Crc<uint16, 0x8005, 0x0000, true, true, 0x0000>> (testContext, "CRC-16/ARC", testString, 0xF796);
	succeeded &= testCrc<uint16, Crc<uint16, 0x1021, 0xFFFF, false, false, 0xFFFF>> (testContext, "CRC-16/GENIBUS", testString, 0xCFBA);
	succeeded &= testCrc<uint16, Crc<uint16, 0x5935, 0xFFFF, false, false, 0x0000>> (testContext, "CRC-16/M17", testString, 0xB483);
	succeeded &= testCrc<uint16, Crc<uint16, 0x8005, 0xFFFF, true, true, 0xFFFF>> (testContext, "CRC-16/USB", testString, 0xC759);

	return succeeded;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CrcTest::testCrc32Variations (ITestContext& testContext)
{
	CString64 testString = "The slow white fox falls below the energetic dog";
	bool succeeded = true;

	succeeded &= testCrc<uint32, Crc32> (testContext, "CRC-32/ISO-HDLC", testString, 0xFD791869);
	succeeded &= testCrc<uint32, Crc32Mpeg2> (testContext, "CRC-32/MPEG-2", testString, 0xB9B976C3);

	return succeeded;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CrcTest::testCrc64 (ITestContext& testContext)
{
	CString64 testString = "The slack snowy fox plummets beneath the animated dog";
	bool succeeded = true;

	using Crc64Ecma = Crc<uint64, 0x42F0E1EBA9EA3693, 0x0000000000000000, false, false, 0x0000000000000000>;
	succeeded &= testCrc<uint64, Crc64Ecma> (testContext, "CRC-64/ECMA", testString, 0x5EB1C582C2BEE1C7);

	return succeeded;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CrcTest::testCrc8Loop (ITestContext& testContext)
{
	Crc<uint8, 0x07, 0x00, false, false, 0x00> crc;
	for(char c = '0'; c <= '9'; c++)
		crc.update (&c, 1);
	if(crc.get () != 0x45)
	{
		CORE_TEST_FAILED ("Crc8 failed for input \"0123456789\" in single bytes");
		return false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CrcTest::testCrc32Loop (ITestContext& testContext)
{
	CString64 input = "Romani ite domum";
	Crc32 crc;
	for(int i = 0; i < 100; i++)
		crc.update (input, ConstString (input).length ());
	if(crc.get () != 0xC8BEAC19)
	{
		CORE_TEST_FAILED ("Crc32 failed for input \"Romani ite domum\" (repeated 100 times)");
		return false;
	}
	return true;
}
