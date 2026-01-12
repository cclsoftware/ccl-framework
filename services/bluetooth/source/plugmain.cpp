//************************************************************************************************
//
// Bluetooth Support
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
// Description : Bluetooth Support Plug-in Entry
//
//************************************************************************************************

#include "bluetoothstatics.h"
#include "plugversion.h"

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

static ClassDesc bluetoothStaticsClass
(
	// ClassID::BluetoothStatics
	UID (0xa7b62c37, 0xe5d0, 0x4411, 0x8e, 0x9d, 0x1c, 0x50, 0xd, 0x73, 0x48, 0xdb),
	PLUG_CATEGORY_COMPONENT,
	PLUG_NAME,
	0, 0, ClassDesc::kSingleton
);

//////////////////////////////////////////////////////////////////////////////////////////////////
// ccl_module_main
//////////////////////////////////////////////////////////////////////////////////////////////////

bool ccl_module_main (int reason)
{
	if(reason == kModuleInit)
	{
		NEW ModuleComponent (PLUG_ID, VENDOR_NAME, PLUG_NAME);
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
		factory->registerClass (bluetoothStaticsClass, MetaClass::createInstance, 
								const_cast<MetaClass*> (&ccl_typeid<CCL::Bluetooth::BluetoothStatics> ()));
	}	
	return factory;
}
