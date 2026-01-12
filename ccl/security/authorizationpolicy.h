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
// Filename    : ccl/security/authorizationpolicy.h
// Description : Authorization Policy
//
//************************************************************************************************

#ifndef _ccl_authorizationpolicy_h
#define _ccl_authorizationpolicy_h

#include "ccl/public/text/cclstring.h"

#include "ccl/public/security/iauthorizationpolicy.h"

#include "ccl/base/storage/storableobject.h"
#include "ccl/base/collections/objectarray.h"

namespace CCL {
namespace Security {
namespace Authorization {

//////////////////////////////////////////////////////////////////////////////////////////////////

#define DECLARE_AUTHPOLICY_CLASS(ClassName,ParentClass,itemType) \
class ClassName: public ParentClass \
{ \
public: \
	DECLARE_CLASS (ClassName, ParentClass) \
	ItemType CCL_API getItemType () const override { return itemType; } \
};

#define DEFINE_AUTHPOLICY_CLASS(ClassName,ParentClass,PersistentName) \
DEFINE_CLASS_PERSISTENT (ClassName,ParentClass,PersistentName)

//************************************************************************************************
// Authorization::Item
//************************************************************************************************

class Item: public Object,
			public IPolicyItem
{
public:
	DECLARE_CLASS (Item, Object)

	PROPERTY_STRING (sid, SID)

	void takePolicyIds (const Item& other);
	bool hasPolicyIds () const { return !policyIds.isEmpty (); }
	bool removePolicyIdsOf (const Item& other);

	// IPolicyItem
	ItemType CCL_API getItemType () const override;
	StringRef CCL_API getItemSID () const override;

	// Object
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;

	CLASS_INTERFACE (IPolicyItem, Object)

protected:
	Vector<String> policyIds;

	// IPolicyItem
	IUnknownIterator* CCL_API newItemIterator () const override { return 0; }
	IPolicyItem* CCL_API findItem (StringRef sid, ItemType type) const override { return 0; }
};

//************************************************************************************************
// Authorization::ContainerItem
//************************************************************************************************

class ContainerItem: public Item
{
public:
	DECLARE_CLASS (ContainerItem, Item)

	ContainerItem ();

	Iterator* newIterator () const;
	const Item* lookup (StringRef sid) const;

	template <class T> const T* lookup (StringRef sid) const
	{ return ccl_cast<T> (lookup (sid)); }

	void take (ContainerItem& other);
	bool removeEqual (const Item& item);

	// Item
	ItemType CCL_API getItemType () const override;
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;

protected:
	ObjectArray children;

	Item* lookupInternal (StringRef sid) const;

	// Item
	IUnknownIterator* CCL_API newItemIterator () const override;
	IPolicyItem* CCL_API findItem (StringRef sid, ItemType type) const override;
};

//************************************************************************************************
// Authorization Item Classes
//************************************************************************************************

DECLARE_AUTHPOLICY_CLASS (Resource, ContainerItem, kResource)
DECLARE_AUTHPOLICY_CLASS (Client, ContainerItem, kClient)
DECLARE_AUTHPOLICY_CLASS (AccessItem, Item, kItem)
DECLARE_AUTHPOLICY_CLASS (AllowedItem, AccessItem, kAccessAllowed)
DECLARE_AUTHPOLICY_CLASS (DeniedItem, AccessItem, kAccessDenied)
DECLARE_AUTHPOLICY_CLASS (AssociatedData, ContainerItem, kAssociatedData)
DECLARE_AUTHPOLICY_CLASS (DataItem, Item, kData)
DECLARE_AUTHPOLICY_CLASS (ConditionItem, ContainerItem, kCondition) // used instead of client, can have children

//************************************************************************************************
// Authorization::Policy
//************************************************************************************************

class Policy: public StorableObject
{
public:
	DECLARE_CLASS (Policy, StorableObject)

	Policy ();

	const ContainerItem& getRoot () const;

	void merge (Policy& other);
	void revoke (const Policy& other);

	bool loadFromJsonStream (IStream& stream);

	// StorableObject
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;

protected:
	ContainerItem root;
};

} // namespace Authorization
} // namespace Security
} // namespace CCL

#endif // _ccl_authorizationpolicy_h
