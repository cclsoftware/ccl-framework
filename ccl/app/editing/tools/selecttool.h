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
// Filename    : ccl/app/editing/tools/selecttool.h
// Description : Selection Tool, Eraser Tool, basic tool actions
//
//************************************************************************************************

#ifndef _ccl_selecttool_h
#define _ccl_selecttool_h

#include "ccl/app/editing/tools/toolaction.h"

#include "ccl/base/storage/configuration.h"

namespace CCL {

//************************************************************************************************
// ToolStrings
//************************************************************************************************

namespace ToolStrings
{
	StringRef Select ();
	StringRef ToggleSelect ();
	StringRef SelectRange ();
	StringRef Zoom ();
}

//************************************************************************************************
// SelectTool
//************************************************************************************************

class SelectTool: public ActionTool
{
public:
	DECLARE_CLASS (SelectTool, ActionTool)

	SelectTool (StringID name = nullptr, StringRef title = nullptr);

	PROPERTY_BOOL (contextMenuOnDoubleTap, ContextMenuOnDoubleTap)

	// ActionTool
	void findActions (ToolActionList& actions, EditView& editView, PointRef where, const GUIEvent& event) override;

private:
	static Configuration::StringValue defaultName;
	static Configuration::StringValue defaultTitle;
};

//************************************************************************************************
// EraserTool
//************************************************************************************************

class EraserTool: public CCL::ActionTool
{
public:
	EraserTool ();

	// SelectTool
	void findActions (ToolActionList& actions, EditView& editView, PointRef where, const GUIEvent& event) override;
};


namespace ToolActions {

//************************************************************************************************
// ToolActions::ToggleSelectAction
//************************************************************************************************

class ToggleSelectAction: public ToolAction
{
public:
	ToggleSelectAction (bool exclusive = false);

	PROPERTY_BOOL (exclusive, Exclusive)

	EditHandler* perform (EditView& editView, const GUIEvent& event, PointRef where) override;
	bool addHelp (IHelpInfoBuilder& helpInfo) override;
};

//************************************************************************************************
// ToolActions::SelectAction
//************************************************************************************************

class SelectAction: public ToolAction
{
public:
	EditHandler* perform (EditView& editView, const GUIEvent& event, PointRef where) override;
	bool addHelp (IHelpInfoBuilder& helpInfo) override;
};

//************************************************************************************************
// ToolActions::DragItemAction
//************************************************************************************************

class DragItemAction: public ToolAction
{
public:
	DragItemAction (bool dragSelection = true); // use selection object as drag data, otherwise the "item"

	EditHandler* perform (EditView& editView, const GUIEvent& event, PointRef where) override;
	bool addHelp (IHelpInfoBuilder& helpInfo) override;

private:
	bool dragSelection = true;
};

//************************************************************************************************
// ToolActions::EditItemAction
/* Calls EditModel::editItem. */
//************************************************************************************************

class EditItemAction: public ToolAction
{
public:
	EditItemAction (bool selectItem = false);

	EditHandler* perform (EditView& editView, const GUIEvent& event, PointRef where) override;

private:
	bool mustSelect = false;
};

//************************************************************************************************
// ToolActions::EditHandlerAction
/* Calls EditModel::createEditHandler. */
//************************************************************************************************

class EditHandlerAction: public ToolAction
{
public:
	EditHandler* perform (EditView& editView, const GUIEvent& event, PointRef where) override;
};

//************************************************************************************************
// ToolActions::ContextMenuAction
//************************************************************************************************

class ContextMenuAction: public ToolAction
{
public:
	EditHandler* perform (EditView& editView, const GUIEvent& event, PointRef where) override;
};

//************************************************************************************************
// ToolActions::UnselectAllAction
//************************************************************************************************

class UnselectAllAction: public ToolAction
{
public:
	EditHandler* perform (EditView& editView, const GUIEvent& event, PointRef where) override;
};

//************************************************************************************************
// ToolActions::DrawSelectionAction
//************************************************************************************************

class DrawSelectionAction: public ToolAction
{
public:
	EditHandler* perform (EditView& editView, const GUIEvent& event, PointRef where) override;
	bool addHelp (IHelpInfoBuilder& helpInfo) override;
};

//************************************************************************************************
// ToolActions::EraserAction
//************************************************************************************************

class EraserAction: public ToolAction
{
public:
	EraserAction ();

	EditHandler* perform (EditView& editView, const GUIEvent& event, PointRef where) override;
	bool addHelp (IHelpInfoBuilder& helpInfo) override;
};

} // namespace ToolActions

} // namespace CCL

#endif // _ccl_selecttool_h
