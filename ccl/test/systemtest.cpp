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
// Filename    : systemtest.cpp
// Description : Unit tests for cclsystem
//
//************************************************************************************************

#include "ccl/base/unittest.h"

#include "ccl/base/storage/url.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/logging.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//************************************************************************************************
// SystemTest
//************************************************************************************************

CCL_TEST (SystemTest, TestResourceIterator)
{
	String moduleString;
	System::GetModuleIdentifier (moduleString, System::GetMainModuleRef ());

	Url resourceUrl (0, Url::kFolder);
	resourceUrl.setProtocol (ResourceUrl::Protocol);
	resourceUrl.setHostName (moduleString);
	
	AutoPtr<IFileIterator> iterator = System::GetFileSystem ().newIterator (resourceUrl, IStream::kOpenMode);
	CCL_TEST_ASSERT (iterator != nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (SystemTest, TestSystemInformation)
{
	ISystemInformation& systemInfo = System::GetSystem ();

	DateTime localTime;
	systemInfo.getLocalTime (localTime);

	String temp;
	temp << "Local time is ";
	temp << localTime.getDate ().getYear () << "/" << localTime.getDate ().getMonth () << "/" << localTime.getDate ().getDay ();
	temp << " " << localTime.getTime ().getHour () << ":" << localTime.getTime ().getMinute () << ":" << localTime.getTime ().getSecond ();
	Logging::debug (temp);
	
	String computerName;
	systemInfo.getComputerName (computerName);
	CCL_TEST_ASSERT (computerName.isEmpty () == false);
	Logging::debug ("Computername is ");
	Logging::debug (computerName);

	String userName;
	systemInfo.getUserName (userName);
	CCL_TEST_ASSERT (userName.isEmpty () == false);
	Logging::debug ("Username is ");
	Logging::debug (userName);
}
