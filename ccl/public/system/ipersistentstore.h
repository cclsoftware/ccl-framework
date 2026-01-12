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
// Filename    : ccl/public/system/ipersistentstore.h
// Description : Persistent Store interface
//
//************************************************************************************************

#ifndef _ccl_ipersistentstore_h
#define _ccl_ipersistentstore_h

#include "ccl/public/base/iunknown.h"
#include "ccl/public/base/variant.h"

namespace CCL {
interface ITypeInfo;
interface IContainer;
interface IMutableArray;
interface IUnknownIterator;

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID
{
	/** PersistentStore class identifer. */
	DEFINE_CID (PersistentStore, 0x0FEE2CD6, 0xB229, 0x47A5, 0xA5, 0x45, 0xBB, 0x07, 0xC8, 0xA6, 0x5A, 0x60)
}

namespace Persistence {

interface IPersistentObject;
interface IExpression;

//************************************************************************************************
// TableMapping
/** How classes are mapped to tables. */
//************************************************************************************************

DEFINE_ENUM (TableMapping)
{
	kFlatTable,			///< allocates a table for this class with all inherited members
	kLinkedTables,		///< allocates a table for this class with columns for it's direct members, inherited members are stored in base class tables(s)
	kEmbedInBaseTable,	///< columns for this classes members are added to the base classes table
	kDefaultMapping		///< class uses same mapping as base class, or kFlatTable if none
};

//************************************************************************************************
// MemberFlags
/** Additional properties for members of registered classes. */
//************************************************************************************************

DEFINE_ENUM (MemberFlags)
{
	kIndexRequired = 1<<0	///< an index should be created on columns for this member
};

//************************************************************************************************
// ObjectID
//************************************************************************************************

typedef int64 ObjectID;

enum { kInvalidOID = -1 };

inline bool isValid (ObjectID oid) { return oid >= 0; }

//************************************************************************************************
// IPersistentStore
// Persistent Store interface
//************************************************************************************************

interface IPersistentStore: IUnknown
{
	/** Register a class to be stored. */
	virtual tresult CCL_API registerClass (const ITypeInfo* typeInfo, TableMapping mapping = kDefaultMapping) = 0;

	/** Define additional properties for a member of a registered class. */
	virtual tresult CCL_API setMemberFlags (const ITypeInfo* typeInfo, const char* memberName, int flags) = 0;

	/** Set file location. */
	virtual tresult CCL_API setLocation (UrlRef url) = 0;

	/** Begin a transaction. */
	virtual tbool CCL_API beginTransaction () = 0;

	/** Commit a transaction. */
	virtual tbool CCL_API commitTransaction () = 0;

	/** Store an object in the store. */
	virtual tresult CCL_API storeObject (IPersistentObject* object) = 0;

	/** Update the objects representation in the store. */
	virtual tresult CCL_API updateObject (IPersistentObject* object) = 0;

	/** Remove object from the store. */
	virtual tresult CCL_API removeObject (IPersistentObject* object) = 0;

	/** Create a query result iterator for given class and (optional) condition. */
	virtual IUnknownIterator* CCL_API query (const ITypeInfo& typeInfo, IExpression* condition) = 0;

	/** Collect all (distinct) value occurances of given class member with (optional) condition. */
	virtual tresult CCL_API collectValues (IMutableArray& values, const ITypeInfo& typeInfo, const char* memberName, IExpression* condition) = 0;

	DECLARE_IID (IPersistentStore)
};

DEFINE_IID (IPersistentStore, 0xFF804C0C, 0x5178, 0x4732, 0x86, 0x2E, 0xD7, 0x79, 0x89, 0x68, 0x06, 0xDE)

//************************************************************************************************
// IPersistentOwner
//************************************************************************************************

interface IPersistentOwner: IUnknown
{
	virtual void CCL_API releaseObject (IPersistentObject* object) = 0;

	DECLARE_IID (IPersistentOwner)
};

DEFINE_IID (IPersistentOwner, 0x51CEA18E, 0xF7BF, 0x463B, 0x87, 0x6D, 0xE1, 0xEC, 0x9C, 0xAC, 0xD2, 0x84)

//************************************************************************************************
// IObjectState
//************************************************************************************************

interface IObjectState
{
	// set values in IPersistentObject::storeMembers ()
	virtual void CCL_API set (const char* name, VariantRef value) = 0;
	virtual void CCL_API setString (const char* name, StringRef value) = 0;
	virtual void CCL_API setContainer (const char* name, const IContainer& container) = 0;

	// get values in IPersistentObject::restoreMembers ()
	virtual VariantRef CCL_API get (const char* name) const = 0;
	virtual IContainer* CCL_API getContainer (const char* name) const = 0;
};

//************************************************************************************************
// IPersistentObject
/** Interface for classes that are stored in a PersistentStore. */
//************************************************************************************************

interface IPersistentObject: IUnknown
{
	/** Connect object to an owner. */
	virtual void CCL_API connectPersistentOwner (IPersistentOwner* owner, ObjectID oid) = 0;

	/** Get object id passed in connectPersistentOwner. */
	virtual ObjectID CCL_API getObjectID () = 0;

	/** Store member values into state. */
	virtual void CCL_API storeMembers (IObjectState& state) const = 0;

	/** Restore member values from state. */
	virtual void CCL_API restoreMembers (IObjectState& state) = 0;

	DECLARE_IID (IPersistentObject)
};

DEFINE_IID (IPersistentObject, 0x2D15EF42, 0x2452, 0x43D6, 0xAF, 0xFB, 0x24, 0xED, 0xD8, 0x9B, 0x35, 0x89)


} // namespace Persistence
} // namespace CCL

#endif // _ccl_ipersistentstore_h
