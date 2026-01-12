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
// Filename    : core/test/corecrttest.cpp
// Description : Core CRT Tests
//
//************************************************************************************************

#include "corecrttest.h"

using namespace Core;
using namespace Test;

//************************************************************************************************
// CoreCrtTest
//************************************************************************************************

CORE_REGISTER_TEST (CoreCrtTest)

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr CoreCrtTest::getName () const
{
	return "Core CRT";
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CoreCrtTest::run (ITestContext& testContext)
{
	bool result = true;

	if(!uidSscanfTest ("{AE5CE9D7-783D-4EEA-A900-057821515D12}", "{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}"))
	{
		CORE_TEST_FAILED ("Standard UID conversion failed")
		result = false;
	}

	if(!uidSscanfTest ("AE5CE9D7783D4EEAA900057821515D12", "%08lX%04X%04X%02X%02X%02X%02X%02X%02X%02X%02X"))
	{
		CORE_TEST_FAILED ("Compact UID conversion failed")
		result = false;
	}

	if(!macSscanfTest ("00:04:9F:06:64:98", "%02X:%02X:%02X:%02X:%02X:%02X"))
	{
		CORE_TEST_FAILED ("Standard MAC conversion failed")
		result = false;
	}

	if(!macSscanfTest ("00-04-9F-06-64-98", "%02X-%02X-%02X-%02X-%02X-%02X"))
	{
		CORE_TEST_FAILED ("System MAC conversion failed")
		result = false;
	}

	if(!macSscanfTest ("00049F066498", "%02X%02X%02X%02X%02X%02X"))
	{
		CORE_TEST_FAILED ("Compact MAC conversion failed")
		result = false;
	};

	int64 value64int;
	if(sscanf ("5100D101", "%" FORMAT_INT64 "x", &value64int) != 1 || value64int != 0x5100D101)
	{
		CORE_TEST_FAILED ("Hex 64-bit conversion failed")
		result = false;
	}

	if(sscanf ("19832313", "%" FORMAT_INT64 "d", &value64int) != 1 || value64int != 19832313)
	{
		CORE_TEST_FAILED ("64-bit decimal conversion failed")
		result = false;
	}

	double valueDouble;
	if(sscanf ("3.1415926535897932384626433832795", "%lf", &valueDouble) != 1 || valueDouble != 3.1415926535897932384626433832795)
	{
		CORE_TEST_FAILED ("double conversion failed");
		result = false;
	}

	float valueFloat;
	if(sscanf ("3.1415926535897932384626433832795", "%f", &valueFloat) != 1 || valueFloat != 3.1415926535897932384626433832795f)
	{
		CORE_TEST_FAILED ("float conversion failed");
		result = false;
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CoreCrtTest::uidSscanfTest (const char *string, const char *format)
{
	const int kUidReferenceValues[] = {static_cast<int> (0xAE5CE9D7), 0x783D, 0x4EEA, 0xa9, 0x00, 0x05, 0x78, 0x21, 0x51, 0x5D, 0x12};

	int v[11];

	memset (v, 0, sizeof(int) * 11);
	if(sscanf (string, format, &v[0], &v[1], &v[2], &v[3], &v[4], &v[5], &v[6], &v[7], &v[8], &v[9], &v[10]) != 11)
		return false;

	for(int n = 0; n < 11; n++)
	{
		if(v[n] != kUidReferenceValues[n])
			return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CoreCrtTest::macSscanfTest (const char *string, const char *format)
{
	const uint8 kMacReferenceValues[] = { 0x00, 0x04, 0x9F, 0x06, 0x64, 0x98 };

	uint8 v[6];

	memset (v, 0, 6);
	if(sscanf (string, format, &v[0], &v[1], &v[2], &v[3], &v[4], &v[5]) != 6)
		return false;
	
	for(int n = 0; n < 6; n++)
	{
		if(v[n] != kMacReferenceValues[n])
			return false;
	}

	return true;
}
