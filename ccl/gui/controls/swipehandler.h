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
// Filename    : ccl/gui/controls/swipehandler.h
// Description : Base class for swipe mousehandlers
//
//************************************************************************************************

#ifndef _ccl_swipehandler_h
#define _ccl_swipehandler_h

#include "ccl/gui/views/mousehandler.h"
#include "ccl/gui/controls/control.h"

#include "ccl/public/base/variant.h"

namespace CCL {

class Control;

//************************************************************************************************
// SwipeMouseHandler
/** Base class for swipe mousehandlers. */
//************************************************************************************************

class SwipeMouseHandler: public MouseHandler
{
public:
	DECLARE_CLASS_ABSTRACT (SwipeMouseHandler, MouseHandler)

	SwipeMouseHandler (Control* control, int swipeMode = kSwipeAny);

	enum SwipeMode { kNoSwipe, kSwipeAny, kSwipeHorizontal, kSwipeVertical };

	PROPERTY_VARIABLE (SwipeMode, swipeMode, SwipeMode)

	PROPERTY_FLAG (flags, 1<<0, ignoreName)
	PROPERTY_FLAG (flags, 1<<1, ignoreTag)

protected:
	struct SwipeCondition
	{
		// a swipe candidate must have the same name, tag and initial value
		Control* control;
		String name;
		int tag;
		Variant value;

		SwipeCondition (Control* control);
	};
	SwipeCondition swipeCondition;
	int flags;

	SwipeMouseHandler (View& view, MetaClassRef controlClass, int swipeMode = kSwipeAny);
	void initStartControl (Control* control);
	Control* findControl (View& parentView, const Point& where) const;
	bool trySwipe ();

	virtual void onSwipeEnter (Control* newControl) = 0;	///< to be implemented by subclass: new control entered
	virtual bool checkCondition (const SwipeCondition& c);	///< check if new control matches the original one, base class checks for same name and tag

protected:
	Control* lastControl;
	MetaClassRef controlClass;
	
private:
	Point origin, lastPos; // in window coords
};

//************************************************************************************************
// SwipeBox
/** Used to allow a "swipe" with the mouse or finger over a row of controls. 

When the user swipes over the swipe box, all views in the area of the swipe box (they don't have to be child views) 
receive mouse events when the mouse enters / moves inside / leaves them. 
This way it's possible to perform one-shot gesture over controls that don't support this directly (like e.g. Button does with it's "swipe" option).

If the swipe box has a parameter ("name" attribute), swiping is only enabled when the parameter has the value "true". */
//************************************************************************************************

class SwipeBox: public Control
{
public:
	DECLARE_CLASS (SwipeBox, Control)
	
	DECLARE_STYLEDEF (customStyles)
	
	static bool isSwiping (View& view);

	SwipeBox (const Rect& size = Rect (), StringID targetClass = nullptr, IParameter* param = nullptr, StyleRef style = 0);

	PROPERTY_MUTABLE_CSTRING (targetClass, TargetClass)

	// Control
	MouseHandler* createMouseHandler (const MouseEvent& event) override;
	ITouchHandler* createTouchHandler (const TouchEvent& event) override;
	bool onMouseDown (const MouseEvent& event) override;

private:
	class MetaMouseHandler;
	
	bool swipeAlways;
};

} // namespace CCL

#endif // _ccl_swipehandler_h
