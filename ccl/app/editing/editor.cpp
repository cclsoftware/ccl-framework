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
// Filename    : ccl/app/editing/editor.cpp
// Description : Editor Component
//
//************************************************************************************************

#include "ccl/app/editing/editor.h"
#include "ccl/app/editing/editview.h"
#include "ccl/app/editing/editmodel.h"
#include "ccl/app/editing/editextension.h"

#include "ccl/app/editing/tools/edittool.h"
#include "ccl/app/editing/tools/toolcollection.h"
#include "ccl/app/editing/tools/toolbar.h"

#include "ccl/base/storage/storage.h"
#include "ccl/base/signalsource.h"
#include "ccl/base/boxedtypes.h"
#include "ccl/base/message.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/app/signals.h"
#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/framework/iclipboard.h"
#include "ccl/public/gui/framework/imenu.h"
#include "ccl/public/guiservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("Edit")
	XSTRING (Cut,			"Cut")
	XSTRING (Copy,			"Copy")
	XSTRING (Paste,			"Paste")
	XSTRING (Delete,		"Delete")
	XSTRING (SelectAll,		"Select All")
	XSTRING (DeselectAll,	"Deselect All")
	XSTRING (Undo,			"Undo")
	XSTRING (Redo,			"Redo")
END_XSTRINGS

//////////////////////////////////////////////////////////////////////////////////////////////////
// Commands
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_COMMANDS (EditorComponent)
	DEFINE_COMMAND ("Edit",		"Cut",				EditorComponent::onEditCut)
	DEFINE_COMMAND ("Edit",		"Copy",				EditorComponent::onEditCopy)
	DEFINE_COMMAND ("Edit",		"Paste",			EditorComponent::onEditPaste)
	DEFINE_COMMAND ("Edit",		"Delete",			EditorComponent::onEditDelete)
	DEFINE_COMMAND ("Edit",		"Select All",		EditorComponent::onSelectAll)
	DEFINE_COMMAND ("Edit",		"Deselect All",		EditorComponent::onDeselect)
	DEFINE_COMMAND ("Edit",		"Invert Selection",	EditorComponent::onInvertSelection)
	DEFINE_COMMAND ("Navigation",	nullptr,				EditorComponent::onNavigation)
END_COMMANDS (EditorComponent)

//************************************************************************************************
// EditorRegistry
//************************************************************************************************

DEFINE_SINGLETON (EditorRegistry)

//////////////////////////////////////////////////////////////////////////////////////////////////

EditorRegistry::EditorRegistry ()
: activeEditor (nullptr),
  activeEditorLocked (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditorRegistry::~EditorRegistry ()
{
	ASSERT (activeEditor == nullptr)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditorRegistry::addEditor (EditorComponent* editor)
{
	editors.add (editor);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditorRegistry::removeEditor (EditorComponent* editor)
{
	ASSERT (editor != activeEditor)
	if(editor == activeEditor)
		setActiveEditor (nullptr);

	bool removed = editors.remove (editor);
	ASSERT (removed)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditorComponent* EditorRegistry::getActiveEditor () const
{
	return activeEditor;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditorRegistry::setActiveEditor (EditorComponent* editor)
{
	if(activeEditor && activeEditorLocked)
	{
		if(editor == nullptr)
			activeEditorLocked = false; // break lock, allow resetting e.g. from removeEditor, EditorComponent::removeEditView
		else
			return;
	}

	if(editor != activeEditor)
	{	
		// when active editor gets removed, try to find another one with an active editview
		if(editor == nullptr)
			ForEach (editors, EditorComponent, otherEditor)
				if(otherEditor != activeEditor && otherEditor->getActiveEditView ())
				{
					editor = otherEditor;
					break;
				}
			EndFor

		activeEditor = editor;
		CCL_PRINTF ("setActiveEditor: %s\n", MutableCString (activeEditor ? activeEditor->getName () : 0).str ())

		SignalSource (Signals::kEditorRegistry).signal (Message (Signals::kEditorActivated, static_cast<IObject*> (editor)));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Iterator* EditorRegistry::getEditors () const
{
	return editors.newIterator ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditorComponent* EditorRegistry::findEditor (MetaClassRef type, bool needsViews) const
{
	ForEach (editors, EditorComponent, editor)
		if(!needsViews || editor->hasEditViews ())
			if(editor->myClass () == type)
				return editor;
	EndFor
	return nullptr;
}

//************************************************************************************************
// EditorComponent::DocumentDirtyGuard
//************************************************************************************************

EditorComponent::DocumentDirtyGuard::DocumentDirtyGuard (IParameter* parameter)
: parameter (nullptr)
{
	init (parameter);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditorComponent::DocumentDirtyGuard::DocumentDirtyGuard (IParameter* parameter, bool checkOnly)
: parameter (nullptr)
{
	if(!checkOnly)
		init (parameter);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditorComponent::DocumentDirtyGuard::init (IParameter* param)
{
	ASSERT (param)
	parameter = param;
	oldValue = param->getValue ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditorComponent::DocumentDirtyGuard::~DocumentDirtyGuard ()
{
	if(parameter && parameter->getValue () != oldValue)
		SignalSource (Signals::kDocumentManager).signal (Message (Signals::kDocumentDirty));
}

//************************************************************************************************
// EditorComponent::NavigationCommand
//************************************************************************************************

class EditorComponent::NavigationCommand: public Object
{
public:
	NavigationCommand (StringID name, int direction, int mode)
	: name (name), direction (direction), mode (mode) {}

	PROPERTY_MUTABLE_CSTRING (name, CommandName)
	PROPERTY_VARIABLE (int, direction, Direction)
	PROPERTY_VARIABLE (int, mode, Mode)
};

//************************************************************************************************
// EditorComponent
//************************************************************************************************

CCL::Configuration::BoolValue EditorComponent::contextMenuSelectAll	("Editing", "contextMenu.selectAll", true);
CCL::Configuration::BoolValue EditorComponent::contextMenuUndo		("Editing", "contextMenu.undo", true);

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (EditorComponent, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

EditorComponent::EditorComponent (StringRef name, StringRef title)
: Component (name, title),
  activeEditView (nullptr),
  model (nullptr),
  editEnvironment (nullptr),
  configuration (0)
{
	toolList = NEW ToolCollection;
	toolList->addObserver (this);

	EditorRegistry::instance ().addEditor (this);

	navigationCommands.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditorComponent::~EditorComponent ()
{
	EditorRegistry::instance ().removeEditor (this);

	if(model)
		model->release ();

	toolList->removeObserver (this);
	toolList->setToolBar (nullptr);
	toolList->release ();

	ASSERT (editViews.isEmpty () == true)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditModel& EditorComponent::getModel ()
{
	if(model == nullptr)
	{
		model = createModel ();
		EditExtensionRegistry::instance ().extendModel (*model, *this);
	}
	return *model;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditModel* EditorComponent::createModel ()
{
	return NEW EditModel (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Iterator* EditorComponent::getEditViews () const
{
	return editViews.newIterator ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditorComponent::showSelection (bool redraw)
{
	ListForEachObject (editViews, EditView, view)
		static_cast<ISelectionViewer*> (view)->showSelection (redraw);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditorComponent::hideSelection (bool redraw)
{
	ListForEachObject (editViews, EditView, view)
		static_cast<ISelectionViewer*> (view)->hideSelection (redraw);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditorComponent::makeSelectedItemsVisible (bool relaxed) 
{
	ListForEachObject (editViews, EditView, view)
		static_cast<ISelectionViewer*> (view)->makeSelectedItemsVisible (relaxed);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditorComponent::setToolBar (ToolBar* toolBar)
{
	toolList->setToolBar (toolBar);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ToolBar* EditorComponent::getToolBar () const
{
	return toolList->getToolBar ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ToolCollection& EditorComponent::getTools ()
{
	ASSERT (toolList != nullptr)
	return *toolList;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditTool* EditorComponent::getDefaultTool () const
{
	return toolList ? toolList->getTool (0) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditTool* EditorComponent::getActiveTool () const
{
	return toolList ? toolList->getActiveTool () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditorComponent::activateTool (EditTool* tool)
{
	if(tool && getActiveTool () != tool)
	{
		getTools ().setActiveTool (tool);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditorComponent::hasEditView (EditView* editView) const
{
	return editViews.contains (editView);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditView* EditorComponent::findEditView (MetaClassRef viewClass) const
{
	for(auto view : iterate_as<EditView> (editViews))
		if(view->canCast (viewClass))
			return view;

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditorComponent::addEditView (EditView* editView)
{
	ASSERT (editViews.contains (editView) == false)
	editViews.add (editView);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditorComponent::removeEditView (EditView* editView)
{
	ASSERT (editViews.contains (editView) == true)
	editViews.remove (editView);

	if(activeEditView == editView
		|| (activeEditView == nullptr && editView->wantsToolActivation ())) // see getActiveEditView ()
	{
		activeEditView = nullptr;

		// give up activeEditorShip if we don't have another edit view candidate
		if(!getActiveEditView ()) 
			if(EditorRegistry::instance ().getActiveEditor () == this)
				EditorRegistry::instance ().setActiveEditor (nullptr);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditorComponent::setActiveEditView (EditView* editView, bool onAttach)
{
	if(editView->wantsToolActivation () == false)
		return;

	CCL_PRINTF ("setActiveEditView: %s\n", MutableCString (editView->myClass ().getPersistentName ()).append (" ").append (editView->getName ()).str ())

	ASSERT (editViews.contains (editView) == true)
	activeEditView = editView;
	onActiveEditViewChanged (editView);

	if(onAttach) // suppress on attach if another editor active via view focus
		if(EditorRegistry::instance ().getActiveEditor ())
		{
			CCL_PRINTF ("  keep active editor: %s\n", MutableCString (EditorRegistry::instance ().getActiveEditor ()->getName ()).str ())
			return;
		}

	EditorRegistry::instance ().setActiveEditor (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditView* EditorComponent::getActiveEditView () const
{
	if(activeEditView)
		return activeEditView;
	
	ListForEachObject (editViews, EditView, editView)
		if(editView->wantsToolActivation ())
			return editView;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditorComponent::onActiveEditViewChanged (EditView* editView)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API EditorComponent::checkCommandCategory (CStringRef category) const
{
	if(category == "Edit" || category == "Navigation") 
		return true; 
	
	if(model)
	{
		for(Object* layer : model->getEditLayers ())
		{
			UnknownPtr<ICommandHandler> handler (layer->asUnknown ());
			if(handler && handler->checkCommandCategory (category))
				return true;
		}
	}

	return SuperClass::checkCommandCategory (category); 
}


//////////////////////////////////////////////////////////////////////////////////////////////////

CCL::tbool CCL_API EditorComponent::interpretCommand (const CCL::CommandMsg& msg)
{
	if(CCL::CommandDispatcher<EditorComponent>::dispatchCommand (msg))
		return true; 

	if(model)
	{
		for(Object* layer : model->getEditLayers ())
		{
			UnknownPtr<ICommandHandler> handler (layer->asUnknown ());
			if(handler && handler->interpretCommand (msg))
				return true;
		}
	}

	return SuperClass::interpretCommand (msg); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditorComponent::onEditCut (CmdArgs args)
{
	if(!getActiveEditView ())
		return false;

	bool canPerform = getModel ().canDeleteSelected ();
	if(args.checkOnly ())
		return canPerform;

	if(canPerform)
	{
		Object* objCopy = getModel ().copySelected (false);
		if(objCopy)
			System::GetClipboard ().setContent (objCopy->asUnknown ());

		getModel ().getSelection ().hide (false);
		getModel ().deleteSelected ();
		getModel ().getSelection ().unselectAll ();
		getModel ().getSelection ().show (true);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditorComponent::onEditCopy (CmdArgs args)
{
	if(!getActiveEditView ())
		return false;

	bool canPerform = !getModel ().getSelection ().isEmpty ();
	if(args.checkOnly ())
		return canPerform;

	if(canPerform)
	{
		Object* objCopy = getModel ().copySelected (false);
		if(objCopy)
			System::GetClipboard ().setContent (objCopy->asUnknown ());
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditorComponent::onEditPaste (CmdArgs args)
{
	if(!getActiveEditView ())
		return false;

	IUnknown* clipboardContent = System::GetClipboard ().getContent ();

	AutoPtr<Object> content;
	content.share (unknown_cast<Object> (clipboardContent)); // only works for objects created in this module

	if(!content && clipboardContent)
	{
		// 2nd try: clipboard could contain text
		String text;
		System::GetClipboard ().getText (text);
		if(!text.isEmpty ())
			content = NEW Boxed::String (text);
	}

	bool canPerform = content && getModel ().canInsertData (content);
	if(args.checkOnly ())
		return canPerform;

	if(canPerform)
	{
		AutoPtr<Object> contentCopy = content->clone ();
		if(contentCopy)
			getModel ().insertData (contentCopy);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditorComponent::onEditDelete (CmdArgs args)
{
	if(!getActiveEditView ())
		return false;

	bool canPerform = getModel ().canDeleteSelected ();
	if(args.checkOnly ())
		return canPerform;

	if(canPerform)
	{
		Selection::Hideout hideout (getModel ().getSelection (), false);

		AutoPtr<Object> nextItem (getModel ().findItemAfterSelection ());

		bool success = getModel ().deleteSelected ();
		getModel ().getSelection ().unselectAll ();

		if(nextItem && success)
		{
			getModel ().selectItem (nextItem);
			getModel ().setFocusItem (nextItem, getActiveEditView ());
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditorComponent::onSelectAll (CmdArgs args)
{
	if(!getActiveEditView ())
		return false;

	bool canPerform = getModel ().containsAnyItems ();
	if(args.checkOnly ())
		return canPerform;

	if(canPerform)
	{
		getModel ().getSelection ().hide (false);
		getModel ().getSelection ().unselectAll (); // just in case...
		getModel ().selectAll ();
		getModel ().getSelection ().show (true);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditorComponent::onDeselect (CmdArgs args)
{
	if(!getActiveEditView ())
		return false;

	bool canPerform = !getModel ().getSelection ().isEmpty ();
	if(args.checkOnly ())
		return canPerform;

	if(canPerform)
	{
		getModel ().getSelection ().hide (false);
		getModel ().getSelection ().unselectAll ();
		getModel ().getSelection ().show (true);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditorComponent::onInvertSelection (CmdArgs args)
{
	if(!getActiveEditView ())
		return false;

	bool canPerform = getModel ().containsAnyItems ();
	if(args.checkOnly ())
		return canPerform;

	if(canPerform)
	{
		getModel ().getSelection ().hide (false);
		getModel ().invertSelection ();
		getModel ().getSelection ().show (true);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

#define INTERPRET_NAVIGATION_CMDS(dir,dirName)\
	if(args.name == dirName)				return getModel ().navigate (dir, EditModel::kNavigate);\
	if(args.name == dirName" Extend")		return getModel ().navigate (dir, EditModel::kNavigateExtend);\
	if(args.name == dirName" Extend Add")	return getModel ().navigate (dir, EditModel::kNavigateExtendAdd);\
	if(args.name == dirName" Skip")			return getModel ().navigate (dir, EditModel::kSkip);

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditorComponent::registerNavigationCommand (StringID commandName, int direction, int mode)
{
	navigationCommands.add (NEW NavigationCommand (commandName, direction, mode));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditorComponent::onNavigation (CmdArgs args)
{
	EditView* editView = getActiveEditView ();
	if(editView)
	{
		if(args.checkOnly ())
			return true;
		else
		{
			INTERPRET_NAVIGATION_CMDS (EditModel::kDirectionLeft,	"Left")
			INTERPRET_NAVIGATION_CMDS (EditModel::kDirectionRight,	"Right")
			INTERPRET_NAVIGATION_CMDS (EditModel::kDirectionUp,		"Up")
			INTERPRET_NAVIGATION_CMDS (EditModel::kDirectionDown,	"Down")
			INTERPRET_NAVIGATION_CMDS (EditModel::kDirectionStart,	"Start")
			INTERPRET_NAVIGATION_CMDS (EditModel::kDirectionEnd,	"End")
			INTERPRET_NAVIGATION_CMDS (EditModel::kDirectionPageUp,	  "Page Up")
			INTERPRET_NAVIGATION_CMDS (EditModel::kDirectionPageDown, "Page Down")
			#undef INTERPRET_NAVIGATION_CMDS

			ForEach (navigationCommands, NavigationCommand, nc)
				if(args.name == nc->getCommandName ())
					return getModel ().navigate ((EditModel::Direction)nc->getDirection (), (EditModel::NavigationMode)nc->getMode ());
			EndFor
		}
	}
	return false;
}


//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API EditorComponent::appendContextMenu (IContextMenu& contextMenu)
{
	ContextMenuBuilder (*this, contextMenu).build ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API EditorComponent::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kChanged && subject == toolList)
	{
		ListForEachObject (editViews, EditView, editView)
			if(editView->wantsToolActivation ())
				editView->setTool (getTools ().getActiveTool ());
		EndFor
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API EditorComponent::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "isActiveEditor")
	{
		var = EditorRegistry::instance ().getActiveEditor () == this;
		return true;
	}
	else if(propertyId == "activeEditView")
	{
		var = ccl_as_unknown (getActiveEditView ());
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditorComponent::load (const Storage& storage)
{
	// load toolbar, if it's our child
	if(ToolBar* toolBar = getToolBar ())
		if(toolBar->getParent () == this)
			loadChild (storage, *toolBar);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditorComponent::save (const Storage& storage) const
{
	// save toolbar, if it's our child
	if(ToolBar* toolBar = getToolBar ())
		if(toolBar->getParent () == this)
			saveChild (storage, *toolBar);
	return true;
}


//************************************************************************************************
// EditorComponent::ContextMenuBuilder
//************************************************************************************************

EditorComponent::ContextMenuBuilder::ContextMenuBuilder (EditorComponent& component, IContextMenu& contextMenu)
: component (component),
  contextMenu (contextMenu)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditorComponent::ContextMenuBuilder::build ()
{
	auto focusItem = unknown_cast<Object> (contextMenu.getFocusItem ());
	if(focusItem)
	{
		// item-oriented commands
		if(component.getModel ().getSelection ().canSelect (focusItem))
		{
			appendEditCommands (component.canPasteOnItem ());
		}
	}
	else
	{
		// no item: global commands
		appendToolCommands ();
		appendEditCommands (true);

		if(contextMenuSelectAll)
		{
			contextMenu.addSeparatorItem ();
			appendSelectionCommands ();
		}

		if(contextMenuUndo)
		{
			contextMenu.addSeparatorItem ();
			appendUndoCommands ();
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditorComponent::ContextMenuBuilder::appendToolCommands ()
{
	if(ToolBar* toolBar = component.getToolBar ())
		toolBar->appendContextMenu (contextMenu);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditorComponent::ContextMenuBuilder::appendEditCommands (bool includingPaste)
{
	appendEditCut ();
	appendEditCopy ();
	if(includingPaste)
		appendEditPaste ();
	appendEditDelete ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditorComponent::ContextMenuBuilder::appendEditCut ()
{
	contextMenu.addCommandItem (XSTR (Cut), CSTR ("Edit"), CSTR ("Cut"), &component);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditorComponent::ContextMenuBuilder::appendEditCopy ()
{
	contextMenu.addCommandItem (XSTR (Copy), CSTR ("Edit"), CSTR ("Copy"), &component);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditorComponent::ContextMenuBuilder::appendEditPaste ()
{
	contextMenu.addCommandItem (XSTR (Paste), CSTR ("Edit"), CSTR ("Paste"), &component);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditorComponent::ContextMenuBuilder::appendEditDelete ()
{
	contextMenu.addCommandItem (XSTR (Delete), CSTR ("Edit"), CSTR ("Delete"), &component);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditorComponent::ContextMenuBuilder::appendSelectionCommands ()
{
	contextMenu.addCommandItem (XSTR (SelectAll), CSTR ("Edit"), CSTR ("Select All"), &component);
	contextMenu.addCommandItem (XSTR (DeselectAll), CSTR ("Edit"), CSTR ("Deselect All"), &component);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditorComponent::ContextMenuBuilder::appendUndoCommands ()
{
	// Note: Undo/Redo are handled globally!
	contextMenu.addCommandItem (XSTR (Undo), CSTR ("Edit"), CSTR ("Undo"), nullptr);
	contextMenu.addCommandItem (XSTR (Redo), CSTR ("Edit"), CSTR ("Redo"), nullptr);
}

