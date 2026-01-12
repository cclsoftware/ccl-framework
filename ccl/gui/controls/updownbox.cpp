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
// Filename    : ccl/gui/controls/updownbox.cpp
// Description : Up/Down Box
//
//************************************************************************************************

#include "ccl/gui/controls/updownbox.h"

#include "ccl/gui/views/mousehandler.h"
#include "ccl/gui/touch/touchhandler.h"
#include "ccl/gui/gui.h"

#include "ccl/public/gui/iparameter.h"

namespace CCL {

//************************************************************************************************
// UpDownButtonMouseHandler
//************************************************************************************************

class UpDownButtonMouseHandler: public PeriodicMouseHandler
{
public:
	UpDownButtonMouseHandler (UpDownButton* button = nullptr)
	: PeriodicMouseHandler (button),
	  hasTriggered (false),
	  touchHandler (false)
	{}

	void onBegin () override
	{ 
		Button* button = (Button*)view;
		button->getParameter ()->beginEdit ();
		button->setMouseState (View::kMouseDown);
	} 
	
	void onRelease (bool canceled) override
	{
		if(!hasTriggered)
			onPeriodic ();

		Button* button = (Button*)view;
		button->setMouseState (View::kMouseNone); 
		button->getParameter ()->endEdit ();
	}

	bool onPeriodic () override
	{
		if(hasTriggered)
		{
			// only trigger again if mouse is still pressed
			KeyState keys;
			GUI.getKeyState (keys);
			if(!keys.isSet (KeyState::kMouseMask) && !isTouchHandler ())
				return true;
		}

		Button* button = (Button*)view;
		IParameter* param = button->getParameter ();

		if(button->getStyle ().isCustomStyle (Styles::kUpDownButtonBehaviorIncrement))
			param->increment ();
		else
			param->decrement ();

		hasTriggered = true;
		return true;
	}
	
	PROPERTY_BOOL (touchHandler, TouchHandler)
	
private:

	bool hasTriggered;
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_STYLEDEF (UpDownButton::customStyles)
	{"increment", Styles::kUpDownButtonBehaviorIncrement},
	{"decrement", Styles::kUpDownButtonBehaviorDecrement},
END_STYLEDEF

//************************************************************************************************
// UpDownButton
//************************************************************************************************

DEFINE_CLASS (UpDownButton, Button)

//////////////////////////////////////////////////////////////////////////////////////////////////

UpDownButton::UpDownButton (const Rect& size, IParameter* param, StyleRef style)
: Button (size, param, style)
{}


//////////////////////////////////////////////////////////////////////////////////////////////////

ITouchHandler* UpDownButton::createTouchHandler (const TouchEvent& event)
{
	auto handler = NEW UpDownButtonMouseHandler (this);
	handler->setTouchHandler (true);
	auto touchHandler = NEW TouchMouseHandler (handler, this);
	touchHandler->addRequiredGesture (GestureEvent::kSingleTap, GestureEvent::kPriorityHighest);
	touchHandler->addRequiredGesture (GestureEvent::kLongPress, GestureEvent::kPriorityHighest);
	return touchHandler;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MouseHandler* UpDownButton::createMouseHandler (const MouseEvent& event)
{
	return NEW UpDownButtonMouseHandler (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ThemeRenderer* UpDownButton::getRenderer ()
{	
	if(!renderer)
	{
		if(visualStyle && visualStyle->getBackgroundImage () || getIcon () || style.isTransparent ())
			return Button::getRenderer ();
		else
			renderer = getTheme ().createRenderer (ThemePainter::kUpDownButtonRenderer, visualStyle);
	}
	return renderer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API UpDownButton::push ()
{
	// e.g. called from onKeyDown for Return, Enter, Space
	if(getStyle ().isCustomStyle (Styles::kUpDownButtonBehaviorIncrement))
		param->increment ();
	else
		param->decrement ();
}

//************************************************************************************************
// UpDownBox
//************************************************************************************************

UpDownBox::UpDownBox (const Rect& size, IParameter* param, StyleRef style)
: View (size, style)
{
	Coord width = size.getWidth ();
	Coord height = size.getHeight ();

	Rect incSize, decSize;
	if(style.isVertical ())
	{
		Coord halfH = height/2;
		incSize (0, 0, width, halfH);
		decSize (0, halfH, width, height);
	}
	else
	{
		Coord halfW = width/2;
		decSize (0, 0, halfW, height);
		incSize (halfW, 0, width, height);
	}

	StyleFlags incStyle (style);
	StyleFlags decStyle (style);
	incStyle.setCustomStyle (Styles::kUpDownButtonBehaviorIncrement);

	addView (NEW UpDownButton (incSize, param, incStyle));
	addView (NEW UpDownButton (decSize, param, decStyle));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UpDownBox::onVisualStyleChanged ()
{
	View::onVisualStyleChanged ();

	if(visualStyle)
	{
		setButtonStyle (ccl_cast<UpDownButton> (getChild (0)), "buttonUp", visualStyle);
		setButtonStyle (ccl_cast<UpDownButton> (getChild (1)), "buttonDown", visualStyle);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UpDownBox::setButtonStyle (UpDownButton* button, StringID backgroundName, VisualStyle* visualStyle)
{
	if(button)
		if(IImage* buttonImage = visualStyle->getImage (backgroundName))
		{
			AutoPtr<VisualStyle> buttonStyle = NEW VisualStyle (*visualStyle);
			buttonStyle->setImage (StyleID::kBackground, buttonImage);
			button->setVisualStyle (buttonStyle);
		}
}
