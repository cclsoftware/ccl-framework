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
// Filename    : ccl/gui/controls/controlxyhandler.h
// Description : mousehandler for xy editing
//
//************************************************************************************************

#ifndef _ccl_controlxyhandler_h
#define _ccl_controlxyhandler_h

#include "ccl/public/gui/iparameter.h"

#include "ccl/gui/views/mousehandler.h"
#include "ccl/gui/system/mousecursor.h"

namespace CCL {

class Control;

//************************************************************************************************
// ControlXYManipulator
//************************************************************************************************

class ControlXYEditManipulator
{
public:
	ControlXYEditManipulator (Control* control, IParameter* _editParam = nullptr, int options = 0);

	enum Direction
	{
		kUndefined,
		kVertical,
		kHorizontal
	};
	
	enum ControlXYOptions
	{
		kReverse = 1 << 0,
		kAccelerated = 1 << 1
	};

	PROPERTY_FLAG (options, ControlXYOptions::kReverse, isReverse)
	PROPERTY_FLAG (options, ControlXYOptions::kAccelerated, isAccelerated)

	virtual void initialize (PointRef where, double when, bool isFineMode, double normalizedStartValue = -1);
	
	void move (PointRef where, double when, bool isFineMode, Direction externalDirection = kUndefined);
	void setXYDistance (int distanceHorizontal, int distanceVertical);
	
protected:

	virtual void setNewValue (double newValue, float delta, bool spreadRange);
	Direction detectDirection (float currentX, float currentY);

	Control* control;
	IParameter* editParam;
	NormalizedValue normalizedValue;
	int distanceHorizontal;
	int distanceVertical;
	double startValue;
    double latestTime;
	Point latestPosition;
	bool isBipolar;
	bool wasFineMode;
	bool directionDetected;
	int options;
};

//************************************************************************************************
// ControlXYMouseHandler
/** Mouse handler for controls - supporting param manipulation on X/Y-axis by detecting the preferred direction */
//************************************************************************************************

class ControlXYMouseHandler: public PeriodicMouseHandler
{
public:
	DECLARE_CLASS (ControlXYMouseHandler, PeriodicMouseHandler)
	
	ControlXYMouseHandler (Control* control = nullptr, bool showEditTooltip = false, int options = 0);
	~ControlXYMouseHandler ();
	
	void onBegin () override;
	bool onMove (int moveFlags) override;
	void onRelease (bool canceled) override;
	bool onPeriodic () override;

	void setXYDistance (int distanceHorizontal, int distanceVertical);
	
protected:
	virtual ControlXYEditManipulator& getManipulator ();
	
	Point previousWhere;
	Control* control;
	ControlXYEditManipulator editManipulator;
	bool showEditTooltip;
	float accuX;
	float accuY;
	double startJumpValue;
	ControlXYEditManipulator::Direction preferredDirection;
	AutoPtr<MouseCursor> verticalSizer;
};

} // namespace CCL


#endif // _ccl_controlxyhandler_h
