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
// Filename    : core/portable/gui/coretouchinput.h
// Description : Touch Input State
//
//************************************************************************************************

#ifndef _coretouchinput_h
#define _coretouchinput_h

#include "core/gui/coregesturerecognition.h"

namespace Core {
namespace Portable {

class View;
struct TouchEvent;

//************************************************************************************************
// TouchItem
/** \ingroup core_gui */
//************************************************************************************************

class TouchItem
{
public:
	TouchItem (TouchID touchID = 0)
	: touchID (touchID), view (nullptr)
	{}

	TouchItem (const TouchItem& item)
	: touchID (item.touchID), view (item.view)
	{}

	TouchItem& operator= (const TouchItem& item) { touchID = item.touchID; view = item.view; return *this;}

	PROPERTY_VARIABLE (TouchID, touchID, ID)
	PROPERTY_POINTER (View, view, View)	///< view that receives touch events for this touch (can be 0)

	bool operator== (const TouchItem& item) { return item.getID () == touchID; }
};

typedef FixedSizeVector<TouchItem, 8> TouchItemVector;

//************************************************************************************************
// Gesture
/** \ingroup core_gui */
//************************************************************************************************

class Gesture: public Core::GestureInfo
{
public:
	Gesture (int type = -1)
	: GestureInfo (type),
	  view (nullptr),
	  priority (-1),
	  userData (0)
	{}

	bool operator== (const Gesture& gesture)
	{
		return &gesture == this;
	}

	PROPERTY_POINTER (View, view, View)			///< view that wants to handle the gesture
	PROPERTY_VARIABLE (int, priority, Priority)	///< the view's priority for this gesture
	int userData;								///< passed to view with each gesture event
};

//************************************************************************************************
// TouchInputState
/** \ingroup core_gui */
//************************************************************************************************

class TouchInputState: public GestureRecognition::GestureSink
{
public:
	TouchInputState (View& rootView);
	~TouchInputState ();

	PROPERTY_POINTER (View, rootView, RootView)
	
	void onTouchInput (const TouchEvent& e);
	void onIdle ();	
	void viewRemoved (View* view);

	// GestureSink
	void onGesture (GestureInfo* gesture, int state, const GestureEventArgs& args) override;

private:
	GestureRecognition gestureRecognition;
	TouchItemVector touchItems;
	FixedSizeVector<Gesture, 8> gestureMemory;
	FixedSizeVector<Gesture*, 8> gestures;

	class TouchInfo;
	enum { kSingleTouchID = 1 };

	TouchItem* addTouchItem (TouchID touchID);
	TouchItem* getTouchItem (TouchID id) const;

	bool collectTouchHandlers (View& view, PointRef where, TouchID touchID);
	void acceptTouchHandler (TouchItem& touchItem, View* view, PointRef where);
	void sendTouchEvent (View& view, PointRef where, int type);

	void onTouchBegan (const TouchEvent& e);
	void onTouchChanged (const TouchEvent& e);
	void onTouchEnded (const TouchEvent& e);
	
	Gesture* getGesture (int gestureType, TouchID touchID, bool add);
	void removeGesture (Gesture* gesture);
	void updateGestureRecognizers ();
};

} // namespace Portable
} // namespace Core

#endif // _coretouchinput_h
