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
// Filename    : ccl/network/netservices.cpp
// Description : Network Service APIs
//
//************************************************************************************************

#include "core/network/corediscovery.h"

#include "ccl/base/kernel.h"
#include "ccl/public/plugins/classfactory.h"

#include "ccl/main/cclmodmain.h"

#include "ccl/network/network.h"
#include "ccl/network/netdiscovery.h"
#include "ccl/network/web/http/client.h"

#include "ccl/public/system/iexecutable.h"
#include "ccl/public/plugins/iscriptingmanager.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/plugservices.h"
#include "ccl/public/netservices.h"
#include "ccl/public/cclversion.h"

using namespace CCL;
using namespace Net;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Globals
//////////////////////////////////////////////////////////////////////////////////////////////////

static Network* theNetwork = nullptr;
static DiscoveryHandler* theDiscoverHandler = nullptr;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Basic Network APIs
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT INetwork& CCL_API System::CCL_ISOLATED (GetNetwork) ()
{
	if(!theNetwork)
		theNetwork = NEW Network;
	return *theNetwork;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT IDiscoveryHandler& CCL_API System::CCL_ISOLATED (GetDiscoveryHandler) ()
{
	if(!theDiscoverHandler)
		theDiscoverHandler = NEW DiscoveryHandler;
	return *theDiscoverHandler;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Initialization
//////////////////////////////////////////////////////////////////////////////////////////////////

static bool _InitializeNetworkFramework (bool state)
{
	if(state)
	{
		System::GetNetwork ();
		ASSERT (theNetwork != nullptr)
		if(!theNetwork->startup ())
		{
			safe_release (theNetwork);
			return false;
		}

		// make sure singleton exists
		Web::HTTP::ConnectionManager::instance ();
	}
	else
	{
		// make sure persistent connections are closed
		Web::HTTP::ConnectionManager::instance ().terminate ();

		if(theNetwork)
			theNetwork->shutdown ();

		safe_release (theNetwork);
		safe_release (theDiscoverHandler);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

#if CCL_STATIC_LINKAGE
tbool CCL_API System::InitializeNetworkFramework (tbool state)
{
	return _InitializeNetworkFramework (state != 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

#else
CCL_KERNEL_INIT_LEVEL (NetworkClasses, kFrameworkLevelFirst)
{
	ClassFactory* classFactory = ClassFactory::instance ();
	VersionDesc version (CCL_PRODUCT_NAME,
						 CCL_VERSION_STRING,
						 CCL_AUTHOR_NAME,
						 CCL_AUTHOR_COPYRIGHT,
						 CCL_PRODUCT_WEBSITE);

	classFactory->setVersion (version);

	// classes exported from cclnet must have namespace "Network"
	Kernel::instance ().registerPublicClasses (*classFactory, "Network");

	System::GetPlugInManager ().registerFactory (classFactory);
	classFactory->release ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_TERM_LEVEL (NetworkClasses, kFrameworkLevelFirst)
{
	ClassFactory* classFactory = ClassFactory::instance ();
	System::GetPlugInManager ().unregisterFactory (classFactory);
	classFactory->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Main Entry
//////////////////////////////////////////////////////////////////////////////////////////////////

bool ccl_module_main (int reason)
{
	// *** Module Init ***
	if(reason == kModuleInit)
	{
		// startup networking
		if(!_InitializeNetworkFramework (true))
			return false;

		System::GetExecutableLoader ().addNativeImage (System::GetCurrentModuleRef ());
		System::GetScriptingManager ().startup (CCLNET_PACKAGE_ID, System::GetCurrentModuleRef (), nullptr, false);
	}
	// *** Module Exit ***
	else if(reason == kModuleExit)
	{
		// shutdown networking
		_InitializeNetworkFramework (false);

		System::GetScriptingManager ().shutdown (System::GetCurrentModuleRef (), false);
		System::GetExecutableLoader ().removeNativeImage (System::GetCurrentModuleRef ());
	}
	return true;
}
#endif // !CCL_STATIC_LINKAGE
