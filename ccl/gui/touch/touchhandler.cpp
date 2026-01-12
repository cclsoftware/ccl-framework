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
// Filename    : ccl/gui/touch/touchhandler.cpp
// Description : Touch Handler
//
//************************************************************************************************

#include "ccl/gui/touch/touchhandler.h"

#include "ccl/gui/controls/control.h"
#include "ccl/gui/windows/window.h"
#include "ccl/gui/popup/popupselector.h"

#include "ccl/public/systemservices.h"

using namespace CCL;

//************************************************************************************************
// TouchHandler
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (TouchHandler, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

TouchHandler::TouchHandler (IView* view)
: AbstractTouchHandler (view)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

TouchHandler::~TouchHandler ()
{
	cancelSignals ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TouchHandler::setProperty (MemberID propertyId, const Variant& var)
{
	if(propertyId == "gesturePriority")
	{
		// apply priority to all handled gestures
		int priority = var.asInt ();
		
		VectorForEach (getRequiredGestures (), GestureItem&, item)
			item.priority = priority;
		EndFor
		return true;
	}
	return SuperClass::setProperty (propertyId, var);
}


//************************************************************************************************
// GestureHandler
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (GestureHandler, TouchHandler)

//////////////////////////////////////////////////////////////////////////////////////////////////

GestureHandler::GestureHandler (View* view)
: TouchHandler (view)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

GestureHandler::GestureHandler (View* view, int gestureType, int priority)
: TouchHandler (view)
{
	addRequiredGesture (gestureType, priority);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API GestureHandler::onGesture (const GestureEvent& event)
{
	if(View* view = unknown_cast<View> (getView ()))
	{
		if(view->isAttached ()) // position is wrong otherwise
		{
			GestureEvent e2 (event);
			Point p (e2.where);
			
			view->windowToClient (p);
			e2.setPosition (p);
			
			return view->onGesture (e2);
		}
	}
	return false;
}

//************************************************************************************************
// TouchMouseHandler
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (TouchMouseHandler, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchMouseHandler::applyGesturePriorities (AbstractTouchHandler& handler, View* view)
{
	int prioH = GestureEvent::kPriorityNormal;
	int prioV = GestureEvent::kPriorityNormal;
	if(Control* control = ccl_cast<Control> (view))
	{
		// boost priority of main direction
		if(control->getStyle ().isHorizontal ())
			prioH = GestureEvent::kPriorityHigh;
		if(control->getStyle ().isVertical ())
			prioV = GestureEvent::kPriorityHigh;
	}
	handler.addRequiredGesture (GestureEvent::kSingleTap);
	handler.addRequiredGesture (GestureEvent::kLongPress);
	handler.addRequiredGesture (GestureEvent::kSwipe|GestureEvent::kHorizontal, prioH);
	handler.addRequiredGesture (GestureEvent::kSwipe|GestureEvent::kVertical, prioV);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TouchMouseHandler::TouchMouseHandler (MouseHandler* mouseHandler, View* view)
: AbstractTouchMouseHandler (mouseHandler, view)
{
	applyGesturePriorities (*this, view);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TouchMouseHandler::~TouchMouseHandler ()
{
	cancelSignals ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TouchMouseHandler::onMove (const TouchEvent& event)
{
	// mouse handler might have switched to another view
	if(MouseHandler* frameworkHandler = unknown_cast<MouseHandler> (mouseHandler))
		if(frameworkHandler->getView () != getView ())
			take_shared<IView> (view, frameworkHandler->getView ());

	return AbstractTouchMouseHandler::onMove (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchMouseHandler::onRelease (const TouchEvent& event, bool canceled)
{
	SharedPtr<MouseHandler> frameworkHandler (unknown_cast<MouseHandler> (mouseHandler));

	AbstractTouchMouseHandler::onRelease (event, canceled);

	if(frameworkHandler)
		frameworkHandler->onRelease (canceled);
}

//************************************************************************************************
// ViewTouchHandler
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (ViewTouchHandler, TouchHandler)

//////////////////////////////////////////////////////////////////////////////////////////////////

ViewTouchHandler::ViewTouchHandler (View* view)
: TouchMouseHandler (nullptr, view)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewTouchHandler::onBegin (const TouchEvent& event)
{
	MouseEvent mouseEvent (AbstractTouchMouseHandler::makeMouseEvent (MouseEvent::kMouseDown, event, *view));	
	if(View* view = unknown_cast<View> (getView ()))
	{
		ASSERT (!view->getWindow () || view->getWindow ()->getMouseHandler () == nullptr)

		view->onMouseDown (mouseEvent);

		if(Window* window = view->getWindow ())
		{
			// take over a mouse handler that might be created during onMouseDown (will feed it with trigger calls on move)
			MouseHandler* mouseHandler = window->detachMouseHandler ();
			if(mouseHandler)
				this->mouseHandler = mouseHandler;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewTouchHandler::onRelease (const TouchEvent& event, bool canceled)
{
	SuperClass::onRelease (event, canceled);
	
	if(!mouseHandler)
	{
		MouseEvent mouseEvent (AbstractTouchMouseHandler::makeMouseEvent (MouseEvent::kMouseUp, event, *view));
		if(View* view = unknown_cast<View> (getView ()))
			view->onMouseUp (mouseEvent);
	}
}

//************************************************************************************************
// NullTouchHandler
//************************************************************************************************

NullTouchHandler::NullTouchHandler (IView* view)
: TouchHandler (view)
{
	addRequiredGesture (GestureEvent::kSingleTap, GestureEvent::kPriorityHighest);
	addRequiredGesture (GestureEvent::kLongPress, GestureEvent::kPriorityHighest);
	addRequiredGesture (GestureEvent::kSwipe, GestureEvent::kPriorityHighest);
	addRequiredGesture (GestureEvent::kZoom, GestureEvent::kPriorityHighest);	
}

//************************************************************************************************
// RemotePopupTouchHandler::RemoteTouchEvent
//************************************************************************************************

RemotePopupTouchHandler::RemoteTouchEvent::RemoteTouchEvent (const TouchEvent& event, PointRef offset)
: TouchEvent (touches, event.eventType)
{
	touchID = event.touchID;
			
	const TouchInfo* touch = event.touches.getTouchInfoByID (event.touchID);
	if(touch)
	{
		TouchInfo popupTouch (*touch);
		popupTouch.where.offset (offset);
		position = popupTouch.where;
		touches.add (popupTouch);
	}
}

//************************************************************************************************
// RemotePopupTouchHandler
//************************************************************************************************

RemotePopupTouchHandler::RemotePopupTouchHandler (View* sourceView, bool _overridePosition)
: TouchHandler (sourceView),
  windowOffset (-kMaxCoord, -kMaxCoord),
  minMoveDistance (0),
  flags (0),
  simulatedGesture (-1),
  startTime (0)
{
	TouchMouseHandler::applyGesturePriorities (*this, sourceView);

	overridePosition (_overridePosition);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

RemotePopupTouchHandler::~RemotePopupTouchHandler ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* RemotePopupTouchHandler::getSourceView () const
{
	return unknown_cast<View> (getView ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Point RemotePopupTouchHandler::getTouchPosition (const TouchEvent& event)
{
	const TouchInfo* touch = event.touches.getTouchInfoByID (event.touchID);
	return touch ? touch->where : Point (-kMaxCoord, -kMaxCoord);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RemotePopupTouchHandler::determineWindowOffset ()
{
	if(windowOffset == Point (-kMaxCoord, -kMaxCoord))
	{
		windowOffset = Point ();

		View* sourceView = getSourceView ();
		Window* sourceWindow = sourceView ? sourceView->getWindow () : nullptr;
		Window* popupWindow = getPopupWindow ();
		ASSERT (sourceWindow && popupWindow)
		if(sourceWindow && popupWindow)
		{
			sourceWindow->clientToScreen (windowOffset);
			popupWindow->screenToClient (windowOffset);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PopupSelector* RemotePopupTouchHandler::getPopupSelector () const
{
	CCL_NOT_IMPL ("RemotePopupTouchHandler::getPopupSelector ")
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RemotePopupTouchHandler::openPopup ()
{
	CCL_NOT_IMPL ("RemotePopupTouchHandler::openPopup")
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Window* RemotePopupTouchHandler::getPopupWindow () const
{
	PopupSelector* popupSelector = getPopupSelector ();
	return popupSelector ? unknown_cast<Window> (popupSelector->getCurrentWindow ()) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITouchHandler* RemotePopupTouchHandler::createTouchHandlerInPopup (const TouchEvent& event, Window& popupWindow)
{
	// find view in popup: use touch position if inside the popup, fallback to popup center
	Point windowPos (getTouchPosition (event));
	if(!popupWindow.getSize ().pointInside (windowPos))
	{
		windowPos = popupWindow.getSize ().getCenter ();
		popupWindow.screenToClient (windowPos);
	}

	if(auto view = popupWindow.findView (windowPos, true))
		if(ITouchHandler* handler = view->createTouchHandler (event))
			return handler;

	return popupWindow.createTouchHandler (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RemotePopupTouchHandler::createRemoteTouchhandler (const TouchEvent& event)
{
	if(!handlerChecked ())
	{
		handlerChecked (true);

		// let derived class create a touch handler for the popup window
		Window* popupWindow = getPopupWindow ();
		if(popupWindow)
		{
			determineWindowOffset ();

			RemoteTouchEvent remoteEvent (event, windowOffset);
			remoteEvent.eventType = TouchEvent::kBegin;

			remoteTouchHandler = createTouchHandlerInPopup (remoteEvent, *popupWindow);
			if(remoteTouchHandler)
			{
				remoteTouchHandler->begin (remoteEvent);

				if(!isAsyncPopup ())
				{
					// if the popup is a synchronous modal dialog, no gesture recognition has been setup yet (dialog blocks, we're stuck in onBegin)
					// as a workaround, we will simulate gesture events for the remoteTouchHandler
					// choose the first continuous one-finger gesture that the handler wants
					int i = 0;
					int gesture = 0;
					int priority = 0;
					while(remoteTouchHandler->getRequiredGesture (gesture, priority, i++))
						if(gesture == GestureEvent::kSwipe || gesture == GestureEvent::kLongPress)
						{
							simulatedGesture = gesture;
							break;
						}

					simulateRemoteGesture (GestureEvent::kBegin, event);
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RemotePopupTouchHandler::simulateRemoteGesture (int state, const TouchEvent& event)
{
	if(simulatedGesture >= 0)
	{
		GestureEvent remoteEvent (simulatedGesture | state, getTouchPosition (event));
		remoteEvent.setPosition (remoteEvent.where + windowOffset);

		remoteTouchHandler->onGesture (remoteEvent);
		forwardGestureProcessed (remoteEvent);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RemotePopupTouchHandler::forwardGestureProcessed (const GestureEvent& remoteEvent)
{
	if(Window* popupWindow = getPopupWindow ())
		popupWindow->onGestureProcessed (remoteEvent, getSourceView ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RemotePopupTouchHandler::openPopupInternal ()
{
	popupOpened (true);
	openPopup ();

	// check if popup is an asynchronous dialog (e.g. iOS); (otherwise popup is already closed here)
	PopupSelector* popupSelector = getPopupSelector ();
	isAsyncPopup (popupSelector && (popupSelector->isOpen () != 0));

	if(isAsyncPopup ())
		determineWindowOffset ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RemotePopupTouchHandler::onBegin (const TouchEvent& event)
{
	startTime = System::GetSystemTicks ();
	initialTouchPos = getTouchPosition (event);

	SharedPtr<Object> holder (this);

	if(openPopupImmediately ())
		openPopupInternal (); // show popup on first touch

	if(isAsyncPopup ()) 
		createRemoteTouchhandler (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RemotePopupTouchHandler::onMove (const TouchEvent& event)
{
	if(event.isHoverEvent ())
		return true;

	if(!hasMoved ())
	{
		Point currentPos (getTouchPosition (event));
		if(ccl_equals (currentPos.x, initialTouchPos.x, getMinMoveDistance ()) && ccl_equals (currentPos.y, initialTouchPos.y, getMinMoveDistance ()))
			return true;
		else
			hasMoved (true);
	}

	SharedPtr<Unknown> holder (this);

	if(!openPopupImmediately () && !popupOpened ())
	{
		openPopupInternal ();
		if(!isAsyncPopup ())
			return true;
	}

	createRemoteTouchhandler (event);

	if(remoteTouchHandler)
	{
		RemoteTouchEvent remoteEvent (event, windowOffset);
		remoteTouchHandler->trigger (remoteEvent);

		if(!isAsyncPopup ())
			simulateRemoteGesture (GestureEvent::kChanged, event);

		if(!wasInsidePopup ())
		{
			if(Window* popupWindow = getPopupWindow ())
			{
				Rect rect;
				popupWindow->getClientRect (rect);
				rect.contract (2);
				if(rect.pointInside (remoteEvent.getPosition ()))
					wasInsidePopup (true);
			}
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RemotePopupTouchHandler::onRelease (const TouchEvent& event, bool canceled)
{
	if(remoteTouchHandler)
	{
		if(!isAsyncPopup ())
			simulateRemoteGesture (GestureEvent::kEnd, event);

		remoteTouchHandler->finish (RemoteTouchEvent (event, windowOffset));
			
		if(overridePosition ())	// popup at "overridePosition" is used for direct manipulation only - close immediately
		{
			if(PopupSelector* popupSelector = getPopupSelector ())
				popupSelector->close ();
		}
		else
		{
			// keep popup open on a quick single tap, close after swipe / long press if the popup window was entered
			int64 now = System::GetSystemTicks ();
			if(now - startTime < 200)
				isSingleTap (true);

			if(!isSingleTap () && wasInsidePopup ())
				if(PopupSelector* popupSelector = getPopupSelector ())
					popupSelector->close ();
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API RemotePopupTouchHandler::onGesture (const GestureEvent& event)
{
	if(event.getState () == GestureEvent::kBegin)
	{
		if(event.getType() == GestureEvent::kSingleTap)
			isSingleTap (true);
		else if(event.getType() == GestureEvent::kLongPress)
		{
			if(openPopupOnLongPress () && !popupOpened ())
				openPopupInternal ();
		}
	}
		
	if(!isSingleTap () && remoteTouchHandler)
	{
		GestureEvent remoteEvent (event);
		remoteEvent.setPosition (remoteEvent.where + windowOffset);
		remoteTouchHandler->onGesture (remoteEvent);

		if(isAsyncPopup ())
			forwardGestureProcessed (remoteEvent);
	}

	return true;
}
