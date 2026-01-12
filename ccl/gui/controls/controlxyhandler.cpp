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
// Filename    : ccl/gui/controls/controlxyhandler.cpp
// Description : mousehandler for xy editing
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/controls/controlxyhandler.h"
#include "ccl/gui/controls/control.h"
#include "ccl/gui/gui.h"

#include "ccl/public/math/mathprimitives.h"

namespace CCL {

//************************************************************************************************
// ControlXYEditManipulator
//************************************************************************************************

ControlXYEditManipulator::ControlXYEditManipulator (Control* control, IParameter* _editParam, int options)
: control (control),
  editParam (_editParam ? _editParam : control->getParameter ()),
  normalizedValue (editParam),
  distanceVertical (ccl_min (100000, editParam->getPrecision ())),
  distanceHorizontal (ccl_min (100000, editParam->getPrecision ())),
  isBipolar (editParam->isBipolar ()),
  startValue (0),
  latestTime (0),
  wasFineMode (false),
  directionDetected (false),
  options (options)
{
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

void ControlXYEditManipulator::initialize (PointRef where, double when, bool isFineMode, double normalizedStartValue)
{
	if(normalizedStartValue != -1)
		normalizedValue.set ((isReverse ()) ? (1 - normalizedStartValue) : normalizedStartValue, true);

	latestTime = when;
	latestPosition = where;

	startValue = (isReverse ()) ? (1 - normalizedValue.get ()) : normalizedValue.get ();
	wasFineMode = isFineMode;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ControlXYEditManipulator::move (PointRef where, double when, bool isFineMode, Direction externalDirection)
{
	double currentYDistance = double(latestPosition.y - where.y);
	double currentXDistance = double(where.x - latestPosition.x);
	
	Direction direction = (externalDirection == kUndefined) ? detectDirection (float(currentXDistance), float(currentYDistance)) : externalDirection;
	
	if(!directionDetected)
	{
		if(direction != kUndefined)
			directionDetected = true;

		return;
	}
	
	if(wasFineMode != isFineMode)
	{
		initialize (where, when, isFineMode);
	}

	switch (direction)
	{
		case kVertical: currentXDistance = 0; break;
		case kHorizontal: currentYDistance = 0; break;
		case kUndefined: currentXDistance = currentYDistance = 0; break;
	}
	
	double delta;
	
	if(isAccelerated ())
	{
		double timeDifference = when - latestTime;
		
		if(timeDifference == 0)
			return;
		
		latestTime = when;
		ASSERT (when != 0) // no event timestamp set
		
		delta = (1.0 / (double)distanceHorizontal) + (1.0 / (double)distanceVertical);
		double range = ccl_bound (editParam->getMax ().asDouble () - editParam->getMin ().asDouble (), 100.0, 10000000.0);
		double speed = (currentYDistance + currentXDistance) / timeDifference;
		
		delta = (delta / range) * speed;
	}
	else
		delta = currentXDistance / (double)distanceHorizontal + currentYDistance / (double)distanceVertical;
	
	latestPosition = where;
	
	if(delta != 0)
	{
		double epsilon = (direction == kVertical) ? 1 / (double)distanceVertical : 1 / (double)distanceHorizontal;

		const float kFineModeFactor = 0.05f;
		if(isFineMode)
		{
			delta *= kFineModeFactor;
			epsilon *= 0.4f;
			float verticalFactor = (distanceVertical / (float)distanceHorizontal);
			if(direction == kVertical)
			{
				delta *= verticalFactor;
				epsilon *= verticalFactor;
			}
		}

		double newValue = startValue + delta;
		double newStartValue = newValue;
		double steps = editParam->getPrecision ();
		if(isFineMode)
			steps /= kFineModeFactor;

		if(steps > 0)
			newValue = ccl_bound<double> (ccl_round<0> (newValue * steps) / steps, 0., 1.);

		if(isBipolar && ccl_equals (newValue, 0.5, epsilon))
			newValue = 0.5;
			
		bool spreadRange = (control->getStyle ().isVertical () && (direction == kHorizontal));
		spreadRange |= (control->getStyle ().isHorizontal () && (direction == kVertical));
		setNewValue (newValue, float(delta), spreadRange);
		startValue = newStartValue;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ControlXYEditManipulator::setNewValue (double newValue, float delta, bool)
{
	normalizedValue.set ((isReverse ()) ? (1 - newValue) : newValue, true);
	startValue = newValue;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ControlXYEditManipulator::setXYDistance (int _distanceHorizontal, int _distanceVertical)
{
	distanceHorizontal = _distanceHorizontal;
	distanceVertical = _distanceVertical;
	isAccelerated (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ControlXYEditManipulator::Direction ControlXYEditManipulator::detectDirection (float currentX, float currentY)
{
	if((ccl_abs (currentX)) < (ccl_abs (currentY)))
		return kVertical;
	else if ((ccl_abs (currentX)) > (ccl_abs (currentY)))
		return kHorizontal;
	
	return kUndefined;
}

//************************************************************************************************
// ControlXYMouseHandler
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ControlXYMouseHandler, PeriodicMouseHandler)

ControlXYMouseHandler::ControlXYMouseHandler (Control* control, bool showEditTooltip, int options)
: PeriodicMouseHandler (control),
  control (control),
  editManipulator (control, control->getParameter (), options),
  showEditTooltip (showEditTooltip),
  accuX (0),
  accuY (0),
  startJumpValue (-1),
  preferredDirection (ControlXYEditManipulator::kUndefined),
  verticalSizer (MouseCursor::createCursor (ThemeElements::kSizeVerticalCursor))
{
	checkKeys (true);
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

ControlXYMouseHandler::~ControlXYMouseHandler ()
{
	tooltipPopup.reserve (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ControlXYMouseHandler::setXYDistance (int distanceHorizontal, int distanceVertical)
{
	getManipulator ().setXYDistance (distanceHorizontal, distanceVertical);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ControlXYMouseHandler::onBegin ()
{
	previousWhere = current.where;
	bool isFineMode = (first.keys.getModifiers () & KeyState::kShift) != 0;
	
	control->getParameter ()->beginEdit ();
	getManipulator ().initialize (first.where, first.eventTime, isFineMode, startJumpValue);
	
	if(showEditTooltip)
	{
		tooltipPopup.setTooltip (control->makeEditTooltip ());
		tooltipPopup.reserve (true);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ControlXYMouseHandler::onRelease (bool canceled)
{
	control->getParameter ()->endEdit ();
	control->killFocus ();
	
	GUI.setCursor (nullptr);

	tooltipPopup.reserve (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ControlXYMouseHandler::onPeriodic ()
{
	// the vertical axis lock (the if-statement && - 0.1 below) can be removed...
	// ...after patent application WO2015036036A1 is rejected
	//if(preferredDirection == ControlXYEditManipulator::kVertical)
	//	return true;
	
	static const float kDamping = 0.8f;

	float deltaX = float (current.where.x - previousWhere.x);
	float deltaY = float (previousWhere.y - current.where.y);

	previousWhere = current.where;

	accuX = accuX * kDamping + (ccl_abs (deltaX) * (1 - kDamping));
	accuY = accuY * kDamping + (ccl_abs (deltaY) * (1 - kDamping));

	accuX = ccl_max<float> (accuX, 0.1f);
	accuY = ccl_max<float> (accuY, 0.1f);

	CCL_PRINTF("accuX/accuY:  %f   %f \n", accuX, accuY)

	if(accuX < accuY)// - 0.1))
		preferredDirection = ControlXYEditManipulator::kVertical;
	else if (accuX > accuY)
		preferredDirection = ControlXYEditManipulator::kHorizontal;
	else
		preferredDirection = ControlXYEditManipulator::kUndefined;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ControlXYMouseHandler::onMove (int moveFlags)
{
	if(moveFlags & kPeriodicMove)	// filter all periodic moves
	{
		PeriodicMouseHandler::onMove (moveFlags);
		return true;
	}
	else if (moveFlags & kMouseMoved)
	{
		bool isFineMode = (current.keys & KeyState::kShift) != 0;
		getManipulator ().move (current.where, current.eventTime, isFineMode, preferredDirection);

		if(preferredDirection == ControlXYEditManipulator::kVertical)
			GUI.setCursor (verticalSizer);
		
		if(showEditTooltip)
		{
	
			
			tooltipPopup.setTooltip (control->makeEditTooltip ());
			tooltipPopup.reserve (true);
		}
	}
	return true;
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

ControlXYEditManipulator& ControlXYMouseHandler::getManipulator ()
{
	return editManipulator;
}

} // namespace CCL
