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
// Filename    : ccl/base/storage/persistence/persistence.h
// Description : Helpers for working with an IPersistentStore
//
//************************************************************************************************

#ifndef _ccl_persistence_h
#define _ccl_persistence_h

#include "ccl/base/object.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/system/ipersistentexpression.h"
#include "ccl/public/system/ipersistentstore.h"

namespace CCL {
namespace Persistence {

//************************************************************************************************
// ObjectQuery
/** Query for objects in a persistent store. */
//************************************************************************************************

template<class T>
class ObjectQuery
{
public:
	ObjectQuery (IPersistentStore& store, IExpression* condition = nullptr);
	~ObjectQuery ();

	IUnknownIterator* createIterator () const;

private:
	IPersistentStore& store;
	IExpression* condition;
};

//************************************************************************************************
// Transaction
//************************************************************************************************

struct Transaction
{
	Transaction (IPersistentStore& store) : store (store)
	{ store.beginTransaction (); }

	~Transaction ()
	{ store.commitTransaction (); }

	IPersistentStore& store;
};

//************************************************************************************************
// PersistentObject
/** Standard implementation template of IPersistentObject. */
//************************************************************************************************

template<class BaseUnknown>
class PersistentObject: public BaseUnknown, 
	                    public IPersistentObject
{
public:
	PersistentObject ();
	~PersistentObject ();

	// IPersistentObject
	void CCL_API connectPersistentOwner (IPersistentOwner* owner, ObjectID oid) override;
	ObjectID CCL_API getObjectID () override;

	// IUnknown
	tresult CCL_API queryInterface (UIDRef iid, void** ptr) override;
	unsigned int CCL_API retain () override;
	unsigned int CCL_API release () override;

private:
	Persistence::IPersistentOwner* _owner;
	Persistence::ObjectID _oid;
};


//////////////////////////////////////////////////////////////////////////////////////////////////
// ObjectQuery inline
//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
inline ObjectQuery<T>::ObjectQuery (IPersistentStore& store, IExpression* condition)
: store (store), condition (condition)
{ if(condition) condition->retain (); }

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
inline ObjectQuery<T>::~ObjectQuery ()
{ if(condition) condition->release (); }

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
inline IUnknownIterator* ObjectQuery<T>::createIterator () const
{ return store.query (ccl_typeid<T> (), condition); }

//////////////////////////////////////////////////////////////////////////////////////////////////
// PersistentObject inline
//////////////////////////////////////////////////////////////////////////////////////////////////

template<class BaseUnknown>
PersistentObject<BaseUnknown>::PersistentObject ()
: _owner (nullptr), _oid (kInvalidOID)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class BaseUnknown>
PersistentObject<BaseUnknown>::~PersistentObject ()
{ if(_owner) _owner->releaseObject (this); }

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class BaseUnknown>
void CCL_API PersistentObject<BaseUnknown>::connectPersistentOwner (IPersistentOwner* owner, ObjectID oid)
{ _owner = owner; _oid = oid; }

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class BaseUnknown>
ObjectID CCL_API PersistentObject<BaseUnknown>::getObjectID ()
{ return _oid; }

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class BaseUnknown>
tresult CCL_API PersistentObject<BaseUnknown>::queryInterface (UIDRef iid, void** ptr)
{
	QUERY_INTERFACE (IPersistentObject)
	return BaseUnknown::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class BaseUnknown>
unsigned int CCL_API PersistentObject<BaseUnknown>::retain ()
{ return BaseUnknown::retain (); }

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class BaseUnknown>
unsigned int CCL_API PersistentObject<BaseUnknown>::release ()
{ return BaseUnknown::release (); }

} // namespace Persistence
} // namespace CCL

#endif // _ccl_persistence_h
