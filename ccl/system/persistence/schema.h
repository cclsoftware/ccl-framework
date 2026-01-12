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
// Filename    : ccl/system/persistence/schema.h
// Description : Database Schema
//
//************************************************************************************************

#ifndef _ccl_schema_h
#define _ccl_schema_h

#include "ccl/public/system/ipersistentstore.h"

#include "ccl/base/collections/objectarray.h"
#include "ccl/public/text/cstring.h"

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace CCL {

namespace Database {
interface IConnection;
interface IStatement;
interface IResultSet; }

namespace Persistence {

class MemberInfo;

//************************************************************************************************
// PClassID
//************************************************************************************************

typedef int64 PClassID;
enum { kInvalidCID = -1 };

//************************************************************************************************
// Column
/** A column of a table. */
//************************************************************************************************

class Column: public Object
{
public:
	enum ColumnType
	{
		kNone = 0,
		kIntegerPrimaryKey,
		kInteger,
		kFloat,
		kString,
		kBlob
	};
	Column (StringID name, ColumnType type);

	PROPERTY_MUTABLE_CSTRING (name, Name)
	PROPERTY_VARIABLE (ColumnType, type, Type)

private:
};

//************************************************************************************************
// Table
/** A table in the database schema. */
//************************************************************************************************

class Table: public Object
{
public:
	Table (StringID name);

	PROPERTY_MUTABLE_CSTRING (name, Name)

	Column* addColumn (StringID name, Column::ColumnType columnType);
	void addColumns (MemberInfo& member);
	tbool create (Database::IConnection* connection);

	#if DEBUG
	void log ();
	#endif

private:
	ObjectArray columns;
	ObjectArray indexColumns;
};

//************************************************************************************************
// ContainerTable
/** A table that represents a container member. */
//************************************************************************************************

class ContainerTable: public Table
{
public:
	ContainerTable (StringID name);
	~ContainerTable ();

	void prepareStatements (Database::IConnection* connection);

	tbool insert (ObjectID ownerOID, PClassID ownerCID, ObjectID elementOID, PClassID elementCID);
	Database::IResultSet* getElements (ObjectID ownerOID, PClassID ownerCID);

private:
	Database::IStatement* insertStatement;
	Database::IStatement* fetchStatement;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

#define kClassesTable "_classes"
#define kOIDColumn "oid"
#define kCIDColumn "cid"
#define kColumnPrefix "_"
#define kCIDColumnPrefix "c"
#define kOwnerOIDColumn "owner_oid"
#define kOwnerCIDColumn "owner_cid"

enum
{
	kOIDIndex = 0,
	kCIDIndex = 1,
	kFirstMemberIndex = 2,
};

///////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace Persistence
} // namespace CCL

#endif // _ccl_schema_h
