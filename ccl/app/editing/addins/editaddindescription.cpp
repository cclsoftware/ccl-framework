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
// Filename    : ccl/app/editing/addins/editaddindescription.cpp
// Description : Edit Add-in Description
//
//************************************************************************************************

#include "ccl/app/editing/addins/editaddindescription.h"

#include "ccl/app/component.h"

#include "ccl/public/app/ieditenvironment.h"

#include "ccl/base/storage/url.h"
#include "ccl/base/storage/storage.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/system/ipackagemetainfo.h"
#include "ccl/public/plugins/icoderesource.h"
#include "ccl/public/systemservices.h"

#include "ccl/public/plugins/iclassfactory.h"
#include "ccl/public/plugservices.h"

#include "ccl/public/gui/framework/icommandtable.h"
#include "ccl/public/gui/framework/iwindowmanager.h"
#include "ccl/public/guiservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("Command")
	XSTRING (View, "View")
END_XSTRINGS

//************************************************************************************************
// EditAddInDescription
//************************************************************************************************

DEFINE_CLASS (EditAddInDescription, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

EditAddInDescription::EditAddInDescription ()
: pluginMetaClass (nullptr),
  windowClass (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditAddInDescription::load (const Storage& storage)
{
	Attributes& a = storage.getAttributes ();
	themeName = a.getString ("themeName");
	formName = a.getString ("formName");
	groupName = a.getString ("groupName");
	return true;
}

//************************************************************************************************
// EditAddInDescription::Registrar
//************************************************************************************************

const CString EditAddInDescription::Registrar::kCommandCategory ("View");

DEFINE_SINGLETON (EditAddInDescription::Registrar)

//////////////////////////////////////////////////////////////////////////////////////////////////

EditAddInDescription::Registrar::Registrar ()
: useCount (0)
{
	descriptions.objectCleanup (true);
	categories.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditAddInDescription::Registrar::~Registrar ()
{
	ASSERT (descriptions.isEmpty ())
	ASSERT (useCount == 0)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditAddInDescription::Registrar::defineCategory (StringRef name, StringRef windowClassPath)
{
	ASSERT (useCount == 0)
	Category* c = NEW Category;
	c->setName (name);
	c->setWindowClassPath (windowClassPath);
	categories.add (c);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditAddInDescription::Registrar::onAddInsInitialize (bool state)
{
	if(state)
	{
		if(++useCount == 1)
		{
			ForEach (categories, Category, c)
				registerAddInClasses (c->getName (), c->getWindowClassPath ());
			EndFor
		}
	}
	else
	{
		if(--useCount == 0)
			unregisterAll ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditAddInDescription::Registrar::isHidden (const IClassDescription& classInfo)
{
	Variant hidden;
	return classInfo.getClassAttribute (hidden, "hidden") && hidden.asBool ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditAddInDescription::Registrar::registerAddInCommands ()
{
	ForEachPlugInClass (PLUG_CATEGORY_EDITADDIN, classInfo)
		if(isHidden (classInfo))
			continue;

		String title;
		classInfo.getLocalizedName (title);
		MutableCString name (classInfo.getName ());
		CommandDescription command (kCommandCategory, name, XSTR (View), title);
		command.classID = classInfo.getClassID ();
		command.englishName = name;
		System::GetCommandTable ().registerCommand (command);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditAddInDescription::Registrar::registerAddInClasses (StringRef subCategory, StringRef windowClassPath)
{
	CString workspaceName (RootComponent::instance ().getApplicationID ());

	ForEachPlugInClass (PLUG_CATEGORY_EDITADDIN, classInfo)
		if(classInfo.getSubCategory () == subCategory)
		{
			EditAddInDescription* description = NEW EditAddInDescription;

			Variant a1, a2, a3, a4;
			if(IPluginMetaClass* pluginMetaClass = System::GetPlugInManager ().createMetaClass (classInfo.getClassID ())) // => external plug-in
			{
				description->setPluginMetaClass (pluginMetaClass);

				if(UnknownPtr<ICodeResource> codeResource = const_cast<IClassDescription*> (ccl_classof (pluginMetaClass)))
				   if(const IAttributeList* metaInfo = codeResource->getMetaInfo ())
					   metaInfo->getAttribute (a1, Meta::kPackageID);
			}
			else // => statically linked
				a1 = RootComponent::instance ().getApplicationID ();

			classInfo.getClassAttribute (a2, "formName");
			classInfo.getClassAttribute (a3, "groupName");
			classInfo.getClassAttribute (a4, "commandName");

			description->setThemeName (a1.asString ());
			description->setFormName (a2.asString ());
			description->setGroupName (a3.asString ());

			ASSERT (!description->getThemeName ().isEmpty ())
			ASSERT (!description->getFormName ().isEmpty ())
			ASSERT (!description->getGroupName ().isEmpty ())

			// *** Register window class ***
			MutableCString windowClassId;
			UID (classInfo.getClassID ()).toCString (windowClassId);
			String formName (description->getFormName ());
			String groupName (description->getGroupName ());
			String controllerUrl (windowClassPath);
			controllerUrl << "/" << classInfo.getName ();
			MutableCString themeName (description->getThemeName ());
			IWindowClass* windowClass = System::GetWindowManager ().registerClass (windowClassId, formName, controllerUrl, groupName, workspaceName, themeName);
			ASSERT (windowClass != nullptr)

			// default command name: class name, can be overridden by class attribute "commandName"
			MutableCString commandName (a4);
			if(commandName.isEmpty ())
				commandName = classInfo.getName ();

			windowClass->setCommand (kCommandCategory, MutableCString (commandName));

			description->setWindowClass (windowClass);

			descriptions.add (description);
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditAddInDescription::Registrar::unregisterAll ()
{
	//ccl_forceGC ();

	ForEach (descriptions, EditAddInDescription, description)
		if(UnknownPtr<IWindowClass> windowClass = description->getWindowClass ())
			System::GetWindowManager ().unregisterClass (windowClass);

		if(IPluginMetaClass* pluginMetaClass = description->getPluginMetaClass ())
			ccl_release (pluginMetaClass);
	EndFor

	descriptions.removeAll ();
}
