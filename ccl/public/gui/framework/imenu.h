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
// Filename    : ccl/public/gui/framework/imenu.h
// Description : Menu Interface
//
//************************************************************************************************

#ifndef _ccl_imenu_h
#define _ccl_imenu_h

#include "ccl/public/gui/icommandhandler.h"

namespace CCL {

interface IMenu;
interface IMenuItem;
interface IMenuExtension;
interface IParameter;
interface IView;
interface IAttributeList;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Built-in menu classes
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID
{
	DEFINE_CID (Menu, 0x1c1ff2c7, 0xeabe, 0x4b0c, 0xab, 0x94, 0xc2, 0x72, 0x8b, 0xfb, 0xc8, 0x12)
	DEFINE_CID (MenuBar, 0x32ac7729, 0x5ee3, 0x4273, 0xaf, 0x9d, 0xaf, 0x50, 0x1e, 0x7c, 0xe5, 0xb0)
	DEFINE_CID (VariantMenuBar, 0xd0d769c9, 0xe469, 0x445a, 0xb1, 0x9, 0x66, 0x7f, 0x55, 0xe1, 0xa0, 0xf5)
	DEFINE_CID (ExtendedMenu, 0xb2af5314, 0xd86d, 0x4bbe, 0x92, 0x90, 0x71, 0x22, 0x88, 0x79, 0x9, 0x5)
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Menu Presentations
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace MenuPresentation
{
	DEFINE_STRINGID (kTree, "TreeMenu")			///< present as tree view
	DEFINE_STRINGID (kNative, "NativeMenu")		///< present as native menu
	DEFINE_STRINGID (kExtended, "ExtendedMenu")	///< present as extended menu
	DEFINE_STRINGID (kCompact, "CompactMenu")	///< present as compact menu (max. 2 visible columns in one window)
	DEFINE_STRINGID (kSingleColumn, "SingleColumn")	///< present as single column menu
}

//************************************************************************************************
// IMenuBar
/** Menu bar interface. 
	\ingroup gui_menu */
//************************************************************************************************

interface IMenuBar: IUnknown
{
	/** Get number of menus. */
	virtual int CCL_API countMenus () const = 0;

	/** Get menu at index. */
	virtual IMenu* CCL_API getMenu (int index) const = 0;

	/** Find menu with given (untranslated) name. */
	virtual IMenu* CCL_API findMenu (StringRef name) const = 0;

	/** Add menu. */
	virtual tbool CCL_API addMenu (IMenu* menu) = 0;

	/** Remove menu. */
	virtual tbool CCL_API removeMenu (IMenu* menu) = 0;

	/** Load menus from XML description (optional: extension and translations). */
	virtual tbool CCL_API loadMenus (UrlRef path, IMenuExtension* extension = nullptr, ITranslationTable* stringTable = nullptr) = 0;

	DECLARE_IID (IMenuBar)
};

DEFINE_IID (IMenuBar, 0x35b765d3, 0xb428, 0x49bf, 0xb6, 0xf3, 0xb2, 0x7b, 0x37, 0xc5, 0x96, 0xec)

//************************************************************************************************
// IVariantMenuBar
/** Variant menu bar interface.
	\ingroup gui_menu */
//************************************************************************************************

interface IVariantMenuBar: IUnknown
{
	/** Set submenu visibility variant. */
	virtual tbool CCL_API setVariant (StringRef variant) = 0;

	DECLARE_IID (IVariantMenuBar)
};

DEFINE_IID (IVariantMenuBar, 0xf37c19c3, 0x6c3e, 0x4900, 0xad, 0xfb, 0x2, 0xa, 0xb, 0x2e, 0x3, 0x88)

//************************************************************************************************
// IMenuExtension
/** Menu extension interface. 
	\ingroup gui_menu */
//************************************************************************************************

interface IMenuExtension: IUnknown
{
	/** Extend the menu. A placeholder name can be passed optionally. */
	virtual void CCL_API extendMenu (IMenu& menu, StringID name) = 0;

	DECLARE_IID (IMenuExtension)
};

DEFINE_IID (IMenuExtension, 0xA019385A, 0xE02C, 0x46E1, 0x9F, 0xE9, 0xE0, 0x83, 0x52, 0x86, 0x02, 0x44)

//************************************************************************************************
// IMenu
/** Menu interface. 
	\ingroup gui_menu */
//************************************************************************************************

interface IMenu: IUnknown
{
	DEFINE_ENUM (MenuAttribute)
	{
		kMenuName,			///< menu name	[String]
		kMenuTitle,			///< menu title	[String]
		kMenuIcon,			///< menu icon	[IImage]
		kMenuData,			///< menu data	[Variant]
		kMenuVariant,		///< menu variant [String]
		kMenuScaleFactor	///< content scale factor [float]
	};

	static const String strSeparator;		///< menu separator
	static const String strLargeVariant;	///< large menu variant
	static const String strFollowIndicator;	///< menu follow indicator

	/** Get menu implementation class. */
	virtual UIDRef CCL_API getMenuClass () const = 0;

	/** Create compatible menu instance. Call addMenu() to use it as sub menu. */
	virtual IMenu* CCL_API createMenu () const = 0;

	/** Get parent of this menu (could be IMenu or IMenuBar). */
	virtual IUnknown* CCL_API getParentUnknown () const = 0;

	/** Get menu attribute. */
	virtual tbool CCL_API getMenuAttribute (Variant& value, MenuAttribute id) const = 0;

	/** Set menu attribute. */
	virtual tbool CCL_API setMenuAttribute (MenuAttribute id, VariantRef value) = 0;

	/** Get number of menu items. */
	virtual int CCL_API countItems () const = 0;

	/** Get menu item at index. */
	virtual IMenuItem* CCL_API getItem (int index) const = 0;

	/** Get index of given menu item. */
	virtual int CCL_API getItemIndex (const IMenuItem* item) const = 0;

	/** Set position for adding new items (incremented automatically, set to -1 when done). */
	virtual void CCL_API setInsertPosition (int index) = 0;

	/** Add an item that fires a command. */
	virtual IMenuItem* CCL_API addCommandItem (StringRef title, CStringRef category = nullptr, CStringRef name = nullptr, ICommandHandler* handler = nullptr) = 0;
	
	/** Find existing command item. */
	virtual IMenuItem* CCL_API findICommandItem (CStringRef category = nullptr, CStringRef name = nullptr, tbool deep = false) const = 0;

	/**	Add separator item. Separators are managed internally to avoid consecutive occurances. */
	virtual void CCL_API addSeparatorItem () = 0;

	/** Add sub menu. */
	virtual IMenuItem* CCL_API addMenu (IMenu* menu) = 0;

	/** Remove menu item. */
	virtual void CCL_API removeItem (IMenuItem* item) = 0;

	/** Remove all menu items. */
	virtual void CCL_API removeAll () = 0;

	/** Load menu items from XML description (optional: handler, extensions, translations). */
	virtual tbool CCL_API loadItems (UrlRef path, ICommandHandler* handler = nullptr, IMenuExtension* extension = nullptr, ITranslationTable* stringTable = nullptr) = 0;

	/** Load menu items from simple attributes. */
	virtual tbool CCL_API loadItems (const IAttributeList& a) = 0;

	/** Save menu items to simple attributes. */
	virtual tbool CCL_API saveItems (IAttributeList& a) const = 0;

	/** Assign command handler recursively after loadItems(). */
	virtual void CCL_API initWithHandler (ICommandHandler* handler) = 0;

//////////////////////////////////////////////////////////////////////////////////////////////////
	
	inline bool isNativeMenu () const	{ return getMenuClass () == ClassID::Menu; }
	inline bool isExtendedMenu () const { return getMenuClass () == ClassID::ExtendedMenu; }

	inline IMenuItem* addCommandItem (const CommandWithTitle& cwt, ICommandHandler* handler = nullptr, bool followIndicator = false)
	{
		if(followIndicator == false)
			return addCommandItem (cwt.title, cwt.category, cwt.name, handler);
		else
			return addCommandItem (String () << cwt.title << strFollowIndicator, cwt.category, cwt.name, handler);
	}

	DECLARE_IID (IMenu)
};

DEFINE_IID (IMenu, 0x3549f3a9, 0xd5d8, 0x49ea, 0xad, 0xb6, 0xd9, 0xa, 0x7, 0xef, 0x17, 0x8d)

//************************************************************************************************
// IExtendedMenu
/** Extended menu interface. 	
	\ingroup gui_menu */
//************************************************************************************************

interface IExtendedMenu: IUnknown
{
	/** Add header item. */
	virtual IMenuItem* CCL_API addHeaderItem (StringRef title) = 0;

	/** Add parameter item (shared). */
	virtual IMenuItem* CCL_API addParameterItem (StringRef title, IParameter* parameter) = 0;

	/** Add view item (shared). */
	virtual IMenuItem* CCL_API addViewItem (IView* view) = 0;

	DECLARE_IID (IExtendedMenu)
};

DEFINE_IID (IExtendedMenu, 0xfde9dbaf, 0x1c63, 0x443e, 0x89, 0xcb, 0x6e, 0x2d, 0x69, 0xa5, 0xc4, 0xa3)

//************************************************************************************************
// IMenuItem
/** Menu item interface.
	\ingroup gui_menu */
//************************************************************************************************

interface IMenuItem: IUnknown
{
	/** Menu item attributes (*... extended menu only). */
	DEFINE_ENUM (ItemAttribute)
	{
		kItemName,		///< item name		[String]
		kItemTitle,		///< item title		[String]
		kItemIcon,		///< item icon		[IImage]
		kItemData,		///< item data		[Variant]
		kItemChecked,	///< checked state	[tbool]
		kItemEnabled,	///< enabled state	[tbool]
		kItemItalic,	///< italic state*	[tbool]
		kItemHelpId,	///< help id		[String]
		kDescription,	///< description*	[String]
		kTooltip,		///< item tooltip*	[String]
		kSplitMenu,		///< split menu*	[IMenu]
		kItemCategory,	///< item category	[String]
		kItemHandler	///< item handler	[ICommandHandler]
	};

	enum Metrics
	{
		kIconSize = 14	///< small menu item icon size
	};

	/** Get menu item attribute. */
	virtual tbool CCL_API getItemAttribute (Variant& value, ItemAttribute id) const = 0;

	/** Set menu item attribute. */
	virtual tbool CCL_API setItemAttribute (ItemAttribute id, VariantRef value) = 0;

	/** Get parent menu. */
	virtual IMenu* CCL_API getParentMenu () const = 0;

	/** Get submenu (if present). */
	virtual IMenu* CCL_API getItemMenu () const = 0;

	/** Select menu item programmatically. */
	virtual tbool CCL_API selectItem () = 0;

	DECLARE_IID (IMenuItem)
};

DEFINE_IID (IMenuItem, 0xd9d1cb64, 0x1c0f, 0x4c16, 0xb6, 0x2f, 0x80, 0xd6, 0xd6, 0xfa, 0x14, 0x9c)

//************************************************************************************************
// MenuPosition
/** Helper class to remember position in a menu. 
	\ingroup gui_menu */
//************************************************************************************************

struct MenuPosition
{
	IMenu* menu;
	IMenuItem* item;

	MenuPosition (IMenu* menu = nullptr, IMenuItem* item = nullptr)
	: menu (menu),
	  item (item)
	{}

	MenuPosition (IMenu& menu)
	: menu (&menu),
	  item (menu.getItem (menu.countItems ()-1))
	{}
};

//************************************************************************************************
// MenuInserter
/** Helper class to manage menu insert position. 	
	\ingroup gui_menu */
//************************************************************************************************

struct MenuInserter
{
	IMenu* menu;

	MenuInserter (const MenuPosition& position)
	: menu (position.menu)
	{
		ASSERT (menu != nullptr)
		if(menu)
		{
			int index = position.item ? menu->getItemIndex (position.item) + 1 : 0;
			menu->setInsertPosition (index);
		}
	}

	MenuInserter (IMenu* menu, int index)
	: menu (menu)
	{
		ASSERT (menu != nullptr)
		if(menu)
			menu->setInsertPosition (index);
	}

	~MenuInserter ()
	{
		if(menu)
			menu->setInsertPosition (-1);
	}
};

} // namespace CCL

#endif // _ccl_icommandtable_h
