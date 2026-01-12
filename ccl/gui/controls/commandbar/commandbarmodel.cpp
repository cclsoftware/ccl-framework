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
// Filename    : ccl/gui/controls/commandbar/commandbarmodel.cpp
// Description : Command Bar Model
//
//************************************************************************************************

#include "ccl/gui/controls/commandbar/commandbarmodel.h"

#include "ccl/gui/graphics/imaging/bitmap.h"
#include "ccl/gui/graphics/nativegraphics.h"

#include "ccl/base/message.h"
#include "ccl/base/kernel.h"
#include "ccl/base/storage/storage.h"
#include "ccl/base/storage/url.h"
#include "ccl/base/security/cryptomaterial.h"

#include "ccl/public/storage/filetype.h"
#include "ccl/public/text/stringbuilder.h"

using namespace CCL;
using namespace CommandBar;

//************************************************************************************************
// ItemTraverser
//************************************************************************************************

ItemTraverser::ItemTraverser ()
: parentItem (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ItemTraverser::traverse (Item* item)
{
	if(!visit (item))
		return false;

	ScopedVar<Item*> p (parentItem, item);

	ForEach (*item, Item, child)
		if(!traverse (child))
			return false;
	EndFor
	return true;
}


//************************************************************************************************
// CommandTarget
//************************************************************************************************

void CommandTarget::fromProperties (const IObject& object)
{
	Variant v1;
	object.getProperty (v1, "name");
	name = VariantString (v1);

	Variant v2;
	object.getProperty (v2, "title");
	title = VariantString (v2);
	
	Variant v3;
	object.getProperty(v3, "category");
	category = VariantString (v3);

	Variant v4;
	object.getProperty (v4, "icon");
	UnknownPtr<IImage> image (v4.asUnknown ());
	icon.share (image);
}

//************************************************************************************************
// CommandBarModel
//************************************************************************************************

DEFINE_CLASS (CommandBarModel, StorableObject)
DEFINE_CLASS_UID (CommandBarModel, 0x31074e2a, 0xf4b0, 0x4827, 0x87, 0x5, 0xb1, 0xce, 0x6d, 0xe, 0x2f, 0x82)

//////////////////////////////////////////////////////////////////////////////////////////////////

CommandBarModel::CommandBarModel ()
: rootItem (*NEW RootItem)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

CommandBarModel::~CommandBarModel ()
{
	rootItem.release ();
	cancelSignals ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Item& CommandBarModel::getRootItem () const
{
	return rootItem;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Item* CommandBarModel::findItem (StringRef id) const
{
	return findItem ([&](Item* item) { return item->getID () == id; });
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Item* CommandBarModel::findParentItem (Item* item) const
{
	struct FindParentTraverser: public ItemTraverser
	{
		Item* child;
		Item* parent;

		FindParentTraverser (Item* child) : child (child), parent (nullptr) {}

		bool visit (Item* item) override
		{
			if(item == child)
			{
				parent = parentItem;
				return false;
			}
			return true;
		}
	};

	if(item == nullptr)
		return &rootItem;

	FindParentTraverser t (item);
	t.traverse (&rootItem);
	return t.parent;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CommandBarModel::adjustInsertContext (Item* item, InsertContext& context) const
{
	Item* parent = context.parent;
	int index = context.index;

	if(parent && item)
	{
		Item* originalParent = findParentItem (item);
		if(parent == &rootItem && ccl_cast<PageItem> (originalParent))
			parent = originalParent;

		// try upwards until parent accepts item
		while(!parent->acceptsChild (item))
		{
			Item* grandParent = findParentItem (parent);
			if(!grandParent)
			{
				parent = nullptr;
				break;
			}

			index = grandParent->getIndex (parent) + 1;
			if(index == grandParent->countChilds ())
				index = -1;
			parent = grandParent;
		}

		if(!parent)
		{
			// reached root item without success, try in child of root
			index = ccl_max (0, index - 1);
			Item* alternativeParent = rootItem.getChild (index);
			if(alternativeParent && alternativeParent->acceptsChild (item) && !ccl_cast<PageItem> (alternativeParent))
			{
				index = -1;
				parent = alternativeParent;
			}
			else
				return false; // later todo (more levels): continue deep
		}

		context.parent = parent;
		context.index = index;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CommandBarModel::addItem (CommandBar::Item* item, CommandBar::InsertContext& context)
{
	if(context.parent && item)
	{
		if(item->getID ().isEmpty ())
			item->setID (UIDString::generate ());

		if(adjustInsertContext (item, context))
		{
			context.parent->addChild (item, context.index);
			deferSignal (NEW Message (kChanged, Variant (ccl_as_unknown (context.parent), true)));
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CommandBarModel::addItem (Item* item, Item* parent, int index)
{
	InsertContext context (parent, index);
	return addItem (item, context);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CommandBarModel::removeItem (Item* item)
{
	Item* parent = findParentItem (item);
	ASSERT (parent)
	if(parent && parent->removeChild (item))
	{
		deferSignal (NEW Message (kChanged, Variant (parent->asUnknown (), true)));
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CommandBarModel::setItemProperty (CommandBar::Item* item, MemberID propertyId, const Variant& var)
{
	if(item && item->setProperty (propertyId, var))
	{
		if(GroupItem* group = ccl_cast<GroupItem> (item))
		{
			if(group != &rootItem)
				group->setRevision (0);
		}
		else if(ccl_cast<ButtonItem> (item) || ccl_cast<CustomItem> (item))
		{
			if(GroupItem* group = unknown_cast<GroupItem> (getParentItem (item)))
				group->setRevision (0);
		}

		deferSignal (NEW Message (kChanged, Variant (item->asUnknown (), true)));
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CommandBarModel::countPages () const
{
	return getRootItem ().countChilds () - 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CommandBar::PageItem* CommandBarModel::getPage (int pageIndex) const
{
	if(pageIndex < 0)
		return nullptr;

	return unknown_cast<CommandBar::PageItem> (getRootItem ().getChildItem (pageIndex + 1));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CommandBarModel::getPageIndex (CommandBar::PageItem* page) const
{
	int index = getRootItem ().getChildIndex (page);
	return index >= 0 ? index - 1 : -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CommandBarModel::setItemProperty (const ICommandBarItem* itemObj, CStringRef propertyId, const Variant& var)
{
	Item* item = unknown_cast<Item> (itemObj);
	return item ? setItemProperty (item, propertyId, var) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandBarModel::checkItemsIDs ()
{
	struct CheckItemsIDs: public ItemTraverser
	{
		bool visit (Item* item) override
		{
			if(item->getID ().isEmpty ())
				item->setID (UIDString::generate ());
			return true;
		}
	};

	CheckItemsIDs ().traverse (&rootItem);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CommandBarModel::load (const Storage& storage)
{
	((GroupItem&)rootItem).removeAll ();

	bool result = rootItem.load (storage);

	// root item must not contain other root items: remove them (unclear how this could happen)
	ObjectList toRemove;
	AutoPtr<Iterator> iter (rootItem.newIterator ());
	for(auto child : *iter)
		if(auto otherRoot = ccl_cast<RootItem> (child))
			toRemove.add (otherRoot);
	for(auto child : iterate_as<Item> (toRemove))
		rootItem.removeChild (child);

	checkItemsIDs ();

	if(result)
		deferSignal (NEW Message (kChanged));
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CommandBarModel::save (const Storage& storage) const
{
	return rootItem.save (storage);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ICommandBarItem* CCL_API CommandBarModel::getItemByID (StringRef id) const
{
	return findItem (id);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ICommandBarItem* CCL_API CommandBarModel::getParentItem (ICommandBarItem* item) const
{
	return findParentItem (unknown_cast<Item> (item));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ICommandBarItem* CCL_API CommandBarModel::addCommandItem (StringRef type, StringRef title, const ICommandBarItem* parentItem, int index)
{
	AutoPtr<CommandBar::Item> item;
	if(type == "Button")
		item = NEW CommandBar::ButtonItem;
	else if(type == "Group")
		item = NEW CommandBar::GroupItem;
	else if(type == "Page")
		item = NEW CommandBar::PageItem;
	else if(type == "CustomItem")
		item = NEW CommandBar::CustomItem;
	if(!item)
		return nullptr;

	item->setTitle (title);

	Item* parent = unknown_cast<Item> (parentItem);
	if(!parent)
		parent = &getRootItem ();

	if(addItem (item, parent, index))
		return item.detach ();
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CommandBarModel::removeCommandItem (ICommandBarItem* item)
{
	return removeItem (unknown_cast<Item> (item));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (CommandBarModel)
	DEFINE_METHOD_ARGR ("getRootItem", "void", "Item")
	DEFINE_METHOD_ARGR ("getParentItem", "item", "Item")
	DEFINE_METHOD_ARGR ("invalidate", "void", "void")
	DEFINE_METHOD_ARGR ("loadItemFromFile", "Url", "Item")
	DEFINE_METHOD_ARGR ("createPage", "void", "PageItem")
	DEFINE_METHOD_ARGR ("getPage", "int", "PageItem")
	DEFINE_METHOD_ARGR ("countPages", "void", "int")
	DEFINE_METHOD_ARGR ("addItem", "item, parent, index", "bool")
	DEFINE_METHOD_ARGR ("checkItemsIDs", "void", "void")
END_METHOD_NAMES (CommandBarModel)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CommandBarModel::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "getRootItem")
	{
		returnValue = getRootItem ().asUnknown ();
		return true;
	}
	else if(msg == "getParentItem")
	{
		if(Item* item = unknown_cast<Item> (msg[0].asUnknown ()))
			returnValue = getParentItem (item);

		return true;
	}
	else if(msg == "invalidate")
	{
		deferSignal (NEW Message (kChanged));
		return true;
	}
	else if(msg == "loadItemFromFile")
	{
		UnknownPtr<IUrl> path = msg[0].asUnknown ();
		if(path)
		{
			AutoPtr<RootItem> loadObj = NEW RootItem ();
			if(loadObj)
			{
				StorableObject::loadFromFile (*loadObj, *path);
				if(loadObj->countChilds () == 1)
					returnValue.takeShared (ccl_as_unknown (loadObj->getChild (0)));
				else
					returnValue.takeShared (ccl_as_unknown (loadObj));
			}
		}

		return true;
	}
	else if(msg == "createPage")
	{
		AutoPtr<PageItem> page (NEW PageItem);
		returnValue.takeShared (ccl_as_unknown (page));
		return true;
	}
	else if(msg == "getPage")
	{
		PageItem* page = getPage (msg[0]);
		if(page)
			returnValue = page->asUnknown ();

		return true;
	}
	else if(msg == "countPages")
	{
		returnValue = countPages ();
		return true;
	}
	else if(msg == "addItem")
	{
		Item* item = unknown_cast<Item> (msg[0]);
		Item* parent = unknown_cast<Item> (msg[1]);
		int index = msg.getArgCount () > 2 ?  msg[2].asInt () : -1;

		ASSERT (item && parent)
		if(item && parent)
		{
			item->retain ();
			returnValue = addItem (item, parent, index);
		}
		return true;
	}
	else if(msg == "checkItemsIDs")
	{
		checkItemsIDs ();
		return true;
	}
	return SuperClass::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// CommandBar::Item
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (Item, Object, "CommandBar.Item")

//////////////////////////////////////////////////////////////////////////////////////////////////

Item::Item ()
: color (0),
  flags (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API Item::getType () const
{
	ASSERT (0)
	return String::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Item* Item::getChild  (int index) const
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Item::getIndex (Item* item) const
{
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ICommandBarItem* CCL_API Item::getChildItem (int index) const
{
	return getChild (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API Item::getChildIndex (ICommandBarItem* item) const
{
	return getIndex (unknown_cast<Item> (item));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API Item::countChilds () const
{
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Item::acceptsChild (Item* child) const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Iterator* Item::newIterator () const
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Item::addChild (Item* item, int index)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Item::removeChild (Item* item)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Item::load (const Storage& storage)
{
	Attributes& a = storage.getAttributes ();
	id = a.getString ("id");
	title = a.getString ("title");
	color = a.getHexValue ("color");
	flags = a.getInt ("flags");
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Item::save (const Storage& storage) const
{
	Attributes& a = storage.getAttributes ();
	//a.set ("id", id); doesn't need to be persistent for now
	if(!title.isEmpty ())
		a.set ("title", title);
	if(color != kNoColor)
		a.setHexValue ("color", color);
	if(flags)
		a.set ("flags", flags);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_PROPERTY_NAMES (Item)
	DEFINE_PROPERTY_NAME ("id")
	DEFINE_PROPERTY_NAME ("title")
	DEFINE_PROPERTY_NAME ("type")
	DEFINE_PROPERTY_NAME ("numChilds")
	DEFINE_PROPERTY_NAME ("flags")
	DEFINE_PROPERTY_NAME ("isReadOnly")
	DEFINE_PROPERTY_NAME ("isTemporary")
	DEFINE_PROPERTY_NAME ("isLeftClickContextMenu")
END_PROPERTY_NAMES (Item)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Item::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "id")
	{
		var = getID ();
		return true;
	}
	if(propertyId == "title")
	{
		var = getTitle ();
		return true;
	}
	if(propertyId == "type")
	{
		var = getType ();
		return true;
	}
	if(propertyId == "color")
	{
		var = (int64)getColor ();
		return true;
	}
	if(propertyId == "numChilds")
	{
		var = countChilds ();
		return true;
	}
	if(propertyId == "flags")
	{
		var = flags;
		return true;
	}
	if(propertyId == "isReadOnly")
	{
		var = isReadOnly ();
		return true;
	}
	if(propertyId == "isTemporary")
	{
		var = isTemporary ();
		return true;
	}
	if(propertyId == "isLeftClickContextMenu")
	{
		var = isLeftClickContextMenu ();
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Item::setProperty (MemberID propertyId, const Variant& var)
{
	if(propertyId == "id")
	{
		setID (var);
		return true;
	}
	if(propertyId == "title")
	{
		setTitle (var);
		return true;
	}
	if(propertyId == "color")
	{
		setColor ((uint32)var.asLargeInt ());
		return true;
	}
	if(propertyId == "flags")
	{
		flags = var.asInt ();
		return true;
	}
	if(propertyId == "isReadOnly")
	{
		isReadOnly (var.asBool ());
		return true;
	}
	if(propertyId == "isTemporary")
	{
		isTemporary (var.asBool ());
		return true;
	}
	if(propertyId == "isLeftClickContextMenu")
	{
		isLeftClickContextMenu (var.asBool ());
		return true;
	}
	return SuperClass::setProperty (propertyId, var);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (Item)
	DEFINE_METHOD_ARGR ("getChildItem", "int", "Item")
	DEFINE_METHOD_ARGR ("getChildIndex", "Item", "int")
	DEFINE_METHOD_ARGR ("saveToFile", "Url", "bool")
	DEFINE_METHOD_ARGR ("cloneItem", "void", "Item")
END_METHOD_NAMES (Item)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Item::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "getChildItem")
	{
		returnValue = ccl_as_unknown (getChild (msg[0].asInt ()));
		return true;
	}
	else if(msg == "getChildIndex")
	{
		UnknownPtr<ICommandBarItem> item (msg[0]);
		if(item)
			returnValue = getChildIndex (item);

		return true;
	}
	else if(msg == "saveToFile")
	{
		UnknownPtr<IUrl> path = msg[0].asUnknown ();
		returnValue = false;
		if(path)
		{
			AutoPtr<RootItem> saveObj = NEW RootItem ();
			saveObj->addChild (this);
			this->retain ();
			StorableObject::saveToFile (*saveObj, *path);
			returnValue = true;
		}

		return true;
	}
	else if(msg == "cloneItem")
	{
		AutoPtr<Object> clone = this->clone ();
		returnValue.takeShared (ccl_as_unknown (clone));

		return true;
	}

	return SuperClass::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// CommandBar::GroupItem
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (GroupItem, Item, "CommandBar.Group")

//////////////////////////////////////////////////////////////////////////////////////////////////

GroupItem::GroupItem ()
: revision (0)
{
	childItems.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Item* GroupItem::getChild  (int index) const
{
	return (Item*)childItems.at (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int GroupItem::getIndex (Item* item) const
{
	return childItems.index (item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API GroupItem::countChilds () const
{
	return childItems.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GroupItem::acceptsChild (Item* child) const
{
	return child == nullptr || ccl_cast<ButtonItem> (child) || ccl_cast<CustomItem> (child);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Iterator* GroupItem::newIterator () const
{
	return childItems.newIterator ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GroupItem::addChild (Item* item, int index)
{
	if(this->getType () != "Root")
		setRevision (0);

	if(childItems.insertAt (index, item))
		return true;

	return childItems.add (item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GroupItem::removeChild (Item* item)
{
	if(childItems.remove (item))
	{
		if(this->getType () != "Root")
			setRevision (0);

		item->release ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GroupItem::removeAll ()
{
	childItems.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GroupItem::load (const Storage& storage)
{
	Attributes& a = storage.getAttributes ();
	name = a.getString ("name");
	layout = a.getString ("layout");
	revision = a.getInt ("revision");
	a.unqueue (childItems, nullptr, ccl_typeid<Item> ());
	return SuperClass::load (storage);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GroupItem::save (const Storage& storage) const
{
	Attributes& a = storage.getAttributes ();
	if(!name.isEmpty ())
		a.set ("name", name);
	if(!layout.isEmpty ())
		a.set ("layout", layout);
	if(revision > 0)
		a.set ("revision", revision);

	for(auto child : iterate_as<Item> (childItems))
		if(!child->isTemporary ())
			a.queue (nullptr, child, Attributes::kShare);

	return SuperClass::save (storage);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_PROPERTY_NAMES (GroupItem)
	DEFINE_PROPERTY_NAME ("name")
	DEFINE_PROPERTY_NAME ("layout")
	DEFINE_PROPERTY_NAME ("revision")
END_PROPERTY_NAMES (GroupItem)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API GroupItem::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "name")
	{
		var = getName ();
		return true;
	}
	else if(propertyId == "layout")
	{
		var = getLayout ();
		return true;
	}
	else if(propertyId == "revision")
	{
		var = getRevision ();
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API GroupItem::setProperty (MemberID propertyId, const Variant& var)
{
	if(propertyId == "name")
	{
		setName (var);
		return true;
	}
	else if(propertyId == "layout")
	{
		setLayout (var);
		return true;
	}
	else if(propertyId == "revision")
	{
		setRevision (var);
		return true;
	}

	return SuperClass::setProperty (propertyId, var);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (GroupItem)
	DEFINE_METHOD_ARGR ("addChildItem", "Item", "bool")
	DEFINE_METHOD_ARGR ("removeChildItem", "Item", "bool")
	DEFINE_METHOD_ARGR ("removeAll", "void", "void")
END_METHOD_NAMES (GroupItem)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API GroupItem::invokeMethod(Variant& returnValue, MessageRef msg)
{
	if(msg == "addChildItem")
	{
		if(Item* child = unknown_cast<Item> (msg[0].asUnknown ()))
		{
			child->retain ();
			int index = -1;
			if(msg.getArgCount () >= 2)
				index = msg[1].asInt ();

			returnValue = addChild (child, index);
		}
		else
			returnValue = false;

		return true;
	}
	else if(msg == "removeChildItem")
	{
		if(Item* child = unknown_cast<Item> (msg[0].asUnknown ()))
			returnValue = removeChild (child);
		else
			returnValue = false;

		return true;
	}
	else if(msg == "removeAll")
	{
		removeAll ();
		return true;
	}
	return SuperClass::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// CommandBar::TabsItem
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (TabsItem, GroupItem, "CommandBar.Tabs")

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TabsItem::acceptsChild (Item* child) const
{
	return child == nullptr || child->isClass (ccl_typeid<GroupItem> ()) || ccl_cast<CustomItem> (child);
}

//************************************************************************************************
// CommandBar::SetupGroupItem
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (SetupGroupItem, GroupItem, "CommandBar.SetupGroup")

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SetupGroupItem::acceptsChild(Item* child) const
{
	return false;
}

//************************************************************************************************
// CommandBar::PageItem
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (PageItem, GroupItem, "CommandBar.Page")

//////////////////////////////////////////////////////////////////////////////////////////////////

PageItem::PageItem ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PageItem::acceptsChild (Item* child) const
{
	return child == nullptr || ccl_cast<GroupItem> (child) || ccl_cast<CustomItem> (child);
}

//************************************************************************************************
// CommandBar::RootItem
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (RootItem, GroupItem, "CommandBar.Root")

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RootItem::acceptsChild (Item* child) const
{
	if(child == nullptr)
		return true;

	VectorForEachFast (acceptedChildClasses, MetaClass*, metaClass)
		if(child->canCast (*metaClass))
			return true;
	EndFor
	return acceptedChildClasses.isEmpty (); // accept all if nothing specified
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API RootItem::setProperty (MemberID propertyId, const Variant& var)
{
	if(propertyId == "acceptedChildClasses")
	{
		// parse list of class names
		String classes = var.asString ();

		acceptedChildClasses.removeAll ();

		ForEachStringToken (classes, ",", className)
			const MetaClass* metaClass = Kernel::instance ().getClassRegistry ().findType (MutableCString (className));
			ASSERT (metaClass)
			if(metaClass)
				acceptedChildClasses.add (const_cast<MetaClass*> (metaClass));
		EndFor
		return true;
	}
	return SuperClass::setProperty (propertyId, var);
}

//************************************************************************************************
// CommandBar::IconData
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (IconData, Object, "CommandBar.Icon")

//////////////////////////////////////////////////////////////////////////////////////////////////

IconData::IconData (IImage* bitmap)
{
	setBitmap (bitmap);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool IconData::load (const Storage& storage)
{
	Attributes& a = storage.getAttributes ();

	AutoPtr<Bitmap> image;
	{
		Security::Crypto::Material pngData;
		pngData.fromBase64 (a.getString ("data"));
		if(!pngData.isEmpty ())
			image = ccl_cast<Bitmap> (Image::loadImage (pngData.asStream (), FileTypes::png));
	}

	String data2x = a.getString ("data2x");
	if(!data2x.isEmpty ())
	{
		AutoPtr<Bitmap> image2x;
		{
			Security::Crypto::Material pngData;
			pngData.fromBase64 (data2x);
			if(!pngData.isEmpty ())
				image2x = ccl_cast<Bitmap> (Image::loadImage (pngData.asStream (), FileTypes::png));
		}

		if(image && image2x)
		{
			NativeBitmap* nativeBitmap2x = image2x->getNativeBitmap ();
			nativeBitmap2x->setContentScaleFactor (2.f);
			AutoPtr<MultiResolutionBitmap> multiResImage = NEW MultiResolutionBitmap (image->getNativeBitmap (), nativeBitmap2x);
			setBitmap (multiResImage);
		}
	}
	else
		setBitmap (image);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool IconData::save (const Storage& storage) const
{
	Attributes& a = storage.getAttributes ();

	a.set ("type", FileTypes::png.getMimeType ());
	if(MultiResolutionBitmap* bitmap = unknown_cast<MultiResolutionBitmap> (this->bitmap))
	{
		Security::Crypto::Material pngData;
		NativeBitmap* nativeBitmap = bitmap->getNativeBitmap ();
		nativeBitmap->retain ();
		if(Bitmap (nativeBitmap).saveToStream (pngData.asStream (), FileTypes::png))
			a.set ("data", pngData.toBase64 ());

		NativeBitmap* nativeBitmap2x = bitmap->getNativeBitmap2x ();
		nativeBitmap2x->retain ();
		if(Bitmap (nativeBitmap2x).saveToStream (pngData.asStream (), FileTypes::png))
			a.set ("data2x", pngData.toBase64 ());
	}
	else if(Bitmap* bitmap = unknown_cast<Bitmap> (this->bitmap))
	{
		Security::Crypto::Material pngData;
		if(bitmap->saveToStream (pngData.asStream (), FileTypes::png))
			a.set ("data", pngData.toBase64 ());
	}

	return true;
}

//************************************************************************************************
// CommandBar::ButtonItem
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (ButtonItem, Item, "CommandBar.Button")

//////////////////////////////////////////////////////////////////////////////////////////////////

ButtonItem::ButtonItem ()
: controlType (kButton),
  menuContent (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ButtonItem::ButtonItem (const ButtonItem& other)
: Item (other),
  controlType (other.controlType),
  cmdCategory (other.cmdCategory),
  cmdName (other.cmdName),
  target (other.target),
  icon (other.icon),
  menuContent (nullptr)
{
	take_shared (menuContent, other.menuContent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ButtonItem::~ButtonItem ()
{
	safe_release (menuContent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ButtonItem::isExternalTarget () const
{
	return getCommandCategory ().isEmpty () && !getCommandName ().isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API ButtonItem::getType () const
{
	switch(controlType)
	{
	default:
	case kButton:		return CCLSTR ("Button");
	case kMenu:			return CCLSTR ("Menu");
	case kSelectBox:	return CCLSTR ("SelectBox");
	}
	return String::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ButtonItem::parseControlType (StringRef string)
{
	if(string == CCLSTR ("Menu"))
		return kMenu;
	else if(string == CCLSTR ("SelectBox"))
		return kSelectBox;

	return kButton;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MenuGroupItem* ButtonItem::getMenuContent () const
{
	return getControlType () == CommandBar::ButtonItem::kMenu ? menuContent : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ButtonItem::setMenuContent (MenuGroupItem* content)
{
	ASSERT (getControlType () == CommandBar::ButtonItem::kMenu)
	take_shared (menuContent, content);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Iterator* ButtonItem::newIterator () const
{
	if(menuContent)
	{
		// establish parent-child relationship to our menu content (group)
		AutoPtr<ObjectList> list (NEW ObjectList);
		list->add (menuContent);
		return NEW HoldingIterator (list, list->newIterator ());
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ButtonItem::load (const Storage& storage)
{
	Attributes& a = storage.getAttributes ();
	cmdCategory = a.getString ("command.category");
	cmdName = a.getString ("command.name");
	controlType = parseControlType (a.getString ("type"));

	if(AttributeQueue* childAttribs = ccl_cast<AttributeQueue> (a.getObject (nullptr)))
		for(Attribute* attribute : iterate_as<Attribute> (*childAttribs))
		{
			if(IconData* iconData = unknown_cast<IconData> (attribute->getValue ()))
				setIcon (iconData->getBitmap ());
			else if(MenuGroupItem* group = unknown_cast<MenuGroupItem> (attribute->getValue ()))
				setMenuContent (group);
		}

	return SuperClass::load (storage);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ButtonItem::save (const Storage& storage) const
{
	Attributes& a = storage.getAttributes ();
	a.set ("type", getType ());
	if(!cmdCategory.isEmpty ())
		a.set ("command.category", cmdCategory);
	if(!cmdName.isEmpty ())
		a.set ("command.name", cmdName);

	if(icon)
		a.queue (nullptr, NEW IconData (icon), Attributes::kOwns);
	if(menuContent)
		a.queue (nullptr, menuContent, Attributes::kShare);

	return SuperClass::save (storage);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_PROPERTY_NAMES (ButtonItem)
	DEFINE_PROPERTY_NAME ("commandCategory")
	DEFINE_PROPERTY_NAME ("commandName")
	DEFINE_PROPERTY_NAME ("icon")
	DEFINE_PROPERTY_NAME ("menuContent")
END_PROPERTY_NAMES (ButtonItem)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ButtonItem::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "commandCategory")
	{
		var = getCommandCategory ();
		return true;
	}
	else if(propertyId == "commandName")
	{
		var = getCommandName ();
		return true;
	}
	else if(propertyId == "icon")
	{
		var.takeShared (getIcon ());
		return true;
	}
	else if(propertyId == "menuContent")
	{
		if(Item* menuContent = getMenuContent ())
			var.takeShared (ccl_as_unknown (menuContent));
		return true;
	}

	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ButtonItem::setProperty (MemberID propertyId, const Variant& var)
{
	if(propertyId == "commandCategory")
	{
		setCommandCategory (MutableCString (var));
		return true;
	}
	else if(propertyId == "commandName")
	{
		setCommandName (MutableCString (var));
		return true;
	}
	else if(propertyId == "type" && var.isInt ())
	{
		setControlType (var.asInt ());
		return true;
	}
	else if(propertyId == "icon")
	{
		UnknownPtr<IImage> icon (var.asUnknown ());
		setIcon (icon);
		return true;
	}

	return SuperClass::setProperty (propertyId, var);
}

//************************************************************************************************
// CommandBar::CustomItem
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (CustomItem, Item, "CommandBar.CustomItem")

//////////////////////////////////////////////////////////////////////////////////////////////////

CustomItem::CustomItem ()
: customType ("CustomItem")
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API CustomItem::getType () const
{
	return getCustomType ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CustomItem::setProperty (MemberID propertyId, const Variant& var)
{
	if(propertyId == "type")
	{
		setCustomType (var.asString ());
		return true;
	}
	return SuperClass::setProperty (propertyId, var);
}

//************************************************************************************************
// CommandBar::MenuGroupItem
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (MenuGroupItem, GroupItem, "CommandBar.Menu")

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MenuGroupItem::acceptsChild (Item* child) const
{
	return child == nullptr || ccl_cast<ButtonItem> (child) || ccl_cast<MenuGroupItem> (child) || ccl_cast<MenuSeparatorItem> (child);
}

//************************************************************************************************
// CommandBar::MenuItem
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (MenuItem, ButtonItem, "CommandBar.MenuItem")

//************************************************************************************************
// CommandBar::MenuSeparatorItem
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (MenuSeparatorItem, Item, "CommandBar.MenuSeparator")
