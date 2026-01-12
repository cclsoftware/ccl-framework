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
// Filename    : ccl/gui/popup/menu.cpp
// Description : Menu classes
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/popup/menu.h"

#include "ccl/gui/gui.h"
#include "ccl/gui/commands.h"
#include "ccl/gui/keyevent.h"
#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/windows/window.h"
#include "ccl/gui/graphics/imaging/image.h"

#include "ccl/base/kernel.h"
#include "ccl/base/message.h"
#include "ccl/base/asyncoperation.h"
#include "ccl/base/storage/storage.h"
#include "ccl/base/storage/xmlarchive.h"

#include "ccl/public/base/istream.h"
#include "ccl/public/storage/iurl.h"
#include "ccl/public/text/itranslationtable.h"
#include "ccl/public/system/inativefilesystem.h"

#include "ccl/public/systemservices.h"
#include "ccl/public/plugservices.h"

namespace CCL {

//************************************************************************************************
// MenuPlaceholder
//************************************************************************************************

class MenuPlaceholder: public Object
{
public:
	DECLARE_CLASS (MenuPlaceholder, Object)

	PROPERTY_MUTABLE_CSTRING (name, Name)

	// Object
	bool load (const Storage& storage) override
	{
		name = storage.getAttributes ().getCString ("name");
		return true;
	}
};

DEFINE_CLASS (MenuPlaceholder, Object)

//************************************************************************************************
// MenuSeparator
//************************************************************************************************

class MenuSeparator: public Object
{
public:
	DECLARE_CLASS (MenuSeparator, Object)
};

DEFINE_CLASS (MenuSeparator, Object)

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// MenuItemIDSet
//************************************************************************************************

MenuItemIDSet::MenuItemIDSet ()
: currentID (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

MenuItemID MenuItemIDSet::newID ()
{
	// hmm...
	return ++currentID;
}

//************************************************************************************************
// MenuItemBase
//************************************************************************************************

DEFINE_CLASS_HIDDEN (MenuItemBase, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

MenuItemBase::MenuItemBase (ICommandHandler* _handler)
{
	if(_handler)
		setHandler (_handler);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ICommandHandler* MenuItemBase::getHandler () const
{
	return handler;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuItemBase::setHandler (ICommandHandler* _handler)
{
	this->handler = _handler;
}

//************************************************************************************************
// MenuItem
//************************************************************************************************

BEGIN_STYLEDEF (MenuItem::propertyNames)
	{"name", MenuItem::kItemName},
	{"title", MenuItem::kItemTitle},
	{"category", MenuItem::kItemCategory},
	{"handler", MenuItem::kItemHandler},
	{"icon", MenuItem::kItemIcon},
	{"data", MenuItem::kItemData},
	{"checked", MenuItem::kItemChecked},
	{"enabled", MenuItem::kItemEnabled},
	{"italic", MenuItem::kItemItalic},
	{"helpid", MenuItem::kItemHelpId},
	{"description", MenuItem::kDescription},
	{"tooltip", MenuItem::kTooltip},
	{"splitmenu", MenuItem::kSplitMenu},
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (MenuItem, MenuItemBase)

//////////////////////////////////////////////////////////////////////////////////////////////////

MenuItem::MenuItem (Menu* parent, StringRef _name, StringRef _title, Menu* subMenu, ICommandHandler* handler)
: MenuItemBase (handler),
  parent (parent),
  name (_name),
  title (_title),
  subMenu (subMenu),
  splitMenu (nullptr),
  icon (nullptr),
  nativeIcon (nullptr),
  flags (0),
  itemID (0)
{
	if(title.isEmpty ())
		title = name;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MenuItem::~MenuItem ()
{
	if(subMenu)
		subMenu->release ();
	if(splitMenu)
		splitMenu->release ();
	if(icon)
		icon->release ();
	if(nativeIcon)
		nativeIcon->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MenuItem* MenuItem::getPreviousItem () const
{
	if(parent)
	{
		int index = parent->getItemIndex (this, true);
		if(--index >= 0)
			return parent->at (index);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MenuItem* MenuItem::getNextItem () const
{
	if(parent)
	{
		int index = parent->getItemIndex (this, true);
		if(++index < parent->countItems ())
			return parent->at (index);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef MenuItem::getTitle () const
{
	return title;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuItem::setTitle (StringRef _title)
{
	if(title != _title)
	{
		title = _title;

		if(parent)
			parent->updateItem (this);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuItem::setIcon (Image* newIcon)
{
	if(icon != newIcon)
	{
		take_shared (icon, newIcon);
		keepNativeIcon (nullptr); // reset cached icon

		if(parent)
			parent->updateItem (this);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuItem::keepNativeIcon (Image* icon)
{
	take_shared (nativeIcon, icon);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Image* MenuItem::getNativeIcon () const
{
	return nativeIcon;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuItem::enable (bool state)
{
	if(state == isEnabled ())
		return;

	if(state)
		flags &= ~kDisabled;
	else
		flags |= kDisabled;

	if(parent)
		parent->updateItem (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuItem::check (bool state)
{
	if(state == isChecked ())
		return;

	if(state)
		flags |= kChecked;
	else
		flags &= ~kChecked;

	if(parent)
		parent->updateItem (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ICommandHandler* MenuItem::getCommandHandler () const
{
	if(handler)
		return handler;

	if(parent)
		return parent->getHandler ();

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MenuItem::makeCommand (CommandMsgEx& msg) const
{
	static const String strMenu = CCLSTR ("Menu");

	if(!category.isEmpty ())
		msg.setCategory (category);
	else if(parent)
		msg.setCategory (parent->getName ());
	else
		msg.setCategory (strMenu);

	msg.setName (name);
	msg.invoker = (IObject*)this;

	return !msg.name.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuItem::init ()
{
	if(subMenu)
		subMenu->init ();
	else
	{
		// reset checked state in main menu bar, in case command is not handled
		if(parent && ccl_cast<MenuBar> (parent->getParent ()))
			check (false);

		bool result = false;

		CommandMsgEx msg;
		if(makeCommand (msg))
		{
			msg.flags |= CommandMsg::kCheckOnly;

			ICommandHandler* handler = getCommandHandler ();
			if(handler)
				result = handler->interpretCommand (msg) != 0;
			else
				result = CommandTable::instance ().interpretCommand (msg);
		}

		enable (result);
	}

	if(splitMenu)
		splitMenu->init ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuItem::updateKey ()
{
	if(subMenu)
	{
		subMenu->updateKeys ();
		return;
	}

	KeyEvent newKey;
	CommandMsgEx msg;
	if(makeCommand (msg))
		if(const KeyEvent* k = CommandTable::instance ().lookupKeyEvent(Command (msg)))
			newKey = *k;

	if(newKey != cachedKey)
	{
		cachedKey = newKey;
		if(parent)
			parent->updateItem (this);
	}

	if(splitMenu)
		splitMenu->updateKeys ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const KeyEvent* MenuItem::getAssignedKey () const
{
	return cachedKey.isValid () ? &cachedKey : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MenuItem::select ()
{
	CommandMsgEx msg;
	if(!makeCommand (msg))
		return false;

	ICommandHandler* handler = getCommandHandler ();
	if(handler)
	{
		if(CommandTable::instance ().isCommandAllowed (msg))
			if(handler->interpretCommand (msg))
			{
				if(CommandTable::instance ().findCommand (msg.category, msg.name))
					CommandTable::instance ().setLastCommand (msg);
			}
	}
	else
		CommandTable::instance ().interpretCommand (msg);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MenuItem::load (const Storage& storage)
{
	Attributes& a = storage.getAttributes ();

	a.get (name, "name");
	a.get (category, "category");
	a.get (helpID, "helpid");

	a.get (title, "title");
	if(title.isEmpty ())
		title = name;

	if(title != Menu::strSeparator)
	{
		UnknownPtr<ITranslationTable> stringTable (storage.getContextUnknown ("stringTable"));
		if(stringTable)
			stringTable->getStringWithUnicodeKey (title, "Menu", title);

		if(a.getBool ("follow"))
			title.append (Menu::strFollowIndicator);

		UnknownPtr<ICommandHandler> handler (storage.getContextUnknown ("handler"));
		if(handler)
			setHandler (handler);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API MenuItem::getItemAttribute (Variant& value, ItemAttribute id) const
{
	switch(id)
	{
	case kItemName : value = getName (); break;
	case kItemTitle : value = getTitle (); break;
	case kItemCategory : value = getCategory (); break;
	case kItemHandler : value = getHandler (); break;
	case kItemIcon : value = static_cast<IImage*> (getIcon ()); break;
	case kItemData : value = getItemData (); break;
	case kItemChecked : value = isChecked (); break;
	case kItemEnabled : value = isEnabled (); break;
	case kItemItalic : value = isItalic (); break;
	case kItemHelpId : value = getHelpIdentifier (); break;
	case kDescription : value = getDescription (); break;
	case kTooltip : value = getTooltip (); break;
	case kSplitMenu : value = static_cast<IMenu*> (splitMenu); break;
	default :
		return false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API MenuItem::setItemAttribute (ItemAttribute id, VariantRef value)
{
	switch(id)
	{
	case kItemName : setName (value.asString ()); break;
	case kItemTitle : setTitle (value.asString ()); break;
	case kItemCategory : setCategory (value.asString ()); break;
	case kItemHandler : setHandler (UnknownPtr<ICommandHandler> (value.asUnknown ())); break;
	case kItemIcon : setIcon (unknown_cast<Image> (value.asUnknown ())); break;
	case kItemData : setItemData (value); break;
	case kItemChecked : check (value.asBool ()); break;
	case kItemEnabled : enable (value.asBool ()); break;
	case kItemItalic : isItalic (value.asBool ()); break;
	case kItemHelpId : setHelpIdentifier (value.asString ()); break;
	case kDescription : setDescription (value.asString ()); break;
	case kTooltip : setTooltip (value.asString ()); break;
	case kSplitMenu : setSplitMenu (unknown_cast<Menu> (value.asUnknown ())); break;
	default :
		return false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API MenuItem::setProperty (MemberID propertyId, const Variant& var)
{
	for(int i = 0; i < ARRAY_COUNT (propertyNames); i++)
		if(propertyId == propertyNames[i].name)
			return setItemAttribute (propertyNames[i].value, var);

	return SuperClass::setProperty (propertyId, var);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API MenuItem::getProperty (Variant& var, MemberID propertyId) const
{
	for(int i = 0; i < ARRAY_COUNT (propertyNames); i++)
		if(propertyId == propertyNames[i].name)
			return getItemAttribute (var, propertyNames[i].value);

	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMenu* CCL_API MenuItem::getParentMenu () const
{
	return getParent ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMenu* CCL_API MenuItem::getItemMenu () const
{
	return getSubMenu ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuItem::setSplitMenu (Menu* menu)
{
	if(splitMenu)
	{
		splitMenu->parent = nullptr;
		splitMenu->release ();
	}
	splitMenu = menu;
	if(splitMenu)
	{
		splitMenu->parent = this;
		splitMenu->retain ();
	}
}

//************************************************************************************************
// Menu
//************************************************************************************************

BEGIN_STYLEDEF (Menu::propertyNames)
	{"name",    Menu::kMenuName},
	{"title",   Menu::kMenuTitle},
	{"icon",    Menu::kMenuIcon},
	{"data",    Menu::kMenuData},
	{"variant", Menu::kMenuVariant},
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_ABSTRACT_HIDDEN (Menu, MenuItemBase)

//////////////////////////////////////////////////////////////////////////////////////////////////

Menu::Menu (StringRef _name, StringRef _title, ICommandHandler* handler)
: MenuItemBase (handler),
  parent (nullptr),
  name (_name),
  title (_title),
  menuIDs (nullptr),
  icon (nullptr),
  insertPosition (-1),
  separatorNeeded (false),
  scaleFactor (0.f) // 0 means not set
{
	items.objectCleanup ();

	if(title.isEmpty ())
		title = name;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Menu::~Menu ()
{
	if(menuIDs)
		menuIDs->release ();
	if(icon)
		icon->release ();

	for(auto item : iterate_as<MenuItem> (items))
		item->parent = nullptr; // in case someone keeps a reference to a MenuItem
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MenuItemIDSet& Menu::getIDSet () const
{
	if(!menuIDs)
		menuIDs = NEW MenuItemIDSet;
	return *menuIDs;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Menu::setIDSet (MenuItemIDSet* ids)
{
	take_shared<MenuItemIDSet> (menuIDs, ids);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Menu::setTitle (StringRef _title)
{
	if(title != _title)
	{
		title = _title;

		if(parent)
		{
			MenuBar* menuBar = ccl_cast<MenuBar> (parent);
			if(menuBar)
				menuBar->updateMenu (this);
			else
			{
				Menu* parentMenu = ccl_cast<Menu> (parent);
				ASSERT (parentMenu != nullptr)
				if(parentMenu)
					parentMenu->updateSubMenu (this);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Menu::setIcon (Image* newIcon)
{
	take_shared (icon, newIcon);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API Menu::countItems () const
{
	return items.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Menu::getItemIndex (const MenuItem* searchItem, bool countSubMenus) const
{
	int index = -1;
	if(countSubMenus)
		index = items.index (searchItem);
	else
	{
		int i = 0;
		ForEach (items, MenuItem, item)
			if(item->isSubMenu ()) // ignore submenus
				continue;

			if(item == searchItem)
			{
				index = i;
				break;
			}

			i++;
		EndFor
	}
	return index;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MenuItem* Menu::findItem (MenuItemID itemID, bool deep) const
{
	ForEach (items, MenuItem, item)
		if(item->isSubMenu ())
		{
			if(deep)
				if(MenuItem* result = item->getSubMenu ()->findItem (itemID, true))
					return result;
		}
		else if(item->getItemID () == itemID)
			return item;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MenuItem* Menu::findItemWithKey (const KeyEvent& key, bool deep) const
{
	ForEach (items, MenuItem, item)
		if(item->isSubMenu ())
		{
			if(deep)
				if(MenuItem* result = item->getSubMenu ()->findItemWithKey (key, true))
					return result;
		}
		else if(const KeyEvent* k = item->getAssignedKey ())
		{
			if(k->isSimilar (key))
				return item;
		}
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MenuItem* Menu::findCommandItem (StringID category, StringID name, bool deep) const
{
	ForEach (items, MenuItem, item)
		if(item->isSubMenu ())
		{
			if(deep)
				if(MenuItem* result = item->getSubMenu ()->findCommandItem (category, name, true))
					return result;
		}
		else
		{
			CommandMsgEx msg;
			if(item->makeCommand (msg) && msg.category == category && msg.name == name)
				return item;
		}
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MenuItem* Menu::findSubMenuItem (Menu* subMenu) const
{
	if(subMenu)
		ForEach (items, MenuItem, item)
			if(item->getSubMenu () == subMenu)
				return item;
		EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MenuItem* Menu::addItem (StringRef name, StringRef title, ICommandHandler* handler)
{
	MenuItem* item = NEW MenuItem (this, name, title, nullptr, handler);
	addItem (item);
	return item;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MenuItem* Menu::addItem (StringRef title, MenuItemID itemID)
{
	MenuItem* item = NEW MenuItem (this, nullptr, title);
	item->setItemID (itemID);
	addItem (item);
	return item;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MenuItem* Menu::addMenu (Menu* menu, bool reuseIDs)
{
	if(reuseIDs)
		menu->setIDSet (&getIDSet ());

	menu->parent = this;
	MenuItem* item = NEW MenuItem (this, nullptr, nullptr, menu);
	addItem (item);
	return item;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Menu::addItem (MenuItem* item)
{
	addSeparatorIfNeeded ();

	if(item->isRegular () && !item->isValidID ())
		item->setItemID (getIDSet ().newID ());

	bool added = false;
	if(insertPosition >= 0)
	{
		added = items.insertAt (insertPosition, item);
		if(added)
			insertPosition++;
	}

	if(!added)
		items.add (item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Menu::addSeparatorItem ()
{
	separatorNeeded = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Menu::addSeparatorIfNeeded ()
{
	if(separatorNeeded)
	{
		separatorNeeded = false; // reset here to avoid endless recursion

		MenuItem* itemBefore = nullptr;
		if(!isEmpty ())
		{
			if(insertPosition > 0)
				itemBefore = at (insertPosition-1);
			else
				itemBefore = at (countItems ()-1);
		}

		if(itemBefore && !itemBefore->isSeparator ())
			addSeparatorInternal ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MenuItem* Menu::addSeparatorInternal ()
{
	MenuItem* item = NEW MenuItem (this, nullptr, strSeparator);
	addItem (item);
	return item;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Menu::setInsertPosition (int index)
{
	if(index == -1)
		addSeparatorIfNeeded (); // flush separator
	else
	{
		ASSERT (separatorNeeded == false) // this could mess up the menu
		separatorNeeded = false;
	}

	insertPosition = index;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMenuItem* CCL_API Menu::addCommandItem (StringRef title, CStringRef category, CStringRef name, ICommandHandler* handler)
{
	ASSERT (!title.isEmpty ()) // title has to be translated!
	MenuItem* item = addItem (String (name), title, handler);
	item->setCategory (String (category));
	item->updateKey ();
	return item;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Menu::updateItem (MenuItem* item)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Menu::updateSubMenu (Menu* subMenu)
{
	ForEach (items, MenuItem, item)
		if(item->getSubMenu () == subMenu)
		{
			updateItem (item);
			return;
		}
	EndFor
	ASSERT (0) // not found!
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Menu::removeItem (MenuItem* item)
{
	items.remove (item);
	item->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Menu::removeAll ()
{
	while(!items.isEmpty ())
		removeItem ((MenuItem*)items.at (0));

	separatorNeeded = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Menu::updateKeys ()
{
	ForEach (items, MenuItem, item)
		item->updateKey ();
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Menu::init ()
{
	ForEach (items, MenuItem, item)
		item->init ();
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Menu::markForGC ()
{
	// If menu has been used by scripts, check subitems as well
	if(ccl_markGC (this->asUnknown ()) == true)
	{
		ForEach (items, MenuItem, item)
			ccl_markGC (item->asUnknown ());
			if(item->getSubMenu ())
				item->getSubMenu ()->markForGC ();
		EndFor
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Menu::loadItems (UrlRef path, ICommandHandler* handler, IMenuExtension* extension, ITranslationTable* stringTable)
{
	bool result = false;
	AutoPtr<IStream> stream = System::GetFileSystem ().openStream (path, IStream::kOpenMode);
	if(stream)
	{
		AutoPtr<Attributes> context = NEW Attributes;
		if(stringTable)
			context->set ("stringTable", stringTable);
		if(extension)
			context->set ("extension", extension);
		if(handler)
			context->set ("handler", handler);

		XmlArchive archive (*stream, context);
		result = archive.loadObject ("Menu", *this);
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Menu::loadItems (const IAttributeList& _a)
{
	AttributeAccessor a (const_cast<IAttributeList&> (_a));
	while(UnknownPtr<IAttributeList> itemAttr = a.unqueueUnknown ("items"))
	{
		Variant title;
		itemAttr->getAttribute (title, "title");

		if(UnknownPtr<IAttributeList> subMenuAttr = AttributeAccessor (*itemAttr).getUnknown ("subMenu"))
		{
			Variant name;
			itemAttr->getAttribute (name, "name");

			IMenu* subMenu = createMenu ();
			subMenu->setMenuAttribute (kMenuName, name);
			subMenu->setMenuAttribute (kMenuTitle, title);
			addMenu (subMenu)->setItemAttribute (IMenuItem::kItemTitle, title);

			subMenu->loadItems (*subMenuAttr);
		}
		else if(title.asString () == strSeparator)
		{
			addSeparatorInternal ();
		}
		else
		{
			Variant name, category;
			itemAttr->getAttribute (name, "name");
			itemAttr->getAttribute (category, "category");

			IMenuItem* menuItem = addCommandItem (title, MutableCString (category), MutableCString (name), nullptr);

			if(AttributeReadAccessor (*itemAttr).getBool ("disabled"))
				menuItem->setItemAttribute (IMenuItem::kItemEnabled, false);
		}

		itemAttr->release (); // one for UnknownPtr
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Menu::initWithHandler (ICommandHandler* handler)
{
	setHandler (handler);

	ForEach (items, MenuItem, item)
		if(item->getSubMenu ())
			item->getSubMenu ()->initWithHandler (handler);
		else
			item->setHandler (handler);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Menu::saveItems (IAttributeList& _a) const
{
	// init to determine enabled states (start from root menu)
	if(!parent)
		ccl_const_cast (this)->init ();

	AttributeAccessor a (_a);
	IAttributeQueue* itemQueue = a.newAttributeQueue ();
	a.set ("items", itemQueue, IAttributeList::kOwns);

	ForEach (items, MenuItem, item)
		IAttributeList* itemAttr = a.newAttributes ();
		itemQueue->addValue (itemAttr, Attributes::kOwns);

		if(Menu* subMenu = item->getSubMenu ())
		{
			Variant name, title;
			subMenu->getMenuAttribute (name, kMenuName);
			subMenu->getMenuAttribute (title, kMenuTitle);
			itemAttr->setAttribute ("name", name);
			itemAttr->setAttribute ("title", title);

			IAttributeList* a2 = a.newAttributes ();
			itemAttr->setAttribute ("subMenu", a2, IAttributeList::kOwns);
			subMenu->saveItems (*a2);
		}
		else if(item->isSeparator ())
		{
			itemAttr->setAttribute ("title", strSeparator);
		}
		else if(item->isRegular ())
		{
			// LATER TODO: menu headers and other extended items!?

			Variant name, category, title;
			item->getItemAttribute (name, IMenuItem::kItemName);
			item->getItemAttribute (category, IMenuItem::kItemCategory);
			item->getItemAttribute (title, IMenuItem::kItemTitle);

			itemAttr->setAttribute ("name", name);
			itemAttr->setAttribute ("category", category);
			itemAttr->setAttribute ("title", title);
			if(!item->isEnabled ())
				itemAttr->setAttribute ("disabled", true);
		}
	EndFor
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Menu::load (const Storage& storage)
{
	Attributes& a = storage.getAttributes ();

	a.get (name, "name");
	a.get (variant, "variant");

	a.get (title, "title");
	if(title.isEmpty ())
		title = name;

	UnknownPtr<ITranslationTable> stringTable (storage.getContextUnknown ("stringTable"));
	if(stringTable)
		stringTable->getStringWithUnicodeKey (title, "Menu", title);

	Object* obj;
	while((obj = a.unqueueObject (nullptr)) != nullptr)
	{
		if(obj->canCast (ccl_typeid<MenuItem> ()))
		{
			MenuItem* item = (MenuItem*)obj;
			item->parent = this;
			addItem (item);
		}
		else if (obj->canCast (ccl_typeid<Menu> ()))
		{
			Menu* menu = (Menu*)obj;
			menu->setIDSet (&getIDSet ());
			menu->parent = this;
			for(int i = 0; i < menu->countItems (); i++)
				menu->at (i)->setItemID (menu->getIDSet ().newID ());
			addItem (NEW MenuItem (this, nullptr, nullptr, menu));
		}
		else if(obj->canCast (ccl_typeid<MenuSeparator> ()))
		{
			addSeparatorInternal (); // enforce separator
			obj->release ();
		}
		else if(obj->canCast (ccl_typeid<MenuPlaceholder> ()))
		{
			MenuPlaceholder* placeholder = (MenuPlaceholder*)obj;
			UnknownPtr<IMenuExtension> extension (storage.getContextUnknown ("extension"));
			if(extension)
				extension->extendMenu (*this, placeholder->getName ());
			placeholder->release ();
		}
		else
			obj->release ();
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API Menu::getParentUnknown () const
{
	Object* p = getParent ();
	return p ? p->asUnknown () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float Menu::getScaleFactorRecursive () const
{
	if(scaleFactor > 0.f)
		return scaleFactor;
	if(Menu* parentMenu = ccl_cast<Menu> (parent))
		return parentMenu->getScaleFactorRecursive ();
	return 1.f;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Menu::getMenuAttribute (Variant& value, MenuAttribute id) const
{
	switch(id)
	{
	case kMenuName : value = getName (); break;
	case kMenuTitle : value = getTitle (); break;
	case kMenuIcon : value = static_cast<IImage*> (getIcon ()); break;
	case kMenuData : value = getMenuData (); break;
	case kMenuVariant : value = getVariant (); break;
	case kMenuScaleFactor : value = getScaleFactorRecursive (); break;
	default :
		return false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Menu::setMenuAttribute (MenuAttribute id, VariantRef value)
{
	switch(id)
	{
	case kMenuName : setName (value.asString ()); break;
	case kMenuTitle : setTitle (value.asString ()); break;
	case kMenuIcon : setIcon (unknown_cast<Image> (value.asUnknown ())); break;
	case kMenuData : setMenuData (value); break;
	case kMenuVariant : setVariant (value); break;
	case kMenuScaleFactor : setScaleFactor (value.asFloat ()); break;
	default :
		return false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Menu::setProperty (MemberID propertyId, const Variant& var)
{
	for(int i = 0; i < ARRAY_COUNT (propertyNames); i++)
		if(propertyId == propertyNames[i].name)
			return setMenuAttribute (propertyNames[i].value, var);

	if(propertyId == "insertPosition")
	{
		setInsertPosition (var.asInt ());
		return true;
	}

	return SuperClass::setProperty (propertyId, var);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Menu::getProperty (Variant& var, MemberID propertyId) const
{
	for(int i = 0; i < ARRAY_COUNT (propertyNames); i++)
		if(propertyId == propertyNames[i].name)
			return getMenuAttribute (var, propertyNames[i].value);

	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (Menu)
	DEFINE_METHOD_NAME ("countItems")
	DEFINE_METHOD_ARGR ("getItem", "index", "MenuItem")
	DEFINE_METHOD_ARGR ("addCommandItem", "title, category, name", "MenuItem")
	DEFINE_METHOD_NAME ("addSeparatorItem")
	DEFINE_METHOD_ARGR ("addMenu", "menu", "MenuItem")
	DEFINE_METHOD_ARGR ("createMenu", nullptr, "Menu")
	DEFINE_METHOD_ARGR ("loadItems", "path, handler", "bool")
	DEFINE_METHOD_ARGR ("findCommandItem", "category, name, deep", "MenuItem")
END_METHOD_NAMES (Menu)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Menu::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "countItems")
	{
		returnValue = countItems ();
		return true;
	}
	else if(msg == "getItem")
	{
		int index = msg[0].asInt ();
		returnValue.takeShared (getItem (index));
		return true;
	}
	else if(msg == "addCommandItem")
	{
		String title (msg[0].asString ());
		MutableCString category (msg[1].asString ());
		MutableCString name (msg[2].asString ());
		UnknownPtr<ICommandHandler> handler (msg.getArgCount () > 3 ? msg[3].asUnknown () : nullptr);

		IMenuItem* item = addCommandItem (title, category, name, handler);
		returnValue.takeShared (item);
		return true;
	}
	else if(msg == "addSeparatorItem")
	{
		addSeparatorItem ();
		return true;
	}
	else if(msg == "addMenu")
	{
		UnknownPtr<IMenu> menu (msg[0]);
		if(menu)
		{
			menu->retain ();
			IMenuItem* item = addMenu (menu);
			returnValue.takeShared (item);
		}
		return true;
	}
	else if(msg == "createMenu")
	{
		AutoPtr<IMenu> menu (createMenu ());
		returnValue.takeShared (menu);
		return true;
	}
	else if(msg == "loadItems")
	{
		UnknownPtr<IUrl> path (msg[0].asUnknown ());
		UnknownPtr<ICommandHandler> handler (msg.getArgCount () > 1 ? msg[1].asUnknown () : nullptr);
		returnValue = path ? loadItems (*path, handler) : false;
		return true;
	}
	else if(msg == "findCommandItem")
	{
		if(msg.getArgCount () >= 2)
		{
			MutableCString category (msg[0].asString ());
			MutableCString name (msg[1].asString ());
			bool deep = msg.getArgCount () > 2 ? msg[2].asBool () : false;

			if(MenuItem* item = findCommandItem (category, name, deep))
				returnValue.takeShared (item->asUnknown ());
		}
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// PopupMenu
//************************************************************************************************

int PopupMenu::activePlatformMenus = 0;
bool PopupMenu::isPlatformMenuActive () { return activePlatformMenus > 0; }

//////////////////////////////////////////////////////////////////////////////////////////////////

PopupMenu* PopupMenu::create (StringRef name, StringRef title)
{
	// create derived platform specific class via class registry
	Object* object = Kernel::instance ().getClassRegistry ().createObject (ClassID::Menu);
	PopupMenu* menu = ccl_cast<PopupMenu> (object);
	if(menu)
	{
		menu->setName (name);
		menu->setTitle (title);
	}
	return menu;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (PopupMenu, Menu)

//////////////////////////////////////////////////////////////////////////////////////////////////

PopupMenu::PopupMenu (StringRef name, StringRef title)
: Menu (name, title)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

PopupMenu::~PopupMenu ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

UIDRef CCL_API PopupMenu::getMenuClass () const
{
	return ClassID::Menu;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMenu* CCL_API PopupMenu::createMenu () const
{
	return PopupMenu::create ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PopupMenu::isAttached () const
{
	MenuBar* menuBar = ccl_cast<MenuBar> (parent);
	if(menuBar && menuBar->isAttached ())
		return true;
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* PopupMenu::popupAsync (const Point& _where, View* view)
{
	Point where (_where);
	if(view)
		view->clientToScreen (where);

	IWindow* w = view ? view->getWindow () : nullptr;
	if(!w)
		w = Desktop.getDialogParentWindow ();

	if(isPlatformMenu ())
		activePlatformMenus++;
	Promise nativePromise (popupPlatformMenu (where, w));
	return return_shared<IAsyncOperation> (nativePromise.then (this, &PopupMenu::onMenuClosed));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PopupMenu::onMenuClosed (IAsyncOperation& nativeOperation)
{
	if(isPlatformMenu ())
		activePlatformMenus--;
	ASSERT (activePlatformMenus >= 0)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MenuItem* PopupMenu::popup (const Point& where, View* view)
{
	Promise promise (popupAsync (where, view));
	while(promise->getState () == AsyncOperation::kStarted)
		GUI.flushUpdates ();

	MenuItemID itemID = (MenuItemID)promise->getResult ().asInt ();
	MenuItem* item = itemID ? findItem (itemID) : nullptr;
	return item;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PopupMenu::addItem (MenuItem* item)
{
	Menu::addItem (item);
	realizeItem (item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PopupMenu::removeItem (MenuItem* item)
{
	unrealizeItem (item);
	Menu::removeItem (item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PopupMenu::realizeItem (MenuItem* item)
{
	CCL_NOT_IMPL ("PopupMenu::realizeItem")
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PopupMenu::unrealizeItem (MenuItem* item)
{
	CCL_NOT_IMPL ("PopupMenu::unrealizeItem")
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* PopupMenu::popupPlatformMenu (const Point& where, IWindow* window)
{
	CCL_NOT_IMPL ("PopupMenu::popupPlatformMenu")
	return nullptr;
}

//************************************************************************************************
// MenuBar
//************************************************************************************************

DEFINE_CLASS (MenuBar, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

MenuBar::MenuBar ()
: window (nullptr)
{
	menus.objectCleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MenuBar::~MenuBar ()
{
	menus.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MenuBar::addMenu (Menu* menu)
{
	return insertMenu (menu, -1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MenuBar::insertMenu (Menu* _menu, int index)
{
	PopupMenu* menu = ccl_cast<PopupMenu> (_menu);
	ASSERT (menu != nullptr)
	if(!menu)
		return false;

	menu->parent = this;
	if(!menus.insertAt (index, menu))
		menus.add (menu);

	insertPlatformMenu (menu);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MenuBar::removeMenu (Menu* _menu)
{
	PopupMenu* menu = ccl_cast<PopupMenu> (_menu);
	ASSERT (menu != nullptr)
	if(!menu)
		return false;

	removePlatformMenu (menu);

	menus.remove (menu);
	menu->parent = nullptr;
	menu->release ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuBar::updateMenu (Menu* menu)
{
	CCL_NOT_IMPL ("MenuBar::updateMenu")
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuBar::activatePlatformMenu ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuBar::insertPlatformMenu (PopupMenu* menu)
{
	CCL_NOT_IMPL ("MenuBar::insertPlatformMenu")
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuBar::removePlatformMenu (PopupMenu* menu)
{
	CCL_NOT_IMPL ("MenuBar::removePlatformMenu")
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuBar::updateKeys ()
{
	ForEach (menus, Menu, m)
		m->updateKeys ();
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuBar::init ()
{
	ForEach (menus, Menu, m)
		m->init ();
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API MenuBar::loadMenus (UrlRef path, IMenuExtension* extension, ITranslationTable* stringTable)
{
	bool result = false;
	AutoPtr<IStream> stream = System::GetFileSystem ().openStream (path, IStream::kOpenMode);
	if(stream)
	{
		AutoPtr<Attributes> context = NEW Attributes;
		if(stringTable)
			context->set ("stringTable", stringTable);
		if(extension)
			context->set ("extension", extension);

		XmlArchive archive (*stream, context);
		result = archive.loadObject ("MenuBar", *this);
	}

	if(result)
		updateKeys ();

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MenuBar::load (const Storage& storage)
{
	Attributes& a = storage.getAttributes ();

	#if DEBUG_LOG
	a.dump();
	#endif

	Menu* menu;
	while((menu = (Menu*)a.unqueueObject (nullptr, ccl_typeid<Menu> ())) != nullptr)
		addMenu (menu);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API MenuBar::countMenus () const
{
	return menus.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMenu* CCL_API MenuBar::getMenu (int index) const
{
	return (Menu*)menus.at (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMenu* CCL_API MenuBar::findMenu (StringRef name) const
{
	ForEach (menus, Menu, m)
		if(m->getName () == name)
			return m;
	EndFor
	return nullptr;
}
