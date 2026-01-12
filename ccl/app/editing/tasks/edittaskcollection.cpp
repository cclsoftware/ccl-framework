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
// Filename    : ccl/app/editing/tasks/edittaskcollection.cpp
// Description : Edit Task Collection
//
//************************************************************************************************

#include "ccl/app/editing/tasks/edittaskcollection.h"
#include "ccl/app/editing/tasks/edittaskhandler.h"

#include "ccl/app/editing/editor.h"
#include "ccl/app/editing/editmodel.h"
#include "ccl/app/editing/editview.h"

#include "ccl/app/params.h"

#include "ccl/base/storage/configuration.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/base/irecognizer.h"
#include "ccl/public/gui/framework/icommandtable.h"
#include "ccl/public/gui/framework/iparametermenu.h"
#include "ccl/public/gui/framework/ipopupselector.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/guiservices.h"
#include "ccl/public/plugservices.h"

#define DEBUG_MENU_PRIORITIES (DEBUG && 0)

namespace CCL {

//************************************************************************************************
// EditTaskParameter
//************************************************************************************************

class EditTaskParameter: public MenuParam,
						 public IParameterMenuCustomize
{
public:
	EditTaskParameter (StringID name, EditTaskCollection& collection);

	// MenuParam
	void CCL_API extendMenu (IMenu& menu, StringID name) override;

	CLASS_INTERFACE (IParameterMenuCustomize, MenuParam)

protected:
	EditTaskCollection& collection;

	// IParameterMenuCustomize
	StringID CCL_API getMenuType () const override;
	tbool CCL_API buildMenu (IMenu& menu, IParameterMenuBuilder& builder) override;
	tbool CCL_API onMenuKeyDown (const CCL::KeyEvent& event) override;

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
};

//************************************************************************************************
// EditTaskContextMenuFilter
//************************************************************************************************

class EditTaskContextMenuFilter: public ObjectFilter
{
public:
	EditTaskContextMenuFilter (IContextMenu& contextMenu, StringID attribute = nullptr, bool invertAttribute = false)
	: contextMenu (contextMenu),
	  attribute (attribute),
	  invertAttribute (invertAttribute)
	{}

	PROPERTY_MUTABLE_CSTRING (attribute, Attribute)
	PROPERTY_BOOL (invertAttribute, InvertAttribute)

	// ObjectFilter
	tbool CCL_API matches (IUnknown* object) const override
	{
		EditTaskDescription* task = unknown_cast<EditTaskDescription> (object);
		ASSERT (task != nullptr)
		if(!task)
			return false;

		// check duplicates
		if(contextMenu.hasCommandItem (task->getCommandCategory (), task->getName ()))
			return false;

		// check attribute
		if(!attribute.isEmpty ())
		{
			if(invertAttribute)
				return !task->getAttributes ().contains (attribute);

			return task->getAttributes ().contains (attribute);
		}

		return true;
	}

protected:
	IContextMenu& contextMenu;
};

//************************************************************************************************
// EditTaskHelper
//************************************************************************************************

class EditTaskHelper
{
public:
	struct MenuOptions
	{
		int flags = 0;
		PROPERTY_FLAG (flags, 1<<0, withIcon)
		PROPERTY_FLAG (flags, 1<<1, indicateMenuFollows)
		PROPERTY_FLAG (flags, 1<<2, subMenusEnabled)
		PROPERTY_FLAG (flags, 1<<3, inplaceMode)
	};

	static void addTaskToMenu (IMenu& menu, EditTaskDescription& task, EditTaskCollection* collection, MenuOptions options);
	static void addTasksToMenu (IMenu& menu, const ObjectArray& tasks, EditTaskCollection* collection, MenuOptions options);

	static EditTaskDescription* findTask (Container& tasks, StringID commandCategory, StringID commandName)
	{
		ForEach (tasks, EditTaskDescription, task)
			if(commandCategory == task->getCommandCategory ())
				if(commandName == task->getName () || commandName == task->getAliasCommandName ())
					return task;
		EndFor
		return nullptr;
	}
};

//************************************************************************************************
// EditTaskInplaceComponent
//************************************************************************************************

class EditTaskInplaceComponent: public Component
{
public:
	EditTaskInplaceComponent (EditTaskHandler* handler, EditTaskCollection* collection);
	~EditTaskInplaceComponent ();

	IView* createMenuItemView ();

	// Component
	IView* CCL_API createView (StringID name, VariantRef data, const Rect& bounds) override;
	tbool CCL_API paramChanged (IParameter* param) override;

protected:
	static constexpr int kApplyTag = 'Aply';

	SharedPtr<EditTaskHandler> handler;
	SharedPtr<EditTaskCollection> collection;
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("EditTask")
	XSTRING (RecentTasks, "Recent items")
	XSTRING (NoRecentTasks, "No recent items")
END_XSTRINGS

//************************************************************************************************
// EditTaskCollection::MenuBuilder
//************************************************************************************************

EditTaskCollection::MenuBuilder::MenuBuilder ()
{
	tasks.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditTaskCollection::MenuBuilder::addtasks (StringRef taskCategory, IObjectFilter* filter)
{
	// sort by menu priority and name (alphabetically)
	ForEachPlugInClass (PLUG_CATEGORY_EDITTASK, desc)
		if(desc.getSubCategory () == taskCategory)
		{
			AutoPtr<EditTaskDescription> task = NEW EditTaskDescription (desc);
			if(!task->isHidden () && (!filter || filter->matches (task->asUnknown ())))
				tasks.addSorted (task.detach ());
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditTaskCollection::MenuBuilder::makeMainMenu (IMenu& menu)
{
	EditTaskHelper::MenuOptions menuOptions;
	menuOptions.withIcon (taskMenuIconsEnabled);
	menuOptions.indicateMenuFollows (true);

	EditTaskHelper::addTasksToMenu (menu, tasks, nullptr, menuOptions);
}

//************************************************************************************************
// EditTaskCollection
//************************************************************************************************

Configuration::BoolValue EditTaskCollection::taskMenuIconsEnabled ("Editing", "taskMenuIconsEnabled", true);
Configuration::BoolValue EditTaskCollection::taskMenuRecentEnabled ("Editing", "taskMenuRecentEnabled", true);
Configuration::BoolValue EditTaskCollection::taskMenuInplaceMode ("Editing", "taskMenuInplaceMode", false);
Configuration::BoolValue EditTaskCollection::taskMenuFlat ("Editing", "taskMenuFlat", false);

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditTaskCollection::registerTaskCommands (StringRef taskCategory)
{
	ObjectArray tasks;
	tasks.objectCleanup (true);

	// sort by menu priority and name (alphabetically)
	ForEachPlugInClass (PLUG_CATEGORY_EDITTASK, desc)
		if(desc.getSubCategory () == taskCategory)
			tasks.addSorted (NEW EditTaskDescription (desc));
	EndFor

	LocalString::BeginScope beginScope ("Command");
	ForEach (tasks, EditTaskDescription, task)
		registerTaskCommand (*task);
	EndFor
	LocalString::EndScope endScope;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditTaskCollection::registerTaskCommand (MetaClassRef taskClass)
{
	const IClassDescription* classDesc = System::GetPlugInManager ().getClassDescription (taskClass.getClassID ());
	if(classDesc)
	{
		EditTaskDescription task (*classDesc);
		if(!task.isHiddenCommand ())
		{
			LocalString::BeginScope beginScope ("Command");
			registerTaskCommand (task);
			LocalString::EndScope endScope;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditTaskCollection::registerTaskCommand (const EditTaskDescription& task)
{
	if(!task.isHiddenCommand ())
	{
		LocalString categoryString (task.getCommandCategory ()); // category must be translated in hosting application
		CommandDescription command (task.getCommandCategory (), task.getName (), categoryString, task.getLocalizedName ());
		command.arguments = task.getCommandArguments ();
		command.classID = task.getClassID ();
		command.englishName = task.getName ();
		System::GetCommandTable ().registerCommand (command);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditTaskCollection::makeMainMenu (IMenu& menu, StringRef taskCategory, IObjectFilter* filter)
{
	MenuBuilder builder;
	builder.addtasks (taskCategory, filter);
	builder.makeMainMenu (menu);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditTaskCollection::makeContextMenu (IContextMenu& contextMenu, Container& taskCollections)
{
	makeContextMenu (contextMenu, taskCollections, taskMenuRecentEnabled);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditTaskCollection::makeContextMenu (IContextMenu& contextMenu, Container& taskCollections, bool withRecentTasks)
{
	UnknownPtr<IMenu> popupMenu (&contextMenu);
	ASSERT (popupMenu != nullptr)
	if(!popupMenu)
		return;

	EditTaskDescription::Registrar& registrar = EditTaskDescription::Registrar::instance ();
	if(withRecentTasks)
	{
		// add recent tasks
		ObjectArray recentTasks;
		const int kMaxRecent = 5;
		for(int i = 0, count = registrar.getRecentTaskCount (); i < count; i++)
		{
			UIDRef cid = registrar.getRecentTask (i);
			ForEach (taskCollections, EditTaskCollection, c)
				if(EditTaskDescription* task = c->findTask (cid))
					if(!task->isHidden () && c->canRunTask (*task))
					{
						// filter duplicates by command, also filter tasks with same display name
						if(!EditTaskHelper::findTask (recentTasks, task->getCommandCategory (), task->getName ())
							&& !recentTasks.findIf<EditTaskDescription> ([&] (const EditTaskDescription& t) { return t.getLocalizedName () == task->getLocalizedName (); }))
							recentTasks.add (task);
						break;
					}
			EndFor

			if(recentTasks.count () >= kMaxRecent)
				break;
		}

		UnknownPtr<IExtendedMenu> extendedMenu (popupMenu);
		if(extendedMenu)
			extendedMenu->addHeaderItem (XSTR (RecentTasks));

		if(recentTasks.isEmpty ())
			popupMenu->addCommandItem (XSTR (NoRecentTasks));
		else
		{
			EditTaskHelper::MenuOptions menuOptions;
			menuOptions.withIcon (taskMenuIconsEnabled);
			menuOptions.indicateMenuFollows (true);

			ArrayForEach (recentTasks, EditTaskDescription, task)
				EditTaskHelper::addTaskToMenu (*popupMenu, *task, nullptr, menuOptions);
			EndFor
		}

		popupMenu->addSeparatorItem ();
	}

	// add tasks of collections
	ForEach (taskCollections, EditTaskCollection, c)
		if(taskMenuFlat)
			c->appendWithFilter (*popupMenu, nullptr);
		else
		{
			IMenu* subMenu = popupMenu->createMenu ();
			subMenu->setMenuAttribute (IMenu::kMenuTitle, c->getTitle ());
			IMenuItem* menuItem = popupMenu->addMenu (subMenu); // add first to reuse identifiers

			if(IImage* categoryIcon = registrar.getCategoryIcon (c->getTaskCategory ()))
				menuItem->setItemAttribute (IMenuItem::kItemIcon, categoryIcon);

			// try to find additional info for this category
			if(const EditTaskDescription::MenuGroup* categoryGroup = registrar.findMenuGroup (MutableCString (c->getTaskCategory ())))
				menuItem->setItemAttribute (IMenuItem::kDescription, categoryGroup->getDescription ());
			
			c->appendWithFilter (*subMenu, nullptr);

			if(subMenu->countItems () == 0)
				popupMenu->removeItem (menuItem);
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (EditTaskCollection, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

EditTaskCollection::EditTaskCollection (StringRef name)
: Component (name.isEmpty () ? CCLSTR ("EditTasks") : name)
{
	tasks.objectCleanup (true);

	paramList.add (NEW EditTaskParameter (CSTR ("tasks"), *this));

	setTitle (EditTaskDescription::Registrar::instance ().getCategoryTitle (getTaskCategory ()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditTaskCollection::collectTasks (IObjectFilter* filter)
{
	tasks.removeAll ();

	StringRef taskCategory (getTaskCategory ());

	// sort by menu priority and name (alphabetically)
	ForEachPlugInClass (PLUG_CATEGORY_EDITTASK, desc)
		if(desc.getSubCategory () == taskCategory)
		{
			EditTaskDescription* task = NEW EditTaskDescription (desc);
			if(!filter || filter->matches (task->asUnknown ()))
			{
				tasks.addSorted (task);

				// a task can have an individual command category
				commandCategories.addOnce (String (task->getCommandCategory ()));
			}
			else
				task->release ();
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditTaskCollection::takeTasks (EditTaskCollection& otherTasks, IObjectFilter* filter)
{
	ObjectList movedTasks;
	for(auto task : otherTasks.tasks)
	{
		if(!filter || filter->matches (task->asUnknown ()))
		{
			tasks.addSorted (task);
			movedTasks.add (task);
		}
	}

	for(auto task : movedTasks)
		otherTasks.tasks.remove (task);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef EditTaskCollection::getTaskCategory () const
{
	return getName (); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditorComponent* EditTaskCollection::getEditor () const
{
	return getParentNode<EditorComponent> ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int EditTaskCollection::getTaskCount () const
{
	return tasks.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditTaskDescription* EditTaskCollection::getTask (int index) const
{
	return (EditTaskDescription*)tasks.at (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditTaskDescription* EditTaskCollection::findTask (UIDRef cid) const
{
	ForEach (tasks, EditTaskDescription, task)
		if(task->getClassID () == cid)
			return task;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditTaskCollection::canRunTask (const EditTaskDescription& task) const
{
	EditorComponent* editor = getEditor ();
	ASSERT (editor != nullptr)
	if(!editor)
		return false;

	EditView* editView = editor->getActiveEditView ();
	if(editView == nullptr)
		return false;

	return editor->getModel ().canPerformTask (*editView, task);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditTaskCollection::runTask (const EditTaskDescription& task, Attributes* arguments, bool hidden)
{
	EditorComponent* editor = getEditor ();
	ASSERT (editor != nullptr)
	if(!editor)
		return false;

	EditView* editView = editor->getActiveEditView ();
	if(editView == nullptr)
		return false;

	bool result = false;
	AutoPtr<EditTaskHandler> handler = EditTaskHandler::createTask (task);
	ASSERT (handler != nullptr)
	if(handler)
	{
		// pass arguments
		if(arguments)
		{
			handler->setSilentMode (true);
			handler->setPersistent (false);
			handler->setSavedValues (arguments);
		}

		// push to recent list
		if(!task.isHidden () && !hidden)
			EditTaskDescription::Registrar::instance ().setRecentTask (task.getClassID ());

		ObjectList candidates;
		candidates.objectCleanup (true);
		if(editor->getModel ().collectTaskCandidates (candidates, *editView, task))
			result = handler->runTask (candidates, editView) == kResultOk;
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditTaskCollection::appendContextMenuWithAttribute (IContextMenu& contextMenu, StringID attribute, int flags)
{
	EditTaskContextMenuFilter filter (contextMenu, attribute, flags & kInvertAttribute);
	appendWithFilter (contextMenu, &filter, flags);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API EditTaskCollection::appendContextMenu (IContextMenu& contextMenu)
{
	EditTaskContextMenuFilter filter (contextMenu);
	appendWithFilter (contextMenu, &filter, kAddSeparators);
	return kResultOk;;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditTaskCollection::appendWithFilter (IContextMenu& contextMenu, IObjectFilter* filter, int flags)
{
	UnknownPtr<IMenu> popupMenu (&contextMenu);
	ASSERT (popupMenu != nullptr)
	if(!popupMenu)
		return;

	appendWithFilter (*popupMenu, filter, flags);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditTaskCollection::appendWithFilter (IMenu& menu, IObjectFilter* filter, int flags)
{
	if(flags & kAddSeparators)
		menu.addSeparatorItem ();

	ObjectArray menuTasks;
	ForEach (tasks, EditTaskDescription, task)
		if(task->isHidden ())
			continue;
		
		if(filter && !filter->matches (task->asUnknown ()))
			continue;

		if(canRunTask (*task))
			menuTasks.add (task);
	EndFor

	EditTaskHelper::MenuOptions menuOptions;
	menuOptions.withIcon (taskMenuIconsEnabled);
	menuOptions.indicateMenuFollows ((flags & kHideMenuFollowDots) == 0);
	menuOptions.inplaceMode (taskMenuInplaceMode);

	EditTaskHelper::addTasksToMenu (menu, menuTasks, this, menuOptions);

	if(flags & kAddSeparators)
		menu.addSeparatorItem ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API EditTaskCollection::checkCommandCategory (CStringRef category) const
{			
	if(commandCategories.contains (String (category)))
		return true;

	return SuperClass::checkCommandCategory (category);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API EditTaskCollection::interpretCommand (const CommandMsg& msg)
{
	EditTaskDescription* task = EditTaskHelper::findTask (tasks, msg.category, msg.name);
	if(!task)
		return false;

	if(canRunTask (*task))
	{
		if(!msg.checkOnly ())
		{
			AutoPtr<Attributes> args;
			if(IAttributeList* arguments = CommandAutomator::getArguments (msg))
			{
				args = NEW PersistentAttributes; // edit tasks require PersistentAttributes!
				args->copyFrom (*arguments);
			}
			 
			runTask (*task, args);
		}
		return true;
	}
	return SuperClass::interpretCommand (msg);
}

//************************************************************************************************
// EditTaskParameter
//************************************************************************************************

EditTaskParameter::EditTaskParameter (StringID name, EditTaskCollection& collection)
: MenuParam (name),
  collection (collection)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API EditTaskParameter::extendMenu (IMenu& menu, StringID name)
{
	ObjectArray menuTasks;
	int count = collection.getTaskCount ();
	for(int i = 0; i < count; i++)
	{
		EditTaskDescription* task = collection.getTask (i);
		if(!task->isHidden ())
			menuTasks.add (task);
	}

	EditTaskHelper::MenuOptions menuOptions;
	menuOptions.withIcon (true);
	menuOptions.indicateMenuFollows (true);

	EditTaskHelper::addTasksToMenu (menu, menuTasks, &collection, menuOptions);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API EditTaskParameter::getMenuType () const
{
	return MenuPresentation::kExtended;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API EditTaskParameter::buildMenu (IMenu& menu, IParameterMenuBuilder& builder)
{
	return false; // use default implementation
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API EditTaskParameter::onMenuKeyDown (const KeyEvent& event)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API EditTaskParameter::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == MenuPopupSelectorBehavior::kCondensedMenuSeparators)
	{
		var = true;
		return true;
	}
	return MenuParam::getProperty (var, propertyId);
}

//************************************************************************************************
// EditTaskHelper
//************************************************************************************************

void EditTaskHelper::addTasksToMenu (IMenu& menu, const ObjectArray& tasks, EditTaskCollection* collection, MenuOptions options)
{
	MutableCString lastGroup;
	UnknownPtr<IExtendedMenu> extendedMenu (&menu);

	IMenu* currentMenu = &menu;
	ArrayForEach (tasks, EditTaskDescription, task)
		if(task->getMenuGroupName () != lastGroup)
		{
			currentMenu = &menu;

			String title;
			bool isSubMenu = false;
			if(const EditTaskDescription::MenuGroup* menuGroup = task->getMenuGroup ())
			{
				title = menuGroup->getTitle ();
				isSubMenu = menuGroup->isSubMenu ();
			}

			if(isSubMenu && options.subMenusEnabled ())
			{
				ASSERT (!title.isEmpty ())
				menu.addSeparatorItem ();
				IMenu* subMenu = menu.createMenu ();
				subMenu->setMenuAttribute (IMenu::kMenuTitle, title);
				menu.addMenu (subMenu);
				currentMenu = subMenu;
			}
			else
			{
				if(!title.isEmpty () && extendedMenu)
					extendedMenu->addHeaderItem (title);
				else
					menu.addSeparatorItem ();
			}
		}

		addTaskToMenu (*currentMenu, *task, collection, options);
		lastGroup = task->getMenuGroupName ();
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditTaskHelper::addTaskToMenu (IMenu& menu, EditTaskDescription& task, EditTaskCollection* collection, MenuOptions options)
{
	String title (task.getLocalizedName ());
	#if DEBUG_MENU_PRIORITIES
	title.prepend (String ().appendFormat ("[%(1)] ", task.getMenuPriority ()));
	#endif

	if(options.inplaceMode () && task.isMenuFollow ())
	{
		// embedd task form into submenu
		AutoPtr<EditTaskHandler> handler = EditTaskHandler::createTask (task);
		ASSERT (handler && collection)
		if(handler && collection)
		{
			handler->setSilentMode (true);

			// task needs to be prepared before view creation
			EditorComponent* editor = collection->getEditor ();
			EditView* editView = editor ? editor->getActiveEditView () : nullptr;
			handler->prepareTask (editView);

			IMenu* subMenu = menu.createMenu ();
			subMenu->setMenuAttribute (IMenu::kMenuTitle, title);
			subMenu->setMenuAttribute (IMenu::kMenuName, String (task.getName ()));
			menu.addMenu (subMenu);

			UnknownPtr<IExtendedMenu> extendedMenu (subMenu);
			AutoPtr<EditTaskInplaceComponent> c = NEW EditTaskInplaceComponent (handler, collection);
			AutoPtr<IView> view = c->createMenuItemView ();
			ASSERT (extendedMenu && view)
			if(extendedMenu && view)
				extendedMenu->addViewItem (view);
		}
	}
	else
	{
		if(options.indicateMenuFollows () && task.isMenuFollow ())
			title << IMenu::strFollowIndicator;

		IMenuItem* item = menu.addCommandItem (title, task.getCommandCategory (), task.getName (), collection);

		String description (task.getLocalizedDescription ());
		if(!description.isEmpty ())
		{
			description.prepend ("\n ");
			description.prepend (task.getLocalizedName ());
			item->setItemAttribute (IMenuItem::kTooltip, description);
		}

		if(options.withIcon ())
		{
			IImage* icon = task.getIcon ();
			item->setItemAttribute (IMenuItem::kItemIcon, Variant (icon));
		}
	}
}

//************************************************************************************************
// EditTaskInplaceComponent
//************************************************************************************************

EditTaskInplaceComponent::EditTaskInplaceComponent (EditTaskHandler* handler, EditTaskCollection* collection)
: handler (handler),
  collection (collection)
{
	ASSERT (handler && collection)

	paramList.addParam ("apply", kApplyTag);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditTaskInplaceComponent::~EditTaskInplaceComponent ()
{
	// View is still alive when in dtor, make sure to delay ccl_release() for the task
	deferDestruction (handler.detach ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* EditTaskInplaceComponent::createMenuItemView ()
{
	return getTheme ()->createView ("CCL/EditTaskInplaceView", this->asUnknown ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API EditTaskInplaceComponent::createView (StringID name, VariantRef data, const Rect& bounds)
{
	if(name == "TaskView")
		return handler->createTaskView ();
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API EditTaskInplaceComponent::paramChanged (IParameter* param)
{
	if(param->getTag () == kApplyTag)
	{
		EditorComponent* editor = collection->getEditor ();
		EditView* editView = editor ? editor->getActiveEditView () : nullptr;
		ASSERT (editor && editView)
		if(editor && editView)
		{
			ObjectList candidates;
			candidates.objectCleanup (true);
			if(editor->getModel ().collectTaskCandidates (candidates, *editView, handler->getDescription ()))
				handler->performTask (candidates, editView);
		}
	}
	return true;
}
