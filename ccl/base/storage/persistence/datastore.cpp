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
// Filename    : ccl/base/storage/persistence/datastore.cpp
// Description : Data Store
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/base/storage/persistence/datastore.h"
#include "ccl/base/storage/persistence/dataitem.h"
#include "ccl/base/storage/persistence/expression.h"

#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/threadsync.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/plugservices.h"

namespace CCL {

//************************************************************************************************
// DataStoreInfo
//************************************************************************************************

class DataStoreInfo: public Persistence::PersistentObject<Object>
{
public:
	DECLARE_CLASS (DataStoreInfo, Object)
	DECLARE_PROPERTY_NAMES (DataStoreInfo)

	PROPERTY_STRING (key, Key)
	PROPERTY_STRING (value, Value)

	// IPersistentObject
	void CCL_API storeMembers (Persistence::IObjectState& state) const override;
	void CCL_API restoreMembers (Persistence::IObjectState& state) override;
};

} // namespace CCL

using namespace CCL;
using namespace Persistence;

//************************************************************************************************
// DataStore::QueryIterator
//************************************************************************************************

class DataStore::QueryIterator: public Iterator
{
public:
	inline QueryIterator (IUnknownIterator* iterator, MetaClassRef metaClass, Threading::CriticalSection& lock)
	: iterator (iterator), metaClass (metaClass), lock (lock)
	{}

	// Iterator
	tbool CCL_API done () const override
	{
		Threading::ScopedLock scopedLock (lock);
		return iterator->done ();
	}

	Object* next () override
	{
		IUnknown* unk;
		{
			Threading::ScopedLock scopedLock (lock);
			unk = iterator->nextUnknown ();
		}
		Object* obj = unknown_cast<Object> (unk);
		return (obj && obj->canCast (metaClass)) ? obj : nullptr;
	}

	void first () override			{ CCL_NOT_IMPL ("DataStore::QueryIterator::first") }
	void last () override			{ CCL_NOT_IMPL ("DataStore::QueryIterator::last") }
	Object* previous () override	{ CCL_NOT_IMPL ("DataStore::QueryIterator::previous") return nullptr; }

protected:
	AutoPtr<IUnknownIterator> iterator;
	MetaClassRef metaClass;
	Threading::CriticalSection& lock;
};

//************************************************************************************************
// DataStoreInfo
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (DataStoreInfo, Object, "StoreInfo")

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_PROPERTY_NAMES (DataStoreInfo)
	DEFINE_PROPERTY_TYPE ("key", ITypeInfo::kString)
	DEFINE_PROPERTY_TYPE ("value", ITypeInfo::kString)
END_PROPERTY_NAMES (DataStoreInfo)

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DataStoreInfo::storeMembers (IObjectState& state) const
{
	state.set ("key", key);
	state.set ("value", value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DataStoreInfo::restoreMembers (IObjectState& state)
{
	key = state.get ("key");
	value = state.get ("value");
}

//************************************************************************************************
// DataStore
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (DataStore, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

DataStore::DataStore (UrlRef location)
: storeLocation (location),
  store (nullptr),
  lock (*NEW Threading::CriticalSection)
{
	insertList.objectCleanup (true);
	updateList.objectCleanup (true);
	removeList.objectCleanup (true);

	if(storeLocation.isEmpty ())
	{
		System::GetSystem ().getLocation (storeLocation, System::kAppSettingsFolder);
		storeLocation.descend ("DataStore.db");
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DataStore::~DataStore ()
{
	ASSERT (insertList.isEmpty () && updateList.isEmpty () && removeList.isEmpty ())

	delete &lock;

	if(store)
		store->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPersistentStore& DataStore::getStore ()
{
	if(!store)
	{
		Threading::ScopedLock scopedLock (lock);

		store = ccl_new<IPersistentStore> (CCL::ClassID::PersistentStore);
		ASSERT (store)
		if(store)
		{
			store->setLocation (storeLocation);

			store->registerClass (&ccl_typeid<DataStoreInfo> ());
			store->registerClass (&ccl_typeid<DataItem> ());
			store->setMemberFlags (&ccl_typeid<DataItem> (), "url", kIndexRequired);
		}
	}
	return *store;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DataStore::setMetaInfo (StringRef key, VariantRef value)
{
	String valueString;
	value.toString (valueString);

	AutoPtr<DataStoreInfo> info (getMetaInfo (key));
	if(info)
	{
		if(info->getValue () != valueString)
		{
			info->setValue (valueString);
			
			Threading::ScopedLock scopedLock (lock);
			getStore ().updateObject (info);
		}
	}
	else
	{
		info = NEW DataStoreInfo;
		info->setKey (key);
		info->setValue (valueString);

		Threading::ScopedLock scopedLock (lock);
		getStore ().storeObject (info);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DataStore::getMetaInfo (Variant& value, StringRef key)
{
	AutoPtr<DataStoreInfo> info (getMetaInfo (key));
	if(info)
	{
		value.fromString (info->getValue ());
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DataStoreInfo* DataStore::getMetaInfo (StringRef key)
{
	Expression condition = Member ("key") == key;

	IterForEach (query<DataStoreInfo> (condition), DataStoreInfo, info)
		info->retain ();
		return info;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DataStore::registerClass (MetaClassRef metaClass)
{
	Threading::ScopedLock scopedLock (lock);

	getStore ().registerClass (&metaClass);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DataStore::setMemberFlags (MetaClassRef metaClass, const char* memberName, int flags)
{
	Threading::ScopedLock scopedLock (lock);

	getStore ().setMemberFlags (&metaClass, memberName, flags);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DataStore::addItem (DataItem* item)
{
	Threading::ScopedLock scopedLock (lock);

	item->retain ();
	insertList.add (item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DataStore::updateItem (DataItem* item)
{
	Threading::ScopedLock scopedLock (lock);

	item->retain ();
	updateList.add (item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DataStore::removeItem (DataItem* item)
{
	Threading::ScopedLock scopedLock (lock);

	item->retain ();
	removeList.add (item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Iterator* DataStore::query (MetaClassRef metaClass, IExpression* condition)
{
	Threading::ScopedLock scopedLock (lock);

	IUnknownIterator* iter = getStore ().query (metaClass, condition);
	return iter ? NEW QueryIterator (iter, metaClass, lock) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DataStore::collectValues (IMutableArray& values, MetaClassRef metaClass, const char* memberName, IExpression* condition)
{
	Threading::ScopedLock scopedLock (lock);

	getStore ().collectValues (values, metaClass, memberName, condition);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DataStore::flush (bool force)
{
	Threading::ScopedLock scopedLock (lock);

	if(insertList.isEmpty () && updateList.isEmpty () && removeList.isEmpty ())
		return;

	if(force == false)
		if(insertList.count () + updateList.count () + removeList.count () < 200)
			return;

	CCL_PRINTF ("DataStore::flush %d INSERTS, %d UPDATES, %d REMOVES\n", insertList.count (), updateList.count (), removeList.count ())

	CCL_PROFILE_START (DataStore)
	{
		IPersistentStore& store = getStore ();
		Transaction trans (getStore ());

		if(!insertList.isEmpty ())
		{
			// add to database
			ForEach (insertList, DataItem, item)
				store.storeObject (item);
			EndFor

			insertList.removeAll ();
		}

		if(!updateList.isEmpty ())
		{
			ForEach (updateList, DataItem, item)
				store.updateObject (item);
			EndFor

			updateList.removeAll ();
		}

		if(!removeList.isEmpty ())
		{
			ForEach (removeList, DataItem, item)
				store.removeObject (item);
			EndFor

			removeList.removeAll ();
		}
	}
	CCL_PROFILE_STOP (DataStore)
}
