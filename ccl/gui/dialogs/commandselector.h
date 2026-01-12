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
// Filename    : ccl/gui/dialogs/commandselector.h
// Description : Command Selector
//
//************************************************************************************************

#ifndef _ccl_commandselector_h
#define _ccl_commandselector_h

#include "ccl/gui/commands.h"
#include "ccl/base/asyncoperation.h"
#include "ccl/base/storage/configuration.h"

#include "ccl/public/base/iobjectnode.h"
#include "ccl/public/gui/icontroller.h"
#include "ccl/public/gui/paramlist.h"
#include "ccl/public/gui/iparamobserver.h"
#include "ccl/public/gui/framework/icommandeditor.h"
#include "ccl/public/gui/framework/iitemmodel.h"

namespace CCL {

class CommandSelector;
class CommandItemFilter;
class DialogBuilder;

//************************************************************************************************
// CommandItem
//************************************************************************************************

class CommandItem: public Object
{
public:
	DECLARE_CLASS_ABSTRACT (CommandItem, Object)

	enum Type
	{
		kRoot,
		kCategory,
		kCommand
	};

	CommandItem (Type type, StringRef title = nullptr);
	CommandItem (CommandCategory& category);
	CommandItem (KnownCommand& command);

	PROPERTY_VARIABLE (Type, type, Type)
	PROPERTY_STRING (title, Title)

	PROPERTY_SHARED_AUTO (CommandCategory, category, Category)
	PROPERTY_SHARED_AUTO (KnownCommand, command, Command)
};

//************************************************************************************************
// CommandTreeModel
//************************************************************************************************

class CommandTreeModel: public Object,
						public ItemViewObserver<AbstractItemModel>
{
public:
	CommandTreeModel (CommandContainer& commands);
	~CommandTreeModel ();

	PROPERTY_POINTER (CommandSelector, target, Target)
	CommandContainer& getCategories () { return categories; }

	PROPERTY_BOOL (keyColumnEnabled, KeyColumnEnabled)
	PROPERTY_BOOL (argColumnEnabled, ArgColumnEnabled)

	// IItemModel
	tbool CCL_API createColumnHeaders (IColumnHeaderList& list) override;
	tbool CCL_API getRootItem (ItemIndex& index) override;
	tbool CCL_API isItemFolder (ItemIndexRef index) override;
	tbool CCL_API canExpandItem (ItemIndexRef index) override;
	tbool CCL_API getSubItems (IUnknownList& items, ItemIndexRef index) override;
	tbool CCL_API getItemTitle (String& title, ItemIndexRef index) override;
	tbool CCL_API drawCell (ItemIndexRef index, int column, const DrawInfo& info) override;
	tbool CCL_API editCell (ItemIndexRef index, int column, const EditInfo& info) override;
	tbool CCL_API onItemFocused (ItemIndexRef index) override;
	tbool CCL_API openItem (ItemIndexRef index, int column, const EditInfo& info) override;
	void CCL_API viewAttached (IItemView* itemView) override;

	CLASS_INTERFACE (IItemModel, Object)

protected:
	CommandContainer& categories;
	CommandItem& rootItem;

	enum ColumnID
	{
		kCommand,
		kKey,
		kArguments
	};

	CCL::Vector<ColumnID> enabledColumns;

	CommandItem* resolve (ItemIndexRef index) const;
};

//************************************************************************************************
// CommandSelector
//************************************************************************************************

class CommandSelector: public Object,
					   public AbstractNode,
					   public AbstractController,
					   public IParamObserver,
					   public ICommandSelector
{
public:
	DECLARE_CLASS (CommandSelector, Object)
	DECLARE_PROPERTY_NAMES (CommandSelector)
	DECLARE_METHOD_NAMES (CommandSelector)

	CommandSelector ();
	~CommandSelector ();
	
	// ICommandSelector
	tresult CCL_API run (CommandDescription& command) override;
	IAsyncOperation* CCL_API runAsync (const CommandDescription& command, tbool popupMode) override;
	tresult CCL_API setCommands (ICommandContainer* commands) override;
	tresult CCL_API getSelectedCommand (CommandDescription& command) const override;

	// IController
	IUnknown* CCL_API getObject (StringID name, UIDRef classID) override;
	DECLARE_PARAMETER_LOOKUP (paramList)

	// IParamObserver
	tbool CCL_API paramChanged (IParameter* param) override;
	void CCL_API paramEdit (IParameter* param, tbool begin) override {}

	// internal (used by CommandTreeModel):
	virtual void onViewAttached (IItemView* itemView);
	virtual tbool onCommandItemFocused (CommandItem& item);
	virtual tbool onCommandItemOpened (CommandItem& item);
	virtual tbool onEditKeyColumn (CommandItem& item, const IItemModel::EditInfo& info);
	virtual void setFocusCommand (KnownCommand* command);

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	CLASS_INTERFACES (Object)

protected:
	String name;
	CommandTreeModel* commandTree;
	ParamList paramList;
	CommandDescription initialCommand;
	CommandItemFilter* searchFilter;
	SharedPtr<KnownCommand> focusCommand;
	DialogBuilder* currentDialog;
	SharedPtr<AsyncOperation> asyncOperation;

	bool runDialog (StringID formName);
	IAsyncOperation* runAsync (StringID formName, bool popupMode);
	bool hasInitialCommand () const;
	void selectCommand (CommandDescription& description);
	void invalidateCommandItem (KnownCommand& command);
	ITreeItem* findCommandTreeItem (KnownCommand& command);
	ITreeItem* findCommandTreeItem (CommandDescription& description);

	void onDialogCompleted (IAsyncOperation&);

	// IObjectNode
	StringRef CCL_API getObjectID () const override;

	// IObject
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

} // namespace CCL

#endif // _ccl_commandselector_h
