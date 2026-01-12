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
// Filename    : ccl/gui/dialogs/commandselector.cpp
// Description : Command Selector
//
//************************************************************************************************

#include "ccl/gui/dialogs/commandselector.h"
#include "ccl/gui/dialogs/dialogbuilder.h"
#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/popup/popupselector.h"
#include "ccl/gui/help/keyglyphpainter.h"
#include "ccl/gui/itemviews/treeview.h"

#include "ccl/base/message.h"
#include "ccl/base/collections/stringlist.h"

#include "ccl/public/base/irecognizer.h"
#include "ccl/public/gui/framework/popupselectorclient.h"
#include "ccl/public/gui/iparameter.h"

using namespace CCL;

namespace CCL {

//************************************************************************************************
// CommandItemFilter
//************************************************************************************************

class CommandItemFilter: public Object,
						 public IObjectFilter
{
public:
	StringList excludedCategories;
	PROPERTY_STRING (searchString, SearchString)

	// IObjectFilter
	tbool CCL_API matches (IUnknown* object) const override
	{	
		// filter categories
		if(!excludedCategories.isEmpty ())
			if(CommandItem* commandItem = unknown_cast<CommandItem> (object))
				if(commandItem->getType () == CommandItem::kCategory)
					if(excludedCategories.contains (commandItem->getTitle ()))
						return false;
				
		// filter search string
		if(!searchString.isEmpty ())
			if(CommandItem* commandItem = unknown_cast<CommandItem> (object))
			{
				switch(commandItem->getType ())
				{
				case CommandItem::kCategory:
					// try all commands in this category
					if(CommandCategory* category = commandItem->getCategory ())
					{
						ForEach (*category, KnownCommand, command)
							if(command->getDisplayName ().contains (searchString, false))
								return true;
						EndFor
					}
					// try category title
					return commandItem->getTitle ().contains (searchString, false);

				case CommandItem::kCommand: // try name & category
					return commandItem->getTitle ().contains (searchString, false)
						|| commandItem->getCommand ()->getDisplayCategory ().contains (searchString, false);                    
				}
			}

		return true;
	}

	CLASS_INTERFACE (IObjectFilter, Object)
};

//////////////////////////////////////////////////////////////////////////////////////////////////

struct CommandItemRecognizer: public Recognizer
{
	CommandItemRecognizer (const CommandDescription& description) : description (description) {}

	tbool CCL_API recognize (IUnknown* object) const override
	{
		if(CommandItem* commandItem = unknown_cast<CommandItem> (object))
			if(KnownCommand* command = commandItem->getCommand ())
				return description.name == command->getName () && description.category == command->getCategory ();
		return false;
	}

private:
	const CommandDescription& description;
};

} // namespace CCL

//////////////////////////////////////////////////////////////////////////////////////////////////
// Tags
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum CommandSelectorTags
	{
		kSearchString = 100,
		kClearSearch
	};
}

//************************************************************************************************
// CommandItem
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (CommandItem, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

CommandItem::CommandItem (Type type, StringRef title)
: type (type),
  title (title)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

CommandItem::CommandItem (CommandCategory& category)
: type (kCategory),
  title (category.getDisplayCategory ())
{
	setCategory (&category);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CommandItem::CommandItem (KnownCommand& command)
: type (kCommand),
  title (command.getDisplayName ())
{
	setCommand (&command);
}

//************************************************************************************************
// CommandSelector
//************************************************************************************************

DEFINE_CLASS (CommandSelector, Object)
DEFINE_CLASS_UID (CommandSelector, 0xebd102b8, 0xb508, 0x4153, 0x81, 0x22, 0x18, 0x20, 0x6f, 0x75, 0x4f, 0xd7)

//////////////////////////////////////////////////////////////////////////////////////////////////

CommandSelector::CommandSelector ()
: commandTree (NEW CommandTreeModel (*CommandTable::instance ().createCategories ())),
  searchFilter (NEW CommandItemFilter),
  currentDialog (nullptr)
{
	commandTree->setTarget (this);

	paramList.setController (this);
	paramList.addString (CSTR ("searchString"), Tag::kSearchString);
	paramList.addParam (CSTR ("clear"), Tag::kClearSearch);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CommandSelector::~CommandSelector ()
{
	cancelSignals ();
	commandTree->release ();
	searchFilter->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CommandSelector::queryInterface (UIDRef iid, void** ptr)
{
	QUERY_INTERFACE (ICommandSelector)
	QUERY_INTERFACE (IController)
	QUERY_INTERFACE (IParamObserver)
	QUERY_INTERFACE (IObjectNode)
	return SuperClass::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CommandSelector::setCommands (ICommandContainer* commands)
{
	CommandContainer* container = unknown_cast<CommandContainer> (commands);
	ASSERT (container)
	if(!container)
		return kResultInvalidArgument;

	safe_release (commandTree);

	container->retain ();
	commandTree = NEW CommandTreeModel (*container);
	commandTree->setTarget (this);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API CommandSelector::getObjectID () const
{
	return name;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_PROPERTY_NAMES (CommandSelector)
	DEFINE_PROPERTY_NAME ("name")
	DEFINE_PROPERTY_NAME ("argColumnEnabled")
	DEFINE_PROPERTY_NAME ("focusCommand")
END_PROPERTY_NAMES (CommandSelector)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CommandSelector::setProperty (MemberID propertyId, const Variant& var)
{
	if(propertyId == "name")
	{
		name = var.asString ();
		return true;
	}
	else if(propertyId == "argColumnEnabled")
	{
		commandTree->setArgColumnEnabled (var.asBool ());
		return true;
	}
	return SuperClass::setProperty (propertyId, var);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CommandSelector::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "focusCommand")
	{
		if(focusCommand)
			var.takeShared (focusCommand->asUnknown ());
		else
			var.clear ();
		return true;
	}
	else if(propertyId == "showPlaceholderLabel")
	{
		String title;
		paramList.byTag (Tag::kSearchString)->toString (title);
		var = title == String::kEmpty;
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (CommandSelector)
	DEFINE_METHOD_ARGS ("addExcludedCategory", "displayName")
END_METHOD_NAMES (CommandSelector)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CommandSelector::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "addExcludedCategory")
	{
		searchFilter->excludedCategories.addOnce (msg[0].asString ());
		return true;
	}
	return SuperClass::invokeMethod (returnValue, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API CommandSelector::getObject (StringID name, UIDRef classID)
{
	if(name == "commandTree")
		return ccl_as_unknown (commandTree);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CommandSelector::hasInitialCommand () const
{
	return initialCommand.isValid ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CommandSelector::runDialog (StringID formName)
{
	int result = DialogResult::kCancel;
	Theme& theme = FrameworkTheme::instance ();
	View* view = unknown_cast<View> (theme.createView (formName, this->asUnknown ()));
	ASSERT (view != nullptr)
	if(view)
	{
		DialogBuilder builder;
		builder.setTheme (theme);
		ScopedVar<DialogBuilder*> scope (currentDialog, &builder);
		result = builder.runDialog (view);
	}
	return result == DialogResult::kOkay;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* CommandSelector::runAsync (StringID formName, bool popupMode)
{
	Theme& theme = FrameworkTheme::instance ();
	View* view = unknown_cast<View> (theme.createView (formName, this->asUnknown ()));
	ASSERT (view != nullptr)
	if(view)
	{
		if(popupMode)
		{
			AutoPtr<PopupSelector> popupSelector = NEW PopupSelector;
			popupSelector->setTheme (&theme);

			UnknownPtr<IView> parentView = Desktop.getApplicationWindow ();
			if(parentView == nullptr)
				parentView = Desktop.getDialogParentWindow ();

			PopupSizeInfo sizeInfo (parentView, PopupSizeInfo::kHCenter|PopupSizeInfo::kVCenter);
			AutoPtr<SimplePopupSelectorClient> popupClient = NEW SimplePopupSelectorClient;
			popupClient->setPopupResult (true);
			popupClient->acceptOnDoubleClick (true);

			return popupSelector->popupAsync (view, popupClient, sizeInfo);
		}
		else
		{
			DialogBuilder* builder = NEW DialogBuilder;
			builder->setTheme (theme);
			currentDialog = builder;
			return builder->runDialogAsync (view);
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CommandSelector::run (CommandDescription& command)
{
	initialCommand = command;

	if(runDialog ("CommandSelectorDialog") && focusCommand)
	{
		focusCommand->getDescription (command);
		return kResultOk;
	}

	return kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* CCL_API CommandSelector::runAsync (const CommandDescription& command, tbool popupMode)
{
	initialCommand = command;

	retain (); // stay alive while dialog is open, so that onDialogCompleted can be called safely

	ASSERT (!asyncOperation)
	asyncOperation = NEW AsyncOperation;
	asyncOperation->setState (AsyncOperation::kStarted);

	Promise dialogPromise (runAsync ("CommandSelectorDialog", popupMode != 0));
	dialogPromise.then (this, &CommandSelector::onDialogCompleted);

	return asyncOperation;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandSelector::onDialogCompleted (IAsyncOperation& dialogOperation)
{
	ASSERT (asyncOperation)
	if(asyncOperation)
	{
		asyncOperation->setResult (dialogOperation.getResult ().asInt () == DialogResult::kOkay ? kResultOk : kResultFailed);
		asyncOperation->setState (AsyncOperation::kCompleted);
	}

	safe_release (currentDialog);

	release (); // release refCount from runAsync
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandSelector::selectCommand (CommandDescription& description)
{
	if(ITreeItem* item = findCommandTreeItem (description))
	{
		// reset filter if it hides the command
		if(!searchFilter->matches (item->getData ()))
			paramList.byTag (Tag::kSearchString)->fromString (String::kEmpty, true);

		commandTree->getItemView ()->setFocusItem (item);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CommandSelector::getSelectedCommand (CommandDescription& command) const
{
	 if(focusCommand)
	{
		focusCommand->getDescription (command);
		return kResultOk;
	 }
	 return kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandSelector::invalidateCommandItem (KnownCommand& command)
{
	if(ITreeItem* item = findCommandTreeItem (command))
		commandTree->getItemView ()->invalidateItem (item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITreeItem* CommandSelector::findCommandTreeItem (KnownCommand& command)
{
	CommandDescription description (command.getCategory (), command.getName ());
	return findCommandTreeItem (description);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITreeItem* CommandSelector::findCommandTreeItem (CommandDescription& description)
{
	IItemView* itemView = commandTree->getItemView ();
	UnknownPtr<ITreeView> treeView (itemView);
	if(treeView)
		if(ITreeItem* tree = treeView->getRootItem ())
		{
			CommandItemRecognizer recognizer (description);
			return tree->findItem (&recognizer, false);
		}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CommandSelector::paramChanged (IParameter* param)
{
	if(param->getTag () == Tag::kSearchString)
	{
		String string;
		param->toString (string);
		searchFilter->setSearchString (string);
		searchFilter->signal (Message (kChanged));
		signal (Message (kPropertyChanged, String ("showPlaceholderLabel")));
	}
	else if(param->getTag () == Tag::kClearSearch)
	{
		paramList.byTag (Tag::kSearchString)->setValue (String::kEmpty);
		searchFilter->setSearchString (String::kEmpty);
		searchFilter->signal (Message (kChanged));
		signal (Message (kPropertyChanged, String ("showPlaceholderLabel")));
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandSelector::onViewAttached (IItemView* itemView)
{
	TreeView* treeView = unknown_cast<TreeView> (itemView);
	if(treeView)
	{
		treeView->setItemFilter (searchFilter);

		// expand all for list-like appearance by default; can be disabled in VisualStyle of TreeView
		treeView->expandItem (treeView->getRootItem (), true, TreeView::kExpandChilds);
		if(!treeView->getVisualStyle ().getMetric ("expandCategories", true))
			treeView->expandItem (treeView->getRootItem (), false, TreeView::kExpandChilds);

		(NEW Message ("selectInitialCommand"))->post (this);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CommandSelector::onCommandItemFocused (CommandItem& item)
{
	setFocusCommand (item.getCommand ());
	signal (Message (kCommandFocused, ccl_as_unknown (item.getCommand ())));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CommandSelector::onCommandItemOpened (CommandItem& item)
{
	if(currentDialog) // currently in run() => accept command & close dialog
	{
		setFocusCommand (item.getCommand ());
		currentDialog->setDialogResult (DialogResult::kOkay);
		currentDialog->close ();
	}
	else
	{
		signal (Message (kCommandSelected, ccl_as_unknown (item.getCommand ())));
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CommandSelector::onEditKeyColumn (CommandItem& item, const IItemModel::EditInfo& info)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandSelector::setFocusCommand (KnownCommand* command)
{
	focusCommand = command;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CommandSelector::notify (ISubject* subject, MessageRef msg)
{
	if(msg == "selectInitialCommand")
		if(hasInitialCommand ())
			selectCommand (initialCommand);	// select initial command in tree
}

//************************************************************************************************
// CommandTreeModel
//************************************************************************************************

CommandTreeModel::CommandTreeModel (CommandContainer& commands)
: rootItem (*NEW CommandItem (CommandItem::kRoot)),
  categories (commands),
  keyColumnEnabled (false),
  argColumnEnabled (false),
  target (nullptr) 
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

CommandTreeModel::~CommandTreeModel ()
{
	rootItem.release ();
	categories.release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CommandItem* CommandTreeModel::resolve (ItemIndexRef index) const
{
	return unknown_cast<CommandItem> (index.getObject ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CommandTreeModel::createColumnHeaders (IColumnHeaderList& list)
{
	enabledColumns.removeAll ();

	// kCommand
	list.addColumn (200);
	enabledColumns.add (kCommand);

	// kKey
	if(isKeyColumnEnabled ())
	{
		list.addColumn (120);  
		enabledColumns.add (kKey);
	}

	// kArguments
	if(isArgColumnEnabled ())
	{
		list.addColumn (200);
		enabledColumns.add (kArguments);
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CommandTreeModel::getRootItem (ItemIndex& index)
{
	index = ItemIndex (rootItem.asUnknown ());
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CommandTreeModel::isItemFolder (ItemIndexRef index)
{
	if(CommandItem* item = resolve (index))
		return item->getType () == CommandItem::kRoot || item->getType () == CommandItem::kCategory;
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CommandTreeModel::canExpandItem (ItemIndexRef index)
{
	return isItemFolder (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CommandTreeModel::getSubItems (IUnknownList& items, ItemIndexRef index)
{
	CommandItem* item = resolve (index);
	if(!item)
		return false;

	if(item->getType () == CommandItem::kRoot)
	{
		ForEach (categories, CommandCategory, category)
			items.add (ccl_as_unknown (NEW CommandItem (*category)));
		EndFor
	}
	else if(item->getType () == CommandItem::kCategory)
	{
		CommandCategory* category = item->getCategory ();
		ASSERT (category)
		ForEach (*category, KnownCommand, command)
			items.add (ccl_as_unknown (NEW CommandItem (*command)));
		EndFor
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CommandTreeModel::getItemTitle (String& title, ItemIndexRef index)
{
	if(CommandItem* item = resolve (index))
	{
		title = item->getTitle ();
		return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CommandTreeModel::drawCell (ItemIndexRef index, int column, const DrawInfo& info)
{
	CommandItem* item = resolve (index);
	if(!item)
		return false;

	ColumnID columnID = enabledColumns.at (column);
	switch(columnID)
	{
	case kCommand:
		{
			Font font (info.style.font);
			if(item->getType () == CommandItem::kCategory)
				font.isBold (true);

			info.graphics.drawString (info.rect, item->getTitle (), font, info.style.textBrush, Alignment::kLeft|Alignment::kVCenter);
		}
		break;

	case kKey:
		if(KnownCommand* command = item->getCommand ())
		{
			if(KeyEvent* key = command->getDefaultKey ())
				KeyGlyphPainter (info.style.font, info.style.textBrush).drawKeyGlyphs (info.graphics, info.rect, *key, Alignment::kLeft|Alignment::kVCenter);
		}
		break;

	case kArguments :
		if(KnownCommand* command = item->getCommand ())
		{
			if(!command->getArguments ().isEmpty ())
				info.graphics.drawString (info.rect, String (command->getArguments ()), info.style.font, info.style.textBrush, Alignment::kLeft|Alignment::kVCenter);
	}
		break;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CommandTreeModel::editCell (ItemIndexRef index, int column, const EditInfo& info)
{ 
	if(column == kKey && target)
		if(CommandItem* item = resolve (index))
			if(item->getCommand ())
				return target->onEditKeyColumn (*item, info);

	return false; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CommandTreeModel::onItemFocused (ItemIndexRef index) 
{
	if(target)
		if(CommandItem* item = resolve (index))
			return target->onCommandItemFocused (*item);

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CommandTreeModel::openItem (ItemIndexRef index, int column, const EditInfo& info)
{
	if(target)
		if(CommandItem* item = resolve (index))
			return target->onCommandItemOpened (*item);

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CommandTreeModel::viewAttached (IItemView* itemView)
{
	ItemViewObserver<AbstractItemModel>::viewAttached (itemView);

	target->onViewAttached (itemView);
}
