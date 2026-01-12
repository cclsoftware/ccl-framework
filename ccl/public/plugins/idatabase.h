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
// Filename    : ccl/public/plugins/idatabase.h
// Description : Database Interfaces
//
//************************************************************************************************

#ifndef _idatabase_h
#define _idatabase_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Class category for database engines. */
#define PLUG_CATEGORY_DATABASEENGINE CCLSTR ("DatabaseEngine")

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID
{
	/** SQLite database engine class identifer. */
	DEFINE_CID (SQLite, 0xDA833DD2, 0x7AC8, 0x423F, 0x89, 0xB7, 0xE9, 0xB5, 0x49, 0x0B, 0x04, 0x90);
}

interface IMemoryStream;

namespace Database {

interface IConnection;
interface IStatement;
interface IResultSet;

//************************************************************************************************
// IDatabaseEngine
/** A database engine can create connections to databases. */
//************************************************************************************************

interface IDatabaseEngine: IUnknown
{
	/** Create a database connection. The connection will be closed when the connection object is destroyed. */
	virtual IConnection* CCL_API createConnection (UrlRef url) = 0;

	DECLARE_IID (IDatabaseEngine)
};

DEFINE_IID (IDatabaseEngine, 0xDA2F2F30, 0x5408, 0x4E86, 0xA9, 0x55, 0x2F, 0x93, 0xB2, 0xE2, 0x66, 0xC8)

//************************************************************************************************
// IConnection
/** Database connection interface. */
//************************************************************************************************

interface IConnection: IUnknown
{
	//////////////////////////////////////////////////////////////////////////////////////////////
	// Create SQL statements
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Create a statement object from a CCL String. */
	virtual IStatement* CCL_API createStatement (StringRef sql) = 0;

	/** Create a statement object from an UTF8 string. */
	virtual IStatement* CCL_API createStatement (const char* sqlUTF8) = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Execute SQL statements with no variables
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Execute an SQL statement that has no result data. */
	virtual tbool CCL_API execute (StringRef sql) = 0;

	/** Execute an SQL statement (UTF8) that has no result data. */
	virtual tbool CCL_API execute (const char* sqlUTF8) = 0;

	/** Execute an SQL statement with a single result. */
	virtual tbool CCL_API execute (StringRef sql, Variant& result) = 0;

	/** Execute an SQL statement (UTF8) with a single result. */
	virtual tbool CCL_API execute (const char* sqlUTF8, Variant& result) = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Transactions
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Begin a transaction. */
	virtual tbool CCL_API beginTransaction () = 0;

	/** Commit a transaction. */
	virtual tbool CCL_API commitTransaction () = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Methods
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Check if the database has a table with that name. */
	virtual tbool CCL_API hasTable (const char* name) = 0;

	/** Check if the database has a table with the given column. */
	virtual tbool CCL_API hasColumn (const char* table, const char* column) = 0;

	/** Check if the database has a table with that name. */
	virtual tbool CCL_API hasView (const char* name) = 0;

	/** Get description of the last error. Returns true if there was an error. */
	virtual tbool CCL_API getLastError (String& message) = 0;

	DECLARE_IID (IConnection)
};

DEFINE_IID (IConnection, 0xA0E20A81, 0xE415, 0x424A, 0x96, 0x7D, 0x95, 0x81, 0x1C, 0xE6, 0x58, 0xED)

//************************************************************************************************
// IStatement
/** An sql statement can be a command like insert, update, delete or a query (select).
	The sql string (passed to IConnection::createStatement) can contain variables ("?") that can
	be bound to values later.  */
//************************************************************************************************

interface IStatement: IUnknown
{
	//////////////////////////////////////////////////////////////////////////////////////////////
	// Bind Variables
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Bind a variable to a value. Index starts at 0. */
	virtual void CCL_API bindVariable (int index, VariantRef value) = 0;

	/** Bind a variable to a value. Index starts at 0. */
	virtual void CCL_API bindVariable (int index, StringRef value) = 0;

	/** Bind a variable to a value. Index starts at 0. */
	virtual void CCL_API bindVariable (int index, const char* string) = 0;

	/** Bind a variable to a value. Index starts at 0. */
	virtual void CCL_API bindVariable (int index, int64 value) = 0;

	/** Bind a variable to a value. Index starts at 0. */
	virtual void CCL_API bindVariable (int index, double value) = 0;

	/** Bind a variable to a blob value. Index starts at 0. */
	virtual void CCL_API bindVariable (int index, const IMemoryStream& blob) = 0;

	/** Unbind a variable (set to NULL). Index starts at 0. */
	virtual void CCL_API unbindVariable (int index) = 0;

	/** Unbind all variables. */
	virtual void CCL_API unbindVariables () = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Execute
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Execute the statement (no result). */
	virtual tbool CCL_API execute () = 0;

	/** Execute the statement (single result value). */
	virtual tbool CCL_API execute (Variant& result) = 0;

	/** Execute the statement. A query statement returns an IResultSet interface. */
	virtual tbool CCL_API execute (IResultSet*& resultSet) = 0;

	/** Execute an insert statement. Returns the record ID of the new record. */
	virtual int64 CCL_API executeInsert () = 0;

	DECLARE_IID (IStatement)
};

DEFINE_IID (IStatement, 0x94F7F886, 0x79D5, 0x4AC2, 0x90, 0x85, 0xAA, 0xC8, 0xE3, 0xC7, 0xB1, 0xAF)

//************************************************************************************************
// IResultSet
/** Allows iterating through a set of results, which is returned by a query statement.*/
//************************************************************************************************

interface IResultSet: IUnknown
{
	//////////////////////////////////////////////////////////////////////////////////////////////
	// Columns info
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Get the number of columns in the result set. */
	virtual int CCL_API countColumns () = 0;

	/** Get the name of a column given by index. */
	virtual const char* CCL_API getColumnName (int index) = 0;

	/** Get the indx of a column given by name. */
	virtual int CCL_API getColumnIndex (const char* columnName) = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Iterate through rows
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Iterate through rows until this returns false. */
	virtual tbool CCL_API nextRow () = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Get column values in current row
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Get column value as Variant */
	virtual tbool CCL_API getValue (int column, Variant& value) = 0;

	/** Get column value as integer */
	virtual int64 CCL_API getIntValue (int column) = 0;

	/** Get column value as double */
	virtual double CCL_API getFloatValue (int column) = 0;

	/** Get column value as String */
	virtual void CCL_API getStringValue (int column, String& string) = 0;

	/** Get column value as C-String */
	virtual const char* CCL_API getStringValue (int column) = 0;

	/** Check if column value is Null */
	virtual tbool CCL_API isNull (int column) = 0;

	DECLARE_IID (IResultSet)
};

DEFINE_IID (IResultSet, 0x3937958A, 0x3EA1, 0x4A76, 0x93, 0x7A, 0xC5, 0x2A, 0x11, 0xB9, 0x7D, 0xEB)

} // namespace Database
} // namespace CCL

#endif // _idatabase_h
