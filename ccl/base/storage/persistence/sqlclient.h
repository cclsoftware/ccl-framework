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
// Filename    : ccl/base/storage/persistence/sqlclient.h
// Description : SQL Client classes
//
//************************************************************************************************

#ifndef _ccl_sqlclient_h
#define _ccl_sqlclient_h

#include "ccl/base/object.h"

#include "ccl/public/base/variant.h"
#include "ccl/public/plugins/idatabase.h"

namespace CCL {
namespace Database {

//************************************************************************************************
// SqlConnection
//************************************************************************************************

class SqlConnection: public Object
{
public:
	DECLARE_CLASS (SqlConnection, Object)

	SqlConnection ();
	~SqlConnection ();

	bool open (UrlRef url);
	void close ();

	bool isOpen () const;
	IConnection* operator -> ();

protected:
	IDatabaseEngine* engine;
	IConnection* connection;
};

//************************************************************************************************
// SqlStatement
//************************************************************************************************

class SqlStatement: public Object
{
public:
	DECLARE_CLASS (SqlStatement, Object)

	SqlStatement ();
	template<typename T> SqlStatement (SqlConnection& c, T sql);
	template<typename T> SqlStatement (SqlConnection& c, T sql, VariantRef var1);
	template<typename T> SqlStatement (SqlConnection& c, T sql, VariantRef var1, VariantRef var2);
	template<typename T> SqlStatement (SqlConnection& c, T sql, VariantRef var1, VariantRef var2, VariantRef var3);
	~SqlStatement ();

	bool create (SqlConnection& c, StringRef sql);
	bool create (SqlConnection& c, const char* sql);

	bool bind (Variant variables[], int count);
	bool bind (VariantRef var1);
	bool bind (VariantRef var1, VariantRef var2);
	bool bind (VariantRef var1, VariantRef var2, VariantRef var3);

	void unbindAll ();

	IResultSet* execute ();

	bool isValid () const;
	IStatement* operator -> ();

protected:
	IStatement* statement;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// SqlConnection inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool SqlConnection::isOpen () const
{ return connection != nullptr; }

inline IConnection* SqlConnection::operator -> ()
{ return connection; }

//////////////////////////////////////////////////////////////////////////////////////////////////
// SqlStatement inline
//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename T>
SqlStatement::SqlStatement (SqlConnection& c, T sql)
: statement (0)
{ bool result = create (c, sql); ASSERT (result) }

template<typename T>
SqlStatement::SqlStatement (SqlConnection& c, T sql, VariantRef var1)
: statement (0)
{ bool result = create (c, sql) && bind (var1); ASSERT (result) }

template<typename T>
SqlStatement::SqlStatement (SqlConnection& c, T sql, VariantRef var1, VariantRef var2)
: statement (0)
{ bool result = create (c, sql) && bind (var1, var2); ASSERT (result) }

template<typename T>
SqlStatement::SqlStatement (SqlConnection& c, T sql, VariantRef var1, VariantRef var2, VariantRef var3)
: statement (0)
{ bool result = create (c, sql) && bind (var1, var2, var3); ASSERT (result) }

inline bool SqlStatement::bind (VariantRef var1)
{ Variant vars[1] = {var1}; return bind (vars, 1); }

inline bool SqlStatement::bind (VariantRef var1, VariantRef var2)
{ Variant vars[2] = {var1, var2}; return bind (vars, 2); }

inline bool SqlStatement::bind (VariantRef var1, VariantRef var2, VariantRef var3)
{ Variant vars[3] = {var1, var2, var3}; return bind (vars, 3); }

inline bool SqlStatement::isValid () const
{ return statement != nullptr; }

inline IStatement* SqlStatement::operator -> ()
{ return statement; }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace Database
} // namespace CCL

#endif // _ccl_sqlclient_h
