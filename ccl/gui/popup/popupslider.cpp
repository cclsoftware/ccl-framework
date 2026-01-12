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
// Filename    : ccl/gui/popup/popupslider.cpp
// Description : Popup Slider
//
//************************************************************************************************

#include "ccl/gui/popup/popupslider.h"
#include "ccl/gui/controls/popupbox.h"

#include "ccl/gui/controls/slider.h"
#include "ccl/gui/touch/touchhandler.h"
#include "ccl/gui/theme/themerenderer.h"
#include "ccl/gui/gui.h"

#include "ccl/base/message.h"

#include "ccl/public/gui/framework/iwindow.h"

using namespace CCL;

//************************************************************************************************
// PopupSlider
//************************************************************************************************

PopupSlider::PopupSlider (IParameter* parameter, StyleRef style)
: parameter (parameter),
  style (style),
  isOverridePosition (style.isCustomStyle (Styles::kPopupBoxBehaviorOverridePosition) ? true : false),
  hasTouchHandler (false),
  forceTouch (false),
  popupFormName (nullptr)
{
	restoreMousePos (style.isCustomStyle (Styles::kPopupBoxBehaviorKeepMousePos) ? false : true);
	wantsMouseUpOutside (true);

	parameter->retain ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
  
PopupSlider::~PopupSlider ()
{
	cancelSignals ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
  
int CCL_API PopupSlider::countParameters () const
{
	return 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* CCL_API PopupSlider::getParameterAt (int index) const
{
	return index == 0 ? parameter : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* CCL_API PopupSlider::findParameter (StringID name) const
{
	if(name == "parameter")
		return parameter;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PopupSlider::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "horizontal")
	{
		var = style.isHorizontal () ? 1 : 0;
		return true;
	}
	if(propertyId == "vertical")
	{
		var = style.isVertical () ? 1 : 0;
		return true;
	}
	if(propertyId == "clientName")
	{
		var = parameter->getName ();
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PopupSlider::setProperty (MemberID propertyId, const Variant& var)
{
	if(propertyId == "forceTouch")
	{
		forceTouch = var.asBool ();
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API PopupSlider::createPopupView (SizeLimit& limits)
{
	if(parameter->isEnabled () == false)
		return nullptr;

	ITheme* theme = ThemeSelector::currentTheme;
	ASSERT (theme)
	
	if(theme)
		if(IView* view = theme->createView (!popupFormName.isEmpty () ? popupFormName : "CCL/Standard.PopupSlider", this->asUnknown ()))
			return view;
	
	return NEW Slider (style.isHorizontal () ? Rect (0, 0, 100, 16) : Rect (0, 0, 16, 100), parameter, style);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITouchHandler* CCL_API PopupSlider::createTouchHandler (const TouchEvent& event, IWindow* window)
{
	View* view = unknown_cast<View> (window);
	if(Slider* slider = window ? findControl (view) : nullptr)
	{
		if(parameter->isEnabled () == false)
			return NEW NullTouchHandler (slider);

		slider->setMode (Styles::kSliderModeRelative);
		if(ITouchHandler* touchHandler = slider->createTouchHandler (event))
		{
			hasTouchHandler = true;
			return touchHandler;
		}

		if(MouseHandler* mouseHandler = slider->createMouseHandler (TouchMouseHandler::makeMouseEvent (MouseEvent::kMouseDown, event, *slider)))
		{
			hasTouchHandler = false;
			return NEW TouchMouseHandler (mouseHandler, slider);
		}
	}
	
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PopupSlider::setToDefault ()
{
	if(parameter->isEnabled ())
	{
		parameter->beginEdit ();
		parameter->setValue (parameter->getDefaultValue (), true);
		parameter->endEdit ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PopupSelectorClient::Result CCL_API PopupSlider::onEventProcessed (const GUIEvent& event, IWindow& popupWindow, IView* view)
{
	if(auto gestureEvent = event.as<GestureEvent> ())
	{
		if(gestureEvent->getState () == GestureEvent::kEnd)
		{
			// end of swipe / longpress gesture (touch released): close popup
			if((gestureEvent->getType () == GestureEvent::kSwipe || gestureEvent->getType () == GestureEvent::kLongPress) && acceptOnMouseUp ())
				return kOkay;
		}
	}
	return PopupSelectorClient::onEventProcessed (event, popupWindow, view);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PopupSlider::mouseWheelOnSource (const MouseWheelEvent& event, IView* source)
{
	Control::handleMouseWheel (event, parameter);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Slider* PopupSlider::findControl (View* parent)
{
	ForEachViewFast (*parent, view)
		Slider* control = ccl_cast<Slider> (view);
		if(control && control->getParameter () == parameter)
			return control;

		if(control = findControl (view))
			return control;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PopupSlider::attached (IWindow& popupWindow)
{
	if(isOverridePosition)
		overridePosition (popupWindow);

	// we don't know yet if touch or mouse
	hasTouchHandler = false;
	
	PopupSelectorClient::attached (popupWindow);
	
	(NEW Message ("attached", &popupWindow))->post (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PopupSlider::notify (ISubject* subject, MessageRef msg)
{
	if(msg == "attached")
	{
		if(!(hasTouchHandler || forceTouch))
		{
			KeyState keys;
			GUI.getKeyState (keys);
			if(!keys.isSet (KeyState::kLButton))
			{
				// if the mouse button has been released meanwhile, close the popup instead of starting to edit
				// in particular, don't capture the mouse (see below)
				Window* window = unknown_cast<Window> (msg[0]);
				if(window)
					window->deferClose ();
				return;
			}

			View* view = unknown_cast<View> (msg[0]);
			if(Slider* slider = view ? findControl (view) : nullptr)
			{
				// move mouse cursor to slider handle
				Point clientMousePos (getHandleCenter (slider));

				if(!style.isCustomStyle (Styles::kPopupBoxBehaviorKeepMousePos))
				{
					Point mousePos (clientMousePos);
					GUI.setMousePosition (slider->clientToScreen (mousePos));
				}
				else
				{
					GUI.getMousePosition (clientMousePos);
					slider->screenToClient (clientMousePos);
					slider->setMode (Styles::kSliderModeRelative);
				}
					
				// trigger mouse down handler
				MouseEvent event (MouseEvent::kMouseDown, clientMousePos, KeyState::kLButton);
				if(slider->isEnabled ())
					slider->onMouseDown (event);

				Window* window = unknown_cast<Window> (msg[0]);
				if(window)
					window->captureMouse (true);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PopupSlider::overridePosition (IWindow& parent)
{
	View* popup (unknown_cast<View> (&parent));
	if(Slider* slider = findControl (popup))
	{
		Point screenHandleCenter (getHandleCenter (slider));
		slider->clientToScreen (screenHandleCenter);

		// slider handle at mouse / touch position (screen coordinates)
		popup->setPosition (popup->getPosition () - screenHandleCenter + cursorPosition);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Point PopupSlider::getHandleCenter (Slider* slider)
{
	Rect handleRect;
	slider->getRenderer ()->getPartRect (slider, Slider::kPartHandle, handleRect);
	return handleRect.getCenter ();
}
