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
// Filename    : ccl/gui/popup/parametermenubuilder.cpp
// Description : Parameter Menu Builder
//
//************************************************************************************************

#include "ccl/gui/popup/parametermenubuilder.h"
#include "ccl/gui/popup/extendedmenu.h"

#include "ccl/base/message.h"

using namespace CCL;

//************************************************************************************************
// ParameterMenuBuilder::ParamData
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ParameterMenuBuilder::ParamData, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ParameterMenuBuilder::ParamData::queryInterface (UIDRef iid, void** ptr)
{
	// delegate to parameter
	if(iid == ccl_iid<IParameter> () && parameter)
		return parameter->queryInterface (iid, ptr);

	// delegate to builder
	if((iid == ccl_iid<ICommandHandler> () || iid == ccl_iid<IParameterMenuBuilder> ()) && builder)
		return builder->queryInterface (iid, ptr);

	return SuperClass::queryInterface (iid, ptr);
}

//************************************************************************************************
// ParameterMenuBuilder
//************************************************************************************************

DEFINE_CLASS (ParameterMenuBuilder, Object)
DEFINE_CLASS_UID (ParameterMenuBuilder, 0xb56d5931, 0x2225, 0x42bf, 0x8c, 0x93, 0xe7, 0x61, 0x8e, 0xf1, 0x71, 0x35)

//////////////////////////////////////////////////////////////////////////////////////////////////

ParameterMenuBuilder::ParameterMenuBuilder (IParameter* param)
: menuIDs (NEW MenuItemIDSet),
  parameter (nullptr),
  defaultTitleEnabled (true),
  extensionEnabled (true)
{
	if(param)
		construct (param);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ParameterMenuBuilder::~ParameterMenuBuilder ()
{
	if(parameter)
	{
		if(previewHandler)
		{
			if(IParameter* original = parameter->getOriginal ())
			{
				ParamPreviewEvent e;
				e.type = ParamPreviewEvent::kCleanupMenu;
				previewHandler->paramPreview (original, e);
			}

			previewHandler = nullptr;
		}

		if(UnknownPtr<IStructuredParameter> structParam = parameter)
			structParam->cleanupStructure ();

		parameter->release ();
	}

	if(menuIDs)
		menuIDs->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ParameterMenuBuilder::construct (IParameter* param)
{
	ASSERT (parameter == nullptr)
	if(parameter)
		return kResultUnexpected;

	parameter = param;
	if(parameter)
	{
		parameter->retain ();

		if(UnknownPtr<IStructuredParameter> structParam = parameter)
			structParam->prepareStructure ();

		if(IParameter* original = parameter->getOriginal ())
			if(previewHandler = original->getController ())
			{
				ParamPreviewEvent e;
				e.type = ParamPreviewEvent::kPrepareMenu;
				previewHandler->paramPreview (original, e);
			}
	}
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Menu* ParameterMenuBuilder::buildMenu (Menu* menu)
{
	if(!parameter)
		return nullptr;

	if(!menu)
		menu = NEW ExtendedMenu;

	if(!buildCustomized (*menu, *parameter))
		buildMenu (*menu, *parameter);

	// the parameter can add additional menu items
	if(extensionEnabled == true)
	{
		UnknownPtr<IMenuExtension> menuExtension (parameter);
		if(menuExtension)
			menuExtension->extendMenu (*menu, parameter->getName ());
	}

	return menu;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ParameterMenuBuilder::prepareMenu (Menu& menu, IParameter& param, StringRef title)
{
	if(menu.getTitle ().isEmpty ()) /// dont overwrite
		menu.setTitle (title);
	menu.setIDSet (menuIDs);

	AutoPtr<ParamData> menuData = NEW ParamData;
	menuData->setBuilder (this); // ensure that this lives as long as the menu
	menuData->setParameter (&param);
	Variant variant (menuData->asUnknown ());
	variant.share ();
	menu.setMenuData (variant);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MenuItem* ParameterMenuBuilder::addItem (Menu& menu, IParameter& param, int value)
{
	String name;
	name.appendIntValue (value);

	String title;
	param.getString (title, value);

	bool isStringParam = param.getType () == IParameter::kString;
	int current = param.getValue ();
	bool selectable = !param.isOutOfRange () && !isStringParam;

	if(title.isEmpty () && (defaultTitleEnabled == false || isStringParam == true)) // avoid integer value as title
	{
		static const String space = CCLSTR (" ");
		title = space;
	}

	MenuItem* item = menu.addItem (name, title, this);
	item->setCategory (CCLSTR ("Param"));

	if(selectable && value == current)
		item->check ();
	return item;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ParameterMenuBuilder::buildCustomized (Menu& menu, IParameter& param)
{
	UnknownPtr<IParameterMenuCustomize> customizer (&param);
	if(customizer)
	{
		String menuTitle (param.getName ()); // use parameter name as title to prevent implicit translation!
		prepareMenu (menu, param, menuTitle);

		if(customizer->buildMenu (menu, *this))
			return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ParameterMenuBuilder::buildMenu (Menu& menu, IParameter& param)
{
	String menuTitle (param.getName ()); // use parameter name as title to prevent implicit translation!
	prepareMenu (menu, param, menuTitle);

	int min = param.getMin ();
	int max = param.getMax ();
	int vOffset = 0;
	int vFactor = 1;
	if(param.isReverse ())
	{
		vOffset = min + max;
		vFactor = -1;
	}

	if(max - min > 10000)
	{
		CCL_WARN ("Huge menu!!!\n", 0)
		return false;
	}

	bool itemChecked = false;
	for(int x = min; x <= max; x++)
	{
		int v = vOffset + x * vFactor;

		MenuItem* item = addItem (menu, param, v);
		if(item->isChecked ())
			itemChecked = true;
	}

	UnknownPtr<IStructuredParameter> structParam (&param);
	if(structParam)
	{
		int numSubParams = structParam->countSubParameters ();
		for(int i = 0; i < numSubParams; i++)
		{
			if(IParameter* p = structParam->getSubParameter (i))
			{
				Menu* subMenu = (Menu*)menu.myClass ().createObject ();
				if(subMenu)
				{
					bool subItemChecked = buildMenu (*subMenu, *p);
					MenuItem* subMenuItem = menu.addMenu (subMenu, false);
					subMenuItem->setTitle (subMenu->getTitle ());
					if(subItemChecked)
					{
						subMenuItem->check ();
						itemChecked = true;
					}
				}
			}
		}
	}
	return itemChecked;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMenu* CCL_API ParameterMenuBuilder::buildIMenu (IMenu* menu)
{
	return buildMenu (unknown_cast<Menu> (menu));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMenuItem* CCL_API ParameterMenuBuilder::addSubMenu (IMenu& _menu, IParameter& param, StringRef title)
{
	Menu* menu = unknown_cast<Menu> (&_menu);
	ASSERT (menu)

	MenuItem* subMenuItem = nullptr;
	Menu* subMenu = menu ? (Menu*)menu->myClass ().createObject () : nullptr;
	if(subMenu)
	{
		prepareMenu (*subMenu, param, title);
		subMenuItem = menu->addMenu (subMenu, false);
		subMenuItem->setTitle (subMenu->getTitle ());
	}
	return subMenuItem;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMenuItem* CCL_API ParameterMenuBuilder::findSubMenu (IMenu& _menu, StringRef title)
{
	Menu* menu = unknown_cast<Menu> (&_menu);
	ASSERT (menu)
	if(!menu)
		return nullptr;
	for(int i = 0; i < menu->countItems (); i++)
	{
		MenuItem* item = menu->at (i);
		if(Menu* subMenu = item->getSubMenu ())
			if(subMenu->getTitle () == title)
				return item;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMenuItem* CCL_API ParameterMenuBuilder::addValueItem (IMenu& _menu, IParameter& param, int value)
{
	Menu* menu = unknown_cast<Menu> (&_menu);
	ASSERT (menu)
	if(!menu)
		return nullptr;

	MenuItem* item = addItem (*menu, param, value);
	if(item->isChecked ())
	{
		// check recursively upwards
		Menu* current = menu;
		while(current)
		{
			Menu* parent = ccl_cast<Menu> (current->getParent ());
			if(!parent)
				break;

			MenuItem* parentItem = parent->findSubMenuItem (current);
			if(!parentItem)
				break;

			parentItem->check ();

			current = parent;
		}
	}
	return item;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* ParameterMenuBuilder::extractParameter (Menu& menu)
{
	// extract the parameter that we have put into menuData
	if(ParamData* menuData = unknown_cast<ParamData> (menu.getMenuData ()))
		return menuData->getParameter ();
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ParameterMenuBuilder* ParameterMenuBuilder::extractBuilder (Menu& menu)
{
	if(ParamData* menuData = unknown_cast<ParamData> (menu.getMenuData ()))
		return menuData->getBuilder ();
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ParameterMenuBuilder::checkCommandCategory (CStringRef category) const
{
	return category == "Param";
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ParameterMenuBuilder::interpretCommand (const CommandMsg& msg)
{
	if(msg.category == "Param")
	{
		// menu item was selected, set the corresponding parameter's value
		MenuItem* selectedItem = unknown_cast<MenuItem> (msg.invoker);
		if(selectedItem && selectedItem->isEnabled ())
		{
			if(!msg.checkOnly ())
			{
				Menu* parentMenu = selectedItem->getParent ();
				ParamData* menuData = unknown_cast<ParamData> (parentMenu->getMenuData ());
				ASSERT (menuData != nullptr)
				IParameter* p = menuData ? menuData->getParameter () : nullptr;
				if(p)
				{
					int64 value = 0;
					selectedItem->getName ().getIntValue (value);

					p->beginEdit ();
					p->setValue (value, true);
					p->endEdit ();
				}
			}
			return true;
		}
	}
	return false;
}
