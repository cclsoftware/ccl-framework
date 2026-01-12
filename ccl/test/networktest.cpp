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
// Filename    : networktest.cpp
// Description : Network Unit Tests
//
//************************************************************************************************

#include "ccl/base/unittest.h"

#include "ccl/base/storage/file.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/base/memorystream.h"
#include "ccl/public/system/logging.h"
#include "ccl/public/systemservices.h"

#include "ccl/public/network/web/iwebservice.h"
#include "ccl/public/network/web/iwebclient.h"
#include "ccl/public/network/web/iwebnewsreader.h"
#include "ccl/public/network/inetdiscovery.h"
#include "ccl/public/netservices.h"

using namespace CCL;
using namespace Net;
using namespace Web;

//////////////////////////////////////////////////////////////////////////////////////////////////

static const String testServerName = CCLSTR ("pkware.com");
static const String testFileName = CCLSTR ("/documents/casestudies/APPNOTE.TXT");

//************************************************************************************************
// WebSuite
//************************************************************************************************

CCL_TEST (WebSuite, TestWebClient)
{
	AutoPtr<IWebClient> client;
	CCL_TEST_ASSERT ((client = System::GetWebService ().createClient (Web::Meta::kHTTP)) != nullptr);
	if(!client)
		return;

	tresult result = kResultOk;
	CCL_TEST_ASSERT ((result = client->connect (testServerName)) == kResultOk);
	if(result != kResultOk)
		return;

	TempFile tempFile ("download.data");
	AutoPtr<IStream> tempStream = tempFile.open (IStream::kCreateMode);
	CCL_TEST_ASSERT (tempStream != nullptr);
	if(!tempStream)
		return;

	CCL_TEST_ASSERT ((result = client->downloadData (testFileName, *tempStream)) == kResultOk);
	if(result != kResultOk)
		return;

	TempFile tempFile2 ("download2.data");
	AutoPtr<IStream> tempStream2 = tempFile2.open (IStream::kCreateMode);
	CCL_TEST_ASSERT (tempStream2 != nullptr);
	if(!tempStream2)
		return;

	CCL_TEST_ASSERT ((result = client->downloadData (testFileName, *tempStream2)) == kResultOk);
	if(result != kResultOk)
		return;

	CCL_TEST_ASSERT ((result = client->disconnect ()) == kResultOk);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (WebSuite, TestNewsReader)
{
	AutoPtr<IWebNewsReader> reader;
	CCL_TEST_ASSERT ((reader = System::GetWebService ().createReader ()) != nullptr);
	if(!reader)
		return;

	Url url ("resource:///atomtest.xml");
	String moduleString;
	System::GetModuleIdentifier (moduleString, System::GetCurrentModuleRef ());
	url.setHostName (moduleString);

	AutoPtr<IStream> stream;
	CCL_TEST_ASSERT ((stream = System::GetFileSystem ().openStream (url)) != nullptr);
	if(!stream)
		return;

	tresult result = kResultOk;
	CCL_TEST_ASSERT ((result = reader->loadFeed (*stream)) == kResultOk);
	if(result != kResultOk)
		return;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (WebSuite, TestSSL)
{
	TempFile tempFile ("downloadSSL.data");
	AutoPtr<IStream> tempStream = tempFile.open (IStream::kCreateMode);
	CCL_TEST_ASSERT (tempStream != nullptr);
	if(!tempStream)
		return;

	Url url ("https://ccl.dev");
	CCL_TEST_ASSERT (System::GetWebService ().downloadData (url, *tempStream) == kResultOk);
}

//************************************************************************************************
// NetworkSuite
//************************************************************************************************

CCL_TEST (NetworkSuite, TestLocalhost)
{
	INetwork& network = System::GetNetwork ();

	String localhost;
	CCL_TEST_ASSERT (network.getLocalHostname (localhost) == kResultOk);

	IPAddress address;
	CCL_TEST_ASSERT (network.getAddressByHost (address, localhost) == kResultOk);

	String ipString;
	CCL_TEST_ASSERT (network.getAddressString (ipString, address) == kResultOk);
	
	IPAddress address2 (address.family);
	CCL_TEST_ASSERT (network.getAddressFromString (address2, ipString) == kResultOk);

#if 1
	Logging::debug ("Local host:");
	Logging::debug (localhost);
	Logging::debug ("IP:");
	Logging::debug (ipString);
#else
	String s;
	s << "Local host name is [" << localhost << "] (IP " << ipString << ")";
	Logging::debug (s);
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (NetworkSuite, TestNetworkStream)
{
	INetwork& network = System::GetNetwork ();

	IPAddress address;
	CCL_TEST_ASSERT (network.getAddressByHost (address, testServerName) == kResultOk);
	address.port = 80;

	AutoPtr<CCL::IStream> netStream = network.openStream (address, kTCP);
	CCL_TEST_ASSERT (netStream != nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (NetworkSuite, TestDiscovery)
{
	IDiscoveryHandler& handler = System::GetDiscoveryHandler ();

	// TODO: register callback!
}
