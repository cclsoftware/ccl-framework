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
// Filename    : sqliteconnection.h
// Description : SQLite database connection
//
//************************************************************************************************

#ifndef _sqliteconnection_h
#define _sqliteconnection_h

#include "ccl/base/object.h"

#include "ccl/public/plugins/idatabase.h"

struct sqlite3;

namespace CCL {
namespace Database {

//************************************************************************************************
// SQLiteConnection
//************************************************************************************************

class SQLiteConnection: public Object,
						public IConnection
{
public:
	SQLiteConnection (sqlite3* connection);
	~SQLiteConnection ();

	// IConnection
	IStatement* CCL_API createStatement (StringRef sql) override;
	IStatement* CCL_API createStatement (const char* sqlUTF8) override;
	tbool CCL_API execute (StringRef sql) override;
	tbool CCL_API execute (const char* sqlUTF8) override;
	tbool CCL_API execute (StringRef sql, Variant& result) override;
	tbool CCL_API execute (const char* sqlUTF8, Variant& result) override;
	tbool CCL_API beginTransaction () override;
	tbool CCL_API commitTransaction () override;
	tbool CCL_API hasTable (const char* name) override;
	tbool CCL_API hasColumn (const char* table, const char* column) override;
	tbool CCL_API hasView (const char* name) override;
	tbool CCL_API getLastError (String& message) override;

	CLASS_INTERFACE (IConnection, Object)

private:
	friend class SQLiteStatement;
	sqlite3* connection;
	int transactions;
};

} // namespace Database
} // namespace CCL

#endif // _sqliteconnection_h
