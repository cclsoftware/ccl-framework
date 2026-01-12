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
// Filename    : ccl/system/persistence/persistentstore.h
// Description : Persistent Store
//
//************************************************************************************************

#ifndef _ccl_persistentstore_h
#define _ccl_persistentstore_h

#include "ccl/public/system/ipersistentstore.h"
#include "ccl/public/storage/iurl.h"

#include "ccl/base/collections/objectarray.h"

namespace CCL {

namespace Database {
interface IDatabaseEngine;
interface IConnection; }

namespace Persistence {

class ClassInfo;
class Table;
class ContainerTable;

//************************************************************************************************
// PersistentStore
//************************************************************************************************

class PersistentStore: public Object,
					   public IPersistentStore
{
public:
	DECLARE_CLASS (PersistentStore, Object)

	PersistentStore ();
	~PersistentStore ();

	static void forceLinkage ();

	// IPersistentStore
	tresult CCL_API registerClass (const ITypeInfo* classInfo, TableMapping mapping = kDefaultMapping) override;
	tresult CCL_API setMemberFlags (const ITypeInfo* typeInfo, const char* memberName, int flags) override;
	tresult CCL_API setLocation (UrlRef url) override;
	tbool CCL_API beginTransaction () override;
	tbool CCL_API commitTransaction () override;
	tresult CCL_API storeObject (IPersistentObject* object) override;
	tresult CCL_API updateObject (IPersistentObject* object) override;
	tresult CCL_API removeObject (IPersistentObject* object) override;
	IUnknownIterator* CCL_API query (const ITypeInfo& typeInfo, IExpression* condition) override;
	tresult CCL_API collectValues (IMutableArray& values, const ITypeInfo& typeInfo, const char* memberName, IExpression* condition) override;

	CLASS_INTERFACE (IPersistentStore, Object)

private:
	Database::IDatabaseEngine* databaseEngine;
	Database::IConnection* connection;
	AutoPtr<IUrl> dbUrl;
	ObjectArray classes;
	ObjectArray tables;

	struct MapMembersArgs;

	ClassInfo* getClassInfo (IUnknown* obj);
	ClassInfo* getClassInfo (const ITypeInfo& typeInfo);
	ClassInfo* getClassInfo (StringID className);

	Database::IConnection* getConnection ();

	static TableMapping resolveDefaultMapping (ClassInfo& classInfo);

	void mapMembersToTable (MapMembersArgs& args, ClassInfo& sourceClass);
	void mapMembersFlat (MapMembersArgs& args, ClassInfo& currentClass);
	void mapClasses ();

	Table* getTable (StringID tableName);
	Table* getClassTable (StringID tableName, bool create = false);
	ContainerTable* getContainerTable (StringID tableName, bool create = false);
};

} // namespace Persistence
} // namespace CCL

#endif // _ccl_persistentstore_h
