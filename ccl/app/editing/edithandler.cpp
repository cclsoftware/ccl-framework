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
// Filename    : ccl/app/editing/edithandler.cpp
// Description : Editing Handler
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/app/editing/edithandler.h"

#include "ccl/app/editing/editview.h"
#include "ccl/app/editing/editmodel.h"
#include "ccl/app/editing/editor.h"

#include "ccl/public/base/variant.h"
#include "ccl/public/gui/framework/isprite.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/iscrollview.h"
#include "ccl/public/gui/iparameter.h"
#include "ccl/public/system/isignalhandler.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/plugservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_IID_ (IEditHandlerHook, 0x926407db, 0x911c, 0x4a6d, 0x91, 0x28, 0xc9, 0x7f, 0x2b, 0xae, 0x53, 0x32)

//************************************************************************************************
// EditHandler
//************************************************************************************************

DEFINE_CLASS_HIDDEN (EditHandler, UserControl::MouseHandler)

//////////////////////////////////////////////////////////////////////////////////////////////////

EditHandler::EditHandler (EditView* view)
: MouseHandler ((UserControl*)view, kAutoScroll),
  ignoreModifier (0),
  wantsCrossCursor (false),
  tooltipUsed (false)
{
	canEscape (true);
	view->editHandlerActive (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditHandler::~EditHandler ()
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditView* EditHandler::getEditView () const
{
	return (EditView*)getControl ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String EditHandler::getCurrentActionCode ()
{
	String actionCode;
	if(hook)
		actionCode = hook->getActionCode (*getEditView (), current);
	return actionCode;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditHandler::updateCursor ()
{
	EditView& editView = *getEditView ();

	if(hook)
	{
		// let hook update the cursor
		String cursorName = hook->getCursor (editView, current);
		if(!cursorName.isEmpty ())
			getEditView ()->setCursor (getEditView ()->getTheme ().getCursor (MutableCString (cursorName)));
	}

	if(editView.hasCrossCursor ())
	{
		bool wantsCross = isWantsCrossCursor ();

		// let hook update the crossCursor state
		if(hook)
			hook->updateCrossCursor (wantsCross, editView, current);

		editView.showCrossCursor (wantsCross);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditHandler::setEditTooltip (StringRef tooltip)
{
	getEditView ()->setEditTooltip (tooltip);
	tooltipUsed = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditHandler::hideEditTooltip ()
{
	getEditView ()->hideEditTooltip ();
	tooltipUsed = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditHandler::getHelp (IHelpInfoBuilder& helpInfo)
{
	return hook ? hook->getHelp (helpInfo) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditHandler::onRelease (bool canceled)
{
	if(hook)
		hook->onRelease (*getEditView (), canceled);

	getEditView ()->updateToolCursor (current); // new scenario after edit handler action

	getEditView ()->editHandlerActive (false);
	if(tooltipUsed)
		getEditView ()->hideEditTooltip ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditHandler::setHookFromArgument (MessageRef msg, int argumentIndex)
{
	if(msg.getArgCount () > argumentIndex)
	{
		UnknownPtr<IEditHandlerHook> hook (msg[argumentIndex]);
		if(hook)
			setHook (hook);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API EditHandler::setProperty (MemberID propertyId, const Variant& var)
{
	if(propertyId == "ignoreModifier")
	{
		setIgnoreModifier (var);
		return true;
	}

	if(propertyId == "wantsCrossCursor")
	{
		setWantsCrossCursor (var);
		return true;
	}

	if(propertyId == "hook")
	{
		UnknownPtr<IEditHandlerHook> hook (var.asUnknown ());
		setHook (hook);
		return true;
	}

	// return true for unknown properties (returning false could make a whole script method invocation fail)
	#if DEBUG
	Debugger::printf ("EditHandler::setProperty: unknown property \"%s\"\n", propertyId.str ());
	#endif
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (EditHandler)
	DEFINE_METHOD_NAME ("setEditTooltip")
END_METHOD_NAMES (EditHandler)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API EditHandler::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "setEditTooltip")
	{
		String tooltip (msg[0].asString ());
		if(tooltip.isEmpty ())
			hideEditTooltip ();
		else
			setEditTooltip (tooltip);
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}


//************************************************************************************************
// NullEditHandler
//************************************************************************************************

DEFINE_CLASS_HIDDEN (NullEditHandler, EditHandler)

//////////////////////////////////////////////////////////////////////////////////////////////////

NullEditHandler::NullEditHandler (EditView* view)
: EditHandler (view),
  viewHolder (*view)
{
	isNullHandler (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NullEditHandler::~NullEditHandler ()
{
	((EditView*)control)->editHandlerActive (false);
}

//************************************************************************************************
// DrawSelectionHandler
//************************************************************************************************

DrawSelectionHandler::DrawSelectionHandler (EditView* view)
: EditHandler (view),
  sprite (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DrawSelectionHandler::onBegin ()
{
	EditHandler::onBegin ();

	ASSERT (sprite == nullptr)
	if(!sprite)
	{
		ITheme& theme = getControl ()->getTheme ();
		Color color = theme.getThemeColor (ThemeElements::kAlphaSelectionColor);

		AutoPtr<IDrawable> shape = NEW SolidDrawable (color);
		sprite = ccl_new<ISprite> (ClassID::FloatingSprite);
		ASSERT (sprite != nullptr)
		sprite->construct (*getControl (), Rect (), shape);
		sprite->takeOpacity (shape);
		sprite->show ();
	}

	if(!first.keys.isSet (KeyState::kShift))
	{
		Selection::Hideout hideout (getEditView ()->getSelection (), false);
		getEditView ()->getSelection ().unselectAll ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DrawSelectionHandler::onMove (int moveFlags)
{
	Rect r;
	r.left = first.where.x;
	r.top = first.where.y;
	r.right = current.where.x;
	r.bottom = current.where.y;
	r.normalize ();

	if(sprite)
		sprite->move (r);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DrawSelectionHandler::onRelease (bool canceled)
{
	Rect rect;
	if(sprite)
	{
		rect = sprite->getSize ();
		sprite->hide ();
		sprite->release ();
		sprite = nullptr;
	}

	if(!rect.isEmpty () && !canceled)
	{
		EditView* editView = getEditView ();
		if(editView->getModel ().selectItems (*editView, rect))
			editView->getSelection ().show ();
	}

	EditHandler::onRelease (canceled);
}

//************************************************************************************************
// DeleteEditHandler
//************************************************************************************************

DeleteEditHandler::DeleteEditHandler (EditView* view)
: EditHandler (view)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DeleteEditHandler::onBegin ()
{
	EditHandler::onBegin ();
	onMove (0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DeleteEditHandler::onMove (int moveFlags)
{
	if(EditView* view = getEditView ())
	{
		EditModel& model = view->getModel ();
		AutoPtr<Object> item (model.findItem (*view, current.where));
		if(item)
		{
			if(model.getSelection ().isSelected (item))
				model.deleteSelected ();
			else
				model.deleteItem (item);
		}
	}
	return true;
}

//************************************************************************************************
// ScrollEditHandler
//************************************************************************************************

ScrollEditHandler::ScrollEditHandler (EditView* view)
: EditHandler (view),
  hasMoved (false)
{
	autoScroll (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollEditHandler::onBegin ()
{
	if(EditView* editView = getEditView ())
	{
		firstPos = first.where;
		editView->clientToWindow (firstPos);
		prevPos = firstPos;

		editView->setCursor (editView->getTheme ().getThemeCursor (ThemeElements::kGrabCursor));
		editView->scrollHandlerActive (true);
	}
	SuperClass::onBegin ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ScrollEditHandler::onMove (int moveFlags)
{
	EditView* editView = getEditView ();

	if(editView && current.keys.isSet (KeyState::kMButton))
	{
		// our view can move between calls, calculate in window coords
		Point currentPos (current.where);
		editView->clientToWindow (currentPos);

		if(!hasMoved)
		{
			// ignore until moved out of a tolerance area (distingish from simple click which will open tool palette popup)
			constexpr Coord kMoveTolerance = 4;

			Point totalDelta (currentPos - firstPos);
			hasMoved = ccl_abs (totalDelta.x) >= kMoveTolerance || ccl_abs (totalDelta.y) >= kMoveTolerance;
		}

		if(hasMoved)
		{
			Point delta (currentPos - prevPos);
			performScrolling (delta);
			prevPos = currentPos;

			// ScrollView receives deferred parameter changes; flush to stay in sync, so that the next clientToWindow () uses the new position
			System::GetSignalHandler ().flush ();
		}
	}

	return SuperClass::onMove (moveFlags);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollEditHandler::performScrolling (PointRef delta)
{
	EditView* editView = getEditView ();
	bool horizontal = ccl_abs (delta.x) >= ccl_abs (delta.y);

	// find scrollable view
	IScrollable* scrollable = editView ? GetViewInterfaceUpwards<IScrollable> (*editView) : nullptr;
	if(scrollable)
	{
		// check if scrollable can scroll in the chosen direction, otherwise try one level more upwards
		IParameter* scrollParam = horizontal ? scrollable->getHScrollParam () : scrollable->getVScrollParam ();
		if(!scrollParam || scrollParam->getMin () == scrollParam->getMax ())
		{
			// IScrollable can be a framework view or a UserControl
			IView* scrollableView = UnknownPtr<IView> (scrollable);
			if(!scrollableView)
				if(auto* userControl = unknown_cast<UserControl> (scrollable))
					scrollableView = *userControl;

			if(scrollableView)
				scrollable = GetViewInterfaceUpwards<IScrollable> (scrollableView->getParentView ());
		}
	}

	if(scrollable)
	{
		if(horizontal)
			scrollable->scrollByH (-delta.x);
		else
			scrollable->scrollByV (-delta.y);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollEditHandler::onRelease (bool canceled)
{
	EditView* editView = getEditView ();
	if(editView)
	{
		editView->scrollHandlerActive (false);

		// if not moved / scrolled: popup tool palette (EditView default behavior on middle click)
		EditorComponent* editor = editView->getModel ().getEditorComponent ();
		if(!hasMoved && editor)
			editor->popupToolPalette (*editView, first.where);
	}
	SuperClass::onRelease (canceled);
}
