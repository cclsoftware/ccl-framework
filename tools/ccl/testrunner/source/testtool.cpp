//************************************************************************************************
//
// CCL Test Runner
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
// Filename    : testtool.cpp
// Description : Headless Test Tool
//
//************************************************************************************************

#include "testtool.h"

#include "ccl/base/unittest.h"

#include "ccl/extras/tools/testcollectionregistry.h"

#include "ccl/public/plugins/icoderesource.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/ipackagehandler.h"
#include "ccl/public/system/logging.h"
#include "ccl/public/plugins/classfactory.h"
#include "ccl/public/plugservices.h"
#include "ccl/public/securityservices.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//************************************************************************************************
// PluginUrlFilter
//************************************************************************************************

class PluginUrlFilter: public Object,
					   public IUrlFilter
{
public:
	PluginUrlFilter (UrlRef pluginUrl)
	: pluginUrl (pluginUrl)
	{}

	// IUrlFilter
	tbool CCL_API matches (UrlRef url) const override
	{
		return pluginUrl.isEqualUrl (url);
	}

	CLASS_INTERFACE (IUrlFilter, Object)

private:
	UrlRef pluginUrl;
};

//************************************************************************************************
// TestTool
//************************************************************************************************

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_ADD_TEST_COLLECTION (TestrunnerInternalTests)

//////////////////////////////////////////////////////////////////////////////////////////////////

TestTool::TestTool ()
: internalTestsLoaded (false)
{
	// Assign factory for strong content encryption
	System::GetPackageHandler ().setCryptoFactory (&System::GetCryptoFactory ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TestTool::~TestTool ()
{
	TestCollectionRegistry::instance ().unregisterTestPlugIns ();

	if(internalTestsLoaded)
	{
		AutoPtr<ClassFactory> classFactory = ClassFactory::instance ();
		System::GetPlugInManager ().unregisterFactory (classFactory);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TestTool::addPluginUrlFromPath (StringRef path)
{
	AutoPtr<Url> url (NEW Url ());
	url->fromDisplayString (path);
	makeAbsolute (*url, path, IUrl::kFile);

	if(!System::GetFileSystem ().fileExists (*url))
		return logFailure (*url);

	pluginUrls.add (url);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TestTool::loadPlugins ()
{
	for(auto& url : pluginUrls)
		loadPlugin (*url);

	TestCollectionRegistry::instance ().registerTestPlugIns ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TestTool::loadInternalTests ()
{
	AutoPtr<ClassFactory> classFactory = ClassFactory::instance ();
	CCL_REGISTER_TEST_COLLECTION (classFactory,
								  UID (0x70a34771, 0xb5fc, 0x9944, 0x8b, 0x5c, 0x16, 0x89, 0xae, 0xdf, 0x5e, 0x43),
								  TestrunnerInternalTests)
	if(System::GetPlugInManager ().registerFactory (classFactory) == kResultOk)
		internalTestsLoaded = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TestTool::loadPlugin (UrlRef url)
{
	Url parentFolder = url;
	parentFolder.ascend ();

	PluginUrlFilter filter (url);
	int numPlugins = System::GetPlugInManager ().scanFolder (parentFolder, CodeResourceType::kNative, PlugScanOption::kRecursive, nullptr, &filter);

	if(numPlugins > 0)
		logSuccess (url);
	else
		logFailure (url);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TestTool::logSuccess (UrlRef pluginUrl)
{
	String displayUrl = UrlDisplayString (pluginUrl);
	Logging::info ("Scanned plug-in %(1)", displayUrl);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TestTool::logFailure (UrlRef pluginUrl)
{
	String displayUrl = UrlDisplayString (pluginUrl);
	Logging::error ("Could not load plug-in from url '%(1)'", displayUrl);
}
