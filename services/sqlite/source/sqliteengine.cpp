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
// Filename    : sqliteengine.cpp
// Description : SQLite database engine
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "sqliteengine.h"
#include "sqliteconnection.h"
#include "sqliteerror.h"

#include "ccl/public/storage/iurl.h"
#include "ccl/public/text/cstring.h"

#include "sqlite3.h"

using namespace CCL;
using namespace Database;

//************************************************************************************************
// SQLiteEngine
//************************************************************************************************

IUnknown* SQLiteEngine::createInstance (UIDRef, void*)
{
	return (IDatabaseEngine*)NEW SQLiteEngine;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SQLiteEngine::SQLiteEngine ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

SQLiteEngine::~SQLiteEngine ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

IConnection* CCL_API SQLiteEngine::createConnection (UrlRef url)
{
	NativePath nativePath (url);

	sqlite3* connection = nullptr;
	int code = sqlite3_open16 (nativePath, &connection);
	if(code == SQLITE_OK)
		return NEW SQLiteConnection (connection);

	#if DEBUG_LOG
	MutableCString msg ("sqlite3_open16: "); 
	String urlStr;
	url.getUrl (urlStr);
	msg.append (urlStr);
	logError (connection, code, msg);
	#endif

	return nullptr;
}
