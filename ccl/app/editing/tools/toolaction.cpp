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
// Filename    : ccl/app/editing/tools/toolaction.cpp
// Description : Tool Actions
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/app/editing/tools/toolaction.h"
#include "ccl/app/editing/editview.h"
#include "ccl/app/editing/editmodel.h"
#include "ccl/app/editing/edithandler.h"

#include "ccl/base/trigger.h"

#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/ipresentable.h"
#include "ccl/public/gui/framework/ihelpmanager.h"
#include "ccl/public/plugservices.h"

namespace CCL {

//************************************************************************************************
// ModifierAction
// Wraps another action, allows performing only when the given modifier combination matches
//************************************************************************************************

class ModifierAction: public ToolAction
{
public:
	ModifierAction (ToolAction* action, int modifiers);

	PROPERTY_AUTO_POINTER (ToolAction, action, Action)
	PROPERTY_VARIABLE (int, modifiers, Modifiers)

	// ToolAction
	bool canPerform (const KeyState& keys) override;
	EditHandler* perform (EditView& editView, const GUIEvent& event, PointRef where) override;
	void onGesture (EditView& editView, const GestureEvent& event, PointRef where) override;
	bool addHelp (IHelpInfoBuilder& helpInfo) override;
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

static AutoPtr<IThemeStatics> themeStatics;
CCL_KERNEL_TERM_LEVEL (ToolAction, kFirstRun)
{
	themeStatics.release ();
}

IThemeStatics& ToolAction::getThemeStatics ()
{
	if(!themeStatics)
		themeStatics = ccl_new<IThemeStatics> (ClassID::ThemeStatics);
	ASSERT (themeStatics)
	return *themeStatics;
}

//************************************************************************************************
// ToolTouchHandler
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (ToolTouchHandler, TouchMouseHandler)

//////////////////////////////////////////////////////////////////////////////////////////////////

ToolTouchHandler::ToolTouchHandler (EditView& editView)
: TouchMouseHandler (nullptr, editView),
  translateHoverEvents (false),
  editView (editView),
  gestureAction (nullptr),
  previewAction (nullptr),
  editHandlerTouchID (TouchEvent::kNoTouchId),
  handlesTouchEvents (false)
{
	actions.setGestureMask (ToolAction::kTouchMask);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ToolTouchHandler::prepareGestures ()
{
	// setup gestures required by actions
	ForEach (actions, ToolAction, action)
		ASSERT (action->getGestures () & ToolAction::kTouchMask)
		for(int gesture = GestureEvent::kSwipe; gesture <= GestureEvent::kDoubleTap; gesture++)
			if(action->getGestures () & (1 << gesture))
			{
				int g = gesture;
				if(action->getGestures () & ToolAction::kExclusiveTouch)
					g |= GestureEvent::kExclusiveTouch;
				if(action->getGestures () & ToolAction::kSuppressTouchContextMenu)
					g |= GestureEvent::kSuppressContextMenu;

				addRequiredGesture (g, action->getGesturePriority ());
			}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditHandler* ToolTouchHandler::createEditHandler (ToolAction& action, const TouchEvent& event)
{
	Point where;
	view->windowToClient (where);

	EditHandler* editHandler = action.perform (editView, event, where);
	if(editHandler)
	{
		// take over EditHandler as wrapped MouseHandler
		ASSERT (!mouseHandler)
		mouseHandler = editHandler;
	}
	return editHandler;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ToolTouchHandler::onHover (const TouchEvent& event)
{
	if(!translateHoverEvents || event.inputDevice != PointerEvent::kPenInput)
		return SuperClass::onHover (event);

	const TouchInfo* touch = event.touches.getTouchInfoByID (event.touchID);
	if(!touch)
		return false;

	MouseEvent mouseEvent;
	if(event.eventType == TouchEvent::kEnter)
	{
		mouseEvent = makeMouseEvent (MouseEvent::kMouseEnter, event, *view);
		lastPos = touch->where;
	}
	else if(event.eventType == TouchEvent::kHover)
	{
		Point currentWhere = touch->where;
		Point lastWhere = lastPos;
		editView.windowToClient (currentWhere);
		editView.windowToClient (lastWhere);

		int eventType = MouseEvent::kMouseMove;
		Rect rect;
		editView.getClientRect (rect);
		if(rect.pointInside (currentWhere) && !rect.pointInside (lastWhere))
			eventType = MouseEvent::kMouseEnter;
		else if(!rect.pointInside (currentWhere) && rect.pointInside (lastWhere))
			eventType = MouseEvent::kMouseLeave;

		mouseEvent = makeMouseEvent (eventType, event, *view);
		lastPos = touch->where;
	}
	else if(event.eventType == TouchEvent::kLeave)
		mouseEvent = makeMouseEvent (MouseEvent::kMouseLeave, event, *view);

	return editView.onViewEvent (mouseEvent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ToolTouchHandler::onBegin (const TouchEvent& event)
{
	if(const TouchInfo* touch = event.touches.getTouchInfoByID (event.touchID))
	{
		touches.add (touch->where);

		if(editHandlerTouchID == TouchEvent::kNoTouchId)
			editHandlerTouchID = touch->id;
	}

	if(!previewAction && !mouseHandler)
	{
		// find preview action (before gesture has begun)
		previewAction = actions.getActions ().findIf<ToolAction> ([] (const ToolAction& a) { return a.getGestures () & ToolAction::kPreview; });
		if(previewAction)
			createEditHandler (*previewAction, event);
	}

	if(!mouseHandler)
	{
		// find action that want's touch events (instead of gesture events)
		for(auto action : iterate_as<ToolAction> (actions.getActions ()))
		{
			if(action->getGestures () & (ToolAction::kRawTouches))
				if(EditHandler* editHandler = createEditHandler (*action, event))
				{
					handlesTouchEvents = true;
					break;
				}
		}
	}
	SuperClass::onBegin (event);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

bool ToolTouchHandler::onMove (const TouchEvent& event)
{
	// only feed mousehandler with events of the first touch
	if(mouseHandler && event.touchID != editHandlerTouchID && editHandlerTouchID != TouchEvent::kNoTouchId)
		return true;

	if(event.eventType == TouchEvent::kEnd)
	{
		onRelease (event, false);
		mouseHandler = nullptr;
	}

	return SuperClass::onMove (event);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

inline tbool CCL_API ToolTouchHandler::addTouch (const TouchEvent& event)
{
	if(event.inputDevice == inputDevice && touches.count () <= 2)
		if(const TouchInfo* touch = event.touches.getTouchInfoByID (event.touchID))
			touches.add (touch->where);

	return true; // swallow other touches
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ToolTouchHandler::allowsCompetingGesture (int gestureType)
{
	if(gestureAction)
		return gestureAction->allowsCompetingGesture (gestureType) == kResultOk;

	bool allow = false;
	for(auto action : iterate_as<ToolAction> (actions.getActions ()))
	{
		tresult result = action->allowsCompetingGesture (gestureType);
		if(result == kResultTrue)
			allow = true;			
		else if(result == kResultFalse)
		{
			allow = false; // one action can ultimately deny for all
			return false;
		}
		else			
			ASSERT (result == kResultNotImplemented) // action doesn't care
	}
	return allow;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ToolTouchHandler::onGesture (const GestureEvent& event)
{
	if(!editView.isAttached ())
		return false;

	if(handlesTouchEvents && !previewAction)
		return true;

	EditView::InputDeviceScope scope (editView, PointerEvent::kTouchInput);

	Point where (event.where);
	editView.windowToClient (where);

	switch(event.getState ())
	{
	case GestureEvent::kBegin:
		{
			if(previewAction)
			{
				if(previewAction->getGestures () & (1<<event.getType ()))
				{
					// a gesture of the preview action has begun: officially accept this action, keep using an already created mouse handler
					if(previewAction->getGestures () & (ToolAction::kSwipe|ToolAction::kZoom|ToolAction::kRotate))
					{
						gestureAction = previewAction;
						gestureAction->onGesture (editView, event, where);
					}
					previewAction = nullptr;
					return true;
				}
				else
				{
					// another gesture has begun: cancel mouse handler of previewAction; try to find a matching action in the loop below
					if(mouseHandler)
					{
						IView* view = editView;
						MouseEvent mouseEvent (makeMouseEvent (MouseEvent::kMouseDown, event, *view));
						mouseHandler->finish (mouseEvent, true);
						mouseHandler = nullptr;
					}

					previewAction = nullptr;
				}
			}

			ForEach (actions, ToolAction, action)
				if(action->getGestures () & (1<<event.getType ()))
				{
					if(action->getGestures () & (ToolAction::kHorizontal|ToolAction::kVertical))
					{
						bool isVertical = ccl_abs (event.amountY) > ccl_abs (event.amountX);
						if(event.getType () == GestureEvent::kZoom)
						{
							if(touches.count () >= 2)
							{
								Coord dx = touches[0].x - touches[1].x;
								Coord dy = touches[0].y - touches[1].y;
								isVertical = dx == 0 || ccl_abs (float(dy)/dx) > 2.f;
								CCL_PRINTF ("isVertical %d (dx %d, dy %d)\n", isVertical, dx, dy)
							}
						}
						bool wantsVertical = (action->getGestures () & ToolAction::kVertical) != 0;
						if(isVertical != wantsVertical)
							continue;
					}

					if(!action->canPerform (event.keys))
						continue;

					if(EditHandler* editHandler = action->perform (editView, event, where))
					{
						// take over first created EditHandler as wrapped MouseHandler
						ASSERT (!mouseHandler)
						mouseHandler = editHandler;
						
						IView* view = editView;
						MouseEvent mouseEvent (makeMouseEvent (MouseEvent::kMouseDown, event, *view));
						editHandler->begin (mouseEvent);

						if(event.getType () == GestureEvent::kSingleTap || event.getType () == GestureEvent::kDoubleTap)
						{
							// the gesture is already done (touches removed), we won't get a final kEnd event that would finish the mousehandler
							mouseEvent.eventType = MouseEvent::kMouseUp;
							editHandler->finish (mouseEvent, false);
							mouseHandler = nullptr; 
						}
						break;
					}
					else if(action->getGestures () & (ToolAction::kSwipe|ToolAction::kZoom|ToolAction::kRotate))
					{
						// take over first action that handles continuous gestures
						gestureAction = action;
						gestureAction->onGesture (editView, event, where);
						break;
					}
				}
			EndFor
		}
		break;

	case GestureEvent::kChanged:
		if(gestureAction)
			gestureAction->onGesture (editView, event, where);
		break;

	case GestureEvent::kEnd:
	case GestureEvent::kFailed:
		if(gestureAction)
		{
			gestureAction->onGesture (editView, event, where);
			gestureAction = nullptr;
		}
		break;

	case GestureEvent::kPossible:
		// forward preliminary event to all actions
		ForEach (actions, ToolAction, action)
			action->onGesture (editView, event, where);
		EndFor
		break;
	}
	return true;
}

//************************************************************************************************
// ToolAction
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (ToolAction, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

void ToolAction::setThemeCursor (ThemeCursorID which)
{
	setCursor (getThemeStatics ().getThemeCursorName (which));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ToolAction::selectItem (EditView& editView, bool exclusive)
{
	Selection& selection = editView.getSelection ();
	CCL_PRINTF ("ToolAction selectItem %p (selected: %s)\n", item, selection.isSelected (item) ? "true" : "false")
	if(item && (exclusive || !selection.isSelected (item)) && editView.getModel ().canSelectItem (item))
	{
		SharedPtr<Unknown> lifeGuard (this);
		Selection::Hideout hideout (selection, false);
		if(exclusive)
			selection.unselectAll ();
		editView.getModel ().selectItem (item);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PointerEvent::InputDevice ToolAction::getInputDevice (const GUIEvent& event)
{
	if(auto* pointerEvent = event.as<PointerEvent> ())
		return pointerEvent->inputDevice;
	else if(event.as<GestureEvent> ())
		return PointerEvent::kTouchInput;
	else
		return PointerEvent::kPointerInput;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MouseEvent ToolAction::makeMouseEvent (const GUIEvent& event, PointRef where)
{
	if(auto mouseEvent = event.as<MouseEvent> ())
		return *mouseEvent;
	else
		return MouseEvent (MouseEvent::kMouseDown, where);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ToolAction::addHelp (IHelpInfoBuilder& helpInfo)
{
	if(!getHelpText ().isEmpty ())
		helpInfo.addOption (0, nullptr, getHelpText ());
	return false;
}

//************************************************************************************************
// ModifierAction
//************************************************************************************************

ModifierAction::ModifierAction (ToolAction* action, int modifiers)
: action (action),
  modifiers (modifiers)
{
	// take attributes from original action
	setGestures (action->getGestures ());
	setGesturePriority (action->getGesturePriority ());
	setTooltip (action->getTooltip ());
	setCursor (action->getCursor ());
	setWantsCrossCursor (action->isWantsCrossCursor ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ModifierAction::canPerform (const KeyState& keys)
{
	return keys.getModifiers () == getModifiers () && action->canPerform (keys);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditHandler* ModifierAction::perform (EditView& editView, const GUIEvent& event, PointRef where)
{
	ASSERT (event.eventClass != GUIEvent::kMouseEvent || canPerform (getKeys (event)))
	return action->perform (editView, event, where);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ModifierAction::onGesture (EditView& editView, const GestureEvent& event, PointRef where)
{
	return action->onGesture (editView, event, where);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ModifierAction::addHelp (IHelpInfoBuilder& helpInfo)
{
	// add the required modifiers to the help options provided by the original action
	class HelpBuilderDelegate: public Unknown, public IHelpInfoBuilder
	{
	public:
		HelpBuilderDelegate (IHelpInfoBuilder& helpInfo, int modifiers)
		: helpInfo (helpInfo), modifiers (modifiers) {}
		
		IHelpInfoBuilder& helpInfo;
		PROPERTY_VARIABLE (int, modifiers, Modifiers)

		void CCL_API setAttribute (AttrID id, VariantRef value) override						{ helpInfo.setAttribute (id, value); }
		void CCL_API addOption (uint32 modifiers, IImage* icon, StringRef text) override		{ helpInfo.addOption (modifiers | getModifiers (), icon, text); }
		void CCL_API addOption (uint32 modifiers, StringID iconName, StringRef text) override	{ helpInfo.addOption (modifiers | getModifiers (), iconName, text); }
		void CCL_API setActiveOption (uint32 modifiers) override								{ helpInfo.setActiveOption (modifiers); }
		CLASS_INTERFACE (IHelpInfoBuilder, Unknown)
	};

	HelpBuilderDelegate builderDelegate (helpInfo, getModifiers ());
	return action->addHelp (builderDelegate);
}

//************************************************************************************************
// ToolActionList
//************************************************************************************************

void ToolActionList::addAction (ToolAction* action, Object* item, int gestures)
{
	if(gestures & gestureMask)
	{
		action->setItem (item);
		action->setGestures (gestures);
		actions.add (action);
	}
	else
		action->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ToolActionList::addActionWithModifiers (ToolAction* action, Object* item, int gestures, int modifiers)
{
	action->setItem (item);
	action->setGestures (gestures);
	addAction (NEW ModifierAction (action, modifiers), item, gestures);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ToolActionList::removeAll ()
{
	actions.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ToolActionList::removeAction (int gestures, const KeyState* keys)
{
	ToolAction* action = getFirstAction (gestures, keys);
	if(action)
	{
		actions.remove (action);
		action->release ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Iterator* ToolActionList::newIterator (int gestures, const KeyState* keys) const
{
	return NEW FilteringIterator (newIterator (), ObjectFilter::create ([gestures, keys] (IUnknown* obj)
	{
		auto action = unknown_cast<ToolAction> (obj);
		return (action->getGestures () & gestures) != 0 && (!keys || action->canPerform (*keys));
	}));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ToolAction* ToolActionList::getFirstAction (int gestures, const KeyState* keys) const
{
	IterForEach (newIterator (gestures, keys), ToolAction, action)
		return action;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ToolActionList::addHelpOption (uint32 modifiers, StringID iconName, StringRef text)
{
	// private action class that only adds a help option
	class HelpAction: public ToolAction
	{
	public:
		PROPERTY_VARIABLE (uint32, modifiers, Modifiers)
		PROPERTY_MUTABLE_CSTRING (iconName, IconName)
		PROPERTY_STRING (text, Text)

		bool addHelp (IHelpInfoBuilder& helpInfo) override
		{
			helpInfo.addOption (getModifiers (), getIconName (), getText ());
			return false;
		}
	};

	auto action = NEW HelpAction;
	action->setModifiers (modifiers);
	action->setIconName (iconName);
	action->setText (text);
	addAction (action);
}

//************************************************************************************************
// ActionTool
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (ActionTool, EditTool)

//////////////////////////////////////////////////////////////////////////////////////////////////

ActionTool::ActionTool (StringID name, StringRef title)
: EditTool (name, title),
  translateHoverEvents (false),
  performingActions (false)
{
	mouseActions.setGestureMask (ToolAction::kMouseMask);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ActionTool::mouseEnter (EditView& editView, const MouseEvent& mouseEvent)
{
	SuperClass::mouseEnter (editView, mouseEvent);

	mouseMove (editView, mouseEvent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ActionTool::mouseMove (EditView& editView, const MouseEvent& mouseEvent)
{
	if(performingActions || editView.getMouseState ())
		return;

	mouseActions.removeAll ();
	findActions (mouseActions, editView, mouseEvent.where, mouseEvent);

	MutableCString cursor;

	// use first click or drag action for cursor
	ToolAction* mouseAction = mouseActions.getFirstAction (ToolAction::kClick|ToolAction::kDrag, &mouseEvent.keys);
	if(mouseAction)
	{
		cursor = mouseAction->getCursor ();
		wantsCrossCursor (mouseAction->isWantsCrossCursor ());
	}

	if(cursor.isEmpty ())
		cursor = getCursorName ();

	setMouseCursor (editView.getTheme ().getCursor (cursor));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ActionTool::mouseLeave (EditView& editView, const MouseEvent& mouseEvent)
{
	setMouseCursor (nullptr); //was: editView.getTheme ().getCursor (0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditHandler* ActionTool::mouseDown (EditView& editView, const MouseEvent& mouseEvent)
{
	if(editView.editHandlerActive ())
		return nullptr;

	mouseMove (editView, mouseEvent);

	ScopedVar<bool> scope (performingActions, true);

	// drag (must detect before single click)
	IterForEach (mouseActions.newIterator (ToolAction::kDrag, &mouseEvent.keys), ToolAction, action)
		if(!editView.detectDrag (mouseEvent))
			break;

		if(action->getCursor ().isEmpty () == false)
		{
			setMouseCursor (editView.getTheme ().getCursor (action->getCursor ()));
			editView.setCursor (mouseCursor);
		}

		if(EditHandler* handler = action->perform (editView, mouseEvent, mouseEvent.where))
			return handler;
	EndFor

	// click
	IterForEach (mouseActions.newIterator (ToolAction::kClick, &mouseEvent.keys), ToolAction, action)
		if(EditHandler* handler = action->perform (editView, mouseEvent, mouseEvent.where))
			return handler;
	EndFor

	// double-click
	IterForEach (mouseActions.newIterator (ToolAction::kDoubleClick, &mouseEvent.keys), ToolAction, action)
		if(!editView.detectDoubleClick (mouseEvent))
			break;

		if(EditHandler* handler = action->perform (editView, mouseEvent, mouseEvent.where))
			return handler;
	EndFor

	// single-click
	IterForEach (mouseActions.newIterator (ToolAction::kSingleClick, &mouseEvent.keys), ToolAction, action)
		if(editView.detectDoubleClick (mouseEvent))
			break;

		if(EditHandler* handler = action->perform (editView, mouseEvent, mouseEvent.where))
			return handler;
	EndFor

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String ActionTool::getTooltip ()
{
	if(ToolAction* action = mouseActions.getFirstAction (ToolAction::kClick))
		return action->getTooltip ();

	return String::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITouchHandler* ActionTool::createTouchHandler (EditView& editView, const TouchEvent& event)
{
	const TouchInfo* touch = event.touches.getTouchInfoByID (event.touchID);
	ASSERT (touch)
	if(touch)
	{
		Point where (touch->where);
		editView.windowToClient (where);

		// collect touch actions
		ToolTouchHandler* handler = NEW ToolTouchHandler (editView);
		findActions (handler->getActions (), editView, where, event);
		handler->prepareGestures ();
		handler->setTranslateHoverEvents (translateHoverEvents);
		return handler;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPresentable* ActionTool::createHelpInfo (EditView& editView, const MouseEvent& mouseEvent)
{
	if(mouseActions.getActions ().isEmpty ())
		return nullptr;

	// todo: cache in IHelpInfoCollection?
	AutoPtr<IHelpInfoBuilder> helpInfo (ccl_new<IHelpInfoBuilder> (CCL::ClassID::HelpInfoBuilder));
	helpInfo->setAttribute (IHelpInfoBuilder::kTitle, getTitle ());
	helpInfo->setAttribute (IHelpInfoBuilder::kIcon, String (getIconName ()));

	for(auto action : iterate_as<ToolAction> (mouseActions.getActions ()))
		if(action->addHelp (*helpInfo))
			break;

	return UnknownPtr<IPresentable> (helpInfo).detach ();
}
