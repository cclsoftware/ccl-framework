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
// Filename    : ccl/system/persistence/classinfo.cpp
// Description : Class Info for persistent classes
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/system/persistence/classinfo.h"
#include "ccl/system/persistence/persistentstore.h"
#include "ccl/system/persistence/sqlwriter.h"

#include "ccl/base/storage/storage.h"
#include "ccl/base/kernel.h"

#include "ccl/public/plugins/idatabase.h"

#include "ccl/public/collections/unknownlist.h"
#include "ccl/public/base/istream.h"
#include "ccl/public/base/variant.h"

using namespace CCL;
using namespace Persistence;
using namespace Database;

//************************************************************************************************
// ClassInfo::TableEntry
//************************************************************************************************

ClassInfo::TableEntry::TableEntry (StringID name)
: name (name),
  numVariables (0),
  insertStatement (nullptr),
  updateStatement (nullptr),
  deleteStatement (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ClassInfo::TableEntry::~TableEntry ()
{
	if(insertStatement)
		insertStatement->release ();
	if(updateStatement)
		updateStatement->release ();
	if(deleteStatement)
		deleteStatement->release ();
}

//************************************************************************************************
// ClassInfo::MappedMember
//************************************************************************************************

ClassInfo::MappedMember::MappedMember (MemberInfo* member, TableEntry* table)
: member (member), table (table), varIndex (0)
{}

//************************************************************************************************
// ClassInfo::ObjectState
//************************************************************************************************

class ClassInfo::ObjectState: public IObjectState
{
public:
	ObjectState (const MemberNames& memberNames)
	: values (memberNames.count ()),
	  memberNames (memberNames.getItems ())
	{}

	inline Variant& at (int index) { return values[index]; }

	// IObjectState
	void CCL_API set (const char* name, VariantRef value) override
	{
		getSlot (name) = value;
	}

	void CCL_API setString (const char* name, StringRef value) override
	{
		getSlot (name) = Variant (value, true); // share
	}

	VariantRef CCL_API get (const char* name) const override
	{
		return getSlot (name);
	}

	void CCL_API setContainer (const char* name, const IContainer& container) override
	{
		getSlot (name) = Variant (&const_cast<IContainer&> (container));
	}

	IContainer* CCL_API getContainer (const char* name) const override
	{
		return (UnknownPtr<IContainer> (getSlot (name)));
	}

private:
	Vector<Variant> values;
	const char** memberNames;

	inline Variant& getSlot (const char* name) const
	{
		for(int i = 0, num = values.getCapacity (); i < num; i++)
			if(strcmp (memberNames[i], name) == 0)
				return values[i];

		static Variant error;
		return error;
	}
};

//************************************************************************************************
// ClassInfo
//************************************************************************************************

ClassInfo::ClassInfo (const ITypeInfo& typeInfo)
: typeInfo (typeInfo),
  tableMapping (kDefaultMapping),
  superClass (nullptr),
  fetchStatement (nullptr),
  classID (kInvalidCID),
  hasContainers (false)
{
	members.objectCleanup (true);
	usedTables.objectCleanup (true);
	mappedMembers.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ClassInfo::~ClassInfo ()
{
	if(fetchStatement)
		fetchStatement->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ClassInfo::addSubClass (ClassInfo* sub)
{
	subClasses.add (sub);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ClassInfo* ClassInfo::findSubClass (StringID className)
{
	if(className == getClassName ())
		return this;

	ForEach (getSubClasses (), ClassInfo, subClass)
		ClassInfo* deepClass = subClass->findSubClass (className);
		if(deepClass)
			return deepClass;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ClassInfo* ClassInfo::findSubClass (PClassID classID)
{
	if(classID == getClassID ())
		return this;

	ForEach (getSubClasses (), ClassInfo, subClass)
		ClassInfo* deepClass = subClass->findSubClass (classID);
		if(deepClass)
			return deepClass;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ClassInfo::addMember (MemberInfo* member)
{
	members.add (member);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ClassInfo::TableEntry* ClassInfo::addTable (Table* table)
{
	return addTable (table->getName ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ClassInfo::TableEntry* ClassInfo::addTable (StringID tableName)
{
	TableEntry* entry = NEW TableEntry (tableName);
	usedTables.add (entry);
	return entry;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ClassInfo::TableEntry* ClassInfo::getTableEntry (StringID name)
{
	ForEach (usedTables, TableEntry, tableEntry)
		if(tableEntry->getName () == name)
			return tableEntry;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ClassInfo::mapMember (MemberInfo* member, TableEntry* tableEntry)
{
	mappedMembers.add (NEW MappedMember (member, tableEntry));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MemberInfo* ClassInfo::getMappedMember (StringID name)
{
	ForEach (mappedMembers, MappedMember, m)
		MemberInfo* member = m->getMember ();
		if(member->getName () == name)
			return member;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ClassInfo::takeInheritedColumns ()
{
	ASSERT (usedTables.isEmpty ())
	ASSERT (mappedMembers.isEmpty ())
	if(superClass)
	{
		// take all tables used by superclass
		ForEach (superClass->usedTables, TableEntry, tableEntry)
			addTable (tableEntry->getName ());
		EndFor

		// take all member-column mappings from superclass
		ForEach (superClass->mappedMembers, ClassInfo::MappedMember, mappedMember)
			TableEntry* tableEntry = getTableEntry (mappedMember->getTable ()->getName ());
			ASSERT (tableEntry) // (was added above)
			mapMember (mappedMember->getMember (), tableEntry);
		EndFor
		return true;
	}
	return false; // no superclass
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ClassInfo::releaseObject (IPersistentObject* object)
{
	cache.removeObject (object);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ClassInfo::writeMemberColumnNames (SqlWriter& sql)
{
	ForEach (mappedMembers, MappedMember, m)
		MemberInfo* member = m->getMember ();
		switch(member->getStorageType ())
		{
			case ITypeInfo::kObject:
			{
				MutableCString classColumn;
				sql << "," << member->makeClassIDColumnName (classColumn);
				// through:
			}
			case ITypeInfo::kPrimitive:
				sql << "," << member->getColumnName ();
			default:
				break;
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ClassInfo::prepare (Database::IConnection* connection)
{
	if(usedTables.count () >= 2)
	{
		// create a view that joins all used tables
		viewName = getClassName ();
		viewName.append ("_v");

		SqlWriter sql;
		sql << "create view " << viewName << " as select a." kOIDColumn" as " kOIDColumn", a." kCIDColumn" as " kCIDColumn;
		writeMemberColumnNames (sql);
		sql << " from ";

		SqlWriter joinConditions;
		char tableVar[3] = " a";

		int index = 0;
		ForEach (usedTables, TableEntry, tableEntry)
			if(index >= 1)
			{
				sql << ",";

				if(index >= 2)
					joinConditions << " AND";
				joinConditions << " a." kOIDColumn"=" << tableVar << "." kOIDColumn;
			}

			sql << tableEntry->getName () << tableVar;
			tableVar[1]++;
			index++;
		EndFor

		if(index > 1)
			sql << " where" << joinConditions;

		if(!connection->hasView (viewName))
			connection->execute (sql);
	}
	else if(TableEntry* te = getTableEntry (0))
	{
		if(1)//multiple classes are using this table (via kLinkedTables or kEmbedInBaseTable)
		{
			viewName = getClassName ();
			viewName.append ("_v");

			SqlWriter sql;
			sql << "create view " << viewName << " as select " kOIDColumn", " kCIDColumn;
			writeMemberColumnNames (sql);
			sql << " from " << te->getName () << " where " kCIDColumn"=";
			sql.writeLiteral (getClassID ());

			if(!connection->hasView (viewName))
				connection->execute (sql);
		}
		else
			viewName = te->getName (); // no view necessary, directly query table
	}

	// build statement for fetching an object by id
	SqlWriter fetchSql;
	fetchSql << "select * from " << getViewName () << " where " kOIDColumn"=?";
	fetchStatement = connection->createStatement (fetchSql);

	// build insert and update statements for all tables used by this class
	SqlWriter insertSql;
	SqlWriter updateSql;
	MutableCString values;
	TableEntry* currentTable = nullptr;
	int varIndex = 0;
	hasContainers = false;

	ForEach (mappedMembers, MappedMember, m)
		if(m->getTable () != currentTable)
		{
			if(currentTable)
			{
				// finish statements for current table
				insertSql << values << ")";
				currentTable->setInsertStatement (connection->createStatement (insertSql));

				updateSql << " where oid=?";
				currentTable->setUpdateStatement (connection->createStatement (updateSql));

				currentTable->setNumVariables (varIndex);
			}

			// begin with next table
			currentTable = m->getTable ();
			ASSERT (currentTable->getInsertStatement () == nullptr)
			varIndex = 0;

			// build insert statement for this classes members in that table
			insertSql.clear () << "insert into " << currentTable->getName () << "(" kOIDColumn"," kCIDColumn;
			values = ")values(?,?";

			// build update statement
			updateSql.clear () << "update " << currentTable->getName () << " set ";
		}

		MemberInfo* member = m->getMember ();
		switch(member->getStorageType ())
		{
			case ITypeInfo::kContainer:
			{
				varIndex--; // has no column in this table
				hasContainers = true;

				if(ContainerTable* table = member->getContainerTable ())
					table->prepareStatements (connection);
				break;
			}
			case ITypeInfo::kObject:
			{
				MutableCString classIdColum;
				insertSql << "," << member->makeClassIDColumnName (classIdColum);	
				values.append (",?");

				if(varIndex != 0)
					updateSql << ",";
				updateSql << classIdColum << "=?";

				varIndex++; // one more for the classID column
				// through:
			}
			case ITypeInfo::kPrimitive:
			{
				insertSql << "," << member->getColumnName ();
				values.append (",?");

				if(varIndex != 0)
					updateSql << ",";
				updateSql << member->getColumnName () << "=?";
				break;
			}
			default:
				break;
		}
		m->setVariableIndex (varIndex++);
		
		memberNames.add (member->getName ());
	EndFor

	if(currentTable)
	{
		// finish statements for last table
		insertSql << values << ")";
		currentTable->setInsertStatement (connection->createStatement (insertSql));

		updateSql << " where oid=?";
		currentTable->setUpdateStatement (connection->createStatement (updateSql));
		
		currentTable->setNumVariables (varIndex);
	}

	// build delete statements for all used tables
	ForEach (usedTables, TableEntry, tableEntry)
		SqlWriter deleteSql;
		deleteSql << "delete from " << tableEntry->getName () << " where " kOIDColumn"=?";
		tableEntry->setDeleteStatement (connection->createStatement (deleteSql));
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ClassInfo::bindMemberValues (ObjectState& objectState, GetStatementFunc getStatementFunc, int firstVariableIndex)
{
	int memberIndex = -1;
	ForEach (mappedMembers, MappedMember, m)
		memberIndex++;
		MemberInfo* member = m->getMember ();
		ASSERT (member->getName () == memberNames[memberIndex])

		IStatement* statement = (m->getTable ()->*getStatementFunc) ();
		int index = firstVariableIndex + m->getVariableIndex ();

		switch(member->getType ())
		{
			case ITypeInfo::kInt:
			case ITypeInfo::kBool:
				statement->bindVariable (index,  objectState.at (memberIndex).lValue);
				continue;

			case ITypeInfo::kFloat:
				statement->bindVariable (index,  objectState.at (memberIndex).fValue);
				continue;

			case ITypeInfo::kString:
				statement->bindVariable (index,  objectState.at (memberIndex).asString ());
				continue;

			case ITypeInfo::kBlob:
				{
					UnknownPtr<IMemoryStream> ms (objectState.at (memberIndex).asUnknown ());
					if(ms)
					{
						statement->bindVariable (index, *ms);
						continue;
					}
				}

			case ITypeInfo::kObject:
			{
				UnknownPtr<IPersistentObject> memberObj (objectState.at (memberIndex));
				if(memberObj)
				{
					if(ClassInfo* concreteMemberClass = member->findConcreteClass (memberObj))
					{
						// note: in case of an update, this might insert the referenced object; it doesn't get updated automatically though
						ObjectID memberOid = concreteMemberClass->insertObject (memberObj);
						if(isValid (memberOid))
						{
							statement->bindVariable (index - 1, concreteMemberClass->getClassID ());
							statement->bindVariable (index, memberOid);
							continue;
						}
					}
				}
				break;
			}
			case ITypeInfo::kContainer:
				continue; // associations in container tables must be done after this object has been inserted

			default:
				break;
		}
		statement->unbindVariable (index);
	EndFor
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ObjectID ClassInfo::insertObject (IPersistentObject* persistentObject)
{
	// only insert if not already in cache
	ObjectID oid = persistentObject->getObjectID ();
	if(isValid (oid))
		return oid;

	// get member values
	ObjectState objectState (memberNames);
	persistentObject->storeMembers (objectState);

	// bind member values to insert statements of corresponding tables
	bindMemberValues (objectState, &TableEntry::getInsertStatement, kFirstMemberIndex);

	// execute insert statements for all tables
	oid = -1;
	ForEach (usedTables, TableEntry, tableEntry)
		IStatement* insertStatement = tableEntry->getInsertStatement ();
		insertStatement->bindVariable (kCIDIndex, getClassID ());
		if(oid == -1)
		{
			insertStatement->unbindVariable (kOIDIndex);
			oid = insertStatement->executeInsert ();
			ASSERT (isValid (oid))

			persistentObject->connectPersistentOwner (this, oid);
			cache.addObject (persistentObject);
		}
		else
		{
			insertStatement->bindVariable (kOIDIndex, oid);
			insertStatement->execute ();
		}
	EndFor

	// insert container associations
	if(hasContainers) 
	{
		int memberIndex = -1;
		ForEach (mappedMembers, MappedMember, m)
			memberIndex++;
			MemberInfo* member = m->getMember ();
			if(member->getStorageType ()== ITypeInfo::kContainer)
			{
				UnknownPtr<IContainer> container (objectState.at (memberIndex));
				if(container)
				{
					ForEachUnknown (*container, obj)
						UnknownPtr<IPersistentObject> element (obj);
						if(element)
						{
							if(ClassInfo* concreteMemberClass = member->findConcreteClass (element))
							{
								// insert element in it's table
								ObjectID elementOid = concreteMemberClass->insertObject (element);
								if(isValid (elementOid))
								{
									// insert association in container table
									member->getContainerTable ()->insert (oid, getClassID (), elementOid, concreteMemberClass->getClassID ());
								}
							}
						}
					EndFor
				}
			}
		EndFor
	}
	return oid;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ClassInfo::updateObject (IPersistentObject* persistentObject)
{
	ObjectID oid = persistentObject->getObjectID ();
	if(!isValid (oid))
		return false;

	// get member values
	ObjectState objectState (memberNames);
	persistentObject->storeMembers (objectState);

	// bind member values to update statements of corresponding tables
	bindMemberValues (objectState, &TableEntry::getUpdateStatement, 0);

	// update values in all used tables
	ForEach (usedTables, TableEntry, tableEntry)
		IStatement* updateStatement = tableEntry->getUpdateStatement ();
		ASSERT (updateStatement)

		int oidIndex = tableEntry->getNumVariables ();
		updateStatement->bindVariable (oidIndex, oid);
		updateStatement->execute ();
	EndFor
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ClassInfo::removeObject (IPersistentObject* persistentObject)
{
	ObjectID oid = persistentObject->getObjectID ();
	if(!isValid (oid))
		return false;

	// delete by oid from all used tables
	ForEach (usedTables, TableEntry, tableEntry)
		IStatement* deleteStatement = tableEntry->getDeleteStatement ();
		ASSERT (deleteStatement)
		deleteStatement->bindVariable (0, oid);
		deleteStatement->execute ();
	EndFor

	cache.removeObject (persistentObject);

	// todo for containers / object references: (maybe using foreign key contraints, slqlite 3.6.19)
	// delete all container table rows where this object is owner (child objects remain; "on delete cascade")
	// delete all container table rows where this object is child element (object just vanishes from the container... or forbidden?)
	// if this object is referenced by another one, remove reference there ("on delete set null")
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPersistentObject* ClassInfo::createObject (IResultSet& resultSet)
{
	ObjectID oid = resultSet.getIntValue (0);
	if(isValid (oid))
	{
		// first lookup in cache
		if(IPersistentObject* persistentObject = cache.lookup (oid))
		{
			persistentObject->retain ();
			return persistentObject;
		}

		ASSERT (getClassID () == resultSet.getIntValue (kCIDIndex))

		if(IUnknown* object = typeInfo.createInstance ())
		{
			// get member values from resultset into objectState
			// this assumes that the field order in the sql query matches our mappedMembers!
			ObjectState objectState (memberNames);
			int index = kFirstMemberIndex;
			int memberIndex = -1;
			ForEach (mappedMembers, MappedMember, m)
				memberIndex++;
				MemberInfo* member = m->getMember ();
				ASSERT (member->getName () == memberNames[memberIndex])

				switch(member->getStorageType ())
				{
					case ITypeInfo::kPrimitive:
					{
						Variant value;
						if(resultSet.getValue (index, value))
							objectState.at (memberIndex) = value;
						break;
					}
					case ITypeInfo::kObject:
					{
						PClassID classID = resultSet.getIntValue (index++);
						ObjectID oid = resultSet.getIntValue (index);
						if(isValid (oid))
							if(ClassInfo* concreteMemberClass = member->getClassInfo ()->findSubClass (classID))
							{
								AutoPtr<IPersistentObject> memberObj (concreteMemberClass->fetchObject (oid));
								if(memberObj)
									objectState.at (memberIndex) = Variant (memberObj, true);
							}
						break;
					}
					case ITypeInfo::kContainer:
					{
						AutoPtr<IResultSet> resultSet (member->getContainerTable ()->getElements (oid, getClassID ()));
						if(resultSet)
						{
							ClassInfo* memberClass = member->getClassInfo ();

							AutoPtr<IUnknownList> list (NEW UnknownList);
							while(resultSet->nextRow())
							{
								ObjectID oid = resultSet->getIntValue (0);
								PClassID classID = resultSet->getIntValue (1);
								if(isValid (oid))
									if(ClassInfo* concreteMemberClass = memberClass->findSubClass (classID))
										if(IPersistentObject* memberObj = concreteMemberClass->fetchObject (oid))
											list->add (memberObj);
							}
							objectState.at (memberIndex) = Variant (list, true);
						}
						index--; // has no column in this table
					}
					default:
						break;
				}
				index++;
			EndFor

			UnknownPtr<IPersistentObject> persistentObject (object);
			if(persistentObject)
			{
				persistentObject->restoreMembers (objectState);
				persistentObject->connectPersistentOwner (this, oid);
				cache.addObject (persistentObject);
				return persistentObject;
			}
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPersistentObject* ClassInfo::fetchObject (ObjectID oid)
{
	AutoPtr<IResultSet> resultSet;
	fetchStatement->bindVariable (kOIDIndex, oid);
	if(fetchStatement->execute (resultSet) && resultSet->nextRow ())
		return createObject (*resultSet);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStatement* ClassInfo::createQueryStatement (IConnection* connection, IExpression* condition)
{
	SqlWriter sql;
	sql << "select * from " << getViewName ();

	if(condition)
		sql.write (" where ").writeExpression (condition, *this);

//	CCL_PRINTF ("createQueryStatement: %s\n", sql);
	return connection->createStatement (sql);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStatement* ClassInfo::createCollectValuesStatement (StringID memberName, IConnection* connection, IExpression* condition)
{
	if(MemberInfo* member = getMappedMember (memberName))
	{
		StringID memberColumn = member->getColumnName ();

		SqlWriter sql;
		sql << "select distinct " << memberColumn << " from " << getViewName ();

		if(condition)
			sql.write (" where ").writeExpression (condition, *this);

		sql << " order by " << memberColumn.str ();

		CCL_PRINTF ("createCollectValuesStatement: %s\n", (const char*)sql);
		return connection->createStatement (sql);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
#if DEBUG
void ClassInfo::logMapping ()
{
	#if DEBUG_LOG
	bool first = true;
	ForEach (mappedMembers, MappedMember, m)
		if(!first)
			CCL_PRINT (", ")
		CCL_PRINTF ("%s.%s", m->getTable ()->getName ().str (), m->getMember ()->getName ().str ())
		first = false;
	EndFor
	#endif
}
#endif


//************************************************************************************************
// MemberInfo
//************************************************************************************************

MemberInfo::MemberInfo (StringID name, int type, StringID className)
: name (name),
  type (type),
  className (className),
  columnName (kColumnPrefix),
  columnIndex (-1),
  flags (0),
  classInfo (nullptr),
  containerTable (nullptr)
{
	columnName.append (name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MemberInfo::~MemberInfo ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString& MemberInfo::makeClassIDColumnName (MutableCString& name)
{
	name = kCIDColumnPrefix;
	return name.append (getColumnName ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline ClassInfo* MemberInfo::findConcreteClass (IPersistentObject* object) const
{
	UnknownPtr<IObject> iObject (object);
	const char* className = iObject ? iObject->getTypeInfo ().getClassName () : getClassName ().str ();
	return getClassInfo ()->findSubClass (className);
}
