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
// Filename    : ccl/system/persistence/classinfo.h
// Description : Class Info for persistent classes
//
//************************************************************************************************

#ifndef _ccl_classinfo_h
#define _ccl_classinfo_h

#include "ccl/system/persistence/schema.h"
#include "ccl/system/persistence/objectcache.h"

namespace CCL {

namespace Database {
interface IConnection;
interface IStatement;
interface IResultSet; }

namespace Persistence {

interface IExpression;
class MemberInfo;
class Table;
class ContainerTable;
class SqlWriter;

//************************************************************************************************
// ClassInfo
//************************************************************************************************

class ClassInfo: public Object,
				 public IPersistentOwner
{
public:
	ClassInfo (const ITypeInfo& typeInfo);
	~ClassInfo ();

	const ITypeInfo& getClassType () const { return typeInfo; }
	CStringPtr getClassName () const { return typeInfo.getClassName (); }
	PROPERTY_VARIABLE (PClassID, classID, ClassID);

	// class hierarchy
	PROPERTY_POINTER (ClassInfo, superClass, SuperClass)
	Container& getSubClasses () { return subClasses; }
	void addSubClass (ClassInfo* subClass);
	ClassInfo* findSubClass (StringID className);
	ClassInfo* findSubClass (PClassID classID);

	// members
	Container& getMembers () { return members; }
	void addMember (MemberInfo* member);

	PROPERTY_VARIABLE (TableMapping, tableMapping, TableMapping)

	void prepare (Database::IConnection* connection);

	ObjectID insertObject (IPersistentObject* object);
	bool updateObject (IPersistentObject* persistentObject);
	bool removeObject (IPersistentObject* persistentObject);

	Database::IStatement* createQueryStatement (Database::IConnection* connection, IExpression* condition = nullptr);
	Database::IStatement* createCollectValuesStatement (StringID memberName, Database::IConnection* connection, IExpression* condition = nullptr);

	IPersistentObject* createObject (Database::IResultSet& resultSet);
	IPersistentObject* fetchObject (ObjectID oid);

	class TableEntry: public Object
	{
	public:
		TableEntry (StringID name);
		~TableEntry ();

		PROPERTY_MUTABLE_CSTRING (name, Name)
		PROPERTY_VARIABLE (int, numVariables, NumVariables)	///< number of sql variables used for members (without oid/cid)
		PROPERTY_POINTER (Database::IStatement, insertStatement, InsertStatement)
		PROPERTY_POINTER (Database::IStatement, updateStatement, UpdateStatement)
		PROPERTY_POINTER (Database::IStatement, deleteStatement, DeleteStatement)
	};

	TableEntry* addTable (Table* table);
	TableEntry* addTable (StringID tableName);
	TableEntry* getTableEntry (StringID name);
	TableEntry* getTableEntry (int index) { return (TableEntry*)usedTables.at (index); }
	void mapMember (MemberInfo* member, TableEntry* table);
	MemberInfo* getMappedMember (StringID name);
	bool takeInheritedColumns ();

	PROPERTY_MUTABLE_CSTRING (viewName, ViewName)

	#if DEBUG
	void logMapping ();
	#endif

	// IPersistentOwner
	void CCL_API releaseObject (IPersistentObject* object) override;

	CLASS_INTERFACE (IPersistentOwner, Object)

private:
	const ITypeInfo& typeInfo;
	ObjectArray usedTables;    ///< of TableEntry, all tables used by this class
	ObjectArray mappedMembers; ///< of MappedMember, all persistent members, including inherited ones
	ObjectArray subClasses;
	ObjectArray members;
	Database::IStatement* fetchStatement; ///< for fetching an object by id
	ObjectCache cache;
	bool hasContainers;

	class MappedMember: public Object
	{
	public:
		MappedMember (MemberInfo* member, TableEntry* table);

		PROPERTY_POINTER (MemberInfo, member, Member)
		PROPERTY_POINTER (TableEntry, table, Table)
		PROPERTY_VARIABLE (int, varIndex, VariableIndex) ///< variable index in insertStatement for the corresponding table (ignoring cid/oid vars!)
	};

	class ObjectState;
	typedef Vector<const char*> MemberNames;
	MemberNames memberNames;

	typedef Database::IStatement* (TableEntry::*GetStatementFunc) () const;
	bool bindMemberValues (ObjectState& objectState, GetStatementFunc getStatementFunc, int firstVariableIndex);
	void writeMemberColumnNames (SqlWriter& sql);
};

//************************************************************************************************
// MemberInfo
//************************************************************************************************

class MemberInfo: public Object
{
public:
	MemberInfo (StringID name, int type, StringID className = nullptr);
	~MemberInfo ();

	PROPERTY_MUTABLE_CSTRING (name, Name)
	PROPERTY_MUTABLE_CSTRING (columnName, ColumnName)
	PROPERTY_VARIABLE (int, columnIndex, ColumnIndex)
	PROPERTY_VARIABLE (int, flags, Flags)

	PROPERTY_FLAG (flags, 1<<0, indexRequired)

	PROPERTY_MUTABLE_CSTRING (className, ClassName)		///< of referenced or contained object
	PROPERTY_POINTER (ClassInfo, classInfo, ClassInfo)	///< of referenced or contained object
	PROPERTY_POINTER (ContainerTable, containerTable, ContainerTable) ///< for type == kContainer

	DataType getType () const { return type; }
	DataType getStorageType () const { return type & 0x0F; } // kPrimitive, kObject or kContainer

	ClassInfo* findConcreteClass (IPersistentObject* object) const;
	MutableCString& makeClassIDColumnName (MutableCString& name);

private:
	DataType type;
};

} // namespace Persistence
} // namespace CCL

#endif // _ccl_classinfo_h
