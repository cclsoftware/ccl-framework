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
// Filename    : ccl/app/controls/dragcontrol.cpp
// Description : Drag Control
//
//************************************************************************************************

#include "ccl/app/controls/dragcontrol.h"

#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/iuserinterface.h"
#include "ccl/public/gui/framework/ipresentable.h"
#include "ccl/public/gui/framework/ihelpmanager.h"
#include "ccl/public/gui/graphics/igraphics.h"
#include "ccl/public/guiservices.h" 
#include "ccl/public/plugservices.h"

using namespace CCL;

//************************************************************************************************
// DragControl
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (DragControl, UserControl)

//////////////////////////////////////////////////////////////////////////////////////////////////

DragControl::DragControl (RectRef size, StyleRef style, StringRef title)
: UserControl (size, style, title),
  cursorName ("GrabCursor"),
  isArmed (false),
  modifier (0),
  helpCollection (nullptr),
  paddingX (0),
  paddingY (0),
  retriggerTooltip (true)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

DragControl::~DragControl ()
{
	safe_release (helpCollection);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DragControl::updateStyle ()
{
	paddingX = 0;
	paddingY = 0;
	
	const IVisualStyle& vs = getVisualStyle ();
	backgroundImage = vs.getBackgroundImage ();
	
	if(backgroundImage)
	{
		Rect src (0, 0, backgroundImage->getWidth (), backgroundImage->getHeight ());
		Rect dst (src);
		Rect client;
		paddingX = vs.getMetric (StyleID::kPaddingLeft, 0);
		paddingY = vs.getMetric (StyleID::kPaddingTop, -(vs.getMetric (StyleID::kPaddingBottom, 0)));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DragControl::canDrag (const MouseEvent& event)
{
	return (event.keys.getModifiers () & modifier) == modifier;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DragControl::beforeDrag (const MouseEvent& event)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DragControl::prepareDrag (CCL::IDragSession& session)
{
	if(getSource ())
		session.setSource (getSource ());
	if(getDataItem ())
		session.getItems ().add (getDataItem (), true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DragControl::attached (IView* parent)
{
	// save original tooltip
	originalTooltip = getTooltip ();
	updateStyle ();
	
	SuperClass::attached (parent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DragControl::checkArmed (const MouseEvent& event)
{
	bool armed = canDrag (event);

	if(armed != isArmed)
	{
		isArmed = armed;
		if(armed)
		{
			setCursor (getTheme ().getCursor (cursorName));

			if(!dragTooltip.isEmpty ())
			{
				setTooltip (dragTooltip);
				if(retriggerTooltip)
					System::GetGUI ().retriggerTooltip (*this);
			}
		}
		else
		{
			setCursor (nullptr);

			if(!dragTooltip.isEmpty ())
			{
				setTooltip (originalTooltip);
				System::GetGUI ().hideTooltip ();
			}
		}
	}
	
	return armed; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DragControl::doDrag (int inputDevice)
{
	AutoPtr<IDragSession> session = ccl_new<IDragSession> (CCL::ClassID::DragSession);
	ASSERT (session != nullptr)
	session->setInputDevice (inputDevice);

	prepareDrag (*session);

	setMouseState (IView::kMouseDown);
	
	setCursor (nullptr);
	session->drag ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DragControl::updateHelp (const MouseEvent& event)
{
	if(getDragTooltip ().isEmpty ()
	|| !System::GetHelpManager ().hasInfoViewers ()
	|| System::GetGUI ().isDragActive ())
		return;

	UnknownPtr<IPresentable> info;
	if(event.eventType != MouseEvent::kMouseLeave)
	{
		if(helpCollection == nullptr)
			helpCollection = ccl_new<IHelpInfoCollection> (CCL::ClassID::HelpInfoCollection);
		
		MutableCString helpId (dragTooltip);
		AutoPtr<IHelpInfoBuilder> helpInfo = return_shared (helpCollection->getInfo (helpId));
		if(helpInfo == nullptr)
		{
			helpInfo = ccl_new<IHelpInfoBuilder> (CCL::ClassID::HelpInfoBuilder);
			helpInfo->addOption (getModifier () | KeyState::kDrag, nullptr, dragTooltip);
			helpCollection->addInfo (helpId, helpInfo);
		}
		info = helpInfo;
	}
	System::GetHelpManager ().showInfo (info);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DragControl::onMouseEnter (const MouseEvent& event)
{
	setMouseState (IView::kMouseOver);
	checkArmed (event);
	updateHelp (event);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DragControl::onMouseMove (const MouseEvent& event)
{
	setMouseState (IView::kMouseOver);
	checkArmed (event);
	updateHelp (event);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DragControl::onMouseLeave (const MouseEvent& event)
{
	setMouseState (IView::kMouseNone);

	isArmed = false;
	if(!dragTooltip.isEmpty ())
		setTooltip (originalTooltip);
	updateHelp (event);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DragControl::onMouseDown (const MouseEvent& event)
{
	if(SuperClass::onMouseDown (event))
		return true;
	
	if(checkArmed (event))
	{
		beforeDrag (event);

		if(detectDrag (event))
		{
			doDrag (IDragSession::kMouseInput);
			return true;
		}
	}
	else
		System::GetGUI ().retriggerTooltip (*this);
	
	return false;
}

////////////////////////////////////////////////////////////////////////

bool DragControl::onGesture (const GestureEvent& event)
{
	if((event.getType () == GestureEvent::kLongPress || event.getType () == GestureEvent::kSwipe)
		&& event.getState () == GestureEvent::kBegin)
    {
		beforeDrag (MouseEvent (AbstractTouchMouseHandler::makeMouseEvent (MouseEvent::kMouseDown, event)));

		doDrag (IDragSession::kTouchInput);
		return true;
    }
    return SuperClass::onGesture (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITouchHandler* CCL_API DragControl::createTouchHandler (const TouchEvent& event)
{
	if(const TouchInfo* touch = event.touches.getTouchInfoByID (event.touchID))
	{
		// want to start dragging on long press
		GestureHandler* handler = NEW GestureHandler (*this);
		handler->addRequiredGesture (GestureEvent::kLongPress, GestureEvent::kPriorityHigh);
		handler->addRequiredGesture (GestureEvent::kSwipe, GestureEvent::kPriorityNormal);
		return handler;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DragControl::draw (const DrawEvent& event)
{
	SuperClass::draw (event);

	if(backgroundImage)
	{
		Rect src (0, 0, backgroundImage->getWidth (), backgroundImage->getHeight ());
		Rect dst (src);
		Rect client;
		if (paddingX != 0 || paddingY != 0)
			dst.offset (paddingX, paddingY);
		else
			dst.center (getClientRect (client));
		
		MutableCString themeName = ThemeNames::kNormal;
		if(isArmed)
		{
			if(getMouseState () == IView::kMouseDown)
				themeName = ThemeNames::kPressed;
			if(getMouseState () == IView::kMouseOver)
				themeName = ThemeNames::kMouseOver;
		}
		
		IImage::Selector (backgroundImage, themeName);
		event.graphics.drawImage (backgroundImage, src, dst);
	}
}
