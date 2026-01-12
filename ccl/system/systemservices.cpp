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
// Filename    : ccl/system/systemservices.cpp
// Description : System Service APIs
//
//************************************************************************************************

#include "ccl/base/kernel.h"

#include "ccl/system/persistence/persistentstore.h"
#include "ccl/system/localization/localemanager.h"
#include "ccl/system/packaging/packagehandler.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/text/translation.h"
#include "ccl/public/system/iexecutable.h"
#include "ccl/public/plugins/itypelibregistry.h"
#include "ccl/public/plugins/classfactory.h"
#include "ccl/public/plugservices.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/cclversion.h"

#if CCL_PLATFORM_WINDOWS
#include "ccl/platform/win/system/cclcoinit.h"
#endif

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Initialization
//////////////////////////////////////////////////////////////////////////////////////////////////

static bool _InitializeSystemFramework (bool state)
{
	if(state)
	{
		#if CCL_PLATFORM_WINDOWS
		System::CoWinRTInitialize ();
		#endif

		// force creation of package handler before locale manager to avoid singleton recreation on exit
		PackageHandler::instance ();

		ITranslationTable* table = nullptr;
		LocaleManager::instance ().initialize ();
		LocaleManager::instance ().loadModuleStrings (table, System::GetCurrentModuleRef (), CSTR ("cclsystem"));
		LocalString::setTable (table);
	}
	else
	{
		if(LocalString::hasTable ())
		{
			System::GetLocaleManager ().unloadStrings (LocalString::getTable ());
			LocalString::tableDestroyed ();
		}

		#if CCL_PLATFORM_WINDOWS
		System::CoWinRTUninitialize ();
		#endif
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

#if CCL_STATIC_LINKAGE
tbool CCL_API System::InitializeSystemFramework (tbool state)
{
	Persistence::PersistentStore::forceLinkage ();

	return _InitializeSystemFramework (state != 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

#else
CCL_KERNEL_INIT_LEVEL (SystemClasses, kFrameworkLevelFirst)
{
	_InitializeSystemFramework (true);

	// register type libraries
	MetaClassRegistry& typeLib = Kernel::instance ().getClassRegistry ();
	typeLib.setLibName (CCLSYSTEM_FILE_DESCRIPTION);
	System::GetTypeLibRegistry ().registerTypeLib (typeLib);

	ClassFactory* classFactory = ClassFactory::instance ();
	VersionDesc version (CCL_PRODUCT_NAME,
						 CCL_VERSION_STRING,
						 CCL_AUTHOR_NAME,
						 CCL_AUTHOR_COPYRIGHT,
						 CCL_PRODUCT_WEBSITE);

	classFactory->setVersion (version);

	// classes exported from cclsystem must have category "System"
	Kernel::instance ().registerPublicClasses (*classFactory, "System");

	System::GetPlugInManager ().registerFactory (classFactory);
	classFactory->release ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_TERM_LEVEL (SystemClasses, kFrameworkLevelFirst)
{
	ClassFactory* classFactory = ClassFactory::instance ();
	System::GetPlugInManager ().unregisterFactory (classFactory);
	classFactory->release ();

	// unregister type libraries
	System::GetTypeLibRegistry ().unregisterTypeLib (Kernel::instance ().getClassRegistry ());

	System::GetExecutableLoader ().removeNativeImage (System::GetCurrentModuleRef ());

	_InitializeSystemFramework (false);
}
#endif // !CCL_STATIC_LINKAGE
