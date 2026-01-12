//************************************************************************************************
//
// TUIO Support
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
// Filename    : plugversion.h
// Description : Plug-in Entry
//
//************************************************************************************************

#include "tuioservice.h"

#include "plugversion.h"

#include "ccl/base/development.h"
#include "ccl/base/storage/url.h"

#include "ccl/app/modulecomponent.h"

#include "ccl/public/plugins/classfactory.h"

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
	UID (0xc7dbe519, 0x1439, 0x45ad, 0x82, 0xe4, 0x18, 0xcc, 0xf3, 0xe6, 0xc3, 0x53),
	PLUG_CATEGORY_USERSERVICE,
	PLUG_NAME,
	String (),
	/* BEGIN_XSTRINGS "TUIO" */
	/* XSTRING */String ("Support for multi-touch via TUIO protocol")
	/* END_XSTRINGS */
);

//////////////////////////////////////////////////////////////////////////////////////////////////
// ccl_module_main
//////////////////////////////////////////////////////////////////////////////////////////////////

bool ccl_module_main (int reason)
{
	if(reason == kModuleInit)
	{
		Url defaultPath;
		GET_DEVELOPMENT_FOLDER_LOCATION (defaultPath, CCL_FRAMEWORK_DIRECTORY "services", "tuio/skin")
		return (NEW ModuleComponent (PLUG_ID, nullptr, PLUG_NAME))->loadTheme (defaultPath);		
	}
	else
		return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// CCLGetClassFactory
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT CCL::IClassFactory* CCL_API CCLGetClassFactory ()
{
	ClassFactory* factory = ClassFactory::instance ();
	if(factory->isEmpty ())
	{
		factory->setLocalizationEnabled (true);
		factory->setVersion (version);
		factory->registerClass (serviceClass, PluginConstructor<TUIOService, IComponent>::createInstance);
	}
	return factory;
}
