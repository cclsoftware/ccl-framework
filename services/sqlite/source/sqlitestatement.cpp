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
// Filename    : sqlitestatement.cpp
// Description : SQLite statement
//
//************************************************************************************************

#define DEBUG_LOG 0
#define LOG_ALL_SQL (0 && DEBUG_LOG)

#include "sqlitestatement.h"
#include "sqliteconnection.h"
#include "sqliteerror.h"

#include "ccl/public/base/variant.h"
#include "ccl/public/base/memorystream.h"

#include "sqlite3.h"

using namespace CCL;
using namespace Database;

//////////////////////////////////////////////////////////////////////////////////////////////////

#if LOG_ERRORS
#define LOG_ERROR1(code) if(code != SQLITE_OK) logError (sqlite3_db_handle (statement), code, sql);
#else
#define LOG_ERROR1(code)
#endif

#define LOG_ERROR LOG_ERROR1(-1)

#if LOG_ALL_SQL
#define LOG_SQL CCL_PRINTF ("%s\n", sql);
#else
#define LOG_SQL
#endif

//************************************************************************************************
// SQLiteStatement
//************************************************************************************************

SQLiteStatement::SQLiteStatement (SQLiteConnection& connection, StringRef sqlString)
: SQLiteStatement (connection, MutableCString (sqlString, Text::kUTF8))
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

SQLiteStatement::SQLiteStatement (SQLiteConnection& connection, const char* sqlStringUTF8)
: statement (nullptr),
  wasExecuted (false)
{
	int code = sqlite3_prepare (connection.connection, sqlStringUTF8, -1, &statement, nullptr);

	sql = sqlStringUTF8;
	LOG_ERROR1 (code)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SQLiteStatement::~SQLiteStatement ()
{
	sqlite3_finalize (statement);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int SQLiteStatement::retryStep (int errorCode)
{
	// handles an expired statement, when schema change was reported by sqlite3_step, as suggested in the docu
	if(errorCode == SQLITE_ERROR)
	{
		errorCode = sqlite3_reset (statement);
		if(errorCode == SQLITE_SCHEMA)
		{
			// prepared statement has expired: reprepare and try again
			sqlite3* connection = sqlite3_db_handle (statement);

			sqlite3_stmt* newStatement = nullptr;
			errorCode = sqlite3_prepare (connection, sql, -1, &newStatement, nullptr);
			LOG_ERROR1 (errorCode)

			// transfer variable bindings to new statemnt
			errorCode = sqlite3_transfer_bindings (statement, newStatement);
			LOG_ERROR1 (errorCode)

			sqlite3_finalize (statement);
			statement = newStatement;

			// note: this starts at the first result row ...
			errorCode = sqlite3_step (statement);
			#if DEBUG_LOG
			if(errorCode >= SQLITE_ROW)
			{
				CCL_PRINTF ("Reprepared statement: %s\n", sql.str ())
			}
			#endif
		}
	}
	LOG_ERROR1 (errorCode)
	return errorCode;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SQLiteStatement::checkReset ()
{
	// reset statement if sqlite_step was called since the last reset or prepare
	if(wasExecuted)
	{
		sqlite3_reset (statement);
		wasExecuted = false;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SQLiteStatement::bindVariable (int index, CCL::VariantRef value)
{
	checkReset ();

	switch(value.getType ())
	{
		case Variant::kInt:
			sqlite3_bind_int64 (statement, index + 1, value.lValue);
			break;
		case Variant::kFloat:
			sqlite3_bind_double (statement, index + 1, value.fValue);
			break;
		case Variant::kString:
		{
			StringChars chars (value.asString ()); // hmm, copied twice...
			sqlite3_bind_text16 (statement, index + 1, chars, -1, SQLITE_TRANSIENT);
			break;
		}
		case Variant::kObject:
		{
			UnknownPtr<IMemoryStream> ms (value.asUnknown ());
			if(ms)
				sqlite3_bind_blob (statement, index + 1, ms->getMemoryAddress (), ms->getBytesWritten (), SQLITE_TRANSIENT);
			else
			{
				//CCL_DEBUGGER ("Can not bind object!")
				sqlite3_bind_null (statement, index + 1);
			}
		}

		default:
			break;
	}
	LOG_ERROR
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SQLiteStatement::bindVariable (int index, const IMemoryStream& blob)
{
	checkReset ();
	sqlite3_bind_blob (statement, index + 1, blob.getMemoryAddress (), blob.getBytesWritten (), SQLITE_TRANSIENT);
	LOG_ERROR
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SQLiteStatement::bindVariable (int index, StringRef value)
{
	checkReset ();
	StringChars chars (value); // hmm, copied twice...
	sqlite3_bind_text16 (statement, index + 1, chars, -1, SQLITE_TRANSIENT);
	LOG_ERROR
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SQLiteStatement::bindVariable (int index, const char* value)
{
	checkReset ();
	sqlite3_bind_text (statement, index + 1, value, -1, SQLITE_TRANSIENT);
	LOG_ERROR
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SQLiteStatement::bindVariable (int index, int64 value)
{
	checkReset ();
	sqlite3_bind_int64 (statement, index + 1, value);
	LOG_ERROR
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SQLiteStatement::bindVariable (int index, double value)
{
	checkReset ();
	sqlite3_bind_double (statement, index + 1, value);
	LOG_ERROR
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SQLiteStatement::unbindVariable (int index)
{
	checkReset ();
	sqlite3_bind_null (statement, index + 1);
	LOG_ERROR
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SQLiteStatement::unbindVariables ()
{
	checkReset ();

	sqlite3_clear_bindings (statement);
	LOG_ERROR
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SQLiteStatement::execute ()
{
	checkReset ();
	wasExecuted = true;

	LOG_SQL
	int errorCode = sqlite3_step (statement);
	if(errorCode >= SQLITE_ROW)
		return true;

	errorCode = retryStep (errorCode);
	return errorCode >= SQLITE_ROW;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SQLiteStatement::execute (Variant& result)
{
	checkReset ();
	wasExecuted = true;

	LOG_SQL
	int errorCode = sqlite3_step (statement);
	LOG_ERROR
	if(errorCode == SQLITE_ROW)
		return getValue (0, result);

	errorCode = retryStep (errorCode);
	if(errorCode == SQLITE_ROW)
		return getValue (0, result);

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SQLiteStatement::execute (IResultSet*& resultSet)
{
	checkReset ();
	wasExecuted = true;

	retain ();
	resultSet = this; // we also implement IResultSet
	LOG_SQL
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 CCL_API SQLiteStatement::executeInsert ()
{
	checkReset ();
	wasExecuted = true;

	LOG_SQL
	int errorCode = sqlite3_step (statement);
	if(errorCode >= SQLITE_ROW)
		return sqlite3_last_insert_rowid (sqlite3_db_handle (statement));

	// try again
	errorCode = retryStep (errorCode);
	if(errorCode >= SQLITE_ROW)
		return sqlite3_last_insert_rowid (sqlite3_db_handle (statement));
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API SQLiteStatement::countColumns ()
{
	return sqlite3_column_count (statement);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const char* CCL_API SQLiteStatement::getColumnName (int index)
{
	return sqlite3_column_name (statement, index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API SQLiteStatement::getColumnIndex (const char* columnName)
{
	int numColumns = sqlite3_column_count (statement);
	for(int i = 0; i < numColumns; i++)
		if(strcmp (sqlite3_column_name (statement, i), columnName) == 0)
			return i;

	LOG_ERROR
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SQLiteStatement::nextRow ()
{
	int errorCode = sqlite3_step (statement);
	if(errorCode == SQLITE_ROW)
		return true;

	// try again
	errorCode = retryStep (errorCode);
	return errorCode == SQLITE_ROW;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SQLiteStatement::getValue (int column, Variant& value)
{
	switch(sqlite3_column_type (statement, column))
	{
		case SQLITE_INTEGER:
			value = sqlite3_column_int64 (statement, column);
			LOG_ERROR
			return true;
		case SQLITE_FLOAT:
			value = sqlite3_column_double (statement, column);
			LOG_ERROR
			return true;
		case SQLITE_TEXT:
		{
			String str ((const uchar*)sqlite3_column_text16(statement, column));
			LOG_ERROR
			value = str;
			value.share ();
			return true;
		}
		case SQLITE_BLOB:
		{
			const void* address = sqlite3_column_blob (statement, column);
			int size = sqlite3_column_bytes (statement, column);
			AutoPtr<MemoryStream> ms = NEW MemoryStream;
			ms->copyFrom (MemoryStream ((void*)address, (unsigned int)size));
			value.takeShared (static_cast<IMemoryStream*> (ms));
			return true;
		}
		case SQLITE_NULL:
		default:
			value.clear ();
			break;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 CCL_API SQLiteStatement::getIntValue (int column)
{
	return sqlite3_column_int64 (statement, column);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

double CCL_API SQLiteStatement::getFloatValue (int column)
{
	return sqlite3_column_double (statement, column);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SQLiteStatement::getStringValue (int column, String& string)
{
	string = (const uchar*)sqlite3_column_text16 (statement, column);
	LOG_ERROR
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const char* CCL_API SQLiteStatement::getStringValue (int column)
{
	const char* string = (const char*)sqlite3_column_text (statement, column);
	LOG_ERROR
	return string;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SQLiteStatement::isNull (int column)
{
	return sqlite3_column_type (statement, column) == SQLITE_NULL;
}
