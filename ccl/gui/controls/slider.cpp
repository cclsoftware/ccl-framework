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
// Filename    : ccl/gui/controls/slider.cpp
// Description : Slider Control
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/public/systemservices.h"

#include "ccl/gui/controls/slider.h"
#include "ccl/gui/controls/swipehandler.h"
#include "ccl/gui/controls/controlxyhandler.h"
#include "ccl/gui/touch/touchhandler.h"

#include "ccl/gui/theme/renderer/sliderrenderer.h"
#include "ccl/gui/windows/window.h"
#include "ccl/gui/gui.h"

#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/iparamobserver.h"

namespace CCL {

//************************************************************************************************
// SliderHandlerBase
//************************************************************************************************

class SliderHandlerBase
{
public:
	SliderHandlerBase (Slider* slider, const Point& clickOffset, int max, bool handleClicked = true)
	: slider (slider),
	  clickOffset (clickOffset),
	  max (max),
	  handleClicked (handleClicked)
	{}

	//////////////////////////////////////////////////////////////////////////////////////////////////
	
	float calcValue (PointRef where)
	{
		return pointToValue (toSliderCoordinates (where));
	}
	
protected:
	Slider* slider;
	Point clickOffset;
	int max;
	bool handleClicked;
	int mode;
	
	//////////////////////////////////////////////////////////////////////////////////////////////////
	
	Point toSliderCoordinates (Point p) const
	{
		p.x -= clickOffset.x;
		p.y -= clickOffset.y;
		
		// coordinates for vertical orientation are 'upside-down'
		if(slider->getStyle ().isVertical ())
			p.y = max - p.y;
		
		return p;
	}
	
	//////////////////////////////////////////////////////////////////////////////////////////////////
	
	float pointToValue (Point p) const
	{
		float value;
		
		if(slider->getStyle ().isVertical ())
			value = (float)p.y / (float)max;
		else
			value = (float)p.x / (float)max;
		
		return value;
	}
	
	
	//////////////////////////////////////////////////////////////////////////////////////////////////
	
	bool movedInWrongDirection (PointRef currentPos, PointRef startPos)
	{
		if(slider->getStyle ().isVertical ())
		{
			if(currentPos.y == startPos.y)
				return true;
		}
		else
		{
			if(currentPos.x == startPos.x)
				return true;
		}
		
		return false;
	}
};

//************************************************************************************************
// SliderMouseHandler
//************************************************************************************************

class SliderMouseHandler: public MouseHandler,
						  public SliderHandlerBase
{
public:
	SliderMouseHandler (Slider* slider, const Point& clickOffset, int max, bool handleClicked)
	: SliderHandlerBase (slider, clickOffset, max, handleClicked),
	  MouseHandler (slider),
	  fineStart (0)
	{
		checkKeys (true);
	}

	~SliderMouseHandler ()
	{
		tooltipPopup.reserve (false);
	}

	void onBegin () override
	{
		slider->getParameter ()->beginEdit ();
		wasFine = (current.keys.getModifiers () == KeyState::kShift);
		fineWhere = current.where;
		fineStart = slider->getValue ();

		if(handleClicked || wasFine)
			onMove (kMouseMoved|kKeysChanged);
		else
			slider->setValue (calcValue (current.where), true);	// must set first value (before first move)
	} 
	
	void onRelease (bool canceled) override
	{
		slider->getParameter ()->endEdit ();
		tooltipPopup.reserve (false);
	}

	bool onMove (int moveFlags) override
	{
		if(movedInWrongDirection (current.where, first.where))
			return true;

		float v = calcValue (current.where);

		// fine mode
		bool isShiftPressed = (current.keys.getModifiers () & KeyState::kShift) != 0;
		if(isShiftPressed != wasFine)
		{
			fineWhere = current.where;
			fineStart = slider->getValue ();
			wasFine = isShiftPressed;
		}
		if(isShiftPressed)
		{
			if(slider->getStyle ().isVertical ())
			{
				float deltaY = float (current.where.y - fineWhere.y);
				v = fineStart - (0.05f * deltaY / (float)max);
			}
			else
			{
				float deltaX = float (current.where.x - fineWhere.x);
				v = fineStart + (0.05f * deltaX / (float)max);
			}
		}
		
		v = ccl_bound<float> (v);

		if(v != slider->getValue ())
			slider->setValue (v, true);

		updateTooltip ();
		return true;
	}

	void updateTooltip ()
	{
		if(view->getStyle ().isCustomStyle (Styles::kSliderBehaviorEditTooltip))
		{
			tooltipPopup.setTooltip (((Control*)view)->makeEditTooltip ());
			tooltipPopup.reserve (true);
		}			
	}
	

protected:
	Point fineWhere;
	float fineStart;
	bool wasFine;
};
	
//************************************************************************************************
// SliderXYMouseHandler
//************************************************************************************************

class SliderXYMouseHandler : public ControlXYMouseHandler,
							 public SliderHandlerBase
{
public:
	SliderXYMouseHandler (Slider* view, int max, bool handleClicked, bool showEditTooltip)
	: SliderHandlerBase (view, Point (), max, handleClicked),
	  ControlXYMouseHandler (view, showEditTooltip, (view->getStyle ().isCustomStyle (Styles::kSliderBehaviorReverse)) ? ControlXYEditManipulator::kReverse : 0)
	{
		int xDistance = max;
		int yDistance = 300;
		
		if(view->getStyle ().isVertical ())
			ccl_swap(xDistance, yDistance);
		
		setXYDistance (xDistance, yDistance);
		
		Rect handleRect;
		slider->getRenderer ()->getPartRect (slider, Slider::kPartHandle, handleRect);
		clickOffset.x = handleRect.getWidth () / 2;
		clickOffset.y = handleRect.getHeight () / 2;
	}

	void onBegin () override
	{
		if(!handleClicked && !(current.keys.getModifiers () == KeyState::kShift) || current.keys.getModifiers () == KeyState::kOption)
			startJumpValue = (calcValue (current.where));

		ControlXYMouseHandler::onBegin ();
	}
};

//************************************************************************************************
// SliderSwipeMouseHandler
//************************************************************************************************

class SliderSwipeMouseHandler: public SwipeMouseHandler
{
public:
	SliderSwipeMouseHandler (Slider* slider = nullptr)
	: SwipeMouseHandler (slider)
	{}

	void onBegin () override
	{
		view->setMouseState (View::kMouseDown); 
	} 
	
	void onRelease (bool canceled) override
	{ 
		view->setMouseState (View::kMouseNone);
	}

	bool onMove (int moveFlags) override
	{
		if(view->isInsideClient (current.where))
			view->setMouseState (View::kMouseDown);
		else if(!trySwipe ())
			view->setMouseState (View::kMouseOver);
		return true;
	}

	void onSwipeEnter (Control* newControl) override
	{
		Slider* slider = (Slider*)view;
		IParameter* srcParam = slider->getParameter ();
		IParameter* dstParam = newControl->getParameter();

		// take value of source slider
		if(srcParam && dstParam && dstParam != srcParam)
			dstParam->takeValue (*srcParam, true);
	}
};

//************************************************************************************************
// SliderTouchHandler
//************************************************************************************************
	
class SliderTouchHandler: public TouchHandler,
						  public SliderHandlerBase
{
public:
	SliderTouchHandler (Slider* view, const Point& clickOffset, int max, bool handleClicked, int sliderMode)
	: SliderHandlerBase (view, clickOffset, max, handleClicked),
	  TouchHandler (view),
	  tooltipPopup (view),
	  mustAdjustFirstPosition (sliderMode == Styles::kSliderModeRelative)
	{
		TouchMouseHandler::applyGesturePriorities (*this, view);
		addRequiredGesture (GestureEvent::kLongPress, GestureEvent::kPriorityHigh); // prefer over drag (of a parent view)
	}
	
	~SliderTouchHandler ()
	{
		tooltipPopup.reserve (false);	
	}
	
	tbool CCL_API onGesture (const GestureEvent& event) override
	{
		Point where (event.where);
		view->windowToClient (where);
		
		switch(event.getState ())
		{
			case GestureEvent::kBegin:
			{				
				beginPosition = where;

				bool isContinuous = event.getType () >= GestureEvent::kSwipe && event.getType () <= GestureEvent::kLongPress;
				if(!isContinuous && handleClicked)
					break; // don't even "begin" if nothing will be changed

				slider->getParameter ()->beginEdit ();
				if(!handleClicked)
					slider->setValue (calcValue (where), true);
					
				where.offset (40, -40);
				tooltipPosition = where;

				// we won't receive kEnd for non-continuous gestures
				if(!isContinuous)
					slider->getParameter ()->endEdit ();

				break;
			}

			case GestureEvent::kChanged:
			{
				if(mustAdjustFirstPosition)
				{
					float velocity = slider->getStyle ().isVertical () ? event.amountY : event.amountX;
					CCL_PRINTF ("Velocity: %f\n", velocity)
					if(ccl_abs (velocity) < 200.f)
					{
						// avoid jump when initial velocity is quite slow:
						// recalc clickOffset (ignore distance moved between gesture begin (first touch pos) and first change)
						Rect handleRect;
						Rect trackRect;
						slider->getRenderer ()->getPartRect (slider, Slider::kPartHandle, handleRect);
						slider->getRenderer ()->getPartRect (slider, Slider::kPartTrack, trackRect);
						clickOffset = where - handleRect.getLeftTop () + trackRect.getLeftTop ();
					}                   
					mustAdjustFirstPosition = false;
				}

				float v = calcValue (where);
				v = ccl_bound<float> (v);		
				if(v != slider->getValue ())
					slider->setValue (v, true);	
				if(view->getStyle ().isCustomStyle (Styles::kSliderBehaviorEditTooltip))
				{
					tooltipPopup.setTooltip (((Control*)view)->makeEditTooltip (), &tooltipPosition);
					tooltipPopup.reserve (true);
				}
				
				break;
			}				
				
			case GestureEvent::kEnd:
			case GestureEvent::kFailed:
				slider->getParameter ()->endEdit ();
				tooltipPopup.reserve (false);
				break;
		}
		return true;
	}

protected:
	UserTooltipPopup tooltipPopup;
	Point tooltipPosition;
	Point beginPosition;
	bool mustAdjustFirstPosition;
};

//************************************************************************************************
// SliderXYTouchHandler
//************************************************************************************************

class SliderXYTouchHandler: public SliderTouchHandler
{
public:
	SliderXYTouchHandler (Slider* view, const Point& clickOffset, int max, bool handleClicked, int sliderMode)
	: SliderTouchHandler (view, clickOffset, max, handleClicked, sliderMode),
	  manipulator (view)
	{
	}
	
	tbool CCL_API onGesture (const GestureEvent& event) override
	{
		Point where (event.where);
		double when = event.eventTime;
		slider->windowToClient (where);
		
		switch(event.getState ())
		{
			case GestureEvent::kBegin:
			{
				SliderTouchHandler::onGesture (event);
				manipulator.initialize (where, when, false);
				break;
			}
			case GestureEvent::kChanged:
			{
				manipulator.move (where, when, false);
				break;
			}
				
			case GestureEvent::kEnd:
			case GestureEvent::kFailed:
			{
				SliderTouchHandler::onGesture (event);
				break;
			}
		}
		return true;
	}
	
	void setXYDistance (int distanceHorizontal, int distanceVertical)
	{
		manipulator.setXYDistance (distanceHorizontal, distanceVertical);
	}

protected:	
	ControlXYEditManipulator manipulator;
};
	
//************************************************************************************************
// RangeSliderXYEditManipulator
//************************************************************************************************

class RangeSliderXYEditManipulator : public ControlXYEditManipulator
{
public:
	
	RangeSliderXYEditManipulator (RangeSlider* slider, const RangeSlider::EditHandlerSetup& setup)
	: ControlXYEditManipulator (slider, setup.mainParam, (slider->getStyle ().isCustomStyle (Styles::kSliderBehaviorReverse)) ? ControlXYEditManipulator::kReverse : 0),
	additionalValue (setup.additionalParam),
	moveLowerHandle (setup.moveLowerHandle),
	moveUpperHandle (setup.moveUpperHandle),
	useLimits (setup.useLimits),
	invertible (setup.invertible),
	altModeSign (1.f),
	startValue2 (0)
	{}
	
	//////////////////////////////////////////////////////////////////////////////////////////////////
	
	void initialize (PointRef where, double when, bool isFineMode, double normalizedStartValue = -1) override
	{
		startValue2 = prepareNormalizedValue (additionalValue.get ());
		ControlXYEditManipulator::initialize (where, when, isFineMode, normalizedStartValue);
	}
	
	//////////////////////////////////////////////////////////////////////////////////////////////////
	
	void setNewValue (double newValue, float delta, bool spreadRange) override
	{
		if(moveLowerHandle && !spreadRange)
		{
			if(useLimits && !invertible)
				if(newValue > startValue2)
					newValue = startValue2;
			
			normalizedValue.set (prepareNormalizedValue (newValue), true);
			startValue = newValue;
			if(!invertible)
				if(startValue > startValue2)
					additionalValue.set (prepareNormalizedValue (newValue), true);
		}
		else if(moveUpperHandle && !spreadRange)
		{
			if(useLimits && !invertible)
				if(newValue < startValue2)
					newValue = startValue2;
			
			normalizedValue.set (prepareNormalizedValue (newValue), true);
			startValue = newValue;
			if(!invertible)
				if(startValue < startValue2)
					additionalValue.set (prepareNormalizedValue (newValue), true);
		}
		else // move/spread range
		{
			if(moveUpperHandle && spreadRange) // up or right should still increase the range
				delta *= -1;
				
			double value = startValue + (delta * (spreadRange ? -1 : 1));
			double value2 = startValue2 + (delta * altModeSign);
			
			if(value <= value2 || invertible)
			{
				if(!useLimits || (value >= 0 && value2 <= 1))
				{
					normalizedValue.set (prepareNormalizedValue (value), true);
					startValue = value;
					additionalValue.set (prepareNormalizedValue (value2), true);
					startValue2 = value2;
				}
				else if(startValue > 0 && startValue2 < 1)
				{
					if(value2 > 1)
					{
						additionalValue.set (prepareNormalizedValue (1), true);
						double newDelta = 1 - startValue2;
						startValue2 = 1;
						value = startValue + (newDelta * (spreadRange ? -1 : 1));
						startValue = value;
						normalizedValue.set (prepareNormalizedValue (value), true);

					}
					else // (value < 0)
					{
						normalizedValue.set (prepareNormalizedValue (0), true);
						double newDelta = -startValue;
						startValue = 0;
						value2 = startValue2 + (newDelta * altModeSign * (spreadRange ? -1 : 1));
						additionalValue.set (prepareNormalizedValue (value2), true);
						startValue2 = value2;
					}
				}
			}
		}
	}
	
	void setAltMode (bool state)
	{
		altModeSign = state ? -1.f : 1.f;
	}
	
private:
	double startValue2;
	NormalizedValue additionalValue;
	bool moveLowerHandle;
	bool moveUpperHandle;
	bool useLimits;
	bool invertible;
	float altModeSign;
	
	double prepareNormalizedValue (double value) const
	{
		return (isReverse ()) ? (1 - value) : value;
	}
};

//************************************************************************************************
// RangeSliderMouseHandler
//************************************************************************************************

class RangeSliderMouseHandler : public ControlXYMouseHandler,
								public SliderHandlerBase
{
public:
	RangeSliderMouseHandler (RangeSlider* slider, const RangeSlider::EditHandlerSetup& setup, bool showRangeTooltip)
	: SliderHandlerBase (slider, setup.clickOffset, setup.max, setup.handleClicked),
	  ControlXYMouseHandler (slider, false),
	  rangeManipulator (slider, setup),
	  mainParameter (setup.mainParam),
	  additionalParameter (setup.additionalParam),
	  wasAltPressed (false),
	  showRangeTooltip (showRangeTooltip),
	  slider (slider),
	  xyEditing (setup.xyEditing),
	  rangeEditing (!setup.moveLowerHandle ^ setup.moveUpperHandle)
	{
		int xDistance = setup.xEditDistance;
		int yDistance = setup.yEditDistance;
		
		if(view->getStyle ().isVertical ())
			ccl_swap(xDistance, yDistance);
		
		rangeManipulator.setXYDistance (xDistance, yDistance);
	}
	
	//////////////////////////////////////////////////////////////////////////////////////////////////
	
	void onBegin () override
	{
		mainParameter->beginEdit ();
		additionalParameter->beginEdit ();
		
		if(!handleClicked && !(current.keys.getModifiers () == KeyState::kShift) || current.keys.getModifiers () == KeyState::kOption)
			startJumpValue = (calcValue (current.where));
		
		fineWhere = current.where;
		fineStart = getMainParameterValue ();
		
		ControlXYMouseHandler::onBegin ();
	}
	
	//////////////////////////////////////////////////////////////////////////////////////////////////
	
	void onRelease (bool canceled) override
	{
		ControlXYMouseHandler::onRelease (canceled);
		
		mainParameter->endEdit ();
		additionalParameter->endEdit ();
	}
	
	//////////////////////////////////////////////////////////////////////////////////////////////////
	
	bool onMove (int moveFlags) override
	{
		if(moveFlags & kPeriodicMove)	// filter all periodic moves
		{
			PeriodicMouseHandler::onMove (moveFlags);
		}
		else if (moveFlags & kMouseMoved)
		{
			if(xyEditing || rangeEditing)
			{
				bool isAltPressed = (current.keys.getModifiers () & KeyState::kOption) != 0;
				rangeManipulator.setAltMode (isAltPressed);
				
				if(xyEditing)
				{
					if(isAltPressed != wasAltPressed)
					{
						preferredDirection = ControlXYEditManipulator::kUndefined;
						previousWhere = current.where;
						accuX = 0.f;
						accuY = 0.f;
						GUI.setCursor (nullptr);
						wasAltPressed = isAltPressed;
					}
				}
				else // range editing
					preferredDirection = slider->getStyle ().isVertical () ? ControlXYEditManipulator::kVertical : ControlXYEditManipulator::kHorizontal;
			
				ControlXYMouseHandler::onMove (moveFlags);
			}
			else
			{
				if(movedInWrongDirection (current.where, first.where))
					return true;
				
				float newValue = calcValue (current.where);
				
				// fine mode
				bool isShiftPressed = (current.keys.getModifiers () & KeyState::kShift) != 0;
				if(isShiftPressed != wasFine)
				{
					fineWhere = current.where;
					fineStart = getMainParameterValue ();
					wasFine = isShiftPressed;
				}
				if(isShiftPressed)
				{
					if(slider->getStyle ().isVertical ())
					{
						float deltaY = float (current.where.y - fineWhere.y);
						newValue = fineStart - (0.05f * deltaY / (float)max);
					}
					else
					{
						float deltaX = float (current.where.x - fineWhere.x);
						newValue = fineStart + (0.05f * deltaX / (float)max);
					}
				}
				
				newValue = ccl_bound<float> (newValue);
				
				float currentValue = getMainParameterValue ();
				float delta = currentValue - newValue;
				
				if(delta != 0)
					rangeManipulator.setNewValue (newValue, delta, false);
				
			}
			
			if(showRangeTooltip)
				updateTooltip ();
		}
		return true;
	}
	
	//////////////////////////////////////////////////////////////////////////////////////////////////
	
	void updateTooltip ()
	{
		if(view->getStyle ().isCustomStyle (Styles::kSliderBehaviorEditTooltip))
		{
			slider->updateTooltip (true);
			tooltipPopup.setTooltip (slider->getTooltip ());
			tooltipPopup.reserve (true);
		}
	}
	
	//////////////////////////////////////////////////////////////////////////////////////////////////
	
	float getMainParameterValue ()
	{
		return (mainParameter == slider->getParameter ()) ? slider->getValue () : slider->getSecondValue ();
	}
	
	//////////////////////////////////////////////////////////////////////////////////////////////////
	
	ControlXYEditManipulator& getManipulator () override
	{
		return rangeManipulator;
	}
	
private:
	RangeSlider* slider;
	IParameter* mainParameter;
	IParameter* additionalParameter;
	bool showRangeTooltip;
	RangeSliderXYEditManipulator rangeManipulator;
	bool wasAltPressed;
	bool xyEditing;
	bool rangeEditing;
	
	// !xyEditing
	Point fineWhere;
	float fineStart;
	bool wasFine;
};

//************************************************************************************************
// RangeSliderTouchHandler
//************************************************************************************************

class RangeSliderTouchHandler: public TouchHandler
{
public:
	RangeSliderTouchHandler (RangeSlider* slider, const RangeSlider::EditHandlerSetup& setup, bool showRangeTooltip)
	: TouchHandler (slider),
	  rangeManipulator (slider, setup),
	  mainParameter (setup.mainParam),
	  additionalParameter (setup.additionalParam),
	  showRangeTooltip (showRangeTooltip),
	  xyEditing (setup.xyEditing),
	  rangeEditing (!setup.moveLowerHandle ^ setup.moveUpperHandle),
	  tooltipPopup (slider),
	  slider (slider)
	{
		rangeManipulator.setXYDistance (setup.xEditDistance, setup.yEditDistance);

		TouchMouseHandler::applyGesturePriorities (*this, slider);
		addRequiredGesture (GestureEvent::kLongPress, GestureEvent::kPriorityHigh); // prefer over drag (of a parent view)
	}
	
	//////////////////////////////////////////////////////////////////////////////////////////////////
	
	~RangeSliderTouchHandler ()
	{
		tooltipPopup.reserve (false);
	}
	
	//////////////////////////////////////////////////////////////////////////////////////////////////
	
	tbool CCL_API onGesture (const GestureEvent& event) override
	{
		Point where (event.where);
		slider->windowToClient (where);
		
		switch(event.getState ())
		{
			case GestureEvent::kBegin:
			{
				bool isContinuous = event.getType () >= GestureEvent::kSwipe && event.getType () <= GestureEvent::kLongPress;
				if(!isContinuous)
					break; // don't even "begin" if nothing will be changed
				
				mainParameter->beginEdit ();
				additionalParameter->beginEdit ();
				
				where.offset (40, -40);
				tooltipPosition = where;
				
				// we won't receive kEnd for non-continuous gestures
				if(!isContinuous)
				{
					mainParameter->endEdit ();
					additionalParameter->endEdit ();
				}

				rangeManipulator.initialize (where, event.eventTime, false);
				break;
			}
			case GestureEvent::kChanged:
			{
				ControlXYEditManipulator::Direction direction = ControlXYEditManipulator::kUndefined;
				if(rangeEditing)
					direction = slider->getStyle ().isVertical () ? ControlXYEditManipulator::kVertical : ControlXYEditManipulator::kHorizontal;
				else
					direction = xyEditing ? ControlXYEditManipulator::kUndefined : ControlXYEditManipulator::kHorizontal;

				rangeManipulator.move (where, event.eventTime, false, direction);
				
				if(showRangeTooltip)
					updateTooltip ();
				
				break;
			}
				
			case GestureEvent::kEnd:
			case GestureEvent::kFailed:
			{
				if(showRangeTooltip)
					tooltipPopup.reserve (false);
				
				mainParameter->endEdit ();
				additionalParameter->endEdit ();
				
				slider->killFocus ();
				
				GUI.setCursor (nullptr);
				
				break;
			}
		}
		
		return true;
	}
	
	//////////////////////////////////////////////////////////////////////////////////////////////////
	
	void updateTooltip ()
	{
		if(view->getStyle ().isCustomStyle (Styles::kSliderBehaviorEditTooltip))
		{
			slider->updateTooltip (true);
			tooltipPopup.setTooltip (slider->getTooltip (), &tooltipPosition);
			tooltipPopup.reserve (true);
		}
	}
	
private:
	
	RangeSlider* slider;
	RangeSliderXYEditManipulator rangeManipulator;
	IParameter* mainParameter;
	IParameter* additionalParameter;
	UserTooltipPopup tooltipPopup;
	Point tooltipPosition;
	bool showRangeTooltip;
	bool xyEditing;
	bool rangeEditing;
};
	
} // namespace CCL

using namespace CCL;

//************************************************************************************************
// Slider
//************************************************************************************************

const Configuration::IntValue Slider::sliderMode ("GUI.Controls.Slider", "mode", Styles::kSliderModeTouch);

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_STYLEDEF (Slider::customStyles)
	{"thinhandle",			Styles::kSliderAppearanceThinHandle},
	{"bargraph",			Styles::kSliderAppearanceBarGraph},
	{"centered",			Styles::kSliderAppearanceCentered},
	{"reverse",				Styles::kSliderBehaviorReverse},
	{"tooltip",				Styles::kSliderBehaviorEditTooltip},
	{"globalmode",			Styles::kSliderBehaviorGlobalMode},
	{"swipe",				Styles::kSliderBehaviorSwipe},
	{"notouchreset",		Styles::kSliderBehaviorNoTouchReset},
	{"tickscale",	    	Styles::kSliderAppearanceTickScale},
	{"xyediting",	    	Styles::kSliderBehaviorXYEdit},
	{"optionclick",			Styles::kSliderBehaviorOptionClick},
	{"resetclick",          Styles::kSliderBehaviorResetClick},
	{"doubletap",           Styles::kSliderBehaviorDoubleTap},
	{"nowheel",				Styles::kSliderBehaviorNoWheel},
	{"passive",			 	Styles::kSliderBehaviorPassive},
	{"inversewheel",	 	Styles::kSliderBehaviorInverseWheel},
	{"defaultcentered",	 	Styles::kSliderAppearanceDefaultCentered},
END_STYLEDEF

BEGIN_STYLEDEF (Slider::modes)
	{"touch",		Styles::kSliderModeTouch},
	{"jump",		Styles::kSliderModeJump},
	{"relative",	Styles::kSliderModeRelative},
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (Slider, ValueControl)
DEFINE_CLASS_UID (Slider, 0x33625008, 0x7dbd, 0x41d6, 0xb9, 0xac, 0xbf, 0x0, 0x41, 0xb, 0xb6, 0xe2)

//////////////////////////////////////////////////////////////////////////////////////////////////

Slider::Slider (const Rect& size, IParameter* param, StyleRef style)
: ValueControl (size, param, style),
  isTouchResetEnabled (!style.isCustomStyle (Styles::kSliderBehaviorNoTouchReset)),
  instanceMode (Styles::kSliderModeDefault),
  xyEditDistance (0)
{
	setWheelEnabled (style.isCustomStyle (Styles::kSliderBehaviorNoWheel) == false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float Slider::getValue () const
{
	if(style.isCustomStyle (Styles::kSliderBehaviorReverse))
		return 1.f - ValueControl::getValue ();
	
	return ValueControl::getValue ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Slider::setValue (float v, bool update)
{
	ValueControl::setValue (style.isCustomStyle (Styles::kSliderBehaviorReverse) ? 1.f - v : v, update);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Slider::notify (ISubject* s, MessageRef msg)
{
	if(msg == kChanged)
		SuperClass::notify (s, msg);
	else
	{
		if(msg == IParameter::kBeginEdit)
		{
			if(setMouseState (kMouseDown))
				redraw ();
		}
		else if(msg == IParameter::kEndEdit)
		{
			if(setMouseState (GUI.getMouseView () == this ? kMouseOver : kMouseNone))
				redraw ();
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ThemeRenderer* Slider::getRenderer ()
{
	if(renderer == nullptr)
		renderer = getTheme ().createRenderer (ThemePainter::kSliderRenderer, visualStyle);
	return renderer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Slider::onMouseWheel (const MouseWheelEvent& event)
{
	if(getStyle ().isCustomStyle (Styles::kSliderBehaviorPassive))
		return false;
		
	if(View::onMouseWheel (event))
		return true;

	if(isWheelEnabled ())
		return tryWheelParam (event, getStyle ().isCustomStyle (Styles::kSliderBehaviorInverseWheel));
	
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Slider::onGesture (const GestureEvent& event)
{
	IParamPreviewHandler* previewHandler = getPreviewHandler ();
	if(event.getType () == GestureEvent::kDoubleTap
	   && !(event.getState () & GestureEvent::kPossible)
	   && canHandleDoubleTap ()
	   && style.isCustomStyle (Styles::kSliderBehaviorDoubleTap)
	   && previewHandler
	)
	{
		ParamPreviewEvent e;
		e.type = ParamPreviewEvent::kDoubleTap;
		previewHandler->paramPreview (param, e);
		return true;
	}

	return SuperClass::onGesture (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Slider::canHandleDoubleTap () const
{
	return isTouchResetEnabled || style.isCustomStyle (Styles::kSliderBehaviorDoubleTap);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MouseHandler* Slider::createMouseHandler (const MouseEvent& event)
{
	if(getStyle ().isCustomStyle (Styles::kSliderBehaviorPassive))
		return nullptr;
		
	if(isResetClick (event))
	{
		IParamPreviewHandler* previewHandler = getPreviewHandler ();
		if(style.isCustomStyle (Styles::kSliderBehaviorResetClick) && previewHandler)
		{
			ParamPreviewEvent e;
			e.type = ParamPreviewEvent::kResetClick;
			previewHandler->paramPreview (param, e);
		}
		else
			performReset ();
		return NEW NullMouseHandler (this); // swallow mouse click
	}

	auto isOptionClick = [] (const MouseEvent& e)
	{
		if(e.eventType == MouseEvent::kMouseDown)
			return e.keys == (KeyState::kOption|KeyState::kLButton);

		return false;
	};

	if(style.isCustomStyle (Styles::kSliderBehaviorOptionClick) && isOptionClick (event))
	{
		if(IParamPreviewHandler* previewHandler = getPreviewHandler ())
		{
			ParamPreviewEvent e;
			e.type = ParamPreviewEvent::kOptionClick;

			previewHandler->paramPreview (param, e);

			return NEW NullMouseHandler (this);
		}
	}

 	int mode;
	Point clickOffset;
	int max;
	bool handleClicked;
	getHandlerParams (event, event.where, mode, clickOffset, max, handleClicked);
	
	// check slider mode
	if(mode == Styles::kSliderModeTouch && !handleClicked)
		return nullptr; // let click pass through
		//return NEW NullMouseHandler (this); // swallow mouse click
	
	if(style.isCustomStyle (Styles::kSliderBehaviorXYEdit))
	{
		SliderXYMouseHandler* handler = NEW SliderXYMouseHandler (this, max, handleClicked, style.isCustomStyle (Styles::kSliderBehaviorEditTooltip));
		if(xyEditDistance > 0)
			handler->setXYDistance (xyEditDistance, xyEditDistance);
		return handler;
	}
	
	return NEW SliderMouseHandler (this, clickOffset, max, handleClicked);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Slider::getHandlerParams (const GUIEvent& event, const Point& where, int& mode, Point& clickOffset, int& max, bool& handleClicked)
{	
	Rect handleRect;
	getRenderer ()->getPartRect (this, kPartHandle, handleRect);
	
	Rect trackRect;
	getRenderer ()->getPartRect (this, kPartTrack, trackRect);
	
	max = getStyle ().isVertical () ? (trackRect.getHeight () - handleRect.getHeight ()) : (trackRect.getWidth  () - handleRect.getWidth ());

	mode = style.isCustomStyle (Styles::kSliderBehaviorGlobalMode) ? sliderMode.getValue () : instanceMode;
	if(SwipeBox::isSwiping (*this))
		mode = Styles::kSliderModeJump;

	// for easier touch input, always use relative mode instead of "touch" mode (a touchy subject)
	if(event.eventClass == GUIEvent::kTouchEvent && mode == Styles::kSliderModeTouch)
		mode = Styles::kSliderModeRelative;

	handleClicked = handleRect.pointInside (where) || mode == Styles::kSliderModeRelative;

	if(handleClicked)
	{
		clickOffset.x = where.x - handleRect.left + trackRect.left;
		clickOffset.y = where.y - handleRect.top + trackRect.top;
	}
	else
	{
		clickOffset.x = handleRect.getWidth () / 2;
		clickOffset.y = handleRect.getHeight () / 2;
	}
	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Slider::onMouseDown (const MouseEvent& event)
{
	if(getStyle ().isCustomStyle (Styles::kSliderBehaviorPassive))
	{
		setMouseState (View::kMouseDown);
		return false;
	}
	
	// right click swipe
	if(style.isCustomStyle (Styles::kSliderBehaviorSwipe) && event.keys.isSet(KeyState::kRButton))
	{
		Rect handleRect;
		getRenderer ()->getPartRect (this, kPartHandle, handleRect);
		if(handleRect.pointInside (event.where) || handleRect.getWidth () < 5 || handleRect.getHeight () < 5)
		{
			MouseHandler* handler = NEW SliderSwipeMouseHandler (this);
			handler->begin (event);
			getWindow ()->setMouseHandler (handler);
			return true;
		}
	}

	return SuperClass::onMouseDown (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITouchHandler* Slider::createTouchHandler (const TouchEvent& event)
{
	if(getStyle ().isCustomStyle (Styles::kSliderBehaviorPassive))
		return nullptr;
	
	Point where = event.touches.getTouchInfoByID (event.touchID)->where;
	windowToClient (where);

	int mode;
	Point clickOffset;
	int max;
	bool handleClicked;
	
	getHandlerParams (event, where, mode, clickOffset, max, handleClicked);
	
	if(mode == Styles::kSliderModeTouch && !handleClicked)
		return nullptr;
	
	if(style.isCustomStyle (Styles::kSliderBehaviorXYEdit))
	{
		SliderXYTouchHandler* handler = NEW SliderXYTouchHandler (this, clickOffset, max, handleClicked, mode);
		if(xyEditDistance > 0)
			handler->setXYDistance (xyEditDistance, xyEditDistance);
		return handler;
	}
	
	return NEW SliderTouchHandler (this, clickOffset, max, handleClicked, mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Slider::onMouseEnter (const MouseEvent& event)
{
	setMouseState (kMouseOver);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Slider::onMouseLeave (const MouseEvent& event)
{
	setMouseState (kMouseNone);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Slider::setXYEditDistance (Coord distance)
{
	if(distance > 0)
	{
		xyEditDistance = distance;
		style.setCustomStyle (Styles::kSliderBehaviorXYEdit, true);
	}
}

//************************************************************************************************
// RangeSlider
//************************************************************************************************

BEGIN_STYLEDEF (RangeSlider::customStyles)
	{"validate",	Styles::kRangeSliderBehaviorValidate},
	{"limit",		Styles::kRangeSliderBehaviorLimit},
	{"invertible",	Styles::kRangeSliderBehaviorInvertible},
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (RangeSlider, Slider)

//////////////////////////////////////////////////////////////////////////////////////////////////

RangeSlider::RangeSlider (const Rect& size, IParameter* _param, IParameter* _param2, StyleRef _style)
: Slider (size, _param, _style),
  param2 (nullptr),
  handleOutreach (0)
{
	// param and param2 are mandatory
	ASSERT(_param)
	ASSERT(_param2)

	share_and_observe_unknown<IParameter> (this, param2, _param2);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

RangeSlider::~RangeSlider ()
{
	share_and_observe_unknown<IParameter> (this, param2, nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float RangeSlider::getSecondValue () const
{
	if(style.isCustomStyle (Styles::kSliderBehaviorReverse))
		return 1.f - (float)NormalizedValue (param2).get ();
	
	return (float)NormalizedValue (param2).get ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RangeSlider::setSecondValue (float v, bool update)
{
	NormalizedValue (param2).set (style.isCustomStyle (Styles::kSliderBehaviorReverse) ? 1.f - v : v, update);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RangeSlider::updateTooltip (bool showEditValue)
{
	int idx = originalTooltip.index ("@value");
	bool hasValueIdentifier = (idx > 0);
	bool valuesUpdated = false;
	String text (hasValueIdentifier ? originalTooltip.subString (0, idx) : (showEditValue ? nullptr : originalTooltip));
	
	if(hasValueIdentifier || showEditValue)
	{
		String text1;
		param->toString (text1);
		String text2;
		param2->toString (text2);
		
		text << text1 << " - " << text2;
		valuesUpdated = true;
	}
	
	setTooltip (text);
	return valuesUpdated;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RangeSlider::performReset ()
{
	Control::performReset ();
	
	if(param2)
	{
		param2->beginEdit ();
		param2->setValue (param2->getDefaultValue (), true);
		param2->endEdit ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MouseHandler* RangeSlider::createMouseHandler (const MouseEvent& event)
{
	if(getStyle ().isCustomStyle (Styles::kSliderBehaviorPassive))
		return nullptr;
		
	if(isResetClick (event))
	{
		performReset ();
		return NEW NullMouseHandler (this); // swallow mouse click
	}
	
	return NEW RangeSliderMouseHandler (this, getEditHandlerSetup (event, event.where), style.isCustomStyle (Styles::kSliderBehaviorEditTooltip));
}


//////////////////////////////////////////////////////////////////////////////////////////////////

ITouchHandler* RangeSlider::createTouchHandler (const TouchEvent& event)
{
	if(getStyle ().isCustomStyle (Styles::kSliderBehaviorPassive))
		return nullptr;
		
	Point where = event.touches.getTouchInfoByID (event.touchID)->where;
	windowToClient (where);
	
	return NEW RangeSliderTouchHandler (this, getEditHandlerSetup (event, where), style.isCustomStyle (Styles::kSliderBehaviorEditTooltip));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RangeSlider::onMouseEnter (const MouseEvent& event)
{
	updateTooltip ();
	
	return SuperClass::onMouseEnter (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RangeSlider::onMouseWheel (const MouseWheelEvent& event)
{
	bool spreadMode = (event.keys.getModifiers () != 0);
	
	if(param && param->isEnabled () && param->canIncrement ())
	{
		int direction = event.getOriginalDirection ();

		bool inverse = getStyle ().isCustomStyle (Styles::kSliderBehaviorInverseWheel);
		if(param->isReverse ())
			inverse = !inverse;

		if(inverse)
		{
			if(direction == MouseWheelEvent::kWheelUp)
				direction = MouseWheelEvent::kWheelDown;
			else if(direction == MouseWheelEvent::kWheelDown)
				direction = MouseWheelEvent::kWheelUp;
		}
		 
		if(direction == MouseWheelEvent::kWheelUp)
		{
			param->beginEdit ();
			param2->beginEdit ();
			param2->increment ();
			if(spreadMode)
				param->decrement ();
			else
				param->increment ();
			param->endEdit ();
			param2->endEdit ();
		}
		if(direction == MouseWheelEvent::kWheelDown)
		{
			param->beginEdit ();
			param2->beginEdit ();
			if(spreadMode)
				param->increment ();
			else
				param->decrement ();
			param2->decrement ();
			param->endEdit ();
			param2->endEdit ();
		}
		
		if(updateTooltip ())
			GUI.retriggerTooltip (this);
		
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ThemeRenderer* RangeSlider::getRenderer ()
{
	if(renderer == nullptr)
		renderer = getTheme ().createRenderer (ThemePainter::kRangeSliderRenderer, visualStyle);
	return renderer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RangeSlider::attached (View* parent)
{
	Slider::attached (parent);
	
	updateStyle ();
	
	originalTooltip = getTooltip ();
	updateTooltip ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API RangeSlider::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kChanged)
	{
		if(UnknownPtr<IParameter> p = subject)
		{
			if(p == param || p == param2)
			{
				if(style.isCustomStyle (Styles::kRangeSliderBehaviorValidate))
					validateParams (p);
				paramChanged ();
				return;
			}
		}
	}
	
	SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RangeSlider::updateStyle ()
{
	if(visualStyle)
	{
		handleOutreach = visualStyle->getMetric ("outreach", 0);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RangeSlider::validateParams (IParameter* master)
{
	float v1 = (float)NormalizedValue (param).get ();
	float v2 = (float)NormalizedValue (param2).get ();
	bool reverse = style.isCustomStyle (Styles::kSliderBehaviorReverse);
	
	if((reverse && v1 < v2) || (!reverse && v1 > v2))
	{
		if(master == param2)
			NormalizedValue (param).set (v2, true);
		else if(master == param)
			NormalizedValue (param2).set (v1, true);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

RangeSlider::EditHandlerSetup RangeSlider::getEditHandlerSetup (const GUIEvent& event, PointRef where)
{
	EditHandlerSetup setup;
	
	Rect handleRect;
	getRenderer ()->getPartRect (this, kFirstHandle, handleRect);
	
	Rect handleRect2;
	getRenderer ()->getPartRect (this, kSecondHandle, handleRect2);
	
	Rect trackRect;
	getRenderer ()->getPartRect (this, kTrackBack, trackRect);
	
	setup.useLimits = style.isCustomStyle (Styles::kRangeSliderBehaviorLimit);
	setup.invertible = style.isCustomStyle (Styles::kRangeSliderBehaviorInvertible);
	setup.max = getStyle ().isVertical () ? (trackRect.getHeight () - handleRect.getHeight ()) : (trackRect.getWidth  () - handleRect.getWidth ());	// we assume handles have same dimensions...
	setup.xyEditing = style.isCustomStyle (Styles::kSliderBehaviorXYEdit);
	setup.xEditDistance = (xyEditDistance != 0) ? xyEditDistance : setup.max;
	setup.yEditDistance = (xyEditDistance != 0) ? xyEditDistance : 300;
	
	int mode = style.isCustomStyle (Styles::kSliderBehaviorGlobalMode) ? sliderMode.getValue () : instanceMode;
	
	// for easier touch input, always use relative mode instead of "touch" mode (a touchy subject)
	if(event.eventClass == GUIEvent::kTouchEvent && mode == Styles::kSliderModeTouch)
		mode = Styles::kSliderModeRelative;
	
	setup.handleClicked = (mode == Styles::kSliderModeRelative) ? true : false;
	
	// flip handle-rects if necessary to be able to move the inverted center-handle 
	bool handlesFlipped = false;
	if(setup.invertible)
	{
		if(handleRect.left > handleRect2.left)
		{	
			Rect tempRect (handleRect);
			handleRect = handleRect2;
			handleRect2 = tempRect;
			handlesFlipped = true;
		}
	}
	
	Rect handleTouchRect;
	Rect handleTouchRect2;
	Rect handleToHandleRect;
	
	// find edit range parameters
	if(getStyle ().isVertical ())
	{
		Coord handleDistance = handleRect2.top - handleRect.bottom;
		Coord handleOutreachToCenter = ccl_min (handleOutreach, ccl_sign(handleDistance) * handleDistance / 4);
		
		handleTouchRect2 (handleRect2.left, 0, handleRect2.right, handleRect2.bottom + handleOutreachToCenter);
		handleTouchRect (handleRect.left, handleRect.top - handleOutreachToCenter, handleRect.right, getHeight ());
		
		if(handleRect.bottom <= handleRect2.bottom)
		{
			handleToHandleRect.left = handleRect.left;
			handleToHandleRect.top = handleRect2.top - handleOutreachToCenter;
			handleToHandleRect.right = handleRect.right;
			handleToHandleRect.bottom = handleRect.bottom + handleOutreachToCenter;
		}
	}
	else
	{
		Coord handleDistance = handleRect2.left - handleRect.right;
		Coord handleOutreachToCenter = ccl_min (handleOutreach, ccl_sign(handleDistance) * handleDistance / 4);
		
		handleTouchRect (0, handleRect.top, handleRect.right + handleOutreachToCenter, handleRect.bottom);
		handleTouchRect2 (handleRect2.left - handleOutreachToCenter, handleRect2.top, getWidth (), handleRect2.bottom);

		if(handleRect.left <= handleRect2.left)
		{
			handleToHandleRect.left = handleRect.left - handleOutreachToCenter;
			handleToHandleRect.top = handleRect.top;
			handleToHandleRect.right = handleRect2.right + handleOutreachToCenter;
			handleToHandleRect.bottom = handleRect.bottom;
		}
	}
	
	setup.clickOffset.x = handleRect.getWidth () / 2;
	setup.clickOffset.y = handleRect.getHeight () / 2;
	
	if(handleTouchRect.pointInside (where))
	{
		setup.mainParam = handlesFlipped ? param2 : param;
		setup.additionalParam = handlesFlipped ? param : param2;
		setup.moveLowerHandle = handlesFlipped ? false : true;
		setup.moveUpperHandle = handlesFlipped ? true : false;
		
		if(handleToHandleRect.pointInside (where))
			setup.handleClicked = true;
		
		if(setup.handleClicked)
		{
			setup.clickOffset.x = where.x - handleRect.left + trackRect.left;
			setup.clickOffset.y = where.y - handleRect.top + trackRect.top;
		}
	}
	else if(handleTouchRect2.pointInside (where))
	{
		setup.mainParam = handlesFlipped ? param : param2;
		setup.additionalParam = handlesFlipped ? param2 : param;
		setup.moveLowerHandle = handlesFlipped ? true : false;
		setup.moveUpperHandle = handlesFlipped ? false : true;
		
		if(handleToHandleRect.pointInside (where))
			setup.handleClicked = true;
		
		if(setup.handleClicked)
		{
			setup.clickOffset.x = where.x - handleRect2.left + trackRect.left;
			setup.clickOffset.y = where.y - handleRect2.top + trackRect.top;
		}
	}
	else
	{
		setup.handleClicked = true;
		setup.mainParam = param;
		setup.additionalParam = param2;
		setup.moveLowerHandle = false;
		setup.moveUpperHandle = false;
		setup.clickOffset.x = where.x - handleRect.left + trackRect.left;
		setup.clickOffset.y = where.y - handleRect.top + trackRect.top;
	}
	
	return setup;
}
