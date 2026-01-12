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
// Filename    : inittest.cpp
// Description : Test Initialization
//
//************************************************************************************************

#include "ccl/app/modulecomponent.h"
#include "ccl/base/unittest.h"
#include "ccl/public/plugins/classfactory.h"
#include "ccl/test/plugversion.h"
#include "testsplugin.h"

#include "ccl/test/coretestsuite.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Test Collection
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_ADD_TEST_COLLECTION(BuiltInTests)
CCL_ADD_CUSTOM_TEST_COLLECTION(CoreTests, CoreTestSuite)

//////////////////////////////////////////////////////////////////////////////////////////////////
// ccl_module_main
//////////////////////////////////////////////////////////////////////////////////////////////////

bool ccl_module_main (int reason)
{
	if(reason == kModuleInit)
	{
		NEW ModuleComponent (PLUG_ID, 0, PLUG_NAME);
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
	ClassFactory* classFactory = ClassFactory::instance ();

	VersionDesc version (CCL_PRODUCT_NAME,
						 CCL_VERSION_STRING,
						 CCL_AUTHOR_NAME,
						 CCL_AUTHOR_COPYRIGHT,
						 CCL_PRODUCT_WEBSITE);

	classFactory->setVersion (version);

	CCL_REGISTER_TEST_COLLECTION (classFactory,
								  UID (0xE6CCA7D0, 0x685B, 0x40E2, 0xB5, 0x14, 0x44, 0xD2, 0x0B, 0xD8, 0x6B, 0xFA),
								  BuiltInTests)

	CCL_REGISTER_TEST_COLLECTION (classFactory,
								  UID (0x7D210E10, 0x201A, 0x7B4E, 0x8E, 0x98, 0x83, 0xB3, 0x33, 0x97, 0xA0, 0xEC),
								  CoreTests)
	
	return classFactory;
}


//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL::TestsPlugin::registerClasses (CCL::ClassFactory& factory)
{
	CCL::ClassFactory* factoryPtr = &factory;
	CCL_REGISTER_TEST_COLLECTION (factoryPtr,
								  UID (0xE6CCA7D0, 0x685B, 0x40E2, 0xB5, 0x14, 0x44, 0xD2, 0x0B, 0xD8, 0x6B, 0xFA),
								  BuiltInTests)
	
	CCL_REGISTER_TEST_COLLECTION (factoryPtr,
								  UID (0x7D210E10, 0x201A, 0x7B4E, 0x8E, 0x98, 0x83, 0xB3, 0x33, 0x97, 0xA0, 0xEC),
								  CoreTests)
}
