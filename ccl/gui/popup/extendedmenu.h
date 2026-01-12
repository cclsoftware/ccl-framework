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
// Filename    : ccl/gui/popup/extendedmenu.h
// Description : Extended Menu
//
//************************************************************************************************

#ifndef _ccl_extendedmenu_h
#define _ccl_extendedmenu_h

#include "ccl/gui/popup/menu.h"

#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/framework/iview.h"

namespace CCL {

class MenuBarControl;

//************************************************************************************************
// ExtendedMenu
//************************************************************************************************

class ExtendedMenu: public Menu,
					public IExtendedMenu
{
public:
	DECLARE_CLASS (ExtendedMenu, Menu)
	DECLARE_METHOD_NAMES (ExtendedMenu)

	class ParameterItem;
	class ViewItem;

	// IExtendedMenu
	IMenuItem* CCL_API addHeaderItem (StringRef title) override;
	IMenuItem* CCL_API addParameterItem (StringRef title, IParameter* parameter) override;
	IMenuItem* CCL_API addViewItem (IView* view) override;

	// Menu
	UIDRef CCL_API getMenuClass () const override;
	IMenu* CCL_API createMenu () const override;

	CLASS_INTERFACE (IExtendedMenu, Menu)

protected:
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

//************************************************************************************************
// ExtendedMenu::ParameterItem
//************************************************************************************************

class ExtendedMenu::ParameterItem: public MenuItem
{
public:
	DECLARE_CLASS_ABSTRACT (ParameterItem, MenuItem)

	ParameterItem (Menu* parent, StringRef title, IParameter* parameter);

	PROPERTY_SHARED_AUTO (IParameter, parameter, Parameter)

	// MenuItem
	void init () override;
};

//************************************************************************************************
// ExtendedMenu::ViewItem
//************************************************************************************************

class ExtendedMenu::ViewItem: public MenuItem
{
public:
	DECLARE_CLASS_ABSTRACT (ViewItem, MenuItem)

	ViewItem (Menu* parent, IView* view);

	PROPERTY_SHARED_AUTO (IView, view, View)

	Coord getWidth () const  { return view ? view->getSize ().getWidth () : 0; }
	Coord getHeight () const { return view ? view->getSize ().getHeight () : 0; }
};

//************************************************************************************************
// ExtendedPopupMenu
//************************************************************************************************

class ExtendedPopupMenu: public PopupMenu
{
public:
	DECLARE_CLASS (ExtendedPopupMenu, PopupMenu)
	
	// PopupMenu
	bool isPlatformMenu () const override;
	void realizeItem (MenuItem* item) override;
	void unrealizeItem (MenuItem* item) override;
	IAsyncOperation* popupPlatformMenu (const Point& where, IWindow* window) override;
	IMenu* CCL_API createMenu () const override;
	
protected:
	ExtendedMenu extendedMenu;
};

//************************************************************************************************
// ExtendedMenuBar
//************************************************************************************************

class ExtendedMenuBar: public MenuBar
{
public:
	DECLARE_CLASS (ExtendedMenuBar, MenuBar)

	ExtendedMenuBar ();
	~ExtendedMenuBar ();

	void attachTo (Window* window, MenuBarControl* control);
	
	// MenuBar
	void updateMenu (Menu* menu) override;
	void insertPlatformMenu (PopupMenu* menu) override;
	void removePlatformMenu (PopupMenu* menu) override;
	tbool CCL_API loadMenus (UrlRef path, IMenuExtension* extension, ITranslationTable* stringTable) override;
};

//************************************************************************************************
// ExtendedVariantMenuBar
//************************************************************************************************

class ExtendedVariantMenuBar: public VariantMenuBar<ExtendedMenuBar>
{
public:
	DECLARE_CLASS (ExtendedVariantMenuBar, ExtendedMenuBar)
};

} // namespace CCL

#endif // _ccl_extendedmenu_h
