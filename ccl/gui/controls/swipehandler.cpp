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
// Filename    : ccl/gui/controls/swipehandler.cpp
// Description : Base class for swipe mousehandlers
//
//************************************************************************************************

#include "ccl/gui/controls/swipehandler.h"

#include "ccl/gui/windows/window.h"
#include "ccl/gui/touch/touchinput.h"
#include "ccl/gui/touch/touchhandler.h"

#include "ccl/base/kernel.h"

#include "ccl/public/gui/iparameter.h"
#include "ccl/gui/controls/button.h"
#include "ccl/gui/controls/slider.h"

using namespace CCL;

//************************************************************************************************
// SwipeMouseHandler::SwipeCondition
//************************************************************************************************

SwipeMouseHandler::SwipeCondition::SwipeCondition (Control* control)
: control (control),
  tag (0)
{
	if(control)
	{
		name = control->getName ();
		if(IParameter* param = control->getParameter ())
		{
			tag = param->getTag ();
			value = param->getValue ();
		}
	}
}

//************************************************************************************************
// SwipeMouseHandler
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (SwipeMouseHandler, MouseHandler)

//////////////////////////////////////////////////////////////////////////////////////////////////

SwipeMouseHandler::SwipeMouseHandler (Control* control, int swipeMode)
: MouseHandler (control),
  controlClass (control->myClass ()),
  swipeCondition (nullptr),
  swipeMode ((SwipeMode)swipeMode),
  lastControl (nullptr),
  flags (0)
{
	initStartControl (control);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SwipeMouseHandler::SwipeMouseHandler (View& view, MetaClassRef controlClass, int swipeMode)
: MouseHandler (&view),
  controlClass (controlClass),
  swipeCondition (nullptr),
  swipeMode ((SwipeMode)swipeMode),
  lastControl (nullptr),
  flags (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SwipeMouseHandler::initStartControl (Control* control)
{
	swipeCondition = SwipeCondition (control);
	lastControl = control;

	// calculate center of start control in window coords
	origin = control->getSize ().getSize () * .5;
	control->clientToWindow (origin);
	lastPos = origin;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Control* SwipeMouseHandler::findControl (View& parentView, const Point& where) const
{
	auto isSupportedControl = [&](View* view)
	{
		if(Control* control = ccl_cast<Control> (view))
		{
			if(control->canCast (controlClass))
			{
				if(Slider* slider = ccl_cast<Slider> (control))
					if(slider->getStyle ().isCustomStyle (Styles::kSliderBehaviorPassive))
						return (Control*)nullptr;
					
				if(Button* button = ccl_cast<Button> (control))
					if(button->getStyle ().isCustomStyle (Styles::kButtonBehaviorPassive))
						return (Control*)nullptr;
			
				return control;
			}
		}
		
		return (Control*)nullptr;
	};
	
	ForEachViewFastReverse (parentView, v)
		Point where2 (where);
		where2.offset (-v->getSize ().left, -v->getSize ().top);
		if(v->isInsideClient (where2))
		{
			if(Control* control = findControl (*v, where2))
				return control;

			if(ccl_cast<SwipeBox> (v))
				continue;

			if(Control* control = isSupportedControl (v))
				return control;
		}
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SwipeMouseHandler::checkCondition (const SwipeMouseHandler::SwipeCondition& c)
{
	return (ignoreName () || c.name == swipeCondition.name)
		&& (ignoreTag () ||  c.tag == swipeCondition.tag);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SwipeMouseHandler::trySwipe ()
{
	bool found = false;
	if(swipeMode != kNoSwipe)
	{
		if(current.eventType == MouseEvent::kMouseUp)
			return false;

		if(Window* window = view->getWindow ())
		{
			Point currPos (current.where);
			view->clientToWindow (currPos);

			bool isHorizontal = (swipeMode == kSwipeHorizontal)
				|| (swipeMode == kSwipeAny && ccl_abs (currPos.x - origin.x) >= ccl_abs (currPos.y - origin.y)); // guess from mouse distance

			// try all coordinates between last and current mouse position
			Point p1 = lastPos;
			Point p2 = currPos;
			lastPos = currPos;

			Coord* c1;
			Coord* c2;
			Coord* c;

			Point p (p1);
			if(isHorizontal)
				p.y = origin.y, c1 = &p1.x, c2 = &p2.x, c = &p.x;
			else
				p.x = origin.x, c1 = &p1.y, c2 = &p2.y, c = &p.y;

			Coord inc = *c2 > *c1 ? 1 : -1;

			while(*c != *c2)
			{
				// find control at position and check the swipe condition
				if(Control* newControl = findControl (*window, p))
				{
					CCL_PRINTF ("trySwipe: %s: %d\n", MutableCString (newControl->getName ()).str (), checkCondition (SwipeCondition (newControl)))

					if(newControl != lastControl && checkCondition (SwipeCondition (newControl)))
					{
						onSwipeEnter (newControl);
						lastControl = newControl;

						if(swipeMode == kSwipeAny)
							swipeMode = isHorizontal ? kSwipeHorizontal : kSwipeVertical; // now we know for sure

						found = true;
					}
				}
				*c += inc;
			}
		}
	}
	return found;
}

//************************************************************************************************
// SwipeBox::MetaMouseHandler
//************************************************************************************************

class SwipeBox::MetaMouseHandler: public SwipeMouseHandler
{
public:
	DECLARE_CLASS_ABSTRACT (MetaMouseHandler, SwipeMouseHandler)

	MetaMouseHandler (View* control, MetaClassRef controlClass, int swipeMode);

	Control* findStartControl (View* metaControl, PointRef where);

	// SwipeMouseHandler
	void onSwipeEnter (Control* newControl) override;
	bool checkCondition (const SwipeCondition& c) override;

	// MouseHandler
	void onBegin () override;
	bool onMove (int moveFlags) override;
	bool onKeyEvent (const KeyEvent& event) override;
	void onRelease (bool canceled) override;
	void CCL_API finish (const MouseEvent& event, tbool canceled = false) override;

protected:
	AutoPtr<MouseHandler> currentHandler;

	MouseEvent makeMouseEvent (int eventType, View* targetView);
	void finishCurrentHandler (bool canceled);
	
	struct SwipeSetter
	{
		SwipeSetter (Control* control)
		: button (ccl_cast<Button> (control)),
		  oldSwipeState (false)
		{
			if(button)
			{
				oldSwipeState = button->getStyle ().isCustomStyle (Styles::kButtonBehaviorSwipe);
				View::StyleModifier (*button).setCustomStyle (Styles::kButtonBehaviorSwipe, true);
			}
		}
		
		~SwipeSetter ()
		{
			if(button)
				View::StyleModifier (*button).setCustomStyle (Styles::kButtonBehaviorSwipe, oldSwipeState);
		}

		Button* button;
		bool oldSwipeState;
	};
};

//************************************************************************************************
// SwipeBox::MetaMouseHandler
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (SwipeBox::MetaMouseHandler, SwipeMouseHandler)

//////////////////////////////////////////////////////////////////////////////////////////////////

SwipeBox::MetaMouseHandler::MetaMouseHandler (View* metaControl, MetaClassRef controlClass, int swipeMode)
: SwipeMouseHandler (*metaControl, controlClass, swipeMode)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Control* SwipeBox::MetaMouseHandler::findStartControl (View* metaControl, PointRef where)
{
	if(Window* window = metaControl->getWindow ())
	{
		Point p (where);
		metaControl->clientToWindow (p);
		return findControl (*window, p);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MouseEvent SwipeBox::MetaMouseHandler::makeMouseEvent (int eventType, View* targetView)
{
	MouseEvent mouseEvent (current);
	mouseEvent.eventType = eventType;

	// translate coordinates to current mouse handler's view
	ASSERT (targetView)
	if(targetView)
	{
		getView ()->clientToWindow (mouseEvent.where);
		targetView->windowToClient (mouseEvent.where);
	}
	return mouseEvent;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SwipeBox::MetaMouseHandler::onSwipeEnter (Control* newControl)
{
	finishCurrentHandler (false);

	MouseEvent mouseEvent (makeMouseEvent (MouseEvent::kMouseDown, newControl));	

	SwipeSetter setter (newControl);
	
	currentHandler = newControl->createMouseHandler (mouseEvent);

	if(auto swipeHandler = ccl_cast<SwipeMouseHandler> (currentHandler))
	{
		swipeHandler->ignoreName (getView ()->getStyle ().isCustomStyle (Styles::kSwipeBoxBehaviorIgnoreName));
		swipeHandler->ignoreTag (getView ()->getStyle ().isCustomStyle (Styles::kSwipeBoxBehaviorIgnoreTag));

		// pass mode to created handler (if already decided)
		if(getSwipeMode () == kSwipeHorizontal || getSwipeMode () == kSwipeVertical)
			swipeHandler->setSwipeMode (getSwipeMode ());
	}

	if(currentHandler)
		currentHandler->begin (mouseEvent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SwipeBox::MetaMouseHandler::checkCondition (const SwipeCondition& c)
{
	// must be inside SwipeBox
	bool valid = c.control != getView  () && getView ()->isInsideClient (current.where);
	
	if(valid && (controlClass == ccl_typeid<Toggle> () || controlClass == ccl_typeid<CheckBox> ()))
		valid = (c.value.asBool () == swipeCondition.value.asBool ());
	
	return valid;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SwipeBox::MetaMouseHandler::onBegin ()
{ 
	if(Control* control = findStartControl (getView (), current.where))
	{
		initStartControl (control);
		onSwipeEnter (control);
	}
} 

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SwipeBox::MetaMouseHandler::onMove (int moveFlags)
{
	if(!lastControl)
		onBegin ();
	else
		trySwipe ();

	if(currentHandler)
	{
		MouseEvent mouseEvent (makeMouseEvent (MouseEvent::kMouseMove, currentHandler->getView ()));
		return currentHandler->trigger (mouseEvent, moveFlags) != 0;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SwipeBox::MetaMouseHandler::onRelease (bool canceled)
{
	finishCurrentHandler (canceled);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SwipeBox::MetaMouseHandler::onKeyEvent (const KeyEvent& event)
{
	if(currentHandler)
		return currentHandler->onKeyEvent (event);

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SwipeBox::MetaMouseHandler::finish (const MouseEvent& event, tbool canceled)
{
	SuperClass::finish (event, canceled);

	if(currentHandler)
	{
		MouseEvent mouseEvent (makeMouseEvent (event.eventType, currentHandler->getView ()));
		currentHandler->finish (mouseEvent, canceled);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SwipeBox::MetaMouseHandler::finishCurrentHandler (bool canceled)
{
	if(currentHandler)
	{
		MouseEvent mouseEvent (makeMouseEvent (MouseEvent::kMouseUp, currentHandler->getView ()));
		currentHandler->finish (mouseEvent, canceled);
		currentHandler->onRelease (canceled);
		currentHandler.release ();
	}
}

//************************************************************************************************
// SwipeBox
//************************************************************************************************

BEGIN_STYLEDEF (SwipeBox::customStyles)
	{"nowheel", Styles::kSwipeBoxBehaviorNoWheel},
	{"ignorename", Styles::kSwipeBoxBehaviorIgnoreName},
	{"ignoretag", Styles::kSwipeBoxBehaviorIgnoreTag},
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (SwipeBox, Control)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SwipeBox::isSwiping (View& view)
{
	if(Window* window = view.getWindow ())
	{
		// via mouse
		if(ccl_cast<MetaMouseHandler> (window->getMouseHandler ()))
			return true;
	
		// via touch
		ForEach (window->getTouchInputState ().getPendingGestures (), Gesture, gesture)
		if(TouchMouseHandler* touchHandler = unknown_cast<TouchMouseHandler> (gesture->getHandler ()))
			if(unknown_cast<SwipeBox> (touchHandler->getView ()))
				return true;
		EndFor
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SwipeBox::SwipeBox (const Rect& size, StringID targetClass, IParameter* _param, StyleRef style)
: Control (size, _param, style),
  targetClass (targetClass),
  swipeAlways (!_param)
{
	setWheelEnabled (style.isCustomStyle (Styles::kSwipeBoxBehaviorNoWheel) == false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MouseHandler* SwipeBox::createMouseHandler (const MouseEvent& event)
{
	if(swipeAlways || param->getValue ().asBool ())
	{
		const MetaClass* metaClass = nullptr;
		if(!targetClass.isEmpty ())
			metaClass = Kernel::instance ().getClassRegistry ().findType (targetClass);

		int mode = SwipeMouseHandler::kSwipeAny;
		if(style.isHorizontal ())
			mode = SwipeMouseHandler::kSwipeHorizontal;
		else if(style.isVertical ())
			mode = SwipeMouseHandler::kSwipeVertical;

		return NEW MetaMouseHandler (this, metaClass ? *metaClass : ccl_typeid<Control> (), mode);
	}
	return SuperClass::createMouseHandler (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITouchHandler* SwipeBox::createTouchHandler (const TouchEvent& event)
{
	// wrap mouse handler, but boost priority
	MouseEvent mouseEvent (TouchMouseHandler::makeMouseEvent (MouseEvent::kMouseDown, event, *this));
	AutoPtr<MouseHandler> mouseHandler (createMouseHandler (mouseEvent));
	if(mouseHandler)
	{
		TouchMouseHandler* touchHandler = NEW TouchMouseHandler (mouseHandler, mouseHandler->getView ());
		touchHandler->addRequiredGesture (GestureEvent::kLongPress|GestureEvent::kPriorityHigh);
		touchHandler->addRequiredGesture (GestureEvent::kSwipe|GestureEvent::kHorizontal, GestureEvent::kPriorityHigh);
		touchHandler->addRequiredGesture (GestureEvent::kSwipe|GestureEvent::kVertical, GestureEvent::kPriorityHigh);
		return touchHandler;
	}
	return SuperClass::createTouchHandler (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SwipeBox::onMouseDown (const MouseEvent& event)
{
	// try own handler first (when target controls are deep childs of swipe box)
	AutoPtr<MouseHandler> mouseHandler (createMouseHandler (event));
	auto swipeHandler = ccl_cast<MetaMouseHandler> (mouseHandler);
	if(swipeHandler && swipeHandler->findStartControl (this, event.where))
		if(tryMouseHandler (event))
			return true;

	return SuperClass::onMouseDown (event);
}
