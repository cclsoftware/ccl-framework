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
// Filename    : sqliteconnection.cpp
// Description : SQLite database connection
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "sqliteconnection.h"
#include "sqlitestatement.h"
#include "sqliteerror.h"

#include "ccl/public/base/variant.h"

#include "sqlite3.h"

using namespace CCL;
using namespace Database;

//************************************************************************************************
// SQLiteConnection
//************************************************************************************************

SQLiteConnection::SQLiteConnection (sqlite3* connection)
: connection (connection),
  transactions (0)
{
	ASSERT (connection)
	sqlite3_busy_timeout (connection, 10000);

	#if LOG_ERRORS
	logError (connection, -1, "SQLiteConnection ()");
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SQLiteConnection::~SQLiteConnection ()
{
	sqlite3_close (connection);
	ASSERT (transactions == 0)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStatement* CCL_API SQLiteConnection::createStatement (StringRef sql)
{
	return NEW SQLiteStatement (*this, sql);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStatement* CCL_API SQLiteConnection::createStatement (const char* sql)
{
	return NEW SQLiteStatement (*this, sql);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SQLiteConnection::execute (StringRef sql)
{
	SQLiteStatement statement (*this, sql);
	return statement.execute ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SQLiteConnection::execute (const char* sqlUTF8)
{
	SQLiteStatement statement (*this, sqlUTF8);
	return statement.execute ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SQLiteConnection::execute (StringRef sql, Variant& result)
{
	SQLiteStatement statement (*this, sql);
	return statement.execute (result);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SQLiteConnection::execute (const char* sqlUTF8, Variant& result)
{
	SQLiteStatement statement (*this, sqlUTF8);
	return statement.execute (result);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SQLiteConnection::beginTransaction ()
{
	// SQLite doesn't support nested transactions
	if(transactions == 0)
	{
		SQLiteStatement statement (*this, "begin");
		if(!statement.execute ())
			return false;
	}
	transactions++;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SQLiteConnection::commitTransaction ()
{
	transactions--;
	if(transactions == 0)
	{
		//SQLiteStatement statement (*this, "commit");
		SQLiteStatement statement (*this, "end");
		return statement.execute ();
	}
	else if(transactions > 0)
		return true;

	transactions = 0;
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SQLiteConnection::hasTable (const char* name)
{
	SQLiteStatement statement (*this, "select name from sqlite_master where type='table' and name=?");
	statement.bindVariable (0, name);

	Variant result;
	return statement.execute (result);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SQLiteConnection::hasColumn (const char* table, const char* column)
{
	SQLiteStatement statement (*this, "select sql from sqlite_master where type='table' and name=?");
	statement.bindVariable (0, table);

	Variant result;
	if(statement.execute (result))
	{
		// search for column name in the create table sql string
		String createTableString = result.asString ();
		return createTableString.contains (String (",") << column << " ") || createTableString.contains (String ("(") << column << " ");
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SQLiteConnection::hasView (const char* name)
{
	SQLiteStatement statement (*this, "select name from sqlite_master where type='view' and name=?");
	statement.bindVariable (0, name);

	Variant result;
	return statement.execute (result);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SQLiteConnection::getLastError (String& message)
{
	int code = sqlite3_errcode (connection);
	if(code != SQLITE_OK && code < SQLITE_ROW)
	{
		message = (const uchar*)sqlite3_errmsg16 (connection);
		message.append ("(");
		message.appendIntValue (code);
		message.append (")");
		return true;
	}
	return false;
}
