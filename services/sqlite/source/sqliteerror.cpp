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
// Filename    : sqliteerror.cpp
// Description : SQLite error logging
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "sqliteerror.h"
#include "ccl/public/base/debug.h"
#include "ccl/public/text/cstring.h"

#include "sqlite3.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

void Database::logError (sqlite3* connection, int code, const char* context)
{
	const char* msg = nullptr;
	if(code >= 0)
		msg = sqlite3_errstr (code);
	else
	{
		code = sqlite3_errcode (connection);
		msg = sqlite3_errmsg (connection);
	}
	if(code != SQLITE_OK && code < SQLITE_ROW)
	{
		MutableCString contextStr;
		if(context)
		{
			contextStr = "; in: ";
			contextStr.append (context);
		}
		CCL_PRINTF ("SQLite error (%d): %s%s\n", code, msg, contextStr.str ());
	}
}
