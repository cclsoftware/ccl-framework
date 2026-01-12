//************************************************************************************************
//
// 3D Model Importer Library
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
// Description : Plug-in Entry
//
//************************************************************************************************

#include "plugversion.h"
#include "modelimporter.h"

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
// ccl_module_main
//////////////////////////////////////////////////////////////////////////////////////////////////

bool ccl_module_main (int reason)
{
	if(reason == kModuleInit)
	{
		NEW ModuleComponent (PLUG_ID, nullptr, PLUG_NAME);
		return true;
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
		//factory->setLocalizationEnabled (true);
		
		factory->registerClass (OBJImporter::getDescription (), PluginConstructor<OBJImporter, IModelImporter3D>::createInstance);
	}	
	return factory;
}
