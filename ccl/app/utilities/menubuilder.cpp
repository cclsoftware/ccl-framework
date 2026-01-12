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
// Filename    : ccl/app/utilities/menubuilder.cpp
// Description : Menu Builder
//
//************************************************************************************************

#include "ccl/app/utilities/menubuilder.h"

#include "ccl/app/component.h"

#include "ccl/public/gui/framework/iview.h"
#include "ccl/public/gui/framework/itheme.h"

using namespace CCL;

//************************************************************************************************
// MenuBuilder
//************************************************************************************************

MenuBuilder::MenuBuilder (IMenu& menu, IUnknown* controller)
: menu (menu),
  controller (controller)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITheme* MenuBuilder::getTheme () const
{
	return RootComponent::instance ().getTheme ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMenu* MenuBuilder::addSubMenu (StringRef title)
{
	IMenu* subMenu = menu.createMenu ();
	subMenu->setMenuAttribute (IMenu::kMenuTitle, title);
	menu.addMenu (subMenu);
	return subMenu;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuBuilder::addSubMenuWithView (StringRef title, StringID formName)
{
	IMenu* subMenu = addSubMenu (title);
	UnknownPtr<IExtendedMenu> extendedMenu (subMenu);
	if(extendedMenu)
	{
		AutoPtr<IView> view = getTheme ()->createView (formName, controller);
		if(view)
			extendedMenu->addViewItem (view);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuBuilder::addViewItem (StringID formName)
{
	UnknownPtr<IExtendedMenu> extendedMenu (&menu);
	if(extendedMenu)
	{
		AutoPtr<IView> view = getTheme ()->createView (formName, controller);
		if(view)
			extendedMenu->addViewItem (view);
	}			
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuBuilder::setItemIcon (IMenuItem* menuItem, StringID iconName)
{
	IImage* icon = getTheme ()->getImage (iconName);
	menuItem->setItemAttribute (IMenuItem::kItemIcon, icon);
}
