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
// Filename    : sqlitestatement.h
// Description : SQLite statement
//
//************************************************************************************************

#ifndef _sqlitestatement_h
#define _sqlitestatement_h

#include "ccl/base/object.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/plugins/idatabase.h"

struct sqlite3_stmt;

namespace CCL {
namespace Database {

class SQLiteConnection;

//************************************************************************************************
// SQLiteStatement
//************************************************************************************************

class SQLiteStatement: public Object,
					   public IStatement,
					   public IResultSet
{
public:
	SQLiteStatement (SQLiteConnection& connection, StringRef sqlString);
	SQLiteStatement (SQLiteConnection& connection, const char* sqlStringUTF8);
	~SQLiteStatement ();

	// IStatement
	void CCL_API bindVariable (int index, VariantRef value) override;
	void CCL_API bindVariable (int index, StringRef value) override;
	void CCL_API bindVariable (int index, const char* value) override;
	void CCL_API bindVariable (int index, int64 value) override;
	void CCL_API bindVariable (int index, double value) override;
	void CCL_API bindVariable (int index, const IMemoryStream& blob) override;
	void CCL_API unbindVariable (int index) override;
	void CCL_API unbindVariables () override;
	tbool CCL_API execute () override;
	tbool CCL_API execute (Variant& result) override;
	tbool CCL_API execute (IResultSet*& resultSet) override;
	int64 CCL_API executeInsert () override;

	// IResultSet
	int CCL_API countColumns () override;
	const char* CCL_API getColumnName (int index) override;
	int CCL_API getColumnIndex (const char* columnName) override;
	tbool CCL_API nextRow () override;
	tbool CCL_API getValue (int column, Variant& value) override;
	int64 CCL_API getIntValue (int column) override;
	double CCL_API getFloatValue (int column) override;
	void CCL_API getStringValue (int column, String& string) override;
	const char* CCL_API getStringValue (int column) override;
	tbool CCL_API isNull (int column) override;

	CLASS_INTERFACE2 (IStatement, IResultSet, Object)

private:
	void checkReset ();
	int retryStep (int errorCode);

	sqlite3_stmt* statement;
	bool wasExecuted;
	MutableCString sql;
};

} // namespace Database
} // namespace CCL

#endif // _sqlitestatement_h
