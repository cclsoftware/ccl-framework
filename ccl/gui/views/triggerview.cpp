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
// Filename    : ccl/gui/views/triggerview.cpp
// Description : Trigger View
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/views/triggerview.h"
#include "ccl/gui/touch/touchhandler.h"
#include "ccl/gui/skin/skininteractive.h"
#include "ccl/gui/skin/skinmodel.h"
#include "ccl/gui/windows/window.h"

#include "ccl/public/gui/framework/abstractdraghandler.h"
#include "ccl/public/gui/icontextmenu.h"

#include "ccl/base/message.h"
#include "ccl/base/trigger.h"

using namespace CCL;

//************************************************************************************************
// TriggerView::EventState
//************************************************************************************************

TriggerView::EventState::EventState ()
: delegateView (nullptr),
  contextMenuEvent (nullptr),
  eventHandled (false)
{}

//************************************************************************************************
// TriggerView::EventStateGuard
/** Resets EventState at beginning and end of scope. */
//************************************************************************************************

class TriggerView::EventStateGuard
{
public:
	EventStateGuard (EventState& state) : state (state) { state = EventState (); }
	~EventStateGuard () { state = EventState (); }

	EventState& state;
};

//************************************************************************************************
// TriggerView
//************************************************************************************************

DEFINE_STRINGID_MEMBER_ (TriggerView, kOnMouseDown, "onMouseDown")
DEFINE_STRINGID_MEMBER_ (TriggerView, kOnSingleClick, "onSingleClick")
DEFINE_STRINGID_MEMBER_ (TriggerView, kOnRightClick, "onRightClick")
DEFINE_STRINGID_MEMBER_ (TriggerView, kOnDoubleClick, "onDoubleClick")
DEFINE_STRINGID_MEMBER_ (TriggerView, kOnDrag, "onDrag")
DEFINE_STRINGID_MEMBER_ (TriggerView, kOnSingleTap, "onSingleTap")
DEFINE_STRINGID_MEMBER_ (TriggerView, kOnDoubleTap, "onDoubleTap")
DEFINE_STRINGID_MEMBER_ (TriggerView, kOnFirstTap, "onFirstTap")
DEFINE_STRINGID_MEMBER_ (TriggerView, kOnLongPress, "onLongPress")
DEFINE_STRINGID_MEMBER_ (TriggerView, kOnSwipe, "onSwipe")
DEFINE_STRINGID_MEMBER_ (TriggerView, kOnSwipeH, "onSwipeH")
DEFINE_STRINGID_MEMBER_ (TriggerView, kOnSwipeV, "onSwipeV")
DEFINE_STRINGID_MEMBER_ (TriggerView, kOnTouch, "onTouch")
DEFINE_STRINGID_MEMBER_ (TriggerView, kOnContextMenu, "onContextMenu")
DEFINE_STRINGID_MEMBER_ (TriggerView, kOnDisplayPropertiesChanged, "onDisplayPropertiesChanged")

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_STYLEDEF (TriggerView::customStyles)
	{"swallowdrag",	Styles::kTriggerViewBehaviorSwallowDrag},
END_STYLEDEF

BEGIN_STYLEDEF (TriggerView::gesturePriorities)
	{"low",			GestureEvent::kPriorityLow},
	{"normal",		GestureEvent::kPriorityNormal},
	{"high",		GestureEvent::kPriorityHigh},
	{"highest",		GestureEvent::kPriorityHighest},
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (TriggerView, View)

/*static*/ TriggerView::EventState TriggerView::eventState;

//////////////////////////////////////////////////////////////////////////////////////////////////

TriggerView::TriggerView (IUnknown* controller, const Rect& size, StyleRef style, StringRef title)
: View (size, style, title),
  controller (controller),
  gesturePriority (GestureEvent::kPriorityNormal),
  flags (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TriggerView::checkTriggers ()
{
	using namespace SkinElements;

	if(!triggersChecked ())
	{
		// check if there are EventTriggers listening to the various events; we will e.g. only try to detect gestures if necessary
		VisualStyle* vs = unknown_cast<VisualStyle> (&getVisualStyle ());
		if(TriggerListElement* triggerList = unknown_cast<TriggerListElement> (vs->getTrigger ()))
		{
			ArrayForEach (*triggerList, SkinElements::Element, e)
				if(TriggerElement* triggerElement = ccl_cast<TriggerElement> (e))
					if(EventTrigger* eventTrigger = ccl_cast<EventTrigger> (triggerElement->getPrototype ()))
					{
						if(eventTrigger->hasEventID (kOnSingleClick))
							wantsSingleClick (true);
						if(eventTrigger->hasEventID (kOnRightClick))
							wantsRightClick (true);
						if(eventTrigger->hasEventID (kOnDoubleClick))
							wantsDoubleClick (true);
						if(eventTrigger->hasEventID (kOnDrag))
							wantsDrag (true);
						if(eventTrigger->hasEventID (kOnSingleTap))
							wantsSingleTap (true);
						if(eventTrigger->hasEventID (kOnDoubleTap))
							wantsDoubleTap (true);
						if(eventTrigger->hasEventID (kOnFirstTap))
							wantsFirstTap (true);
						if(eventTrigger->hasEventID (kOnLongPress))
							wantsLongPress (true);
						if(eventTrigger->hasEventID (kOnSwipe))
							wantsSwipe (true);
						if(eventTrigger->hasEventID (kOnSwipeH))
							wantsSwipeH (true);
						if(eventTrigger->hasEventID (kOnSwipeV))
							wantsSwipeV (true);
						if(eventTrigger->hasEventID (kOnTouch))
							wantsTouch (true);
						if(eventTrigger->hasEventID (kOnContextMenu))
							wantsContextMenu (true);
						if(eventTrigger->hasEventID (kOnAttached))
							wantsAttached (true);
						if(eventTrigger->hasEventID (kOnRemoved))
							wantsRemoved (true);
					}
			EndFor
		}
		triggersChecked (true);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TriggerView::signal (MessageRef msg)
{
	// for all signals we emit, set our theme as context for trigger actions
	ThemeSelector themeSelector (getTheme ());

	SuperClass::signal (msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TriggerView::attached (View* parent)
{
	SuperClass::attached (parent);

	checkTriggers ();
	if(wantsAttached ())
		signal (Message (kOnAttached));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TriggerView::removed (View* parent)
{
	checkTriggers ();
	if(wantsRemoved ())
		signal (Message (kOnRemoved));

	SuperClass::removed (parent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TriggerView::onDisplayPropertiesChanged (const DisplayChangedEvent& event)
{
	signal (Message (kOnDisplayPropertiesChanged));

	SuperClass::onDisplayPropertiesChanged (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TriggerView::setController (IUnknown* c)
{
	controller = c;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API TriggerView::getController () const
{
	return controller;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TriggerView::onMouseDown (const MouseEvent& event)
{
	SharedPtr<Object> holder (this);
	EventStateGuard scope (eventState);

	// a delegate view that creates a mouse handler can be set in one of the various trigger events
	auto tryMouseHandlerDelegate = [&] ()
	{
		if(eventState.delegateView)
		{
			MouseEvent event2 (event);
			clientToWindow (event2.where);
			eventState.delegateView->windowToClient (event2.where);
			MouseHandler* handler = eventState.delegateView->createMouseHandler (event2);
			if(handler)
			{
				if(handler->isNullHandler ())
					handler->release ();
				else
				{
					getWindow ()->setMouseHandler (handler);
					handler->begin (event2);
				}
				return true;
			}
		}
		return false;
	};

	signal (Message (kOnMouseDown));

	if(!isAttached ())
		return true;	// view might have been removed as result of a trigger action

	checkTriggers ();

	if(tryMouseHandlerDelegate () || eventState.eventHandled)
		return true;

	if(wantsDrag () && detectDrag (event))
	{
		signal (Message (kOnDrag));

		tryMouseHandlerDelegate ();
		return true;
	}

	bool mustExit = false;
	if(wantsDoubleClick ())
	{
		int oldRetainCount = getRetainCount ();

		if(detectDoubleClick (event))
		{
			signal (Message (kOnDoubleClick));

			tryMouseHandlerDelegate ();
			return true;
		}

		// a parent view of this (and siblings) might have been removed while waiting for double click (flushUpdates -> timer -> unpredictable actions...)
		// this TriggerView is protected by the SharedPtr above, but we must return true in this case to exit all loops in View::onMouseDown calls in the callstack;
		// we check this by watching our retainCount	
		int newRetainCount = getRetainCount ();
		if(newRetainCount  < oldRetainCount)
			mustExit = true;
	}

	if(event.keys.isSet (KeyState::kRButton))
	{
		if(wantsRightClick ())
			signal (Message (kOnRightClick));
	}
	else if(wantsSingleClick ())
		signal (Message (kOnSingleClick));

	if(mustExit)
		return true;

	if(tryMouseHandlerDelegate () || eventState.eventHandled)
		return true;

	return View::onMouseDown (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TriggerView::onGesture (const GestureEvent& event)
{
	if(event.getType () == GestureEvent::kSingleTap)
	{
		if(wantsFirstTap () && !wantsSingleTap ())
		{
			// ignore if we have already emitted kOnFirstTap when handling kDoubleTap/kPossible
			bool alreadyHandled = wantsDoubleTap ()
				&& event.eventTime - lastGestureEvent.eventTime < 2.
				&& ccl_abs (event.where.x - lastGestureEvent.where.x) < 5
				&& ccl_abs (event.where.y - lastGestureEvent.where.y) < 5
				&& lastGestureEvent.getType () == GestureEvent::kDoubleTap
				&& lastGestureEvent.getState () == GestureEvent::kPossible;

			if(!alreadyHandled)
				signal (Message (kOnFirstTap));
		}
		else 
			signal (Message (kOnSingleTap));

		lastGestureEvent = event;
		return true;
	}
	if(event.getType () == GestureEvent::kDoubleTap)
	{
		if(event.getState () == GestureEvent::kPossible)
			signal (Message (kOnFirstTap));
		else
			signal (Message (kOnDoubleTap));

		lastGestureEvent = event;
		return true;
	}
	if(event.getType () == GestureEvent::kLongPress)
	{
		if(event.getState () == GestureEvent::kBegin)
			signal (Message (kOnLongPress));

		lastGestureEvent = event;
		return true;
	}
	if(event.getType () == GestureEvent::kSwipe)
	{
		if(event.getState () == GestureEvent::kBegin)
		{
			if(wantsSwipe ())
				signal (Message (kOnSwipe));

			if(wantsSwipeH () && event.isHorizontal ())
				signal (Message (kOnSwipeH));

			if(wantsSwipeV () && event.isVertical ())
				signal (Message (kOnSwipeV));
		}
		lastGestureEvent = event;
		return true;
	}
	return SuperClass::onGesture (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITouchHandler* TriggerView::createTouchHandler (const TouchEvent& event)
{
	checkTriggers ();

	if(wantsTouch ())
	{
		EventStateGuard scope (eventState);

		signal (Message (kOnTouch));

		if(eventState.delegateView)
		{
			ITouchHandler* delegateHandler = eventState.delegateView->createTouchHandler (event);
			if(delegateHandler)
			{
				if(gesturePriority != GestureEvent::kPriorityNormal)
					Property (UnknownPtr<IObject>(delegateHandler), "gesturePriority").set (gesturePriority);

				return delegateHandler;
			}
		}
	}

	GestureHandler* handler = NEW GestureHandler (this);
	if(wantsSingleTap () || wantsFirstTap ())
		handler->addRequiredGesture (GestureEvent::kSingleTap, gesturePriority);
	if(wantsDoubleTap () || wantsFirstTap ())
		handler->addRequiredGesture (GestureEvent::kDoubleTap, gesturePriority);
	if(wantsLongPress ())
		handler->addRequiredGesture (GestureEvent::kLongPress, gesturePriority);
	if(wantsSwipeH () || wantsSwipe ())
		handler->addRequiredGesture (GestureEvent::kSwipe|GestureEvent::kHorizontal, gesturePriority);
	if(wantsSwipeV () || wantsSwipe ())
		handler->addRequiredGesture (GestureEvent::kSwipe|GestureEvent::kVertical, gesturePriority);
	return handler;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TriggerView::onDragEnter (const DragEvent& event)
{
	if(style.isCustomStyle (Styles::kTriggerViewBehaviorSwallowDrag))
	{
		class DragSwallower: public Object,
							 public AbstractDragHandler
		{
		public:
			// IDragHandler
			tbool CCL_API isNullHandler () const override { return true; }
			CLASS_INTERFACE (IDragHandler, Object)
		};

		AutoPtr<IDragHandler> nullHandler = NEW DragSwallower;
		event.session.setDragHandler (nullHandler);

		if(event.session.getSource () == this->asUnknown ()) // no stop sign if dragged from this view
			event.session.setResult (IDragSession::kDropMove);
		else
			event.session.setResult (IDragSession::kDropNone);
		return true;
	}
	else
		return SuperClass::onDragEnter (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TriggerView::onContextMenu (const ContextMenuEvent& event)
{
	if(wantsContextMenu ())
	{
		EventStateGuard scope (eventState);
		eventState.contextMenuEvent = &event;

		signal (Message (kOnContextMenu));

		if(eventState.eventHandled)
			return true;

		return false;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_PROPERTY_NAMES (TriggerView)
	DEFINE_PROPERTY_NAME ("touchDelegate") ///< view that creates a touch handler; must only be set in an "onTouch" event
	DEFINE_PROPERTY_NAME ("mouseDelegate") ///< view that creates a mouse handler; must only be set in an "onMouseDown", "onDrag", "onSingleClick" or "onDoubleClick" event
	DEFINE_PROPERTY_NAME ("contextID") ///< id that describes context menu usage; must only be set in an "onContextMenu" event
	DEFINE_PROPERTY_NAME ("eventHandled") ///< set to 1 in "onMouseDown" or "onSingleClick" to swallow the current mouse event
	DEFINE_PROPERTY_NAME ("ignoresFocus") ///< if set to 1, clicking on this view does not steal another view's focus
END_PROPERTY_NAMES (TriggerView)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TriggerView::setProperty (MemberID propertyId, const Variant& var)
{
	if(propertyId == "touchDelegate" || propertyId == "mouseDelegate")
	{
		eventState.delegateView = unknown_cast<View> (var);
		return true;
	}
	if(propertyId == "contextID")
	{
		if(eventState.contextMenuEvent)
			eventState.contextMenuEvent->contextMenu.setContextID (MutableCString (var.asString ()));
		return true;
	}
	if(propertyId == "contextMenuAlign")
	{
		// adjust context menu position specified via alignment flags
		MutableCString pos (var.asString ());
		if(!pos.isEmpty () && eventState.contextMenuEvent)
		{
			Point where (eventState.contextMenuEvent->where);
			Alignment align (Core::EnumInfo::parseMultiple (pos, SkinElements::AlignElement::alignStyles));

			switch(align.getAlignH ())
			{
			case Alignment::kLeft:
				where.x = 0;
				break;

			case Alignment::kRight:
				where.x = getWidth ();
				break;

			case Alignment::kHCenter:
				where.x = getWidth () / 2;
				break;
			}

			switch(align.getAlignV ())
			{
			case Alignment::kTop:
				where.y = 0;
				break;

			case Alignment::kBottom:
				where.y = getHeight ();
				break;

			case Alignment::kVCenter:
				where.y = getHeight () / 2;
				break;
			}

			eventState.contextMenuEvent->setPosition (where);
		}
		return true;
	}
	if(propertyId == "eventHandled")
	{
		eventState.eventHandled = var.parseInt () != 0;
		return true;
	}
	if(propertyId == "ignoresFocus")
	{
		ignoresFocus (var.asBool ());
		return true;
	}
	return SuperClass::setProperty (propertyId, var);
}
