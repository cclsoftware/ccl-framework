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
// Filename    : ccl/app/editing/tasks/edittaskdescription.cpp
// Description : Edit Task Description
//
//************************************************************************************************

#include "ccl/app/editing/tasks/edittaskdescription.h"

#include "ccl/app/utilities/pluginclass.h"
#include "ccl/public/app/iedittask.h"

#include "ccl/base/boxedtypes.h"
#include "ccl/base/storage/settings.h"

#include "ccl/public/gui/icommandhandler.h"
#include "ccl/public/text/translation.h"
#include "ccl/public/plugservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_TERM_LEVEL (EditTaskDescription, kAppLevel)
{
	if(EditTaskDescription::Registrar::peekInstance ())
		EditTaskDescription::Registrar::instance ().terminate ();
}

//************************************************************************************************
// EditTaskDescription
//************************************************************************************************

EditTaskDescription::Category* EditTaskDescription::defineCategory (StringRef name, const LocalString& translation, StringRef description)
{
	ASSERT (!name.isEmpty () && !Registrar::instance ().findCategory (name))

	Category* c = NEW Category (name, translation.getText (), translation.getKey ());
	Registrar::instance ().addCategory (c);

	// description is saved in root menu group
	if(!description.isEmpty ())
		defineMenuGroup (MutableCString (name))->setDescription (description);

	return c;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditTaskDescription::MenuGroup* EditTaskDescription::defineMenuGroup (StringID id, StringRef title, int priority)
{
	ASSERT (!id.isEmpty () && !Registrar::instance ().findMenuGroup (id))

	MenuGroup* group = NEW MenuGroup (id, title);
	group->setPriority (priority);
	Registrar::instance ().addMenuGroup (group);
	return group;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CommandWithTitle EditTaskDescription::getTaskCommandWithTitle (MetaClassRef taskClass)
{
	const IClassDescription* description = System::GetPlugInManager ().getClassDescription (taskClass.getClassID ());
	ASSERT (description)
	if(description)
	{
		EditTaskDescription task (*description);
		return CommandWithTitle (task.getCommandCategory (), task.getName (), task.getLocalizedName ());
	}
	return CommandWithTitle ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (EditTaskDescription, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

EditTaskDescription::EditTaskDescription ()
: menuPriority (1000),
  menuFollow (false),
  iconChecked (false),
  hiddenCommand (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditTaskDescription::EditTaskDescription (const IClassDescription& description)
: menuPriority (1000),
  menuFollow (false),
  iconChecked (false),
  hiddenCommand (false)
{
	assign (description);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditTaskDescription::assign (const IClassDescription& description)
{
	cid = description.getClassID ();
	name = description.getName ();
	category = description.getSubCategory ();
	description.getLocalizedName (localizedName);
	description.getLocalizedDescription (localizedDescription);

	description.getClassAttributes (attributes);

	// a task can override the command category
	commandCategory = attributes.getCString ("commandCategory");
	if(commandCategory.isEmpty ())
	{
		if(Category* c = Registrar::instance ().findCategory (category))
			commandCategory = c->getCommandCategory ();

		ASSERT (!commandCategory.isEmpty ()) // should be defined explicitely!!!
		if(commandCategory.isEmpty ())
			commandCategory = category;
	}

	aliasCommandName = attributes.getCString ("commandAlias");

	hiddenCommand = attributes.getBool ("hidden");
	if(hiddenCommand) // defaults to hidden in menu, too
		menuPriority = -1;

	menuGroupName = attributes.getCString ("menuGroup");
	attributes.getInt (menuPriority, "menuPriority");
	menuFollow = attributes.getBool ("menuFollow");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* EditTaskDescription::getIcon ()
{
	if(!iconChecked)
	{
		iconChecked = true;

		// 1) try icon provided by implementing module
		PlugInMetaInfo metaInfo (getClassID ());
		if(IImage* infoIcon = metaInfo.getImage ())
			icon = infoIcon;

		// 2) try icon from application skin
		if(icon == nullptr)
		{
			PlugInClass classInfo;
			classInfo.setClassID (getClassID ());
			classInfo.setCategory (PLUG_CATEGORY_EDITTASK);
			classInfo.setSubCategory (getCategory ());
			classInfo.setName (String (getName ()));

			// Note: Icon must be an exact match, display none otherwise.
			// Category icon isn't used here.
			if(IImage* classIcon = classInfo.getExactIcon ())
				icon = classIcon;
		}
	}
	return UnknownPtr<IImage> (icon);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const EditTaskDescription::MenuGroup* EditTaskDescription::getMenuGroup () const
{
	return Registrar::instance ().findMenuGroup (getMenuGroupName ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Attributes& EditTaskDescription::getAttributes () const
{
	return attributes;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString EditTaskDescription::getCommandArguments () const
{
	return attributes.getCString ("arguments");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditTaskDescription::isHidden () const
{
	return menuPriority == -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditTaskDescription::hasOption (StringID id) const
{
	Variant value;
	attributes.getAttribute (value, id);
	return value.parseInt ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int EditTaskDescription::compare (const Object& obj) const
{
	const EditTaskDescription* other = ccl_cast<EditTaskDescription> (&obj);
	if(other)
	{
		int groupDiff = Registrar::instance ().compareMenuGroup (menuGroupName, other->menuGroupName);
		if(groupDiff)
			return groupDiff;

		int prioDiff = menuPriority - other->getMenuPriority ();
		if(prioDiff)
			return prioDiff;

		//return localizedName.compare (other->localizedName);
		return name.compare (other->name); // keep order language-independent
	}
	else
		return SuperClass::compare (obj);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditTaskDescription::toString (String& string, int flags) const
{
	string = localizedName;
	return true;
}

//************************************************************************************************
// EditTaskDescription::Category
//************************************************************************************************

DEFINE_CLASS_HIDDEN (EditTaskDescription::Category, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

EditTaskDescription::Category::Category (StringRef name, StringRef title, StringID commandCategory)
: name (name),
  title (title),
  commandCategory (commandCategory)
{}

//************************************************************************************************
// EditTaskDescription::MenuGroup
//************************************************************************************************

DEFINE_CLASS_HIDDEN (EditTaskDescription::MenuGroup, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

EditTaskDescription::MenuGroup::MenuGroup (StringID id, StringRef title)
: id (id),
  title (title),
  priority (0),
  flags (0)
{}

//************************************************************************************************
// EditTaskDescription::Registrar
//************************************************************************************************

DEFINE_SINGLETON (EditTaskDescription::Registrar)

//////////////////////////////////////////////////////////////////////////////////////////////////

EditTaskDescription::Registrar::Registrar ()
{
	categories.objectCleanup (true);
	menuGroups.objectCleanup (true);
	recentList.objectCleanup (true);

	initialize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditTaskDescription::Registrar::initialize ()
{
	Settings::instance ().addSaver (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditTaskDescription::Registrar::terminate ()
{
	Settings::instance ().removeSaver (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditTaskDescription::Registrar::addMenuGroup (MenuGroup* group)
{
	menuGroups.add (group);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const EditTaskDescription::MenuGroup* EditTaskDescription::Registrar::findMenuGroup (StringID id) const
{
	if(!id.isEmpty ())
		ArrayForEach (menuGroups, MenuGroup, group)
			if(group->getID () == id)
				return group;
		EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int EditTaskDescription::Registrar::compareMenuGroup (StringID leftId, StringID rightId) const
{
	const EditTaskDescription::MenuGroup* leftGroup = findMenuGroup (leftId);
	const EditTaskDescription::MenuGroup* rightGroup = findMenuGroup (rightId);
	if(leftGroup && rightGroup)
		return leftGroup->getPriority () - rightGroup->getPriority ();
	else
		return leftId.compare (rightId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditTaskDescription::Registrar::addCategory (Category* category)
{
	categories.add (category);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditTaskDescription::Category* EditTaskDescription::Registrar::findCategory (StringRef category) const
{
	ArrayForEach (categories, Category, c)
		if(c->getName () == category)
			return c;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String EditTaskDescription::Registrar::getCategoryTitle (StringRef category) const
{
	String title;
	if(Category* c = findCategory (category))
		title = c->getTitle ();
	SOFT_ASSERT (!title.isEmpty (), "Edit task category not defined!\n")  // should be defined explicitely!!!
	if(title.isEmpty ())
		title = category;
	return title;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* EditTaskDescription::Registrar::getCategoryIcon (StringRef category, StringID subGroup) const
{
	// TODO: subgroup + cache icons!
	return PlugInCategory (PLUG_CATEGORY_EDITTASK, category).getIcon ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int EditTaskDescription::Registrar::getRecentTaskCount () const
{
	return recentList.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UIDRef EditTaskDescription::Registrar::getRecentTask (int index) const
{
	if(Boxed::UID* cid = (Boxed::UID*)recentList.at (index))
		return *cid;
	else
		return kNullUID;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditTaskDescription::Registrar::setRecentTask (UIDRef cid)
{
	int index = recentList.index (Boxed::UID (cid));
	if(index != 0)
	{
		if(index == -1)
		{
			recentList.insertAt (0, NEW Boxed::UID (cid));
			// TODO: limit number of entries???
		}
		else
		{
			Boxed::UID* existing = (Boxed::UID*)recentList.at (index);
			recentList.removeAt (index);
			recentList.insertAt (0, existing);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditTaskDescription::Registrar::flush (Settings& settings)
{
	Attributes& a = settings.getAttributes ("RecentEditTasks");
	a.removeAll ();
	a.queue (nullptr, recentList, Attributes::kShare);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditTaskDescription::Registrar::restore (Settings& settings)
{
	Attributes& a = settings.getAttributes ("RecentEditTasks");

	ObjectArray temp;
	temp.objectCleanup (true);
	a.unqueue (temp, nullptr, ccl_typeid<Boxed::UID> ());

	ArrayForEach (temp, Boxed::UID, uid)
		if(!recentList.contains (*uid)) // filter duplicates (should not happen)
			recentList.add (return_shared (uid));
	EndFor
}
