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
// Filename    : core/portable/gui/coretouchinput.cpp
// Description : Touch Input State
//
//************************************************************************************************

#define DEBUG_LOG (0 && DEBUG)

#include "core/portable/gui/coretouchinput.h"
#include "core/portable/gui/coreview.h"
#include "core/system/coretime.h"

#include "core/gui/coregesturerecognition.impl.h"

using namespace Core;
using namespace Portable;

//************************************************************************************************
// TouchInputState::TouchInfo
//************************************************************************************************

class TouchInputState::TouchInfo: public Core::TouchInfo
{
public:
	TouchInfo (const TouchEvent& event)
	: Core::TouchInfo (translateType (event.type),
	  kSingleTouchID,
	  event.where,
	  SystemClock::getMilliseconds ())
	{}

private:
	static inline int translateType (int type)
	{
		switch(type)
		{
		case TouchEvent::kDown:	return kTouchBegin;
		case TouchEvent::kMove:	return kTouchMove;
		case TouchEvent::kUp:	return kTouchEnd;
		}
		return kTouchMove;
	};
};

//************************************************************************************************
// TouchInputState
//************************************************************************************************

TouchInputState::TouchInputState (View& rootView)
: rootView (&rootView)
{
	gestureMemory.setCount (gestureMemory.getCapacity ());

	gestureRecognition.setGestureSink (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TouchInputState::~TouchInputState ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

TouchItem* TouchInputState::addTouchItem (TouchID touchID)
{
	TouchItem touchItem (touchID);
	touchItems.add (touchItem);
	return &touchItems.last ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TouchItem* TouchInputState::getTouchItem (TouchID id) const
{
	VectorForEach (touchItems, const TouchItem&, item)
		if(item.getID () == id)
			return const_cast<TouchItem*> (&item);
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TouchInputState::collectTouchHandlers (View& view, PointRef where, TouchID touchID)
{
	bool result = false;

	// try child views first
	if(ContainerView* cv = view.asContainer ())
		VectorForEachReverse (cv->getChildren (), View*, child)
			if(child->isEnabled () && child->getSize ().pointInside (where))
			{
				Point where2 (where);
				where2.offset (-child->getSize ().left, -child->getSize ().top);

				if(collectTouchHandlers (*child, where2, touchID))
					result = true;
			}
		EndFor

	// get gestures handled by this view
	GestureVector gestureCodes;
	view.getHandledGestures (gestureCodes, where);
	if(!gestureCodes.isEmpty ())
	{
		VectorForEach (gestureCodes, int, g)
			int gestureType = g & kGestureTypeMask;
			int priority = g & kGesturePrioritiesMask;

			Gesture* gesture = getGesture (gestureType, touchID, true);
			if(gesture && gesture->getPriority () < priority)
			{
				#if DEBUG_LOG
				DebugPrintf ("gesture: \"%s\" handled by \"%s\" (Prio: %d)\n", GestureInfo::getGestureName (gestureType), view.getName ().str (), (priority >> 1) / kGesturePriorityLow);
				#endif

				gesture->setView (&view);
				gesture->setPriority (priority);
				gesture->addTouch (touchID);
				result = true;
			}
		EndFor
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchInputState::acceptTouchHandler (TouchItem& touchItem, View* view, PointRef where)
{
	touchItem.setView (view);

	sendTouchEvent (*view, where, TouchEvent::kDown);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchInputState::sendTouchEvent (View& view, PointRef where, int type)
{
	Point p (where);
	view.rootToClient (p);

	TouchEvent event ((TouchEvent::Type)type, p);
	view.onTouchInput (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchInputState::onTouchInput (const TouchEvent& e)
{
	switch(e.type)
	{
	case TouchEvent::kDown:
		onTouchBegan (e);
		break;

	case TouchEvent::kMove:
		onTouchChanged (e);
		break;

	case TouchEvent::kUp:
		onTouchEnded (e);
		break;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchInputState::onTouchBegan (const TouchEvent& e)
{
	if(!rootView)
		return;

	Point where (e.where);
	rootView->rootToClient (where);

	TouchItem* touchItem = addTouchItem (kSingleTouchID);

	// collect views and their handled gestures
	bool gesturesRequired = collectTouchHandlers (*rootView, where, kSingleTouchID);
	if(!gesturesRequired)
	{
		// if no candidate requires a specific gesture, accept the root view
		acceptTouchHandler (*touchItem, rootView, where);
	}
	else
	{
		// if there is only one handler for all gestures, accept it immediately
		View* commonHandler = nullptr;
		VectorForEach (gestures, Gesture*, gesture)
			if(gesture->getTouchIDs ().count () == 1 && gesture->getTouchIDs ().at (0) == touchItem->getID ())
			{
				View* handler = gesture->getView ();
				if(!handler)
				{
					commonHandler = nullptr;
					break;
				}

				if(commonHandler == nullptr)
					commonHandler = handler;
				else if(handler != commonHandler)
				{
					commonHandler = nullptr;
					break;
				}
			}
		EndFor

		if(commonHandler)
		{
			Point where (e.where);
			commonHandler->rootToClient (where);

			#if DEBUG_LOG
			DebugPrintf ("onGesture: %s\n", GestureInfo::getGestureName (kGesturePossible));
			#endif

			int userData = 0;
			GestureEvent event (userData, kGesturePossible, where);
			commonHandler->onGestureInput (event);
		}

		#if 0
		if(commonHandler)
			acceptTouchHandler (*touchItem, commonHandler, e.where);
		#endif
	}

	updateGestureRecognizers ();

	TouchInfo touchInfo (e);
	gestureRecognition.onTouchBegan (touchInfo);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchInputState::onTouchChanged (const TouchEvent& e)
{
	if(TouchItem* item = getTouchItem (kSingleTouchID))
	{
		// feed to touch view, if one was accepted
		if(View* view = item->getView ())
			sendTouchEvent (*view, e.where, e.type);

		TouchInfo touchInfo (e);
		gestureRecognition.onTouchChanged (touchInfo);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchInputState::onTouchEnded (const TouchEvent& e)
{
	TouchID touchID = kSingleTouchID;
	if(TouchItem* item = getTouchItem (touchID))
	{
		// feed to touch view, if one was accepted
		if(View* view = item->getView ())
			sendTouchEvent (*view, e.where, e.type);

		TouchInfo touchInfo (e);
		gestureRecognition.onTouchEnded (touchInfo);

		touchItems.remove (*item);

		bool isLast = touchItems.count () == 1;

		// check if a double tap gesture is pending for this touch
		Gesture* doubleTap = getGesture (kGestureDoubleTap, touchID, false);
		if(doubleTap)
		{
			if(doubleTap->canRemove ())
				doubleTap = nullptr; // already done
			else
				isLast = false;
		}

		// remove touch from gestures, remove obsolete gestures
		// note: some gestures (e.g. double tap) must survive touches
		VectorForEachReverse (gestures, Gesture*, gesture)
			if(gesture->getTouchIDs ().remove (touchID))
			{
				// a single tap gesture must survive it's touch if a double tap is still pending (might be triggered via double tap timeout)
				if(gesture->canRemove () && !(doubleTap && gesture->getType () == kGestureSingleTap))
					removeGesture (gesture);
				else
					isLast = false;
			}
		EndFor

		if(isLast)
			VectorForEachReverse (gestures, Gesture*, gesture)
				removeGesture (gesture);
			EndFor
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchInputState::viewRemoved (View* view)
{
	// give up all gestures and touches referring to affected views
	ContainerView* removedContainer = removedContainer = view->asContainer ();

	VectorForEachReverse (gestures, Gesture*, gesture)
		if(gesture->getView () == view || (removedContainer && removedContainer->isChildView (gesture->getView ())))
			removeGesture (gesture);
	EndFor

	VectorForEachReverse (touchItems, const TouchItem&, item)
		if(item.getView () == view || (removedContainer && removedContainer->isChildView (item.getView ())))
			touchItems.remove (item);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Gesture* TouchInputState::getGesture (int gestureType, TouchID touchID, bool add)
{
	VectorForEach (gestures, Gesture*, gesture)
		if(gesture->getType () == gestureType)
			if(gesture->getTouchIDs ().contains (touchID))
				return gesture;
	EndFor

	if(add)
	{
		// find empty slot in gesture memory
		VectorForEach (gestureMemory, Gesture&, slot)
			if(slot.getType () == -1)
			{
				slot = Gesture (gestureType);
				gestures.add (&slot);
				return &slot;
			}
		EndFor
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchInputState::removeGesture (Gesture* gesture)
{
	gestureRecognition.stopRecognizing (gesture);

	// remove pointer, set slot empty (other gestures must stay at their memory locations)
	gestures.remove (gesture);
	*gesture = Gesture ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchInputState::updateGestureRecognizers ()
{
	VectorForEach (gestures, Gesture*, gesture)
		if(!gestureRecognition.isRecognizing (gesture))
			gestureRecognition.startRecognizing (gesture);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchInputState::onGesture (GestureInfo* gestureInfo, int state, const GestureEventArgs& args)
{
	Gesture* gesture = static_cast<Gesture*> (gestureInfo);

	#if DEBUG_LOG
	DebugPrintf ("onGesture: %s\n", GestureInfo::getGestureName (gestureInfo->getType () | state));
	#endif

	switch(state)
	{
	case kGestureBegin:
		if(gesture->getView ())
		{
			#if 0
			View* view = gesture->getView ();
			// beginning of gesture: accept handlers of associated touches
			VectorForEach (gesture->getTouchIDs (), TouchID, touchID)
				if(TouchItem* touchItem = getTouchItem (touchID))
					acceptTouchHandler (*touchItem, view, args.where);
			EndFor
			#endif

			// remove other pending gestures with touches from this gesture
			VectorForEachReverse (gestures, Gesture*, otherGesture)
				if(otherGesture != gesture)
					VectorForEach (gesture->getTouchIDs (), TouchID, id)
						if(otherGesture->getTouchIDs ().contains (id))
						{
							removeGesture (otherGesture);
							break;
						}
					EndFor
			EndFor
		}
		break;

	case kGestureChanged:
		break;

	case kGestureEnd:
		break;

	case kGestureFailed:
		break;
	}

	if(View* view = gesture->getView ())
	{
		Point where (Coord (args.where.x + 0.5f), Coord (args.where.y + 0.5f));
		view->rootToClient (where);
		GestureEvent event (gesture->userData, gesture->getType () | state, where, args.amountX, args.amountY);
		view->onGestureInput (event);
	}

	// remove succeeded tap gestures when touch is already gone
	if(state != kGestureFailed && gesture->isTap () && gesture->getTouchIDs ().isEmpty ())
	{
		VectorForEachReverse (gestures, Gesture*, g)
			if(g->isTap () && g->getTouchIDs ().isEmpty ())
				removeGesture (g);
		EndFor
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchInputState::onIdle ()
{
	gestureRecognition.processIdle (SystemClock::getMilliseconds ());
}
