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
// Filename    : ccl/app/editing/addins/editaddincollection.cpp
// Description : Edit Add-in Collection
//
//************************************************************************************************

#include "ccl/app/editing/addins/editaddincollection.h"
#include "ccl/app/editing/addins/editaddindescription.h"

#include "ccl/app/utilities/pluginclass.h"

#include "ccl/public/app/iedittask.h"
#include "ccl/public/app/ieditenvironment.h"

#include "ccl/base/message.h"
#include "ccl/base/storage/settings.h"

#include "ccl/public/base/iarrayobject.h"
#include "ccl/public/storage/ipersistattributes.h"
#include "ccl/public/gui/framework/imenu.h"
#include "ccl/public/gui/framework/viewbox.h"
#include "ccl/public/gui/framework/iskinmodel.h"
#include "ccl/public/gui/framework/controlproperties.h"
#include "ccl/public/gui/framework/iwindowmanager.h"
#include "ccl/public/plugservices.h"
#include "ccl/public/guiservices.h"

using namespace CCL;

//************************************************************************************************
// EditAddInCollection::AddInItem
//************************************************************************************************

EditAddInCollection::AddInItem::AddInItem (IUnknown* unknown, const IClassDescription& classInfo)
: unknown (unknown),
  menuPriority (1000)
{
	MutableCString id;
	UID (classInfo.getClassID ()).toCString (id);
	setID (id);
	setName (classInfo.getName ());
	classInfo.getLocalizedName (title);

	Variant priority;
	if(classInfo.getClassAttribute (priority, "menuPriority"))
		setMenuPriority (priority);

	if(unknown == nullptr) // item is used for sorting only
		return;

	// initialize ui attributes
	MutableCString windowClassId = getID ();
	IWindowClass* windowClass = System::GetWindowManager ().findWindowClass (windowClassId);
	if(windowClass == nullptr) // fallback to class name
	{
		windowClassId = getName ();
		windowClass = System::GetWindowManager ().findWindowClass (windowClassId);
	}

	setWindowClassID (windowClassId);

	ASSERT (windowClass != nullptr)
	if(windowClass)
	{
		MutableCString category, name;
		windowClass->getCommand (category, name);
		setCommandCategory (category);
		setCommandName (name);

		Variant groupID;
		UnknownPtr<IObject> (windowClass)->getProperty (groupID, "group");
		setGroupID (groupID.asString ());
	}

	// 1) try icon provided by implementing module
	IImage* icon = nullptr;
	PlugInMetaInfo metaInfo (classInfo.getClassID ());
	if(IImage* infoIcon = metaInfo.getImage ())
		icon = infoIcon;

	// 2) try icon from application skin
	if(icon == nullptr)
		icon = PlugInClass (classInfo).getIcon (true);
	
	setIcon (icon);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int EditAddInCollection::AddInItem::compare (const Object& obj) const
{
	const AddInItem& other = (const AddInItem&)obj;
	return menuPriority - other.menuPriority;
}

//************************************************************************************************
// EditAddInCollection
//************************************************************************************************

void EditAddInCollection::makeMainMenu (IMenu& menu, StringRef subCategory)
{
	// sort by menu priority
	ObjectArray items;
	items.objectCleanup (true);
	ForEachPlugInClass (PLUG_CATEGORY_EDITADDIN, classInfo)
		if(classInfo.getSubCategory () == subCategory && !EditAddInDescription::Registrar::isHidden (classInfo))
			items.addSorted (NEW AddInItem (nullptr, classInfo));
	EndFor

	ArrayForEach (items, AddInItem, item)
		IMenuItem* menuItem = menu.addCommandItem (item->getTitle (), EditAddInDescription::Registrar::kCommandCategory, MutableCString (item->getName ()));
		menuItem->setItemAttribute (IMenuItem::kItemData, PLUG_CATEGORY_EDITADDIN); // mark items for removing them later in removeFromMenu
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditAddInCollection::removeFromMenu (IMenu& menu)
{
	// remove the items we added before
	int numItems = menu.countItems ();
	for(int i = numItems - 1; i >= 0; i--)
	{
		Variant itemData;
		IMenuItem* menuItem = menu.getItem (i);
		if(menuItem && menuItem->getItemAttribute (itemData, IMenuItem::kItemData) && itemData == PLUG_CATEGORY_EDITADDIN)
			menu.removeItem (menuItem);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (EditAddInCollection, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

EditAddInCollection::EditAddInCollection (StringRef name)
: Component (name.isEmpty () ? CCLSTR ("AddIns") : name)
{
	addIns.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditAddInCollection::~EditAddInCollection ()
{
	ASSERT (addIns.isEmpty () == true)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditAddInCollection::collectAddIns (StringRef subCategory, IEditEnvironment* environment)
{
	EditAddInDescription::Registrar::instance ().onAddInsInitialize (true);

	ForEachPlugInClass (PLUG_CATEGORY_EDITADDIN, classInfo)
		if(classInfo.getSubCategory () != subCategory)
			continue;

		IUnknown* unknown = ccl_new<IObjectNode> (classInfo.getClassID ()); // IObjectNode *must* be implemented!
		ASSERT (unknown != nullptr)
		if(!unknown)
			continue;

		AddInItem* addIn = NEW AddInItem (unknown, classInfo);
		addIns.addSorted (addIn);

		if(UnknownPtr<IComponent> iComponent = unknown)
		{
			tresult tr = iComponent->initialize (environment);
			ASSERT (tr == kResultOk)
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API EditAddInCollection::terminate ()
{
	ccl_forceGC ();

	ForEach (addIns, AddInItem, addIn)
		if(UnknownPtr<IComponent> iComponent = addIn->getPlugInUnknown ())
			iComponent->terminate ();

		ccl_release (addIn->getPlugInUnknown ());
	EndFor
	addIns.removeAll ();

	EditAddInDescription::Registrar::instance ().onAddInsInitialize (false);

	return SuperClass::terminate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* CCL_API EditAddInCollection::findParameter (StringID name) const
{
	IParameter* param = SuperClass::findParameter (name);

	// find window class params by AddIn name
	if(!param)
	{
		String nameString (name);
		ForEach (addIns, AddInItem, addIn)
			if(addIn->getName () == nameString)
			{
				UnknownPtr<IController> wm (&System::GetWindowManager ());
				param = wm->findParameter (addIn->getWindowClassID ());
				break;
			}
		EndFor
	}
	return param;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IObjectNode* CCL_API EditAddInCollection::findChild (StringRef id) const
{
	ForEach (addIns, AddInItem, addIn)
		if(addIn->getName () == id)
		{
			UnknownPtr<IObjectNode> iNode (addIn->getPlugInUnknown ());
			ASSERT (iNode)
			return iNode;
		}
	EndFor
	return SuperClass::findChild (id);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API EditAddInCollection::getChildDelegates (IMutableArray& delegates) const
{
	ForEach (addIns, AddInItem, addIn)
		delegates.addArrayElement (addIn->getName ());
	EndFor
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditAddInCollection::makeMainMenu (IMenu& menu)
{
	for(auto* addIn : iterate_as<AddInItem> (addIns))
	{
		UID cid;
		cid.fromCString (addIn->getID ());
		const IClassDescription* classInfo = System::GetPlugInManager ().getClassDescription (cid);
		ASSERT (classInfo)
		if(classInfo && !EditAddInDescription::Registrar::isHidden (*classInfo))
		{
			IMenuItem* menuItem = menu.addCommandItem (addIn->getTitle (), EditAddInDescription::Registrar::kCommandCategory, MutableCString (addIn->getName ()));
			menuItem->setItemAttribute (IMenuItem::kItemData, PLUG_CATEGORY_EDITADDIN); // mark items for removing them later in removeFromMenu
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API EditAddInCollection::checkCommandCategory (CStringRef category) const
{
	if(!addIns.isEmpty ())
		return true;
	return SuperClass::checkCommandCategory (category);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API EditAddInCollection::interpretCommand (const CommandMsg& msg)
{
	// delegate to add-ins
	ArrayForEach (addIns, AddInItem, addIn)
		if(UnknownPtr<ICommandHandler> c = addIn->getPlugInUnknown ())
			if(c->checkCommandCategory (msg.category))
				if(c->interpretCommand (msg))
					return true;
	EndFor
	return SuperClass::interpretCommand (msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditAddInCollection::restore (Settings& settings)
{
	ForEach (addIns, AddInItem, addIn)
		if(UnknownPtr<IPersistAttributes> p = addIn->getPlugInUnknown ())
		{
			String id ("EditAddIn/");
			id << addIn->getID ();

			Attributes& a = settings.getAttributes (id);
			p->restoreValues (a);
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditAddInCollection::flush (Settings& settings)
{
	ForEach (addIns, AddInItem, addIn)
		if(UnknownPtr<IPersistAttributes> p = addIn->getPlugInUnknown ())
		{
			String id ("EditAddIn/");
			id << addIn->getID ();

			Attributes& a = settings.getAttributes (id);
			a.removeAll ();
			p->storeValues (a);
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API EditAddInCollection::createView (StringID name, VariantRef data, const Rect& bounds)
{
	if(name.contains ("addIn"))
	{
		int index = 0;
		::sscanf (name, "@addIn[%d]", &index);
		AddInItem* addIn = (AddInItem*)addIns.at (index);
		ASSERT (addIn)
		if(addIn)
		{
			String groups;
			UnknownPtr<ISkinCreateArgs> args (data.asUnknown ());
			ASSERT (args)
			if(args)
			{
				Variant var;
				args->getVariable (var, "addIn.groups");
				groups = var.asString ();
			}
			
			// return empty view if group condition not satisfied
			if(!groups.contains (addIn->getGroupID ()))
				return ccl_new<IView> (ClassID::NullView);


			AddInItem* firstMatching = nullptr;
			AddInItem* lastMatching = nullptr;
			for (auto addIn: iterate_as<AddInItem> (addIns))
			{
				if(groups.contains (addIn->getGroupID ()))
				{
					lastMatching = addIn;
					
					if(!firstMatching)
						firstMatching = addIn;
				}
			}
			
			Rect r (bounds);
			if(r.isEmpty ())
				r (0, 0, 32, 32);

			UnknownPtr<IController> wm (&System::GetWindowManager ());
			IParameter* windowParam = wm->findParameter (addIn->getWindowClassID ());
			ASSERT (windowParam != nullptr)

			ControlBox button (ClassID::Toggle, windowParam, r);

			if(addIn == firstMatching)
				ViewBox::StyleModifier (button).setCommonStyle (Styles::kLeft);
			if(addIn == lastMatching)
				ViewBox::StyleModifier (button).setCommonStyle (Styles::kRight);

			button.setAttribute (kButtonIcon, addIn->getIcon ());
			button.setTooltip (String () << "@cmd.title[" << addIn->getCommandCategory () << "|" << addIn->getCommandName () << "]");

			return button;
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API EditAddInCollection::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "numAddIns")
	{
		var = addIns.count ();
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}
