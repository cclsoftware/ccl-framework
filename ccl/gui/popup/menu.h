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
// Filename    : ccl/gui/popup/menu.h
// Description : Menu classes
//
//************************************************************************************************

#ifndef _ccl_menu_h
#define _ccl_menu_h

#include "ccl/public/base/variant.h"
#include "ccl/public/gui/icommandhandler.h"
#include "ccl/public/gui/framework/imenu.h"
#include "ccl/public/gui/framework/styleflags.h"
#include "ccl/public/gui/framework/guievent.h"

#include "ccl/base/collections/objectarray.h"

namespace CCL {

class Menu;
class MenuBar;
class View;
class Window;
class Image;
class MenuItemIDSet;

interface IWindow;
interface IAsyncOperation;
struct CommandMsgEx;

/** Menu Item identifier. */
typedef int MenuItemID;

//************************************************************************************************
// MenuItemBase
//************************************************************************************************
// TODO: extract all common aspects of Menu/MenuItem to base class!

class MenuItemBase: public Object
{
public:
	DECLARE_CLASS (MenuItemBase, Object)

	MenuItemBase (ICommandHandler* handler = nullptr);

	ICommandHandler* getHandler () const;
	void setHandler (ICommandHandler* handler);

protected:
	SharedPtr<ICommandHandler> handler;
};

//************************************************************************************************
// MenuItem
//************************************************************************************************

class MenuItem: public MenuItemBase,
				public IMenuItem
{
public:
	DECLARE_CLASS (MenuItem, MenuItemBase)

	MenuItem (Menu* parent = nullptr,
			  StringRef name = nullptr,
			  StringRef title = nullptr,
			  Menu* subMenu = nullptr,
			  ICommandHandler* handler = nullptr);
	~MenuItem ();

	DECLARE_STYLEDEF (propertyNames)

	enum Flags
	{
		kChecked  = 1<<0,
		kDisabled = 1<<1,
		kItalic   = 1<<2,
		kIsHeader = 1<<3
	};

	Menu* getParent () const;
	Menu* getSubMenu () const;
	Menu* getSplitMenu () const;
	MenuItem* getPreviousItem () const;	///< get previous item in parent menu
	MenuItem* getNextItem () const;		///< get next item in parent menu

	PROPERTY_OBJECT   (Variant, itemData, ItemData) ///< arbitrary data associated with this item
	PROPERTY_VARIABLE (MenuItemID, itemID, ItemID)
	PROPERTY_STRING   (name, Name)
	PROPERTY_STRING   (category, Category)
	PROPERTY_STRING   (helpID, HelpIdentifier)
	PROPERTY_STRING   (description, Description)
	PROPERTY_STRING   (tooltip, Tooltip)

	ICommandHandler* getCommandHandler () const; ///< get handler recursive

	bool isSeparator () const;
	bool isSubMenu () const;
	bool isRegular () const;
	bool isValidID () const;

	StringRef getTitle () const;
	void setTitle (StringRef title);

	Image* getIcon () const;
	void setIcon (Image* icon);
	void keepNativeIcon (Image* icon); ///< temporary, if image conversion is required
	Image* getNativeIcon () const;

	void enable (bool state = true);
	bool isEnabled () const;

	void check (bool state = true);
	bool isChecked () const;

	PROPERTY_FLAG (flags, kItalic, isItalic)
	PROPERTY_FLAG (flags, kIsHeader, isHeader)

	bool makeCommand (CommandMsgEx& msg) const;

	const KeyEvent* getAssignedKey () const;

	void updateKey ();		///< update keyboard shortcut
	virtual void init ();	///< when menu becomes visible
	bool select ();			///< menu item has been choosen by user

	// MenuItemBase
	bool load (const Storage& storage) override;

	CLASS_INTERFACE (IMenuItem, MenuItemBase)

protected:
	friend class Menu;
	Menu* parent;
	String title;
	Image* icon;
	Image* nativeIcon;
	Menu* subMenu;
	Menu* splitMenu;
	int flags;
	KeyEvent cachedKey;

	void setSplitMenu (Menu* menu);

	// IMenuItem
	tbool CCL_API getItemAttribute (Variant& value, ItemAttribute id) const override;
	tbool CCL_API setItemAttribute (ItemAttribute id, VariantRef value) override;
	IMenu* CCL_API getParentMenu () const override;
	IMenu* CCL_API getItemMenu () const override;
	tbool CCL_API selectItem () override { return select (); }

	// IObject
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
};

//************************************************************************************************
// Menu
//************************************************************************************************

class Menu: public MenuItemBase,
			public IMenu

{
public:
	DECLARE_CLASS_ABSTRACT (Menu, MenuItemBase)
	DECLARE_METHOD_NAMES (Menu)

	Menu (StringRef name = nullptr, StringRef title = nullptr, ICommandHandler* handler = nullptr);
	~Menu ();

	DECLARE_STYLEDEF (propertyNames)

	Object* getParent () const;

	MenuItemIDSet& getIDSet () const;
	void setIDSet (MenuItemIDSet* ids);

	PROPERTY_OBJECT (Variant, menuData, MenuData) ///< arbitrary data associated with this menu
	PROPERTY_STRING (name, Name)
	PROPERTY_STRING (variant, Variant)
	PROPERTY_VARIABLE (float, scaleFactor, ScaleFactor)
	float getScaleFactorRecursive () const;

	PROPERTY_STRING (initialSubMenuPath, InitialSubMenuPath)

	StringRef getTitle () const;
	void setTitle (StringRef title);

	Image* getIcon () const;
	void setIcon (Image* icon);

	bool isEmpty () const;
	int CCL_API countItems () const override; ///< [IMenu]
	MenuItem* at (int idx) const;
	int getItemIndex (const MenuItem* item, bool countSubMenus) const;
	MenuItem* findItem (MenuItemID itemID, bool deep = true) const;
	MenuItem* findItemWithKey (const KeyEvent& key, bool deep = true) const;
	MenuItem* findCommandItem (StringID category, StringID name, bool deep = false) const;
	MenuItem* findSubMenuItem (Menu* subMenu) const;

	MenuItem* addItem (StringRef name, StringRef title = nullptr, ICommandHandler* handler = nullptr);
	MenuItem* addItem (StringRef title, MenuItemID itemID);
	MenuItem* addMenu (Menu* menu, bool reuseIDs = true);
	void CCL_API addSeparatorItem () override; ///< [IMenu]

	virtual void addItem (MenuItem* item);
	virtual void updateItem (MenuItem* item);
	virtual void updateSubMenu (Menu* subMenu);
	virtual void removeItem (MenuItem* item);

	void updateKeys ();
	void init ();

	void markForGC (); ///< remove script references to all menu items

	// MenuItemBase
	bool load (const Storage& storage) override;

	CLASS_INTERFACE (IMenu, MenuItemBase)

protected:
	friend class MenuItem;
	Object* parent;
	String title;
	Image* icon;
	ObjectArray items;
	mutable MenuItemIDSet* menuIDs;
	int insertPosition;
	bool separatorNeeded;

	void addSeparatorIfNeeded ();
	MenuItem* addSeparatorInternal ();

	// IMenu
	IUnknown* CCL_API getParentUnknown () const override;
	tbool CCL_API getMenuAttribute (Variant& value, MenuAttribute id) const override;
	tbool CCL_API setMenuAttribute (MenuAttribute id, VariantRef value) override;
	IMenuItem* CCL_API getItem (int index) const override { return at (index); }
	int CCL_API getItemIndex (const IMenuItem* item) const override { return getItemIndex (unknown_cast<MenuItem> (item), true); }
	void CCL_API setInsertPosition (int index) override;
	IMenuItem* CCL_API addCommandItem (StringRef title, CStringRef category, CStringRef name, ICommandHandler* handler) override;
	IMenuItem* CCL_API findICommandItem (CStringRef category = nullptr, CStringRef name = nullptr, tbool deep = false) const override { return findCommandItem (category, name, deep != 0); }
	IMenuItem* CCL_API addMenu (IMenu* menu) override { return addMenu (unknown_cast<Menu> (menu)); }
	void CCL_API removeItem (IMenuItem* item) override { return removeItem (unknown_cast<MenuItem> (item)); }
	void CCL_API removeAll () override;
	tbool CCL_API loadItems (UrlRef path, ICommandHandler* handler = nullptr, IMenuExtension* extension = nullptr, ITranslationTable* stringTable = nullptr) override;
	tbool CCL_API loadItems (const IAttributeList& a) override;
	tbool CCL_API saveItems (IAttributeList& a) const override;
	void CCL_API initWithHandler (ICommandHandler* handler) override;

	// IObject
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

//************************************************************************************************
// MenuItemIDSet
//************************************************************************************************

class MenuItemIDSet: public Object
{
public:
	MenuItemIDSet ();

	MenuItemID newID ();

protected:
	MenuItemID currentID;
};

//************************************************************************************************
// PopupMenu
/** System popup menu. */
//************************************************************************************************

class PopupMenu: public Menu
{
public:
	DECLARE_CLASS (PopupMenu, Menu)

	static PopupMenu* create (StringRef name = nullptr, StringRef title = nullptr);

	PopupMenu (StringRef name = nullptr, StringRef title = nullptr);
	~PopupMenu ();

	static bool isPlatformMenuActive ();

	bool isAttached () const;

	MenuItem* popup (const Point& where, View* view = nullptr); ///< modal
	IAsyncOperation* popupAsync (const Point& where, View* view = nullptr);

	// Menu
	using Menu::addItem;
	void addItem (MenuItem* item) override;
	void removeItem (MenuItem* item) override;
	UIDRef CCL_API getMenuClass () const override;
	IMenu* CCL_API createMenu () const override;

protected:
	friend class MenuBar;

	static int activePlatformMenus;

	void onMenuClosed (IAsyncOperation& nativeOperation);

	virtual bool isPlatformMenu () const { return true ;}

	// platform-specific:
	virtual void realizeItem (MenuItem* item);
	virtual void unrealizeItem (MenuItem* item);
	virtual IAsyncOperation* popupPlatformMenu (const Point& where, IWindow* window);
};

//************************************************************************************************
// MenuBar
/** System menu bar. */
//************************************************************************************************

class MenuBar: public Object,
			   public IMenuBar
{
public:
	DECLARE_CLASS (MenuBar, Object)

	MenuBar ();
	~MenuBar ();

	bool isAttached () const;
	virtual bool addMenu (Menu* menu);
	virtual bool removeMenu (Menu* menu);
	Iterator* newIterator () const;

	virtual void updateMenu (Menu* menu);
	void updateKeys ();
	void init ();

	// IMenuBar
	int CCL_API countMenus () const override;
	IMenu* CCL_API getMenu (int index) const override;
	IMenu* CCL_API findMenu (StringRef name) const override;
	tbool CCL_API loadMenus (UrlRef path, IMenuExtension* extension = nullptr, ITranslationTable* stringTable = nullptr) override;

	// Object
	bool load (const Storage& storage) override;

	CLASS_INTERFACE (IMenuBar, Object)

protected:
	friend class Window;
	friend class WindowManager;
	Window* window;
	ObjectArray menus;

	bool insertMenu (Menu* menu, int index);

	virtual void activatePlatformMenu ();
	virtual void insertPlatformMenu (PopupMenu* menu);
	virtual void removePlatformMenu (PopupMenu* menu);

	// IMenu
	tbool CCL_API addMenu (IMenu* menu) override { return addMenu (unknown_cast<Menu> (menu)); }
	tbool CCL_API removeMenu (IMenu* menu) override { return removeMenu (unknown_cast<Menu> (menu)); }
};

//************************************************************************************************
// VariantMenuBar
/** Variant menu bar. */
//************************************************************************************************

template<class MenuBase>
class VariantMenuBar: public MenuBase,
					  public IVariantMenuBar
{
public:
	VariantMenuBar ();

	// IVariantMenuBar
	tbool CCL_API setVariant (StringRef variant) override;

	// MenuBar
	bool addMenu (Menu* menu) override;
	bool removeMenu (Menu* menu) override;
	IMenu* CCL_API findMenu (StringRef name) const override;

	CLASS_INTERFACE (IVariantMenuBar, MenuBar)

protected:
	ObjectArray managedMenus;
	String currentVariant;

	bool checkVisible (Menu* menu) const;
	void showMenu (Menu* menu, bool state, int index = -1);
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// MenuItem inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline Menu* MenuItem::getParent () const		{ return parent; }
inline Image* MenuItem::getIcon () const		{ return icon; }
inline bool MenuItem::isSeparator () const		{ return title == Menu::strSeparator; }
inline bool MenuItem::isSubMenu () const		{ return subMenu != nullptr; }
inline bool MenuItem::isEnabled () const		{ return (flags & kDisabled) == 0; }
inline bool MenuItem::isChecked () const		{ return (flags & kChecked) != 0; }
inline Menu* MenuItem::getSubMenu () const		{ return subMenu; }
inline Menu* MenuItem::getSplitMenu () const	{ return splitMenu; }
inline bool MenuItem::isValidID () const		{ return itemID != 0; }
inline bool MenuItem::isRegular () const		{ return !isSeparator () && !isSubMenu () && !isHeader (); }

//////////////////////////////////////////////////////////////////////////////////////////////////
// (Popup)Menu inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline Object* Menu::getParent () const			{ return parent; }
inline StringRef Menu::getTitle () const		{ return title; }
inline Image* Menu::getIcon () const			{ return icon; }
inline bool Menu::isEmpty () const				{ return items.isEmpty (); }
inline MenuItem* Menu::at (int idx) const		{ return (MenuItem*)items.at (idx); }

//////////////////////////////////////////////////////////////////////////////////////////////////
// MenuBar inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool MenuBar::isAttached () const		{ return window != nullptr; }
inline Iterator* MenuBar::newIterator () const	{ return menus.newIterator (); }

//************************************************************************************************
// VariantMenuBar implementation
//************************************************************************************************

template<class MenuBase>
inline VariantMenuBar<MenuBase>::VariantMenuBar ()
{
	managedMenus.objectCleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class MenuBase>
inline bool VariantMenuBar<MenuBase>::checkVisible (Menu* menu) const
{
	if(menu->getVariant ().isEmpty ()) // no variant assigned => always visible
		return true;

	if(currentVariant.isEmpty ()) // no variant selected => default
		return menu->getVariant ().contains (CCLSTR ("default"));

	return menu->getVariant ().contains (currentVariant);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class MenuBase>
inline void VariantMenuBar<MenuBase>::showMenu (Menu* menu, bool state, int index)
{
	if(state)
	{
		if(!MenuBase::menus.contains (menu))
		{
			menu->retain ();
			menu->updateKeys ();
			MenuBase::insertMenu (menu, index);
			ASSERT (menu->getRetainCount () == 2)
		}
	}
	else
	{
		if(MenuBase::menus.contains (menu))
		{
			MenuBase::removeMenu (menu);
			ASSERT (menu->getRetainCount () == 1)
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class MenuBase>
inline bool VariantMenuBar<MenuBase>::addMenu (Menu* menu)
{
	managedMenus.add (menu);

	if(checkVisible (menu))
		showMenu (menu, true);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class MenuBase>
inline bool VariantMenuBar<MenuBase>::removeMenu (Menu* menu)
{
	showMenu (menu, false);

	managedMenus.remove (menu);
	menu->release ();

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class MenuBase>
inline IMenu* CCL_API VariantMenuBar<MenuBase>::findMenu (StringRef name) const
{
	IMenu* menu = MenuBase::findMenu (name);
	if(menu)
		return menu;

	ForEach (managedMenus, Menu, m)
		if(m->getName () == name)
			return m;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class MenuBase>
inline tbool CCL_API VariantMenuBar<MenuBase>::setVariant (StringRef variant)
{
	currentVariant = variant;

	int visibleIndex = 0;
	ForEach (managedMenus, Menu, menu)
		bool state = checkVisible (menu);
		showMenu (menu, state, visibleIndex);
		if(state)
			visibleIndex++;
	EndFor

	return true;
}

} // namespace CCL

#endif // _ccl_menu
