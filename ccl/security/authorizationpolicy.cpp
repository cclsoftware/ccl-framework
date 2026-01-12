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
// Filename    : ccl/security/authorizationpolicy.cpp
// Description : Authorization Policy
//
//************************************************************************************************

#include "ccl/security/authorizationpolicy.h"

#include "ccl/base/storage/storage.h"
#include "ccl/base/storage/jsonarchive.h"

using namespace CCL;
using namespace Security;
using namespace Authorization;

//************************************************************************************************
// Authorization::Policy
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (Policy, StorableObject, "AuthorizationPolicy")

//////////////////////////////////////////////////////////////////////////////////////////////////

Policy::Policy ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ContainerItem& Policy::getRoot () const
{
	return root;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Policy::merge (Policy& other)
{
	root.take (other.root);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Policy::revoke (const Policy& other)
{
	ForEach (other.root, Item, item)
		root.removeEqual (*item);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Policy::loadFromJsonStream (IStream& stream)
{
	JsonArchive jsonArchive (stream);
	jsonArchive.isTypeIDEnabled (true);
	Attributes rootAttr;
	return jsonArchive.loadAttributes (nullptr, rootAttr) && root.load (Storage (rootAttr));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Policy::load (const Storage& storage)
{
	bool result = root.load (storage);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Policy::save (const Storage& storage) const
{
	return root.save (storage);
}

//************************************************************************************************
// Authorization Item Classes
//************************************************************************************************

DEFINE_AUTHPOLICY_CLASS (Resource, ContainerItem, "AuthResource")
DEFINE_AUTHPOLICY_CLASS (Client, ContainerItem, "AuthClient")
DEFINE_AUTHPOLICY_CLASS (AccessItem, Item, "AccessItem")
DEFINE_AUTHPOLICY_CLASS (AllowedItem, AccessItem, "AccessAllowed")
DEFINE_AUTHPOLICY_CLASS (DeniedItem, AccessItem, "AccessDenied")
DEFINE_AUTHPOLICY_CLASS (AssociatedData, ContainerItem, "AuthAssociatedData")
DEFINE_AUTHPOLICY_CLASS (DataItem, Item, "AuthData")
DEFINE_AUTHPOLICY_CLASS (ConditionItem, ContainerItem, "AuthCondition")

//************************************************************************************************
// Authorization::Item
//************************************************************************************************

DEFINE_CLASS_HIDDEN (Item, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

void Item::takePolicyIds (const Item& other)
{
	for(StringRef id : other.policyIds)
		policyIds.addOnce (id);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Item::removePolicyIdsOf (const Item& other)
{
	bool result = false;
	for(StringRef id : other.policyIds)
		result |= policyIds.remove (id);

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Item::ItemType CCL_API Item::getItemType () const
{
	return kItem;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API Item::getItemSID () const
{
	return getSID ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Item::load (const Storage& storage)
{
	Attributes& a = storage.getAttributes ();
	sid = a.getString ("sid");
	policyIds.addOnce (a.getString ("policyId"));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Item::save (const Storage& storage) const
{
	Attributes& a = storage.getAttributes ();
	a.set ("sid", sid);
	if(sid.isEmpty () && policyIds.count () == 1) // for root only
		a.set ("policyId", policyIds.first ());

	return true;
}

//************************************************************************************************
// Authorization::ContainerItem
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ContainerItem, Item)

//////////////////////////////////////////////////////////////////////////////////////////////////

ContainerItem::ContainerItem ()
{
	children.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Item::ItemType CCL_API ContainerItem::getItemType () const
{
	return kContainer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Iterator* ContainerItem::newIterator () const
{
	return children.newIterator ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknownIterator* CCL_API ContainerItem::newItemIterator () const
{
	return newIterator ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Item* ContainerItem::lookup (StringRef sid) const
{
	return lookupInternal (sid);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Item* ContainerItem::lookupInternal (StringRef sid) const
{
	static const String kAny ("*");
	static const String kMultiSeparator (",");

	Item* fallbackResult = nullptr;
	for(Item* item : iterate_as<Item> (children))
	{
		if(item->getSID ().contains (kMultiSeparator))
		{
			ForEachStringToken (item->getSID (), kMultiSeparator, token)
				token.trimWhitespace ();
				if(token == sid)
					return item;
			EndFor
		}
		else if(item->getSID () == sid)
			return item;
		else if(item->getSID () == kAny)
			fallbackResult = item;
	}

	return fallbackResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ContainerItem::take (ContainerItem& other)
{
	for(Item* otherItem : iterate_as<Item> (other.children))
	{
		if(IPolicyItem* policyItem = findItem (otherItem->getSID (), otherItem->getItemType ()))
		{
			if(ContainerItem* otherContainer = ccl_cast<ContainerItem> (otherItem))
			{
				if(ContainerItem* currentContainer = unknown_cast<ContainerItem> (policyItem))
					currentContainer->take (*otherContainer);
			}
			else if(Item* currentItem = unknown_cast<Item> (policyItem))
				currentItem->takePolicyIds (*otherItem);
		}
		else
			children.add (return_shared (otherItem));
	}

	other.children.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ContainerItem::removeEqual (const Item& item)
{
	Item* ownItem = lookupInternal (item.getSID ());
	if(ownItem == nullptr)
		return false;

	if(ContainerItem* containerItem = ccl_cast<ContainerItem> (ownItem))
	{
		const ContainerItem* otherContainer = ccl_cast<ContainerItem> (&item);
		ASSERT (otherContainer) // type mismatch: the same sid is used for a container and for a non-container item
		if(otherContainer)
		{
			if(otherContainer != containerItem)
			{
				for(Item* child : iterate_as<Item> (otherContainer->children))
					containerItem->removeEqual (*child);

				if(containerItem->children.isEmpty ())
				{
					children.remove (containerItem);
					containerItem->release ();
				}
			}
			else
			{
				children.remove (containerItem);
				containerItem->release ();
			}

			return true;
		}

		return false;
	}

	if(ownItem->removePolicyIdsOf (item))
	{
		if(!ownItem->hasPolicyIds ())
		{
			children.remove (ownItem);
			ownItem->release ();
		}

		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPolicyItem* CCL_API ContainerItem::findItem (StringRef sid, ItemType type) const
{
	const Item* item = lookup (sid);
	if(!item)
		return nullptr;

	if(type != kItem)
		if(item->getItemType () != type)
			return nullptr;

	return const_cast<Item*> (item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ContainerItem::load (const Storage& storage)
{
	if(!SuperClass::load (storage))
		return false;

	Attributes& a = storage.getAttributes ();
	// anonymous array (XML)
	a.unqueue (children, nullptr, ccl_typeid<Item> ());
	// additionally, check for named array (JSON)
	a.unqueue (children, "children", ccl_typeid<Item> ());

	for(Item* child : iterate_as<Item> (children))
		child->takePolicyIds (*this);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ContainerItem::save (const Storage& storage) const
{
	Attributes& a = storage.getAttributes ();
	a.queue (nullptr, children);
	return SuperClass::save (storage);
}
