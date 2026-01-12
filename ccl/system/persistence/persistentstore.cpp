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
// Filename    : ccl/system/persistence/persistentstore.cpp
// Description : Persistent Store
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/system/persistence/persistentstore.h"
#include "ccl/system/persistence/queryresult.h"
#include "ccl/system/persistence/classinfo.h"

#include "ccl/public/base/iarrayobject.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/plugins/idatabase.h"
#include "ccl/public/plugservices.h"
#include "ccl/public/systemservices.h"

using namespace CCL;
using namespace Persistence;
using namespace Database;

//************************************************************************************************
// PersistentStore::MapMembersArgs
//************************************************************************************************

struct PersistentStore::MapMembersArgs
{
	MapMembersArgs (ClassInfo& destClass, Table& table, ClassInfo::TableEntry* tableEntry)
	: destClass (destClass),
	  tableEntry (tableEntry),
	  table (table)
	{}

	ClassInfo& destClass;
	ClassInfo::TableEntry* tableEntry;
	Table& table;
};

//************************************************************************************************
// PersistentStore
//************************************************************************************************

void PersistentStore::forceLinkage ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (PersistentStore, Object)
DEFINE_CLASS_NAMESPACE (PersistentStore, NAMESPACE_CCL)
DEFINE_CLASS_CATEGORY (PersistentStore, "System")
DEFINE_CLASS_UID (PersistentStore, 0x0FEE2CD6, 0xB229, 0x47A5, 0xA5, 0x45, 0xBB, 0x07, 0xC8, 0xA6, 0x5A, 0x60)

//////////////////////////////////////////////////////////////////////////////////////////////////

TableMapping PersistentStore::resolveDefaultMapping (ClassInfo& classInfo)
{
	TableMapping mapping = classInfo.getTableMapping ();
	if(mapping == kDefaultMapping)
	{
		ClassInfo* superClass = classInfo.getSuperClass ();
		mapping = superClass ? resolveDefaultMapping (*superClass) : kFlatTable;
		classInfo.setTableMapping (mapping);
	}
	return mapping;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PersistentStore::PersistentStore ()
: databaseEngine (nullptr),
  connection (nullptr)
{
	classes.objectCleanup (true);
	tables.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PersistentStore::~PersistentStore ()
{
	// classes & tables may contain statements that must be released before the connection
	classes.removeAll ();
	tables.removeAll ();

	if(connection)
		connection->release ();
	if(databaseEngine)
		ccl_release (databaseEngine);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PersistentStore::setLocation (UrlRef url)
{
	if(connection)
		return kResultFailed;

	url.clone (dbUrl);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PersistentStore::registerClass (const ITypeInfo* typeInfo, TableMapping mapping)
{
	ASSERT (!getClassInfo (*typeInfo))

	if(!typeInfo)
		return kResultInvalidArgument;

	ClassInfo* classInfo = NEW ClassInfo (*typeInfo);
	classInfo->setTableMapping (mapping);
	if(const ITypeInfo::PropertyDefinition* properties = typeInfo->getPropertyNames ())
	{
		for(int i = 0; properties[i].name != nullptr; i++)
		{
			const ITypeInfo* memberTypeInfo = properties[i].typeInfo;

			// object and container members need typeinfo
			ASSERT (memberTypeInfo || (properties[i].type != ITypeInfo::kObject && properties[i].type != ITypeInfo::kContainer))

			MemberInfo* member = NEW MemberInfo (properties[i].name, properties[i].type);
			if(memberTypeInfo)
				member->setClassName (memberTypeInfo->getClassName ());
			classInfo->addMember (member);
		}
	}
	classes.add (classInfo);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PersistentStore::setMemberFlags (const ITypeInfo* typeInfo, const char* memberName, int flags)
{
	if(!typeInfo)
		return kResultInvalidArgument;

	if(ClassInfo* classInfo = getClassInfo (*typeInfo))
	{
		ForEach (classInfo->getMembers (), MemberInfo, member)
			if(member->getName () == memberName)
			{
				member->setFlags (flags);
				return kResultOk;
			}
		EndFor
	}
	return kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ClassInfo* PersistentStore::getClassInfo (IUnknown* obj)
{
	UnknownPtr<IObject> object (obj);
	if(object)
		return getClassInfo (object->getTypeInfo ().getClassName ());
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ClassInfo* PersistentStore::getClassInfo (const ITypeInfo& typeInfo)
{
	return getClassInfo (typeInfo.getClassName ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ClassInfo* PersistentStore::getClassInfo (StringID className)
{
	ForEach (classes, ClassInfo, classInfo)
		if(className == classInfo->getClassName ())
			return classInfo;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Table* PersistentStore::getTable (StringID tableName)
{
	ForEach (tables, Table, table)
		if(tableName == table->getName ())
			return table;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Table* PersistentStore::getClassTable (StringID tableName, bool create)
{
	Table* table = getTable (tableName);
	if(!table && create)
	{
		table = NEW Table (tableName);
		table->addColumn (kOIDColumn, Column::kIntegerPrimaryKey);
		table->addColumn (kCIDColumn, Column::kInteger);
		tables.add (table);
	}
	return table;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ContainerTable* PersistentStore::getContainerTable (StringID tableName, bool create)
{
	ContainerTable* table = (ContainerTable*)getTable (tableName); /// hmmm...
	if(!table && create)
	{
		table = NEW ContainerTable (tableName);
		tables.add (table);
	}
	return table;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PersistentStore::mapMembersToTable (MapMembersArgs& args, ClassInfo& sourceClass)
{
	// add columns for direct members of this class
	ForEach (sourceClass.getMembers (), MemberInfo, member)
		args.table.addColumns (*member);
		args.destClass.mapMember (member, args.tableEntry);

		if(member->getType () == ITypeInfo::kContainer)
		{
			// create a table for the container
			MutableCString containerName ("_");
			containerName.append (args.table.getName ()).append (member->getColumnName ());
			ContainerTable* containerTable = getContainerTable (containerName, true);
			member->setContainerTable (containerTable);
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PersistentStore::mapMembersFlat (MapMembersArgs& args, ClassInfo& currentClass)
{
	// recursively add members of all parent classes
	ClassInfo* superClass = currentClass.getSuperClass ();
	if(superClass)
		mapMembersFlat (args, *superClass);

	// add columns for direct members of this class
	mapMembersToTable (args, currentClass);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PersistentStore::mapClasses ()
{
	if(connection->hasTable (kClassesTable))
	{
		// assign classIDs from classes table to ClassInfo objects
		AutoPtr<IStatement> statement (connection->createStatement ("select cid, class from " kClassesTable));
		AutoPtr<IResultSet> resultSet;
		if(statement && statement->execute (resultSet))
		{
			while(resultSet->nextRow ())
			{
				PClassID classID = resultSet->getIntValue (0);
				const char* className = resultSet->getStringValue (1);
				ClassInfo* classInfo = getClassInfo (className);
				if(classInfo)
					classInfo->setClassID (classID);
			}
		}
	}
	else
		connection->execute ("create table " kClassesTable" (cid INTEGER PRIMARY KEY, class TEXT)");

	AutoPtr<IStatement> insertClassStatement;

	// first establish superclass <-> subclass relations
	ForEach (classes, ClassInfo, classInfo)
		const ITypeInfo* superType = classInfo->getClassType ().getParentType ();
		if(superType)
		{
			ClassInfo* superClass = getClassInfo (*superType);
			if(superClass)
			{
				classInfo->setSuperClass (superClass);
				superClass->addSubClass (classInfo);
			}
		}

		// resolve member classes
		ForEach (classInfo->getMembers (), MemberInfo, member)
			int storageType = member->getStorageType ();
			if(storageType == ITypeInfo::kObject || storageType == ITypeInfo::kContainer)
				member->setClassInfo (getClassInfo (member->getClassName ()));
		EndFor

		// insert new classes into classes table
		if(classInfo->getClassID () == kInvalidCID)
		{
			if(!insertClassStatement)
				insertClassStatement = connection->createStatement ("insert into " kClassesTable" (class) values (?)");
			if(insertClassStatement)
			{
				insertClassStatement->bindVariable (0, classInfo->getClassName ());
				classInfo->setClassID (insertClassStatement->executeInsert ());
			}
		}
	EndFor

	// todo: reorder classInfos so that base classes are before their derived classes. Then we can assume below that base classes have already been mapped

	// resolve default table mappings
	ForEach (classes, ClassInfo, classInfo)
		resolveDefaultMapping (*classInfo);
	EndFor

	ForEach (classes, ClassInfo, classInfo)
		switch(classInfo->getTableMapping ())
		{
			case kFlatTable:
			{
				// flat table with all parent class members
				Table* table = getClassTable (classInfo->getClassName (), true);

				MapMembersArgs args (*classInfo, *table, classInfo->addTable (table));
				mapMembersFlat (args, *classInfo);
				break;
			}
			case kLinkedTables:
			{
				classInfo->takeInheritedColumns ();
				
				// todo: also skip that table if classInfo has only kContainer members (which produces no columns)
				if(!classInfo->getMembers ().isEmpty ()) // todo: otherwise the table may be needed later, if a subclass uses kEmbedInBaseTable
				{
					// map direct members to a new table
					Table* table = getClassTable (classInfo->getClassName (), true);
					MapMembersArgs args (*classInfo, *table, classInfo->addTable (table));
					mapMembersToTable (args, *classInfo);
				}
				break;
			}
			case kEmbedInBaseTable:
				CCL_NOT_IMPL ("Mapping kEmbedInBaseTable not implemented yet!")
				break;
		}
	EndFor

	#if DEBUG_LOG
	CCL_PRINTLN ("Tables:")
	ForEach (tables, Table, table)
		table->log ();
	EndFor
	CCL_PRINTLN ("Classes:")
	ForEach (classes, ClassInfo, classInfo)
		CCL_PRINTF ("  %s: (", classInfo->getClassName ())
		classInfo->logMapping ();
		CCL_PRINTLN (")")
	EndFor
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IConnection* PersistentStore::getConnection ()
{
	if(!connection)
	{
		if(!databaseEngine)
			databaseEngine = ccl_new<IDatabaseEngine> (CCL::ClassID::SQLite);

		if(databaseEngine)
		{
			System::GetFileSystem ().createFolder (*dbUrl);
			connection = databaseEngine->createConnection (*dbUrl);
		}

		if(connection)
		{
			mapClasses ();

			// create or adjust tables in database
			ForEach (tables, Table, table)
				table->create (connection);
			EndFor

			// prepare statements
			ForEach (classes, ClassInfo, classInfo)
				classInfo->prepare (connection);
			EndFor
		}
	}
	return connection;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PersistentStore::beginTransaction ()
{
	IConnection* connection = getConnection ();
	return connection ? connection->beginTransaction () : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PersistentStore::commitTransaction ()
{
	IConnection* connection = getConnection ();
	return connection ? connection->commitTransaction () : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PersistentStore::storeObject (IPersistentObject* object)
{
	ClassInfo* classInfo = getClassInfo (object);
	if(classInfo && getConnection ())
	{
		classInfo->insertObject (object);
		return kResultOk;
	}
	return kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PersistentStore::updateObject (IPersistentObject* object)
{
	ClassInfo* classInfo = getClassInfo (object);
	if(classInfo && getConnection ())
		if(classInfo->updateObject (object))
			return kResultOk;

	return kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PersistentStore::removeObject (IPersistentObject* object)
{
	ClassInfo* classInfo = getClassInfo (object);
	if(classInfo && getConnection ())
		if(classInfo->removeObject (object))
			return kResultOk;

	return kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknownIterator* CCL_API PersistentStore::query (const ITypeInfo& typeInfo, IExpression* condition)
{
	IConnection* connection = getConnection ();
	ClassInfo* classInfo = getClassInfo (typeInfo);
	if(classInfo && connection)
		return NEW QueryResultIterator (connection, classInfo, condition);

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PersistentStore::collectValues (IMutableArray& values, const ITypeInfo& typeInfo, const char* memberName, IExpression* condition)
{
	IConnection* connection = getConnection ();
	ClassInfo* classInfo = getClassInfo (typeInfo);
	if(classInfo && connection)
	{
		// todo: might have to query multiple tables for subclasses
		AutoPtr<IStatement> statement (classInfo->createCollectValuesStatement (memberName, connection, condition));
		AutoPtr<IResultSet> resultSet;
		if(statement && statement->execute (resultSet))
		{
			while(resultSet->nextRow ())
			{
				Variant v;
				resultSet->getValue (0, v);
				values.addArrayElement (v);
			}
			return kResultOk;
		}
	}
	return kResultFailed;
}
