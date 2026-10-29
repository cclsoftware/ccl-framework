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
#include "ccl/public/base/iprogress.h"
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
// TestCancelProgress
//************************************************************************************************

class TestCancelProgress: public Unknown,
 						  public AbstractProgressNotify
{
public:
	TestCancelProgress (bool canceledImmediately)
	: canceled (canceledImmediately)
	{}

	// AbstractProgressNotify
	tbool CCL_API isCanceled () override
	{ return canceled; }

	CLASS_INTERFACE (IProgressNotify, Unknown)

private:
	bool canceled;
};

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
	CCL_TEST_ASSERT (System::GetWebService ().downloadData ({url, *tempStream}) == kResultOk);
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

CCL_TEST (NetworkSuite, TestConnectTimeout)
{
	INetwork& network = System::GetNetwork ();

	// 192.0.2.1 is TEST-NET-1 (RFC 5737): reserved, unrouted on virtually any real network, so a
	// SYN sent to it is silently dropped.
	IPAddress address (kInternet);
	CCL_TEST_ASSERT (network.getAddressFromString (address, CCLSTR ("192.0.2.1")) == kResultOk);
	address.port = 81;

	static const int kTestTimeout = 2000;
	int64 start = System::GetSystemTicks ();
	AutoPtr<CCL::IStream> netStream = network.openStream (address, kTCP, kTestTimeout);
	int64 elapsed = System::GetSystemTicks () - start;

	CCL_TEST_ASSERT (netStream == nullptr); // an unreachable/black-holed peer must fail, not succeed
	CCL_TEST_ASSERT (elapsed < int64 (kTestTimeout) * 3);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (NetworkSuite, TestConnectCancel)
{
	INetwork& network = System::GetNetwork ();

	IPAddress address (kInternet);
	CCL_TEST_ASSERT (network.getAddressFromString (address, CCLSTR ("192.0.2.1")) == kResultOk);
	address.port = 81;

	TestCancelProgress alreadyCanceled (true);

	// a generous connect timeout, to prove cancellation
	static const int kGenerousTimeout = 30000;
	int64 start = System::GetSystemTicks ();
	AutoPtr<CCL::IStream> netStream = network.openStream (address, kTCP, kGenerousTimeout, &alreadyCanceled);
	int64 elapsed = System::GetSystemTicks () - start;

	CCL_TEST_ASSERT (netStream == nullptr);
	CCL_TEST_ASSERT (elapsed < 5000); // must return promptly on cancel, nowhere near the 30s timeout budget
}

//////////////////////////////////////////////////////////////////////////////////////////////////

// Dedicated hostnames for the timeout/cancel tests below, distinct from testServerName.
// ".invalid" is reserved by RFC 2606 to never resolve.
static const String testTimeoutHostName = CCLSTR ("networktest-timeout.invalid");
static const String testCancelHostName = CCLSTR ("networktest-cancel.invalid");

CCL_TEST (NetworkSuite, TestDnsResolveTimeout)
{
	// A 1ms timeout is shorter than any real DNS round trip, so this deterministically exercises
	// the bounded-wait path, proving getAddressByHost() returns promptly instead of blocking for
	// however long the underlying resolver call would otherwise take.
	INetwork& network = System::GetNetwork ();

	IPAddress address;
	int64 start = System::GetSystemTicks ();
	network.getAddressByHost (address, testTimeoutHostName, 1, nullptr);
	int64 elapsed = System::GetSystemTicks () - start;

	CCL_TEST_ASSERT (elapsed < 5000); // bounded regardless of outcome (success/failure/aborted)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (NetworkSuite, TestDnsResolveCancel)
{
	INetwork& network = System::GetNetwork ();

	IPAddress address;
	TestCancelProgress alreadyCanceled (true);

	static const uint32 kGenerousTimeout = 30000;
	int64 start = System::GetSystemTicks ();
	tresult result = network.getAddressByHost (address, testCancelHostName, kGenerousTimeout, &alreadyCanceled);
	int64 elapsed = System::GetSystemTicks () - start;

	CCL_TEST_ASSERT (result == kResultAborted || result == kResultFailed);
	CCL_TEST_ASSERT (elapsed < 5000); // must return promptly on cancel, nowhere near the 30s timeout budget
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (NetworkSuite, TestDiscovery)
{
	IDiscoveryHandler& handler = System::GetDiscoveryHandler ();

	// TODO: register callback!
}
