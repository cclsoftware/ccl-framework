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
// Filename    : core/test/corenetworktest.cpp
// Description : Core Network Tests
//
//************************************************************************************************

#include "corenetworktest.h"

#include "core/network/corenetwork.h"

using namespace Core;
using namespace Sockets;
using namespace Test;

//************************************************************************************************
// networkTest
//************************************************************************************************

CORE_REGISTER_TEST (NetworkTest)

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr NetworkTest::getName () const
{
	return "Core Network";
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool NetworkTest::run (ITestContext& testContext)
{
	if(!Network::startup ())
	{
		CORE_TEST_FAILED ("Failed to startup network.")
		return false;
	}
	
	bool succeeded = true;

	IPAddress address;
	CStringPtr hostname = "ccl.dev";
	
	Network::getAddressByHost (address, hostname);

	if(address.byteSize == 0)
	{
		succeeded = false;
		CORE_TEST_FAILED ("Could not get an IP address by hostname")
	}

	CStringBuffer<STRING_STACK_SPACE_MAX> message;
	message.appendFormat ("IP address of %s is %d.%d.%d.%d.", hostname, address.ip.address[0], address.ip.address[1], address.ip.address[2], address.ip.address[3]);
	CORE_TEST_MESSAGE (message);

	Network::shutdown ();
	
	return succeeded;
}
