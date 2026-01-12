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
// Filename    : ccl/app/editing/tasks/edittaskdescription.h
// Description : Edit Task Description
//
//************************************************************************************************

#ifndef _ccl_edittaskdescription_h
#define _ccl_edittaskdescription_h

#include "ccl/base/singleton.h"
#include "ccl/base/storage/attributes.h"
#include "ccl/base/storage/isettings.h"

namespace CCL {

interface IImage;
interface IClassDescription;
class LocalString;
struct CommandWithTitle;

//************************************************************************************************
// EditTaskDescription
/** Edit task description. */
//************************************************************************************************

class EditTaskDescription: public Object
{
public:
	DECLARE_CLASS (EditTaskDescription, Object)

	EditTaskDescription ();
	EditTaskDescription (const IClassDescription& description);

	class Category;
	class MenuGroup;
	class Registrar;

	static Category* defineCategory (StringRef name, const LocalString& translation, StringRef description = nullptr); ///< define task category
	static MenuGroup* defineMenuGroup (StringID id, StringRef title = nullptr, int priority = 0); ///< define menu group
	static CommandWithTitle getTaskCommandWithTitle (MetaClassRef taskClass);

	void assign (const IClassDescription& description);

	PROPERTY_OBJECT (UID, cid, ClassID)
	PROPERTY_STRING (category, Category)
	PROPERTY_STRING (localizedName, LocalizedName)
	PROPERTY_STRING (localizedDescription, LocalizedDescription)
	PROPERTY_MUTABLE_CSTRING (name, Name)
	PROPERTY_MUTABLE_CSTRING (commandCategory, CommandCategory)
	PROPERTY_MUTABLE_CSTRING (aliasCommandName, AliasCommandName)
	PROPERTY_MUTABLE_CSTRING (menuGroupName, MenuGroupName)
	PROPERTY_VARIABLE (int, menuPriority, MenuPriority) ///< lower values first
	PROPERTY_BOOL (menuFollow, MenuFollow)
	PROPERTY_BOOL (hiddenCommand, HiddenCommand) ///< no command registration

	IImage* getIcon ();
	const MenuGroup* getMenuGroup () const;
	const Attributes& getAttributes () const;
	MutableCString getCommandArguments () const;
	bool isHidden () const; ///< hidden in menu
	bool hasOption (StringID id) const;

	// Object
	int compare (const Object& obj) const override;
	bool toString (String& string, int flags = 0) const override;

protected:
	Attributes attributes;
	SharedPtr<IUnknown> icon;
	bool iconChecked;
};

//************************************************************************************************
// EditTaskDescription::Category
/** Edit task category. */
//************************************************************************************************

class EditTaskDescription::Category: public Object
{
public:
	DECLARE_CLASS (Category, Object)

	Category (StringRef name = nullptr, StringRef title = nullptr, StringID commandCategory = nullptr);

	PROPERTY_STRING (name, Name)
	PROPERTY_STRING (title, Title)
	PROPERTY_MUTABLE_CSTRING (commandCategory, CommandCategory)
};

//************************************************************************************************
// EditTaskDescription::MenuGroup
/** Edit task menu group. */
//************************************************************************************************

class EditTaskDescription::MenuGroup: public Object
{
public:
	DECLARE_CLASS (MenuGroup, Object)

	MenuGroup (StringID id = nullptr, StringRef title = nullptr);

	PROPERTY_MUTABLE_CSTRING (id, ID)
	PROPERTY_STRING (title, Title)
	PROPERTY_STRING (description, Description)
	PROPERTY_VARIABLE (int, priority, Priority) ///< lower values first

	enum Flags { kIsSubMenu = 1<<0 };
	PROPERTY_VARIABLE (int, flags, Flags)
	PROPERTY_FLAG (flags, kIsSubMenu, isSubMenu)
};

//************************************************************************************************
// EditTaskDescription::Registrar
/** Edit task registrar. */
//************************************************************************************************

class EditTaskDescription::Registrar: public Object,
									  public Singleton<Registrar>,
									  public ISettingsSaver
{
public:
	Registrar ();

	void terminate ();

	void addMenuGroup (MenuGroup* group);
	const MenuGroup* findMenuGroup (StringID id) const;
	int compareMenuGroup (StringID id1, StringID id2) const;

	void addCategory (Category* category);
	Category* findCategory (StringRef category) const;
	String getCategoryTitle (StringRef category) const;
	IImage* getCategoryIcon (StringRef category, StringID subGroup = nullptr) const;

	int getRecentTaskCount () const;
	UIDRef getRecentTask (int index) const;
	void setRecentTask (UIDRef cid);

	CLASS_INTERFACE (ISettingsSaver, Object)

protected:
	ObjectArray categories;
	ObjectArray menuGroups;
	ObjectArray recentList;

	void initialize ();

	// ISettingsSaver
	void restore (Settings&) override;
	void flush (Settings&) override;
};

} // namespace CCL

#endif // _ccl_edittaskdescription_h
