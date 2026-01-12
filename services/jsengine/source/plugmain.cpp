//************************************************************************************************
//
// JavaScript Engine
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
// Description : JavaScript Plug-in Entry
//
//************************************************************************************************

#include "jsengine.h"
#include "plugversion.h"

#include "ccl/base/unittest.h"
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
	PLUG_CLASS_UID,
	PLUG_CATEGORY_SCRIPTENGINE,
	PLUG_NAME
);

//////////////////////////////////////////////////////////////////////////////////////////////////
// Test Factory
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_ADD_TEST_COLLECTION (JScriptTests)

//////////////////////////////////////////////////////////////////////////////////////////////////
// CCLGetClassFactory
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT IClassFactory* CCL_API CCLGetClassFactory ()
{
	ClassFactory* factory = ClassFactory::instance ();
	if(factory->isEmpty ())
	{
		factory->setVersion (version);
		factory->registerClass (serviceClass, JScript::Engine::createInstance);

		#if DEBUG
		CCL_REGISTER_TEST_COLLECTION (factory,
									  UID (0x704C83F3, 0x6C5A, 0x4EB2, 0x89, 0xCC, 0x7A, 0xE7, 0xA3, 0xCA, 0xC8, 0x41),
									  JScriptTests)
		#endif
	}
	return factory;
}
