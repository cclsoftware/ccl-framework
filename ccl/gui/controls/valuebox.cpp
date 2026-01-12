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
// Filename    : ccl/gui/controls/valuebox.cpp
// Description : Value Box
//
//************************************************************************************************

#include "ccl/gui/controls/valuebox.h"

#include "ccl/gui/gui.h"
#include "ccl/gui/windows/window.h"
#include "ccl/gui/views/mousehandler.h"
#include "ccl/gui/touch/touchhandler.h"
#include "ccl/gui/controls/controlxyhandler.h"
#include "ccl/public/gui/iparameter.h"

namespace CCL {

//************************************************************************************************
// ValueBoxTouchHandler
//************************************************************************************************

class ValueBoxTouchHandler: public TouchHandler
{
public:
	ValueBoxTouchHandler (ValueBox* valueBox)
    : TouchHandler (valueBox),
	  manipulator (valueBox, nullptr, ControlXYEditManipulator::kAccelerated)
    {
		addRequiredGesture (GestureEvent::kSwipe|GestureEvent::kVertical,   GestureEvent::kPriorityHigh);
		addRequiredGesture (GestureEvent::kSwipe|GestureEvent::kHorizontal, GestureEvent::kPriorityNormal);
		addRequiredGesture (GestureEvent::kLongPress, GestureEvent::kPriorityHigh);
		addRequiredGesture (GestureEvent::kSingleTap, GestureEvent::kPriorityHigh);
	}

	tbool CCL_API onGesture (const GestureEvent& event) override
	{
		ValueBox* valueBox = static_cast<ValueBox*> (view);

		Point where (event.where);
		double when = event.eventTime;

		view->windowToClient (where);
		bool isFineMode = (event.keys.getModifiers () & KeyState::kShift) != 0;

		// single tap: focus edit control
		if(event.getType () == GestureEvent::kSingleTap)
		{
			valueBox->takeFocus ();
			valueBox->takeEditFocusInternal (true);
			return true;
		}

		// continuous gesture: edit value
		switch(event.getState ())
		{
		case GestureEvent::kBegin:
			valueBox->closeNativeTextControl ();
			valueBox->getParameter ()->beginEdit ();
			manipulator.initialize (where, when, isFineMode);
			break;

		case GestureEvent::kChanged:
			manipulator.move (where, when, isFineMode);
			GUI.flushUpdates (false); // redraw
			break;

		case GestureEvent::kEnd:
			valueBox->getParameter ()->endEdit ();
			valueBox->killFocus ();
			break;
		}
		return true;
	}

private:
	ControlXYEditManipulator manipulator;
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// ValueBox
//************************************************************************************************

BEGIN_STYLEDEF (ValueBox::customStyles)
	{"nodrag", Styles::kValueBoxBehaviorNoDrag},
	{"xyediting", Styles::kValueBoxBehaviorXYEdit},
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (ValueBox, EditBox)
DEFINE_CLASS_UID (ValueBox, 0x3D2770D5, 0x7C36, 0x40E9, 0xB7, 0xCC, 0x27, 0x79, 0xB3, 0x7D, 0x32, 0x7E);

//////////////////////////////////////////////////////////////////////////////////////////////////

ValueBox::ValueBox (const Rect& size, IParameter* param, StyleRef style)
: EditBox (size, param, style),
  xyEditDistance (200)
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ThemeRenderer* ValueBox::getRenderer ()
{
	if(renderer == nullptr)
		renderer = getTheme ().createRenderer (ThemePainter::kValueBoxRenderer, visualStyle);
	return renderer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ValueBox::canHandleDoubleTap () const
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ValueBox::onFocus (const FocusEvent& event)
{
	// don't create native control yet, might be a drag gesture or reset-click (but we can't distinguish MouseEvent from KeyEvent here)
	if(!isEditing () && event.eventType == FocusEvent::kSetFocus && event.directed)
		return true;

	return SuperClass::onFocus (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ValueBox::onKeyUp (const KeyEvent& event)
{
	if(!isEditing () && event.vKey == VKey::kTab)
	{
		Window* window = getWindow ();
		if(window && window->getFocusView () == this)
			takeEditFocusInternal (true); // assuming focus navigation: now start editing text (postponed in onFocus)
	}
	return SuperClass::onKeyUp (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ValueBox::onMouseDown (const MouseEvent& event)
{
	if(!event.keys.isSet (KeyState::kLButton))
		return SuperClass::onMouseDown (event);

	if(style.isCustomStyle (Styles::kValueBoxBehaviorNoDrag))
	{
		if(event.keys.getModifiers() == KeyState::kOption)
			return false;
		
		if(isResetClick (event)) // let resetclick fall through
			return false;
		
		if(style.isCustomStyle (Styles::kTextBoxBehaviorDoubleClickEdit))
			return handleClick (event);
			
		if(detectDrag (event))
			return false;

		takeEditFocusInternal (true);
		return true;
	}
	else if(!isEditing ())
	{
		if(tryMouseHandler (event)) // e.g. drag to change value, reset-click
			return true;
		else
			takeEditFocusInternal (true); // now start editing text (postponed in onFocus)
	}

	return SuperClass::onMouseDown (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MouseHandler* ValueBox::createMouseHandler (const MouseEvent& event)
{
	if(MouseHandler* handler = SuperClass::createMouseHandler (event))
		return handler;
	
	MouseCursor* cursor = GUI.getCursor ();
	if(style.isCustomStyle (Styles::kValueBoxBehaviorXYEdit))
	{
		AutoPtr<MouseCursor> verticalCursor (MouseCursor::createCursor (ThemeElements::kSizeVerticalCursor));
		GUI.setCursor (verticalCursor);
	}
	
	if(detectDrag (event))
	{
		closeNativeTextControl ();
		ControlXYMouseHandler* handler = NEW ControlXYMouseHandler (this, false, ControlXYEditManipulator::kAccelerated);
		if(style.isCustomStyle (Styles::kValueBoxBehaviorXYEdit))
			handler->setXYDistance (xyEditDistance, xyEditDistance);
		
		return handler;
	}
	
	GUI.setCursor (cursor);
	takeFocus ();
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ValueBox::setXYEditDistance (Coord distance)
{
	if(distance > 0)
	{
		xyEditDistance = distance;
		style.setCustomStyle (Styles::kValueBoxBehaviorXYEdit, true);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITouchHandler* ValueBox::createTouchHandler (const TouchEvent& event)
{
	return NEW ValueBoxTouchHandler (this);
}

