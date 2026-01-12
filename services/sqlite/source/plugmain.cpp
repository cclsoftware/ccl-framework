//************************************************************************************************
//
// SQLite Database Engine
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
// Description : SQLite Plug-in Entry
//
//************************************************************************************************

#include "sqliteengine.h"
#include "plugversion.h"

#include "ccl/base/unittest.h"
#include "ccl/public/plugins/classfactory.h"

using namespace CCL;
using namespace Database;

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

static ClassDesc engineClass
(
	PLUG_CLASS_UID,
	PLUG_CATEGORY_DATABASEENGINE,
	PLUG_NAME
);

//////////////////////////////////////////////////////////////////////////////////////////////////
// Test Factory
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_ADD_TEST_COLLECTION(SQLiteTests)

//////////////////////////////////////////////////////////////////////////////////////////////////
// CCLGetClassFactory
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT IClassFactory* CCL_API CCLGetClassFactory ()
{
	ClassFactory* factory = ClassFactory::instance ();
	if(factory->isEmpty ())
	{
		factory->setVersion (version);
		factory->registerClass (engineClass, SQLiteEngine::createInstance);

		#if DEBUG
		CCL_REGISTER_TEST_COLLECTION (factory,
									  UID (0xF2DB416C, 0x1626, 0x404C, 0x9B, 0xA1, 0xD2, 0x96, 0xED, 0xBE, 0xDC, 0x3B),
									  SQLiteTests)
		#endif
	}
	return factory;
}
