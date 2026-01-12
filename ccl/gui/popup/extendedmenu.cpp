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
// Filename    : ccl/gui/popup/extendedmenu.cpp
// Description : Extended Menu
//
//************************************************************************************************

#include "ccl/gui/popup/extendedmenu.h"

#include "ccl/gui/popup/menubarcontrol.h"
#include "ccl/gui/popup/menucontrol.h"
#include "ccl/gui/popup/popupselector.h"

#include "ccl/base/asyncoperation.h"
#include "ccl/base/kernel.h"
#include "ccl/base/message.h"

using namespace CCL;

//************************************************************************************************
// ExtendedMenu
//************************************************************************************************

DEFINE_CLASS (ExtendedMenu, Menu)
DEFINE_CLASS_UID (ExtendedMenu, 0xb2af5314, 0xd86d, 0x4bbe, 0x92, 0x90, 0x71, 0x22, 0x88, 0x79, 0x9, 0x5)

//////////////////////////////////////////////////////////////////////////////////////////////////

UIDRef CCL_API ExtendedMenu::getMenuClass () const
{
	return ClassID::ExtendedMenu;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMenu* CCL_API ExtendedMenu::createMenu () const
{
	return NEW ExtendedMenu;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMenuItem* CCL_API ExtendedMenu::addHeaderItem (StringRef title)
{
	separatorNeeded = false; // header serves as separator, too

	MenuItem* item = NEW MenuItem (this, nullptr, title);
	item->isHeader (true);
	addItem (item);
	return item;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMenuItem* CCL_API ExtendedMenu::addParameterItem (StringRef title, IParameter* parameter)
{
	ASSERT (parameter)
	MenuItem* item = NEW ParameterItem (this, title, parameter);
	addItem (item);
	return item;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMenuItem* CCL_API ExtendedMenu::addViewItem (IView* view)
{
	ASSERT (view)
	MenuItem* item = NEW ViewItem (this, view);
	addItem (item);
	return item;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (ExtendedMenu)
	DEFINE_METHOD_ARGR ("addHeaderItem", "title", "MenuItem")
END_METHOD_NAMES (ExtendedMenu)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ExtendedMenu::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "addHeaderItem")
	{
		returnValue.takeShared (addHeaderItem (msg[0].asString ()));
		return true;		
	}
	return SuperClass::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// ExtendedMenu::ParameterItem
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (ExtendedMenu::ParameterItem, MenuItem)

//////////////////////////////////////////////////////////////////////////////////////////////////

ExtendedMenu::ParameterItem::ParameterItem (Menu* parent, StringRef title, IParameter* parameter)
: MenuItem (parent, nullptr, title)
{
	setParameter (parameter);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExtendedMenu::ParameterItem::init ()
{
	enable (parameter && parameter->isEnabled ());
}

//************************************************************************************************
// ExtendedMenu::ViewItem
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (ExtendedMenu::ViewItem, MenuItem)

//////////////////////////////////////////////////////////////////////////////////////////////////

ExtendedMenu::ViewItem::ViewItem (Menu* parent, IView* view)
: MenuItem (parent, nullptr, nullptr)
{
	setView (view);
}

//************************************************************************************************
// ExtendedPopupMenu
//************************************************************************************************

DEFINE_CLASS (ExtendedPopupMenu, PopupMenu)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExtendedPopupMenu::isPlatformMenu () const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExtendedPopupMenu::realizeItem (MenuItem* item)
{
	int index = getItemIndex (item, true);
	ASSERT (index >= 0)
	
	MenuInserter inserter (&extendedMenu, index);
	extendedMenu.addItem (return_shared (item));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExtendedPopupMenu::unrealizeItem (MenuItem* item)
{
	extendedMenu.removeItem (item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* ExtendedPopupMenu::popupPlatformMenu (const Point& where, IWindow* parentWindow)
{
	View* parentView = unknown_cast<View> (parentWindow);
	PopupSizeInfo sizeInfo (parentView, PopupSizeInfo::kForceFixedPosition);
	Point where2 (where);
	sizeInfo.where = parentView->screenToClient (where2);
	
	PopupSelector popupSelector;
	popupSelector.setTheme (parentView->getTheme ());
	popupSelector.setVisualStyle (parentView->getTheme ().getStandardStyle (ThemePainter::kPopupMenuStyle));
	popupSelector.setMenuMode (true);
	
	SharedPtr<MenuControl> control = NEW MenuControl (&extendedMenu);
	popupSelector.popup (control, control->getPopupClient (), sizeInfo);
	
	MenuItem* resultItem = control->getResultItem ();
	return AsyncOperation::createCompleted (resultItem ? resultItem->getItemID () : 0, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMenu* CCL_API ExtendedPopupMenu::createMenu () const
{
	return NEW ExtendedPopupMenu ();
}

//************************************************************************************************
// ExtendedMenuBar
//************************************************************************************************

DEFINE_CLASS (ExtendedMenuBar, MenuBar)
DEFINE_CLASS (ExtendedVariantMenuBar, ExtendedMenuBar)

//////////////////////////////////////////////////////////////////////////////////////////////////

ExtendedMenuBar::ExtendedMenuBar ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ExtendedMenuBar::~ExtendedMenuBar ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExtendedMenuBar::attachTo (Window* newWindow, MenuBarControl* control)
{
	window = newWindow;
	Rect size;
	control->autoSize ();
	size = control->getSize ();
	size.setWidth (window->getWidth ());
	control->setSize (size);
	control->setSizeMode (IView::kAttachLeft | IView::kAttachRight);
	control->setMenuBar (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExtendedMenuBar::insertPlatformMenu (PopupMenu* menu)
{
	// observing MenuBarControl will invaliate itself
	signal (Message (kChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExtendedMenuBar::removePlatformMenu (PopupMenu* menu)
{
	signal (Message (kChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExtendedMenuBar::updateMenu (Menu* menu)
{
	signal (Message (kChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ExtendedMenuBar::loadMenus (UrlRef path, IMenuExtension* extension, ITranslationTable* stringTable)
{
	// While loading, menu objects are created via class name "Menu", which defaults to a platform implementation.
	// To create ExtendedPopupMenu instead, we replace the registered class temporarily.
	MetaClassReplaceScope scope ("Menu", ccl_typeid<ExtendedPopupMenu> ());

	return SuperClass::loadMenus (path, extension, stringTable);
}
