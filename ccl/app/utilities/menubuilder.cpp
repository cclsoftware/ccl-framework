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

#include "ccl/base/message.h"

#include "ccl/public/gui/framework/iview.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/iparameter.h"

using namespace CCL;

//************************************************************************************************
// MenuBuilder::ParamMenuItemObserver
//************************************************************************************************

class MenuBuilder::ParamMenuItemObserver: public Object
{
public:
	DECLARE_CLASS_ABSTRACT (ParamMenuItemObserver, Object)

	ParamMenuItemObserver (IMenuItem* item, IParameter* param, bool closeOnChange);
	~ParamMenuItemObserver ();
	
	void updateMenuValue ();
	
	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

protected:
	IMenuItem* item;
	IParameter* param;
	bool closeOnChange;
};

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

void MenuBuilder::addSubMenuInternal (IMenu*& subMenu, IMenuItem*& subMenuItem, StringRef title)
{
	subMenu = menu.createMenu ();
	subMenu->setMenuAttribute (IMenu::kMenuTitle, title);
	subMenuItem = menu.addMenu (subMenu);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMenu* MenuBuilder::addSubMenu (StringRef title)
{
	IMenu* subMenu = nullptr;
	IMenuItem* subMenuItem = nullptr;
	addSubMenuInternal (subMenu, subMenuItem, title);
	return subMenu;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMenuItem* MenuBuilder::addSubMenuWithView (StringRef title, StringID formName, IAttributeList* arguments)
{
	IMenu* subMenu = nullptr;
	IMenuItem* subMenuItem = nullptr;
	addSubMenuInternal (subMenu, subMenuItem, title);

	UnknownPtr<IExtendedMenu> extendedMenu (subMenu);
	if(extendedMenu)
	{
		AutoPtr<IView> view = getTheme ()->createView (formName, controller, arguments);
		if(view)
			extendedMenu->addViewItem (view);
	}
	return subMenuItem;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMenuItem* MenuBuilder::addViewItem (StringID formName, IAttributeList* arguments)
{
	UnknownPtr<IExtendedMenu> extendedMenu (&menu);
	if(extendedMenu)
	{
		AutoPtr<IView> view = getTheme ()->createView (formName, controller, arguments);
		if(view)
			return extendedMenu->addViewItem (view);
	}			
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuBuilder::setItemIcon (IMenuItem* menuItem, StringID iconName)
{
	IImage* icon = getTheme ()->getImage (iconName);
	menuItem->setItemAttribute (IMenuItem::kItemIcon, icon);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuBuilder::setItemValueFromParam (IMenuItem* item, IParameter* param, bool closeOnChange)
{
	if(item && param)
	{
		AutoPtr<ParamMenuItemObserver> paramObserver = NEW ParamMenuItemObserver (item, param, closeOnChange);
		item->setItemAttribute (IMenuItem::kItemData, Variant (paramObserver->asUnknown (), true));
	}
}

//************************************************************************************************
// MenuBuilder::ParamMenuItemObserver
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (MenuBuilder::ParamMenuItemObserver, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

MenuBuilder::ParamMenuItemObserver::ParamMenuItemObserver (IMenuItem* item, IParameter* param, bool closeOnChange)
: item (item), 
  param (param),
  closeOnChange (closeOnChange)
{
	ISubject::addObserver (param, this);
	updateMenuValue ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MenuBuilder::ParamMenuItemObserver::~ParamMenuItemObserver ()
{
	ISubject::removeObserver (param, this);
	cancelSignals ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuBuilder::ParamMenuItemObserver::updateMenuValue ()
{
	if(item)
	{
		String valueString;
		param->toString (valueString);
		item->setItemAttribute (IMenuItem::kItemValue, valueString);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API MenuBuilder::ParamMenuItemObserver::notify (ISubject* subject, MessageRef msg)
{
	if(msg == "closeSubmenu")
	{
		if(IMenu* menu = item->getParentMenu ())
		{
			Variant control;
			menu->getMenuAttribute (control, IMenu::kMenuControl);
			UnknownPtr<ICompactMenuControl> compactControl (control.asUnknown ());
			ASSERT (compactControl.isValid ());
			if(compactControl)
				compactControl->closeDeepestMenu ();
		}
	}
	else if(msg == kChanged && isEqualUnknown (subject, param))
	{
		updateMenuValue ();
		if(closeOnChange)
			(NEW Message ("closeSubmenu"))->post (this, 300);
	}
}

