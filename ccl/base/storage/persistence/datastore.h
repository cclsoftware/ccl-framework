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
// Filename    : ccl/base/storage/persistence/datastore.h
// Description : Data Store
//
//************************************************************************************************

#ifndef _ccl_datastore_h
#define _ccl_datastore_h

#include "ccl/base/storage/url.h"

#include "ccl/base/collections/objectlist.h"

namespace CCL {

class DataItem;
class DataStoreInfo;
interface IMutableArray;

namespace Threading {
class CriticalSection; }

namespace Persistence {
interface IExpression;
interface IPersistentStore; }
	
//************************************************************************************************
// DataStore
/** Stores and retrieves objects in a database file.
	Classes must be derived from DataItem and must be registered in the DataStore. */
//************************************************************************************************

class DataStore: public Object
{
public:
	DECLARE_CLASS_ABSTRACT (DataStore, Object)

	DataStore (UrlRef storeLocation = Url ());
	~DataStore ();

	// location of database file (default: "DataStore.db" in application settings folder)
	void setLocation (UrlRef storeLoc);
	UrlRef getLocation () const; 

	// global meta info
	void setMetaInfo (StringRef key, VariantRef value);
	bool getMetaInfo (Variant& value, StringRef key);

	// register additional classes to be stored
	void registerClass (MetaClassRef metaClass);
	void setMemberFlags (MetaClassRef metaClass, const char* memberName, int flags);

	// add / update / remove items
	void addItem (DataItem* item);	///< takes ownership
	void updateItem (DataItem* item);
	void removeItem (DataItem* item);

	// perform pending database operations
	void flush (bool force);

	// query
	Iterator* query (MetaClassRef metaClass, Persistence::IExpression* condition = nullptr);
	template <class T> Iterator* query (Persistence::IExpression* condition = nullptr);
	void collectValues (IMutableArray& values, MetaClassRef metaClass, const char* memberName, Persistence::IExpression* condition = nullptr);
	template <class T> void collectValues (IMutableArray& values, const char* memberName, Persistence::IExpression* condition = nullptr);

protected:
	Url storeLocation;
	Persistence::IPersistentStore* store;
	ObjectList insertList;
	ObjectList updateList;
	ObjectList removeList;
	Threading::CriticalSection& lock;

	class QueryIterator;

	Persistence::IPersistentStore& getStore ();
	DataStoreInfo* getMetaInfo (StringRef key);
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline void DataStore::setLocation (UrlRef storeLoc)
{ storeLocation = storeLoc; }

inline UrlRef DataStore::getLocation () const
{ return storeLocation; }

template <class T> Iterator* DataStore::query (Persistence::IExpression* condition)
{ return query (ccl_typeid<T> (), condition); }

template <class T> void DataStore::collectValues (IMutableArray& values, const char* memberName, Persistence::IExpression* condition)
{ collectValues (values, ccl_typeid<T> (), memberName, condition); }

} // namespace CCL

#endif // _ccl_datastore_h
