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
// Filename    : ccl/app/editing/tools/selecttool.cpp
// Description : Selection Tool
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/app/editing/tools/selecttool.h"

#include "ccl/app/editing/editview.h"
#include "ccl/app/editing/editmodel.h"
#include "ccl/app/editing/edithandler.h"

#include "ccl/public/gui/framework/iwindow.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/ihelpmanager.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/plugservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("Tools")
	XSTRING (SelectTool, "Select Tool")
	XSTRING (EraserTool, "Eraser Tool")
END_XSTRINGS

BEGIN_XSTRINGS ("ToolHelp")
	XSTRING (Drag, "Drag")
	XSTRING (Select, "Select")
	XSTRING (ToggleSelect, "Toggle Select")
	XSTRING (SelectRange, "Select Range")
	XSTRING (DrawSelection, "Draw Selection")
	XSTRING (DrawSelectionAdd, "Draw Selection (add)")
	XSTRING (DeleteObjects, "Delete Objects")
	XSTRING (Zoom, "Zoom")
END_XSTRINGS

//************************************************************************************************
// ToolStrings
//************************************************************************************************

StringRef ToolStrings::Select ()		{ return XSTR (Select); }
StringRef ToolStrings::ToggleSelect ()	{ return XSTR (ToggleSelect); }
StringRef ToolStrings::SelectRange ()	{ return XSTR (SelectRange); }
StringRef ToolStrings::Zoom ()			{ return XSTR (Zoom); }

//************************************************************************************************
// ToolActions::ToggleSelectAction
//************************************************************************************************

ToolActions::ToggleSelectAction::ToggleSelectAction (bool exclusive)
: exclusive (exclusive)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditHandler* ToolActions::ToggleSelectAction::perform (EditView& editView, const GUIEvent& event, PointRef where)
{
	if(item)
	{
		editView.getModel ().setFocusItem (item, &editView);

		// toggle selection state
		if(editView.getSelection ().isSelected (item))
			editView.getModel ().unselectItem (item);
		else
			selectItem (editView, isExclusive ());
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ToolActions::ToggleSelectAction::addHelp (IHelpInfoBuilder& helpInfo)
{
	helpInfo.addOption (0, nullptr, XSTR (ToggleSelect));
	return false;
}

//************************************************************************************************
// ToolActions::SelectAction
//************************************************************************************************

EditHandler* ToolActions::SelectAction::perform (EditView& editView, const GUIEvent& event, PointRef where)
{
	if(item)
	{
		editView.getModel ().setFocusItem (item, &editView);
		editView.getModel ().setAnchorItem (item, &editView);
		selectItem (editView);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ToolActions::SelectAction::addHelp (IHelpInfoBuilder& helpInfo)
{
	helpInfo.addOption (0, nullptr, XSTR (Select));
	return false;
}

//************************************************************************************************
// ToolActions::DragItemAction
//************************************************************************************************

ToolActions::DragItemAction::DragItemAction (bool dragSelection)
: dragSelection (dragSelection)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditHandler* ToolActions::DragItemAction::perform (EditView& editView, const GUIEvent& event, PointRef where)
{
	if(item)
	{
		CCL_PRINTF ("DragItemAction item = %p\n", item)
		if(dragSelection)
		{
			// select item, drag selection
			editView.getModel ().setFocusItem (item, &editView);
			if(!editView.getSelection ().isSelected (item))
				selectItem (editView, true);

			if(editView.getModel ().dragSelection (editView, where, getInputDevice (event)))
				return NEW NullEditHandler (&editView); // stop other actions from happening
		}
		else
		{
			// drag the item
			// todo: could be an EditModel method as above
			AutoPtr<IDragSession> session (ccl_new<IDragSession> (ClassID::DragSession));
			session->setSource (editView.asUnknown ());
			session->setInputDevice (getInputDevice (event));
			session->getItems ().add (ccl_as_unknown (item), true);;

			Rect rect;
			if(editView.getModel ().getItemSize (rect, editView, item))
			{
				session->setSize (rect);

				Point offset;
				offset.x = where.x - rect.left;
				offset.y = where.y - rect.top;
				session->setOffset (offset);
			}
			session->drag ();
			return NEW NullEditHandler (&editView);
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ToolActions::DragItemAction::addHelp (IHelpInfoBuilder& helpInfo)
{
	helpInfo.addOption (KeyState::kDrag, nullptr, XSTR (Drag));
	return false;
}

//************************************************************************************************
// ToolActions::EditItemAction
//************************************************************************************************

ToolActions::EditItemAction::EditItemAction (bool selectItem)
: mustSelect (selectItem)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditHandler* ToolActions::EditItemAction::perform (EditView& editView, const GUIEvent& event, PointRef where)
{
	if(mustSelect && !editView.getModel ().getSelection ().isSelected (item))
		selectItem (editView, true);

	editView.getModel ().editItem (item, editView);
	return nullptr;
}

//************************************************************************************************
// ToolActions::EditHandlerAction
//************************************************************************************************

EditHandler* ToolActions::EditHandlerAction::perform (EditView& editView, const GUIEvent& event, PointRef where)
{
	return editView.getModel ().createEditHandler (item, editView, makeMouseEvent (event, where));
}

//************************************************************************************************
// ToolActions::ContextMenuAction
//************************************************************************************************

EditHandler* ToolActions::ContextMenuAction::perform (EditView& editView, const GUIEvent& event, PointRef where)
{
	selectItem (editView);

	if(IWindow* window = editView.getWindow ())
	{
		Point p (where);
		editView.clientToWindow (p);
		window->popupContextMenu (p);
	}

	return NEW NullEditHandler (&editView);
}

//************************************************************************************************
// ToolActions::UnselectAllAction
//************************************************************************************************

EditHandler* ToolActions::UnselectAllAction::perform (EditView& editView, const GUIEvent& event, PointRef where)
{
	// deselect all
	bool isCtrlPressed = getKeys (event).isSet (KeyState::kCommand);
	if(!isCtrlPressed)
	{
		Selection& selection = editView.getSelection ();
		selection.hide (false); // don't redraw yet!
		selection.unselectAll ();
		selection.show ();
	}
	return nullptr;
}

//************************************************************************************************
// ToolActions::DrawSelectionAction
//************************************************************************************************

EditHandler* ToolActions::DrawSelectionAction::perform (EditView& editView, const GUIEvent& event, PointRef where)
{
	if(!getKeys (event).isSet (KeyState::kShift))
	{
		Selection::Hideout hideout (editView.getModel ().getSelection ());
		editView.getModel ().getSelection ().unselectAll ();
	}
	return editView.getModel ().drawSelection (editView, makeMouseEvent (event, where));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ToolActions::DrawSelectionAction::addHelp (IHelpInfoBuilder& helpInfo)
{
	helpInfo.addOption (0, nullptr, XSTR (DrawSelection));
	helpInfo.addOption (KeyState::kShift, nullptr, XSTR (DrawSelectionAdd));
	return false;
}

//************************************************************************************************
// ToolActions::EraserAction
//************************************************************************************************

ToolActions::EraserAction::EraserAction ()
{
	setCursor ("EraserCursor");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditHandler* ToolActions::EraserAction::perform (EditView& editView, const GUIEvent& event, PointRef where)
{
	return editView.getModel ().dragEraser (editView, MouseEvent ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ToolActions::EraserAction::addHelp (IHelpInfoBuilder& helpInfo)
{
	helpInfo.addOption (0, nullptr, XSTR (DeleteObjects));
	return false;
}

//************************************************************************************************
// SelectTool
//************************************************************************************************

Configuration::StringValue SelectTool::defaultName	("Editing.SelectTool", "name", "Select Tool");
Configuration::StringValue SelectTool::defaultTitle	("Editing.SelectTool", "title");

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (SelectTool, ActionTool)

//////////////////////////////////////////////////////////////////////////////////////////////////

SelectTool::SelectTool (StringID name, StringRef title)
: ActionTool (name, title),
  contextMenuOnDoubleTap (true)
{
	if(name.isEmpty ())
		setName (MutableCString (defaultName));

	if(title.isEmpty ())
		setTitle (defaultTitle.getValue ().isEmpty () ? XSTR (SelectTool) : defaultTitle);

	setIconName ("SelectTool");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SelectTool::findActions (ToolActionList& actions, EditView& editView, PointRef where, const GUIEvent& event)
{
	AutoPtr<Object> item = editView.getModel ().findItem (editView, where);
	if(item)
	{
		// shift: toggle select state of item; no modifier: select item exlusive
		actions.addActionWithModifiers (NEW ToolActions::ToggleSelectAction, item, ToolAction::kSingleTap|ToolAction::kClick, KeyState::kShift);
		actions.addActionWithModifiers (NEW ToolActions::SelectAction, item, ToolAction::kSingleTap|ToolAction::kClick, 0);

		actions.addAction (NEW ToolActions::DragItemAction, item, ToolAction::kLongPress|ToolAction::kDrag);
		actions.addAction (NEW ToolActions::EditItemAction (true), item, ToolAction::kDoubleTap|ToolAction::kDoubleClick);
	}
	else
	{
		actions.addAction (NEW ToolActions::UnselectAllAction, nullptr, ToolAction::kClick);
		actions.addAction (NEW ToolActions::DrawSelectionAction, nullptr, ToolAction::kDrag);
	}

	if(isContextMenuOnDoubleTap ())
		actions.addAction (NEW ToolActions::ContextMenuAction, item, ToolAction::kDoubleTap);
}

//************************************************************************************************
// EraserTool
//************************************************************************************************

EraserTool::EraserTool ()
: ActionTool ("Eraser Tool", XSTR (EraserTool))
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EraserTool::findActions (ToolActionList& actions, EditView& editView, PointRef where, const GUIEvent& event)
{
	actions.addAction (NEW ToolActions::EraserAction, nullptr, ToolAction::kClick);
}
