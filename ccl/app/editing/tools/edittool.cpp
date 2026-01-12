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
// Filename    : ccl/app/editing/tools/edittool.cpp
// Description : Editing Tool
//
//************************************************************************************************

#include "ccl/app/editing/tools/edittool.h"

#include "ccl/app/editing/editview.h"

#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/imousecursor.h"

using namespace CCL;

//************************************************************************************************
// NativeToolSet
//************************************************************************************************

NativeToolSet::NativeToolSet ()
{
	tools.objectCleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Iterator* NativeToolSet::getTools ()
{
	return tools.newIterator ();
}

//************************************************************************************************
// EditToolMode
//************************************************************************************************

DEFINE_CLASS_HIDDEN (EditToolMode, Object)

//************************************************************************************************
// EditTool
//************************************************************************************************

DEFINE_CLASS_HIDDEN (EditTool, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

EditTool::EditTool (StringID name, StringRef title)
: name (name),
  title (title),
  mouseCursor (nullptr),
  activeMode (nullptr),
  flags (0),
  ignoreModifier (0)
{
	modes.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditTool::~EditTool ()
{
	setMouseCursor (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditTool::addMode (EditToolMode* mode)
{
	modes.add (mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditTool::setActiveMode (StringID modeName)
{
	ListForEachObject (modes, EditToolMode, mode)
		if(mode->getName () == modeName)
		{
			setActiveMode (mode);
			return true;
		}
	EndFor

	return false; // keep old mode
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditTool::setActiveMode (EditToolMode* mode)
{
	activeMode = mode;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditTool* EditTool::getActiveModeHandler ()
{
	// the active mode may have a Tool implementation that is used instead of this
	if(activeMode && activeMode->getHandler ())
		return activeMode->getHandler ();

	return this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMouseCursor* EditTool::getMouseCursor () const
{
	return mouseCursor;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditTool::setMouseCursor (IMouseCursor* cursor)
{
	take_shared (mouseCursor, cursor);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditTool::mouseEnter (EditView& editView, const MouseEvent& mouseEvent)
{
	// init mouse cursor
	if(!mouseCursor && !cursorName.isEmpty ())
		setMouseCursor (editView.getTheme ().getCursor (cursorName));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditTool::mouseMove (EditView& editView, const MouseEvent& mouseEvent)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditTool::mouseLeave (EditView& editView, const MouseEvent& mouseEvent)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditHandler* EditTool::mouseDown (EditView& editView, const MouseEvent& mouseEvent)
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITouchHandler* EditTool::createTouchHandler (EditView& editView, const TouchEvent& event)
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String EditTool::getTooltip ()
{
	return String::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPresentable* EditTool::createHelpInfo (EditView& editView, const MouseEvent& mouseEvent)
{
	return nullptr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void EditTool::onAttached (EditView& editView, bool state)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditTool::onContextMenu (IContextMenu& contextMenu)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditTool::extendModeMenu (IMenu& menu)
{
	return false;
}
