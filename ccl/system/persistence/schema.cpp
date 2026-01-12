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
// Filename    : ccl/system/persistence/schema.cpp
// Description : Database Schema
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/system/persistence/schema.h"
#include "ccl/system/persistence/classinfo.h"
#include "ccl/system/persistence/sqlwriter.h"

#include "ccl/public/plugins/idatabase.h"
#include "ccl/public/text/cclstring.h"

using namespace CCL;
using namespace Persistence;
using namespace Database;

//************************************************************************************************
// Table
//************************************************************************************************

Table::Table (StringID name)
: name (name)
{
	columns.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Column* Table::addColumn (StringID name, Column::ColumnType columnType)
{
	Column* column = NEW Column (name, columnType);
	columns.add (column);
	return column;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Table::addColumns (MemberInfo& member)
{
	Column::ColumnType columnType = Column::kNone;
	switch(member.getType ())
	{
		case ITypeInfo::kObject:
		{
			// add classID column before oid column
			MutableCString columnName;
			addColumn (member.makeClassIDColumnName (columnName), Column::kInteger);
			columnType = Column::kInteger;
			break;
		}
		case ITypeInfo::kContainer:
			return; // no columns in host class, container is established via an associative table
		case ITypeInfo::kInt:
			columnType = Column::kInteger;
			break;
		case ITypeInfo::kFloat:
			columnType = Column::kFloat;
			break;
		case ITypeInfo::kString:
			columnType = Column::kString;
			break;
		case ITypeInfo::kBlob:
			columnType = Column::kBlob;
			break;
	}

	Column* column = addColumn (member.getColumnName (), columnType);

	if(member.indexRequired ())
		indexColumns.add (column);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool Table::create (IConnection* connection)
{
	tbool result = true;

	auto writeColumnSpec = [](SqlWriter& sql, Column& column)
	{
		sql << column.getName ();

		switch(column.getType ())
		{
		case Column::kIntegerPrimaryKey:
			sql << " INTEGER PRIMARY KEY";
			break;
		case Column::kInteger:
			sql << " INTEGER";
			break;
		case Column::kFloat:
			sql << " REAL";
			break;
		case Column::kString:
			sql << " TEXT";
			break;
		case Column::kBlob:
			sql << " BLOB";
			break;
		default:
			sql << " NONE";
			break;
		}
	};

	if(!connection->hasTable (getName ()))
	{
		SqlWriter sql;
		sql << "create table " << getName () << " (";

		bool first = true;
		ForEach (columns, Column, column)
			if(first)
				first = false;
			else
				sql << ",";
			
			writeColumnSpec (sql, *column);
		EndFor
		sql << ")";
		result = connection->execute (sql);
	}
	else
	{
		// check if new columns have to be added (removing or modifying type is not supported)
		ObjectArray newColumns;
		ForEach (columns, Column, column)
			if(!connection->hasColumn (getName (), column->getName ()))
				newColumns.add (column);
		EndFor

		if(!newColumns.isEmpty ())
		{
			ForEach (newColumns, Column, column)
				SqlWriter alterSql;
				alterSql << "alter table " << getName () << " add column ";
				writeColumnSpec (alterSql, *column);
				result = connection->execute (alterSql);
			EndFor
		}
	}

	// create indexes
	ForEach (indexColumns, Column, column)
		CStringRef columnName = column->getName (); 

		SqlWriter indexSql;
		indexSql << "create index if not exists " << getName () << columnName << " on " << getName () << " (" << columnName << ")";
		result = connection->execute (indexSql);
	EndFor

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

#if DEBUG
void Table::log ()
{
	#if DEBUG_LOG
	CCL_PRINTF ("Table: %s (", name.str ());
	bool first = true;
	ForEach (columns, Column, column)
		if(!first)
		{
			CCL_PRINT (", ")
		}
		CCL_PRINTF ("%s", column->getName ().str ())
		first = false;
	EndFor
	CCL_PRINTLN (")")
	#endif
}
#endif

//************************************************************************************************
// ContainerTable
//************************************************************************************************

ContainerTable::ContainerTable (StringID name)
: Table (name),
  insertStatement (nullptr),
  fetchStatement (nullptr)
{
	addColumn (kOwnerOIDColumn, Column::kInteger);	// oid of owner
	addColumn (kOwnerCIDColumn, Column::kInteger);	// classID of owner
	addColumn (kOIDColumn, Column::kInteger);		// oid of contained object
	addColumn (kCIDColumn, Column::kInteger);		// classID of contained object
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ContainerTable::~ContainerTable ()
{
	if(insertStatement)
		insertStatement->release ();
	if(fetchStatement)
		fetchStatement->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ContainerTable::prepareStatements (IConnection* connection)
{
	SqlWriter sql;
	sql << "insert into " << getName () << "(" kOwnerOIDColumn"," kOwnerCIDColumn"," kOIDColumn"," kCIDColumn")values(?,?,?,?)";
	insertStatement = connection->createStatement (sql);

	sql.clear () << "select " kOIDColumn"," kCIDColumn" from " << getName () << " where " kOwnerOIDColumn"=? and " kOwnerCIDColumn"=?";
	fetchStatement = connection->createStatement (sql);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool ContainerTable::insert (ObjectID ownerOID, PClassID ownerCID, ObjectID elementOID, PClassID elementCID)
{
	if(insertStatement)
	{
		insertStatement->bindVariable (0, ownerOID);
		insertStatement->bindVariable (1, ownerCID);
		insertStatement->bindVariable (2, elementOID);
		insertStatement->bindVariable (3, elementCID);
		return insertStatement->execute ();
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IResultSet* ContainerTable::getElements (ObjectID ownerOID, PClassID ownerCID)
{
	IResultSet* result = nullptr;
	if(fetchStatement)
	{
		fetchStatement->bindVariable (0, ownerOID);
		fetchStatement->bindVariable (1, ownerCID);
		fetchStatement->execute (result);
	}
	return result;
}

//************************************************************************************************
// Column
//************************************************************************************************

Column::Column (StringID name, ColumnType type)
: name (name),
  type (type)
{}
