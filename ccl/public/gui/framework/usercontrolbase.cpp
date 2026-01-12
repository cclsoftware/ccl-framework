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
// Filename    : ccl/public/gui/framework/usercontrolbase.cpp
// Description : Abstract User Control
//
//************************************************************************************************

#include "ccl/public/gui/framework/usercontrolbase.h"
#include "ccl/public/gui/framework/isprite.h"

#include "ccl/public/base/variant.h"
#include "ccl/public/base/iobject.h"
#include "ccl/public/plugservices.h"

using namespace CCL;

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* GetViewInterfaceUpwards (UIDRef iid, IView* view)
{
	IUnknown* iface = nullptr;

	IView* v = view;
	while(v)
	{
		v->queryInterface (iid, (void**)&iface);
		if(iface)
		{
			iface->release ();
			return iface;
		}

		UnknownPtr<IUserControlHost> uch (v);
		if(uch)
		{
			uch->getUserControl ()->queryInterface (iid, (void**)&iface);
			if(iface)
			{
				iface->release ();
				return iface;
			}
		}

		v = v->getParentView ();
	}
	return nullptr;
}

} // namespace CCL

//************************************************************************************************
// AbstractUserControl
//************************************************************************************************

void AbstractUserControl::construct (RectRef size, StyleRef style, StringRef title)
{
	ASSERT (view == nullptr)
	view = ccl_new<IView> (ClassID::UserControlHost);
	ASSERT (view != nullptr)
	UnknownPtr<IUserControlHost> (view)->setUserControl (this);
	release (); // we are now owned by the framework view

	ViewBox::construct (size, style, title);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AbstractUserControl::dispose ()
{
	if(view)
		view->release ();
	else
		release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API AbstractUserControl::onViewEvent (const GUIEvent& event)
{
	bool result = false;
	switch(event.eventClass)
	{
	// *** View Events ***
	case GUIEvent::kViewEvent :
		switch(event.eventType)
		{
		case ViewEvent::kDraw :
			draw ((DrawEvent&)event);
			result = true;
			break;

		case ViewEvent::kViewsChanged :
			onViewsChanged ();
			break;

		case ViewEvent::kAttached :
			attached (((ViewParentEvent&)event).parent);
			result = true;
			break;

		case ViewEvent::kRemoved :
			removed (((ViewParentEvent&)event).parent);
			result = true;
			break;

		case ViewEvent::kActivate :
			onActivate (true);
			result = true;
			break;

		case ViewEvent::kDeactivate :
			onActivate (false);
			result = true;
			break;

		case ViewEvent::kSized :
			onSize (((ViewSizeEvent&)event).delta);
			result = true;
			break;

		case ViewEvent::kMoved :
			onMove (((ViewSizeEvent&)event).delta);
			result = true;
			break;

		case ViewEvent::kChildSized :
			onChildSized (((ViewSizeEvent&)event).child, ((ViewSizeEvent&)event).delta);
			result = true;
			break;

		case ViewEvent::kVisualStyleChanged :
			onVisualStyleChanged ();
			result = true;
			break;
		}
		break;

	// *** Mouse Events ***
	case GUIEvent::kMouseEvent :
		switch(event.eventType)
		{
		case MouseEvent::kMouseDown :
			result = onMouseDown ((MouseEvent&)event);
			break;

		case MouseEvent::kMouseUp :
			result = onMouseUp ((MouseEvent&)event);
			break;

		case MouseEvent::kMouseEnter :
			result = onMouseEnter ((MouseEvent&)event);
			break;

		case MouseEvent::kMouseMove :
			result = onMouseMove ((MouseEvent&)event);
			break;

		case MouseEvent::kMouseLeave :
			result = onMouseLeave ((MouseEvent&)event);
			break;
		}
 		break;

	case GUIEvent::kMouseWheelEvent :
		result = onMouseWheel ((MouseWheelEvent&)event);
		break;

	case GUIEvent::kContextMenuEvent :
		result = onContextMenu ((ContextMenuEvent&)event);
		break;

	case GUIEvent::kTooltipEvent :
		result = onTrackTooltip ((TooltipEvent&)event);
		break;

	// *** Multitouch Events ***
	case GUIEvent::kGestureEvent :
		result = onGesture ((GestureEvent&)event);
		break;

	// *** Keyboard Events ***
	case GUIEvent::kFocusEvent :
		result = onFocus ((FocusEvent&)event);
		break;

	case GUIEvent::kKeyEvent :
		switch(event.eventType)
		{
		case KeyEvent::kKeyDown :
			result = onKeyDown ((KeyEvent&)event);
			break;
		case KeyEvent::kKeyUp :
			result = onKeyUp ((KeyEvent&)event);
			break;
		}
		break;

	// *** Drag Events ***
	case GUIEvent::kDragEvent :
		switch(event.eventType)
		{
		case DragEvent::kDragEnter :
			result = onDragEnter ((DragEvent&)event);
			break;

		case DragEvent::kDragOver :
			result = onDragOver ((DragEvent&)event);
			break;

		case DragEvent::kDragLeave :
			result = onDragLeave ((DragEvent&)event);
			break;

		case DragEvent::kDrop :
			result = onDrop ((DragEvent&)event);
			break;
		}
		break;
		
	// *** Other ***	
	case GUIEvent::kDisplayChangedEvent :
		onDisplayPropertiesChanged (((DisplayChangedEvent&)event));
		result = true;
		break;

	case GUIEvent::kColorSchemeEvent :
		onColorSchemeChanged ((ColorSchemeEvent&)event);
		result = true;
		break;
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMouseHandler* CCL_API AbstractUserControl::createMouseHandler (const MouseEvent& event)
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITouchHandler* CCL_API AbstractUserControl::createTouchHandler (const TouchEvent& event)
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDragHandler* CCL_API AbstractUserControl::createDragHandler (const DragEvent& event)
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API AbstractUserControl::getController () const
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAccessibilityProvider* CCL_API AbstractUserControl::getCustomAccessibilityProvider () const
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AbstractUserControl::onViewsChanged ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AbstractUserControl::attached (IView* parent)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AbstractUserControl::removed (IView* parent)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AbstractUserControl::onActivate (bool state)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AbstractUserControl::onSize (PointRef delta)
{
	// default behavior is to resize child views according to their size mode
	getChildren ().delegateEvent (ViewSizeEvent (delta, ViewEvent::kSized));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AbstractUserControl::onMove (PointRef delta)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AbstractUserControl::onChildSized (IView* child, PointRef delta)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AbstractUserControl::onVisualStyleChanged ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AbstractUserControl::onDisplayPropertiesChanged (const DisplayChangedEvent& event)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AbstractUserControl::onColorSchemeChanged (const ColorSchemeEvent& event)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AbstractUserControl::draw (const DrawEvent& event)
{
	// draw child views
	getChildren ().delegateEvent (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AbstractUserControl::onMouseDown (const MouseEvent& event)
{
	// delegate mouse down to child views
	return getChildren ().delegateEvent (event) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AbstractUserControl::onMouseUp (const MouseEvent& event)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AbstractUserControl::onMouseWheel (const MouseWheelEvent& event)
{
	// delegate mouse wheel to child views
	return getChildren ().delegateEvent (event) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AbstractUserControl::onMouseEnter (const MouseEvent& event)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AbstractUserControl::onMouseMove (const MouseEvent& event)
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AbstractUserControl::onMouseLeave (const MouseEvent& event)
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AbstractUserControl::onContextMenu (const ContextMenuEvent& event)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AbstractUserControl::onTrackTooltip (const TooltipEvent& event)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AbstractUserControl::onGesture (const GestureEvent& event)
{
	// delegate gesture to child views
	return getChildren ().delegateEvent (event) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AbstractUserControl::setMouseHandler (IMouseHandler* handler)
{
	UnknownPtr<IUserControlHost> (view)->setMouseHandler (handler);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AbstractUserControl::onFocus (const FocusEvent& event)
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AbstractUserControl::onKeyDown (const KeyEvent& event)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AbstractUserControl::onKeyUp (const KeyEvent& event)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AbstractUserControl::onDragEnter (const DragEvent& event)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AbstractUserControl::onDragOver (const DragEvent& event)
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AbstractUserControl::onDragLeave (const DragEvent& event)
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AbstractUserControl::onDrop (const DragEvent& event)
{
	return false;
}

//************************************************************************************************
// AbstractMouseHandler
//************************************************************************************************

int CCL_API AbstractMouseHandler::getFlags () const
{
	return flags;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AbstractMouseHandler::begin (const MouseEvent& event)
{
	first = previous = current = event;
	onBegin ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API AbstractMouseHandler::trigger (const MouseEvent& event, int moveFlags)
{
	current = event;
	bool result = onMove (moveFlags);
	previous = current;
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API AbstractMouseHandler::triggerKey (const KeyEvent& event)
{
	current.keys = event.state.keys;
	return onKeyEvent (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AbstractMouseHandler::finish (const MouseEvent& event, tbool canceled)
{
	if(!canceled)
		current = event;
	onRelease (canceled != 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AbstractMouseHandler::onBegin ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AbstractMouseHandler::onMove (int moveFlags)
{
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void AbstractMouseHandler::onRelease (bool canceled)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AbstractMouseHandler::onKeyEvent (const KeyEvent& event)
{
	return false;
}
