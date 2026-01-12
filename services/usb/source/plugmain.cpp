//************************************************************************************************
//
// USB Support
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
// Description : USB Support Plug-in Entry
//
//************************************************************************************************

#include "usbhidstatics.h"
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

static ClassDesc usbhidStaticsClass
(
	// ClassID::UsbHidStatics
	UID (0x65849bb9, 0x630, 0x403f, 0xb0, 0x65, 0xf, 0x88, 0x5c, 0xd, 0x70, 0xc6),
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
		factory->registerClass (usbhidStaticsClass, MetaClass::createInstance, 
								const_cast<MetaClass*> (&ccl_typeid<CCL::Usb::UsbHidStatics> ()));
	}	
	return factory;
}
