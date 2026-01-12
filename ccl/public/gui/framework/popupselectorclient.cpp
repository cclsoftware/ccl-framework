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
// Filename    : ccl/public/gui/framework/popupselectorclient.cpp
// Description : Popup Selector Client base class
//
//************************************************************************************************

#include "ccl/public/gui/framework/popupselectorclient.h"

#include "ccl/public/gui/framework/guievent.h"
#include "ccl/public/gui/framework/iwindow.h"
#include "ccl/public/gui/framework/iview.h"
#include "ccl/public/gui/framework/viewbox.h"
#include "ccl/public/gui/framework/iscrollview.h"

using namespace CCL;

//************************************************************************************************
// PopupSelectorClient
//************************************************************************************************

PopupSelectorClient::PopupSelectorClient (int flags)
 : flags (flags),
   behavior (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API PopupSelectorClient::createPopupView (SizeLimit& limits)
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PopupSelectorClient::attached (IWindow& popupWindow)
{
	flags &= ~kIgnoreMouseUp; // reset
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PopupSelectorClient::isIgnoringMouseClick () const
{
	ASSERT ((flags & kIgnoreMouseUp) == 0 || (acceptOnDoubleClick () && acceptOnMouseUp ()))
	return (flags & kIgnoreMouseUp) != 0; // see below
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PopupSelectorClient::Result CCL_API PopupSelectorClient::onMouseDown (const MouseEvent& event, IWindow& popupWindow)
{
	if(acceptOnMouseDown () && hasPopupResult () && event.keys.isSet (KeyState::kRButton) == false)
		return kOkay;

	if(acceptOnDoubleClick () && acceptOnMouseUp ())
	{
		// we received a mouse down, that means that the user did not "drag" into the menu; in this case we want the popup to stay open
		flags |= kIgnoreMouseUp;
	}

	return kIgnore;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PopupSelectorClient::Result CCL_API PopupSelectorClient::onMouseUp (const MouseEvent& event, IWindow& popupWindow)
{
	if(flags & kIgnoreMouseUp)
		return kIgnore;

	if(acceptOnMouseUp () && hasPopupResult () && event.keys.isSet (KeyState::kRButton) == false)
		return kOkay;

	return kIgnore;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PopupSelectorClient::Result CCL_API PopupSelectorClient::onKeyDown (const KeyEvent& event)
{
	switch(event.vKey)
	{
		case VKey::kEscape:
			return kCancel;

		case VKey::kReturn:
		case VKey::kEnter:
			if(hasPopupResult ())
				return kOkay;
			// through:
		default:
			return kIgnore;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PopupSelectorClient::Result CCL_API PopupSelectorClient::onKeyUp (const KeyEvent& event)
{
	return kIgnore;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PopupSelectorClient::Result CCL_API PopupSelectorClient::onEventProcessed (const GUIEvent& event, IWindow& popupWindow, IView* view)
{
	// after a mousedown was processed, check if it's a doubleclick that should close the popup
	auto mouseEvent = event.as<MouseEvent> ();
	if(acceptOnDoubleClick ()
		&& mouseEvent
		&& event.eventType == MouseEvent::kMouseDown
		&& hasPopupResult ())
	{
		UnknownPtr<IView> view (&popupWindow);
		if(view->detectDoubleClick (*mouseEvent))
			return kOkay;
	}
	else if(auto gestureEvent = event.as<GestureEvent> ())
	{
		// interpret mouse-oriented flags for touch gestures
		// hmm, only works if the gesture was requested anyway by some view's handler; but adding an own handler to force detection (lowest prio) could alter the detection result
		switch(gestureEvent->getType ())
		{
		case GestureEvent::kSingleTap:
			if(gestureEvent->getState () == GestureEvent::kBegin)
				if(acceptOnMouseDown () && hasPopupResult ())
					return kOkay;
			break;

		case GestureEvent::kDoubleTap:
			if(gestureEvent->getState () == GestureEvent::kBegin)
				if(acceptOnDoubleClick () && hasPopupResult ())
					return kOkay;
			break;

		case GestureEvent::kSwipe:
		case GestureEvent::kLongPress:
			if(gestureEvent->getState () == GestureEvent::kEnd)
			{
				if(acceptAfterSwipe () && hasPopupResult ())
				{
					if(UnknownPtr<IScrollView> (view).isValid ())
						return kIgnore; // do not handle swipes again that were used to scroll

					return kOkay;
				}
			}
			break;
		}
	}
	return kIgnore;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PopupSelectorClient::onPopupClosed (Result result)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

int32 CCL_API PopupSelectorClient::getPopupBehavior ()
{
	return behavior;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PopupSelectorClient::mouseWheelOnSource (const MouseWheelEvent& event, IView* source)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITouchHandler* CCL_API PopupSelectorClient::createTouchHandler (const TouchEvent& event, IWindow* window)
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PopupSelectorClient::setCursorPosition (PointRef where)
{
	cursorPosition = where;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PopupSelectorClient::setToDefault ()
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PopupSelectorClient::checkPopupLimits (IView* view, const SizeLimit& limits)
{
	if(view)
	{
		SizeLimit sizeLimits (limits);
		if(view->hasExplicitSizeLimits ())
			sizeLimits.include (view->getSizeLimits ());

		view->setSizeLimits (sizeLimits);

		Rect r (view->getSize ());
		sizeLimits.makeValid (r);
		view->setSize (r);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PopupSelectorClient::hasPopupResult ()
{
	return true;
}

//************************************************************************************************
// SimplePopupSelectorClient
//************************************************************************************************

SimplePopupSelectorClient::SimplePopupSelectorClient ()
: popupResult (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SimplePopupSelectorClient::hasPopupResult ()
{
	return popupResult;
}
