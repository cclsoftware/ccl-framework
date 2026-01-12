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
// Filename    : ccl/gui/controls/commandbar/commandbarmodel.h
// Description : Command Bar Model
//
//************************************************************************************************

#ifndef _ccl_commandbarmodel_h
#define _ccl_commandbarmodel_h

#include "ccl/base/storage/storableobject.h"
#include "ccl/base/collections/objectarray.h"

#include "ccl/public/gui/framework/icommandeditor.h"
#include "ccl/public/gui/graphics/iimage.h"

#include "ccl/public/text/cclstring.h"
#include "ccl/public/text/cstring.h"

namespace CCL {

namespace CommandBar {
struct InsertContext;
class Item;
class ButtonItem;
class PageItem;
class MenuGroupItem;
}

//************************************************************************************************
// CommandBarModel
//************************************************************************************************

class CommandBarModel: public StorableObject,
					   public ICommandBarModel
{
public:
	DECLARE_CLASS (CommandBarModel, StorableObject)
	DECLARE_METHOD_NAMES (CommandBarModel)

	CommandBarModel ();
	~CommandBarModel ();

	CommandBar::Item& getRootItem () const;
	CommandBar::Item* findItem (StringRef id) const;
	CommandBar::Item* findParentItem (CommandBar::Item* item) const;

	template<class Lambda>
	CommandBar::Item* findItem (const Lambda recognize) const;

	bool adjustInsertContext (CommandBar::Item* item, CommandBar::InsertContext& context) const;

	bool addItem (CommandBar::Item* item, CommandBar::InsertContext& context);
	bool addItem (CommandBar::Item* item, CommandBar::Item* parent, int index = -1);
	bool removeItem (CommandBar::Item* item);
	bool setItemProperty (CommandBar::Item* item, MemberID propertyId, const Variant& var);

	int countPages () const;
	CommandBar::PageItem* getPage (int pageIndex) const;
	int getPageIndex (CommandBar::PageItem* page) const;

	// ICommandBarModel
	ICommandBarItem* CCL_API getItemByID (StringRef id) const override;
	ICommandBarItem* CCL_API getParentItem (ICommandBarItem* item) const override;
	const ICommandBarItem* CCL_API addCommandItem (StringRef type, StringRef title, const ICommandBarItem* parentItem, int index = -1) override;
	tbool CCL_API removeCommandItem (ICommandBarItem* item) override;
	tbool CCL_API setItemProperty (const ICommandBarItem* item, CStringRef propertyId, const Variant& var) override;

	// Object
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;

	CLASS_INTERFACE (ICommandBarModel, StorableObject)

protected:
	CommandBar::Item& rootItem;

	void checkItemsIDs ();

	// Object
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

namespace CommandBar {

//************************************************************************************************
// CommandBar::Item
//************************************************************************************************

class Item: public Object,
			public ICommandBarItem
{
public:
	DECLARE_CLASS (Item, Object)
	DECLARE_PROPERTY_NAMES (Item)
	DECLARE_METHOD_NAMES (Item)

	Item ();

	PROPERTY_STRING (id, ID)
	PROPERTY_STRING (title, Title)
	PROPERTY_VARIABLE (uint32, color, Color)

	enum { kNoColor = 0 };

	// ICommandBarItem
	StringRef CCL_API getType () const override;
	int CCL_API countChilds () const override;
	ICommandBarItem* CCL_API getChildItem (int index) const override;
	int CCL_API getChildIndex (ICommandBarItem* item) const override;

	virtual Item* getChild (int index) const;
	virtual int getIndex (Item* item) const;
	virtual bool acceptsChild (Item* child) const;
	virtual Iterator* newIterator () const;

	virtual bool addChild (Item* item, int index = -1);
	virtual bool removeChild (Item* item);

	// flags 0..15 are reserved for custom usage
	PROPERTY_FLAG (flags, 1<<16, isReadOnly)
	PROPERTY_FLAG (flags, 1<<17, isTemporary)
	PROPERTY_FLAG (flags, 1<<18, isLeftClickContextMenu)

	// Object
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;

	CLASS_INTERFACE (ICommandBarItem, Object)

protected:
	int flags;
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

//************************************************************************************************
// CommandBar::InsertContext
//************************************************************************************************

struct InsertContext
{
	Item* parent;
	int index;

	InsertContext (Item* parent = nullptr, int index = -1)
	: parent (parent), index (index)
	{}
};

//************************************************************************************************
// CommandBar::ItemTraverser
//************************************************************************************************

class ItemTraverser
{
public:
	ItemTraverser ();
	bool traverse (Item* item);

protected:
	Item* parentItem;	///< parent of visited item

	virtual bool visit (Item* item) = 0; ///< return false to cancel traversal
};

//************************************************************************************************
// CommandBar::CommandTarget
//************************************************************************************************

class CommandTarget: public Object
{
public:
	PROPERTY_STRING (name, Name)
	PROPERTY_STRING (title, Title)
	PROPERTY_STRING (category, Category)
	PROPERTY_SHARED_AUTO (IImage, icon, Icon)

	void fromProperties (const IObject& object);
};

//************************************************************************************************
// CommandBar::IconData
//************************************************************************************************

class IconData: public Object
{
public:
	DECLARE_CLASS (IconData, Object)

	IconData (IImage* bitmap = nullptr);

	PROPERTY_SHARED_AUTO (IImage, bitmap, Bitmap)

	// Object
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;
};

//************************************************************************************************
// CommandBar::ButtonItem
//************************************************************************************************

class ButtonItem: public Item
{
public:
	DECLARE_CLASS (ButtonItem, Item)
	DECLARE_PROPERTY_NAMES (ButtonItem)

	ButtonItem ();
	ButtonItem (const ButtonItem&);
	~ButtonItem ();

	bool isExternalTarget () const;

	PROPERTY_MUTABLE_CSTRING (cmdCategory, CommandCategory)
	PROPERTY_MUTABLE_CSTRING (cmdName, CommandName)
	PROPERTY_SHARED_AUTO (CommandTarget, target, Target)
	PROPERTY_SHARED_AUTO (IImage, icon, Icon)

	PROPERTY_VARIABLE (int, controlType, ControlType)

	enum ControlType
	{
		kButton = 0,
		kMenu,
		kSelectBox
	};

	MenuGroupItem* getMenuContent () const;
	void setMenuContent (MenuGroupItem* menuContent);

private:
	MenuGroupItem* menuContent;

	static int parseControlType (StringRef string);

	// Item
	StringRef CCL_API getType () const override;
	Iterator* newIterator () const override;

	// Object
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
};

//************************************************************************************************
// CommandBar::GroupItem
//************************************************************************************************

class GroupItem: public Item
{
public:
	DECLARE_CLASS (GroupItem, Item)
	DECLARE_PROPERTY_NAMES (GroupItem)
	DECLARE_METHOD_NAMES (GroupItem)

	GroupItem ();

	void removeAll ();
	bool addChild (Item* item, int index = -1) override;
	bool removeChild (Item* item) override;
	Item* getChild  (int index) const override;
	int CCL_API countChilds () const override;

	PROPERTY_STRING (name, Name)
	PROPERTY_STRING (layout, Layout)
	PROPERTY_VARIABLE (int, revision, Revision)

	// Item
	Iterator* newIterator () const override;

protected:
	ObjectArray childItems;

	// Item
	StringRef CCL_API getType () const override { return CCLSTR ("Group"); }
	int getIndex (Item* item) const override;
	bool acceptsChild (Item* child) const override;

	// Object
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;

	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

//************************************************************************************************
// MenuGroupItem
//************************************************************************************************

class MenuGroupItem: public GroupItem
{
public:
	DECLARE_CLASS (MenuGroupItem, GroupItem)

	// GroupItem
	bool acceptsChild (Item* child) const override;
};

//************************************************************************************************
// MenuItem
//************************************************************************************************

class MenuItem: public ButtonItem
{
public:
	DECLARE_CLASS (MenuItem, ButtonItem)
};

//************************************************************************************************
// MenuSeparatorItem
//************************************************************************************************

class MenuSeparatorItem: public Item
{
public:
	StringRef CCL_API getType () const override { return CCLSTR ("Separator"); }
	DECLARE_CLASS (MenuSeparatorItem, Item)
};

//************************************************************************************************
// CommandBar::TabsItem
//************************************************************************************************

class TabsItem: public GroupItem
{
public:
	DECLARE_CLASS (TabsItem, GroupItem)

protected:
	// GroupItem
	StringRef CCL_API getType () const override { return CCLSTR ("Tabs"); }
	bool acceptsChild (Item* child) const override;
};

//************************************************************************************************
// CommandBar::CustomItem
//************************************************************************************************

class CustomItem: public Item
{
public:
	DECLARE_CLASS (CustomItem, Item)

	CustomItem ();

	PROPERTY_STRING (customType, CustomType)

private:
	// Item
	StringRef CCL_API getType () const override;

	// Object
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
};

//************************************************************************************************
// CommandBar::SetupGroupItem
//************************************************************************************************

class SetupGroupItem: public GroupItem
{
public:
	DECLARE_CLASS (SetupGroupItem, GroupItem)

protected:
	// GroupItem
	StringRef CCL_API getType () const override { return CCLSTR ("SetupGroup"); }
	bool acceptsChild (Item* child) const override;
};

//************************************************************************************************
// CommandBar::PageItem
//************************************************************************************************

class PageItem: public GroupItem
{
public:
	DECLARE_CLASS (PageItem, GroupItem)

	PageItem ();

protected:
	// GroupItem
	StringRef CCL_API getType () const override { return CCLSTR ("Page"); }
	bool acceptsChild (Item* child) const override;
};

//************************************************************************************************
// CommandBar::RootItem
//************************************************************************************************

class RootItem: public GroupItem
{
public:
	DECLARE_CLASS (RootItem, GroupItem)

protected:
	// GroupItem
	StringRef CCL_API getType () const override { return CCLSTR ("Root"); }
	bool acceptsChild (Item* child) const override;
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;

private:
	Vector<MetaClass*> acceptedChildClasses;
};

} // namespace CommandBar

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Lambda>
struct FindItemTraverser: public CommandBar::ItemTraverser
{
	const Lambda recognize;
	CommandBar::Item* foundItem;

	FindItemTraverser (const Lambda recognize) : recognize (recognize), foundItem (nullptr) {}

	bool visit (CommandBar::Item* item) override
	{
		if(recognize (item))
		{
			foundItem = item;
			return false;
		}
		return true;
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Lambda>
inline CommandBar::Item* CommandBarModel::findItem (const Lambda recognize) const
{
	FindItemTraverser<Lambda> t (recognize);
	t.traverse (&rootItem);
	return t.foundItem;
}

} // namespace CCL

#endif // _ccl_commandbarmodel_h
