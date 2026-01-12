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
// Filename    : cclstatic.cpp
// Description : Framework Initialization (static linkage)
//
//************************************************************************************************

#include "ccl/main/cclstatic.h"

#include "ccl/base/kernel.h"

#include "ccl/public/plugins/classfactory.h"
#include "ccl/public/plugservices.h"

#if CCL_STATIC_ENABLE_SYSTEM
#include "ccl/public/systemservices.h"
#endif
#if CCL_STATIC_ENABLE_NETWORK
#include "ccl/public/netservices.h"
#endif
#if CCL_STATIC_ENABLE_GUI
#include "ccl/public/guiservices.h"
#endif

#include "ccl/public/cclversion.h"

using namespace CCL;

//************************************************************************************************
// FrameworkInitializerStatic
//************************************************************************************************

FrameworkInitializerStatic::FrameworkInitializerStatic ()
: success (false),
  classFactory (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FrameworkInitializerStatic::initializeFrameworkLevel ()
{
	success = 
#if CCL_STATIC_ENABLE_SYSTEM
	System::InitializeSystemFramework (true) &&
#endif 
#if CCL_STATIC_ENABLE_NETWORK
	System::InitializeNetworkFramework (true) &&
#endif
#if CCL_STATIC_ENABLE_GUI
	System::InitializeGUIFramework (true) &&
#endif
	Kernel::instance ().initializeLevel (kFirstRun);
	
	if(success)
		registerClasses ();	

	return success;	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FrameworkInitializerStatic::terminateApplicationLevel ()
{
	Kernel::instance ().terminateLevel (kFirstRun);

	unregisterClasses ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FrameworkInitializerStatic::terminateFrameworkLevel ()
{
	if(!success)
		return;
	
	Kernel::instance ().terminate ();
	
#if CCL_STATIC_ENABLE_GUI
	System::InitializeGUIFramework (false);
#endif
#if CCL_STATIC_ENABLE_NETWORK
	System::InitializeNetworkFramework (false);
#endif
#if CCL_STATIC_ENABLE_SYSTEM
	System::InitializeSystemFramework (false);
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FrameworkInitializerStatic::registerClasses ()
{
	classFactory = NEW ClassFactory;
	classFactory->setVersion (VersionDesc (CCL_PRODUCT_NAME, CCL_VERSION_STRING, CCL_AUTHOR_NAME, CCL_AUTHOR_COPYRIGHT, CCL_PRODUCT_WEBSITE));
	Kernel::instance ().registerPublicClasses (*classFactory); // CCL namespace
#if CCL_STATIC_ENABLE_SYSTEM
	Kernel::instance ().registerPublicClasses (*classFactory, "System");
#endif
#if CCL_STATIC_ENABLE_NETWORK
	Kernel::instance ().registerPublicClasses (*classFactory, "Network");
#endif
	System::GetPlugInManager ().registerFactory (classFactory);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FrameworkInitializerStatic::unregisterClasses ()
{
	if(classFactory)
		System::GetPlugInManager ().unregisterFactory (classFactory);
	safe_release (classFactory);
}
