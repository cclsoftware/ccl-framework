//************************************************************************************************
//
// CCL Spy
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
// Filename    : plugmain.cpp
// Description : Spy Plug-in Entry
//
//************************************************************************************************

#include "spyservice.h"
#include "plugversion.h"

#include "ccl/base/development.h"
#include "ccl/base/storage/url.h"
#include "ccl/base/storage/attributes.h"

#include "ccl/app/modulecomponent.h"

#include "ccl/public/plugins/classfactory.h"
#include "ccl/public/plugins/icomponent.h"
#include "ccl/public/plugservices.h"
#include "ccl/public/cclversion.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Version
//////////////////////////////////////////////////////////////////////////////////////////////////

static VersionDesc version 
(
	PLUG_NAME,
	CCL_VERSION_STRING,
	CCL_AUTHOR_NAME,
	CCL_AUTHOR_COPYRIGHT,
	CCL_PRODUCT_WEBSITE
);

//////////////////////////////////////////////////////////////////////////////////////////////////
// Exported classes
//////////////////////////////////////////////////////////////////////////////////////////////////

static ClassDesc serviceClass
(
	PLUG_CLASS_UID,				// unique class id
	PLUG_CATEGORY_USERSERVICE,	// category
	PLUG_NAME,					// plugin name
	String (),
	String (PLUG_NAME " Development Tool")
);

//////////////////////////////////////////////////////////////////////////////////////////////////
// ccl_module_main
//////////////////////////////////////////////////////////////////////////////////////////////////

bool ccl_module_main (int reason)
{
	if(reason == kModuleInit)
	{
		Url defaultPath;
		GET_DEVELOPMENT_FOLDER_LOCATION (defaultPath, CCL_FRAMEWORK_DIRECTORY "services", "cclspy/skin")
		return (NEW ModuleComponent (PLUG_ID, nullptr, PLUG_NAME))->loadTheme (defaultPath);
	}
	else
		return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// CCLGetClassFactory
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT IClassFactory* CCL_API CCLGetClassFactory ()
{
	ClassFactory* factory = ClassFactory::instance ();
	if(factory->isEmpty ())
	{
		factory->setVersion (version);

		AutoPtr<Attributes> attributes = NEW Attributes;
		attributes->set (Meta::kServicePriority, 1); // start before other services
		factory->registerClass (serviceClass, SpyService::createInstance, nullptr, attributes);
	}
	
	return factory;
}
