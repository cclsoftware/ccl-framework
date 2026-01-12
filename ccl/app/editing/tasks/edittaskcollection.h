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
// Filename    : ccl/app/editing/tasks/edittaskcollection.h
// Description : Edit Task Collection
//
//************************************************************************************************

#ifndef _ccl_edittaskcollection_h
#define _ccl_edittaskcollection_h

#include "ccl/app/component.h"

#include "ccl/base/collections/objectarray.h"
#include "ccl/base/collections/stringlist.h"
#include "ccl/base/storage/configuration.h"

namespace CCL {

interface IMenu;
interface IObjectFilter;

class EditTaskDescription;
class EditorComponent;

//************************************************************************************************
// EditTaskCollection
//************************************************************************************************

class EditTaskCollection: public Component
{
public:
	DECLARE_CLASS (EditTaskCollection, Component)

	EditTaskCollection (StringRef name = nullptr);

	static void registerTaskCommands (StringRef taskCategory);
	static void registerTaskCommand (MetaClassRef taskClass);
	static void makeMainMenu (IMenu& menu, StringRef taskCategory, IObjectFilter* filter = nullptr);
	static void makeContextMenu (IContextMenu& contextMenu, Container& taskCollections);
	static void makeContextMenu (IContextMenu& contextMenu, Container& taskCollections, bool withRecentTasks);

	void collectTasks (IObjectFilter* filter = nullptr);
	void takeTasks (EditTaskCollection& otherTasks, IObjectFilter* filter = nullptr);
	StringRef getTaskCategory () const;
	EditorComponent* getEditor () const;

	int getTaskCount () const;
	EditTaskDescription* getTask (int index) const;
	EditTaskDescription* findTask (UIDRef cid) const;
	bool canRunTask (const EditTaskDescription& task) const;
	bool runTask (const EditTaskDescription& task, Attributes* arguments = nullptr, bool hidden = false);

	enum TaskMenuOptions
	{
		kAddSeparators = 1 << 0,
		kHideMenuFollowDots = 1 << 1,
		kInvertAttribute = 1 << 2
	};

	void appendContextMenuWithAttribute (IContextMenu& contextMenu, StringID attribute, int flags = kAddSeparators);

	class MenuBuilder;

	// Component
	tresult CCL_API appendContextMenu (IContextMenu& contextMenu) override;
	tbool CCL_API checkCommandCategory (CStringRef category) const override;
	tbool CCL_API interpretCommand (const CommandMsg& msg) override;

protected:
	static CCL::Configuration::BoolValue taskMenuIconsEnabled;
	static CCL::Configuration::BoolValue taskMenuRecentEnabled;
	static CCL::Configuration::BoolValue taskMenuInplaceMode;
	static CCL::Configuration::BoolValue taskMenuFlat;

	ObjectArray tasks;
	StringList commandCategories;

	static void registerTaskCommand (const EditTaskDescription& task);
	void appendWithFilter (IContextMenu& contextMenu, IObjectFilter* filter, int flags = 0);
	void appendWithFilter (IMenu& menu, IObjectFilter* filter, int flags = 0);
};

//************************************************************************************************
// EditTaskCollection::MenuBuilder
//************************************************************************************************

class EditTaskCollection::MenuBuilder
{
public:
	MenuBuilder ();

	void addtasks (StringRef taskCategory, IObjectFilter* filter);
	void makeMainMenu (IMenu& menu);

private:
	ObjectArray tasks;
};

} // namespace CCL

#endif // _ccl_edittaskcollection_h
