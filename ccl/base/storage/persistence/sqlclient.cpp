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
// Filename    : ccl/base/storage/persistence/sqlclient.cpp
// Description : SQL Client classes
//
//************************************************************************************************

#include "ccl/base/storage/persistence/sqlclient.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/plugservices.h"

using namespace CCL;
using namespace Database;

//************************************************************************************************
// SqlConnection
//************************************************************************************************

DEFINE_CLASS_HIDDEN (SqlConnection, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

SqlConnection::SqlConnection ()
: engine (nullptr),
  connection (nullptr)
{
	engine = ccl_new<Database::IDatabaseEngine> (ClassID::SQLite);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SqlConnection::~SqlConnection ()
{
	ASSERT (!isOpen ())
	close ();

	ccl_release (engine);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SqlConnection::open (UrlRef url)
{
	ASSERT (!isOpen ())
	if(isOpen ())
		return false;

	connection = engine ? engine->createConnection (url) : nullptr;
	return connection != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SqlConnection::close ()
{
	safe_release (connection);
}

//************************************************************************************************
// SqlStatement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (SqlStatement, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

SqlStatement::SqlStatement ()
: statement (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

SqlStatement::~SqlStatement ()
{
	safe_release (statement);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SqlStatement::create (SqlConnection& c, StringRef sql)
{
	safe_release (statement);
	statement = c.isOpen () ? c->createStatement (sql) : nullptr;
	return statement != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SqlStatement::create (SqlConnection& c, const char* sql)
{
	safe_release (statement);
	statement = c.isOpen () ? c->createStatement (sql) : nullptr;
	return statement != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SqlStatement::bind (Variant variables[], int count)
{
	ASSERT (isValid ())
	if(!isValid ())
		return false;

	for(int i = 0; i < count; i++)
		statement->bindVariable (i, variables[i]);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SqlStatement::unbindAll ()
{
	ASSERT (isValid ())
	if(statement)
		statement->unbindVariables ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IResultSet* SqlStatement::execute ()
{
	ASSERT (isValid ())
	IResultSet* resultSet = nullptr;
	if(statement)
		statement->execute (resultSet);
	return resultSet;
}
