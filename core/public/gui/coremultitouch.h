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
// Filename    : core/public/gui/coremultitouch.h
// Description : Multi-touch and gesture recognition
//
//************************************************************************************************

#ifndef _coremultitouch_h
#define _coremultitouch_h

#include "core/public/gui/corepoint.h"

namespace Core {

/** ID of one touch. */
typedef UIntPtr TouchID;

//************************************************************************************************
// TouchEventType
/** Types of touch events. 
	\ingroup core_gui gui */
//************************************************************************************************

enum TouchEventType
{
	kTouchBegin,	///< Pointer has touched the screen
	kTouchMove,		///< Pointer has moved on the screen
	kTouchEnd,		///< Pointer was removed from the screen (loses contact, but might still be detected)

	// optional hover states
	kTouchEnter,	///< Pointer has appeared (but might not have touched the screen yet)
	kTouchHover,	///< Pointer hovered above the screen (no contact)
	kTouchLeave,	///< Pointer disappeared

	// cancel touch processing
	kTouchCancel	///< Touches should be discarded, e.g. because of triggered palm rejection
};

//************************************************************************************************
// GestureEventType
/**	Types of gesture events. 
	\ingroup core_gui gui */
//************************************************************************************************

enum GestureEventType
{
	// Touch Gestures
	kGestureSwipe,
	kGestureZoom,
	kGestureRotate,
	kGestureLongPress,
	kGestureSingleTap,
	kGestureDoubleTap,

	// Pen Gestures
	kGesturePenPrimary,

	// States (optional)
	kGestureBegin	 = 1<<16,
	kGestureChanged	 = 1<<17,
	kGestureEnd		 = 1<<18,
	kGestureFailed	 = 1<<19,
	kGesturePossible = 1<<20,

	// Constraints (optional)
	kGestureHorizontal = 1<<21,
	kGestureVertical = 1<<22,
	kGestureExclusiveTouch = 1<<23,	///< gesture will be ignored when multiple touches are involved (only supported for swipe); another gesture might apply instead
	kGestureSuppressContextMenu = 1<<24, ///< a possible longpress context menu will be suppressed if any handler candidate provides this flag
	
	kGestureConstraintsMask = kGestureHorizontal|kGestureVertical,
	kGestureStatesMask = kGestureBegin|kGestureChanged|kGestureEnd|kGestureFailed|kGesturePossible,
	kGestureTypeMask = kGestureSwipe|kGestureZoom|kGestureRotate|kGestureLongPress|kGestureSingleTap|kGestureDoubleTap
};

//************************************************************************************************
// GesturePriorities
/** Gesture priority types. 
	\ingroup core_gui gui */
//************************************************************************************************

enum GesturePriorities
{
	kGesturePriorityLow		= 1<<24,
	kGesturePriorityNormal	= 1<<25,
	kGesturePriorityHigh	= 1<<26,
	kGesturePriorityHighest	= 1<<27,

	kGesturePrioritiesMask = kGesturePriorityLow|kGesturePriorityNormal|kGesturePriorityHigh|kGesturePriorityHighest
};

//************************************************************************************************
// TouchInfo
/** Touch Info.
	\ingroup core_gui gui */
//************************************************************************************************

struct TouchInfo
{	
	int type;		///< @see TouchEventType
	TouchID id;
	Point where;
	PointF whereF;
	int64 time;		///< milliseconds (1000 equals one second)

	TouchInfo (int type = kTouchBegin, 
			   TouchID id = 0,
			   PointRef where = Point (),
			   int64 time = 0)
	: type (type),
	  id (id),
	  where (where),
	  whereF ((CoordF)where.x, (CoordF)where.y),
	  time (time)
	{}
	
	TouchInfo (int type,
			   TouchID id,
			   PointFRef whereF,
			   int64 time = 0)
	: type (type),
	  id (id),
	  where (Coord (whereF.x + 0.5f), Coord (whereF.y + 0.5f)),
	  whereF (whereF),
	  time (time)
	{}

	bool operator == (const TouchInfo& other) const
	{	
		return id == other.id ?	true : false;
	}

	void setPosition (PointRef p)
	{
		where = p;
		whereF ((CoordF)where.x, (CoordF)where.y);
	}

	void setPosition (PointFRef p)
	{
		where (Coord (p.x + 0.5f), Coord (p.y + 0.5f));
		whereF = p;
	}
};

} // namespace Core

#endif // _coremultitouch_h
