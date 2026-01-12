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
// Filename    : ccl/app/controls/usercontrol.cpp
// Description : User Control
//
//************************************************************************************************

#include "ccl/app/controls/usercontrol.h"

#include "ccl/public/gui/framework/ipresentable.h"
#include "ccl/public/gui/framework/ihelpmanager.h"
#include "ccl/public/guiservices.h"
#include "ccl/public/plugservices.h"

using namespace CCL;

//************************************************************************************************
// UserControl
//************************************************************************************************

DEFINE_CLASS_HIDDEN (UserControl, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

UserControl::UserControl (RectRef size, StyleRef style, StringRef title)
: customAccessibilityProvider (nullptr)
{
	construct (size, style, title);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UserControl::~UserControl ()
{
	safe_release (customAccessibilityProvider);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserControl::resetSizeLimits ()
{
	setSizeLimits (SizeLimit ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AbstractTouchMouseHandler* UserControl::wrapMouseHandler (const TouchEvent& event)
{
	IView* view = *this;
	MouseEvent mouseEvent (AbstractTouchMouseHandler::makeMouseEvent (MouseEvent::kMouseDown, event, *view));

	AutoPtr<IMouseHandler> mouseHandler (createMouseHandler (mouseEvent));
	if(mouseHandler)
		return NEW TouchMouseHandler (mouseHandler, *this);

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAccessibilityProvider* CCL_API UserControl::getCustomAccessibilityProvider () const
{
	return customAccessibilityProvider;
}

//************************************************************************************************
// UserControl::MouseHandler
//************************************************************************************************

DEFINE_CLASS_HIDDEN (UserControl::MouseHandler, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

UserControl::MouseHandler::MouseHandler (UserControl* control, int flags)
: AbstractMouseHandler (flags),
  control (control)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

UserControl* UserControl::MouseHandler::getControl () const
{
	return control;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API UserControl::MouseHandler::begin (const MouseEvent& event)
{
	AbstractMouseHandler::begin (event);

	// try to get help and show it
	if(System::GetHelpManager ().hasInfoViewers ())
	{
		AutoPtr<IHelpInfoBuilder> builder (ccl_new<IHelpInfoBuilder> (ClassID::HelpInfoBuilder));
		if(builder)
			if(getHelp (*builder))
				System::GetHelpManager ().showInfo (UnknownPtr<IPresentable> (builder));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UserControl::MouseHandler::getHelp (IHelpInfoBuilder& helpInfo)
{
	return false;
}

//************************************************************************************************
// UserControl::DragHandler
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (UserControl::DragHandler, CCL::DragHandler)

//////////////////////////////////////////////////////////////////////////////////////////////////

UserControl::DragHandler::DragHandler (UserControl& control)
: CCL::DragHandler (control),
  control (control)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

UserControl& UserControl::DragHandler::getControl () const
{
	return control;
}

//************************************************************************************************
// UserControl::TouchMouseHandler
//************************************************************************************************

UserControl::TouchMouseHandler::TouchMouseHandler (IMouseHandler* mouseHandler, UserControl& control)
: AbstractTouchMouseHandler (mouseHandler, control),
  control (control)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

UserControl& UserControl::TouchMouseHandler::getControl () const
{
	return control;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API UserControl::TouchMouseHandler::trigger (const TouchEvent& event)
{
	tbool result = AbstractTouchHandler::trigger (event);

	if(result && mouseHandler)
	{
		// create autoScroller if required
		if(!autoScroller && (mouseHandler->getFlags () & IMouseHandler::kAutoScroll))
		{
			autoScroller = ccl_new<IAutoScroller> (CCL::ClassID::AutoScroller);
			autoScroller->construct (view);
		}

		// trigger autoScroller with screen position of touch
		if(autoScroller)
			if(const TouchInfo* touch = event.touches.getTouchInfoByID (event.touchID))
			{
				Point where (touch->where);
				view->windowToClient (where);
				view->clientToScreen (where);
				autoScroller->trigger (where, mouseHandler->getFlags ());
			}
	}
	return result;
}

//************************************************************************************************
// UserControl::TouchHandler
//************************************************************************************************

UserControl::TouchHandler::TouchHandler (UserControl& control)
: AbstractTouchHandler (control),
  control (control)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

UserControl& UserControl::TouchHandler::getControl () const
{
	return control;
}

//************************************************************************************************
// UserControl::GestureHandler
//************************************************************************************************

UserControl::GestureHandler::GestureHandler (UserControl& control)
: AbstractTouchHandler (control),
  control (control)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API UserControl::GestureHandler::onGesture (const GestureEvent& event)
{
	GestureEvent e2 (event);
	control.windowToClient (e2.where);
	return control.onGesture (e2);
}

//********************************************************************************************
// UserControl::AccessibilityProvider
//********************************************************************************************

UserControl::AccessibilityProvider::AccessibilityProvider (UserControl& control)
: control (control)
{}
