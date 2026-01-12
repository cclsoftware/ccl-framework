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
// Filename    : ccl/gui/views/touchinput.cpp
// Description : Touch Input State
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/touch/touchinput.h"
#include "ccl/gui/touch/touchhandler.h"
#include "ccl/gui/touch/touchcollection.h"
#include "ccl/gui/touch/gesturemanager.h"

#include "ccl/gui/controls/selectbox.h"
#include "ccl/gui/popup/popupselector.h"
#include "ccl/gui/windows/window.h"
#include "ccl/gui/windows/dialog.h"
#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/system/dragndrop.h"
#include "ccl/gui/gui.h"

#include "ccl/base/message.h"

#include "ccl/public/systemservices.h"
#include "ccl/public/gui/framework/abstractdraghandler.h"

//////////////////////////////////////////////////////////////////////////////////////////////////

#define LOG_WINDOW 0
#define TOUCHID_IS_NUMBER (CCL_PLATFORM_WINDOWS || CCL_PLATFORM_ANDROID)

//////////////////////////////////////////////////////////////////////////////////////////////////

#if TOUCHID_IS_NUMBER
#define TOUCHID_STR(touchId) MutableCString ().appendFormat ("%d", (int)touchId).str ()
#else
#define TOUCHID_STR(touchId) CCL::Debugger::ObjectID ((void*)touchId).str
#endif

#if LOG_WINDOW
#define WINDOW_STR MutableCString ("[").append (rootView.myClass ().getPersistentName ()).append ("] ").str ()
#else
#define WINDOW_STR ""
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////

#if DEBUG_LOG
static void logGesture (const CCL::GestureEvent& event, CCL::CStringPtr viewName)
{
	static CCL::GestureEvent lastEvent;
	static CCL::MutableCString lastViewName;
	enum { kMinDist = 8 };

	// swallow same messages when moved inside a tolerance
	if(event.eventType != lastEvent.eventType
		|| ccl_abs (event.where.x - lastEvent.where.x) > kMinDist
		|| ccl_abs (event.where.y - lastEvent.where.y) > kMinDist
		|| ccl_abs (event.amountX - lastEvent.amountX) > 0.001
		|| ccl_abs (event.amountY - lastEvent.amountY) > 0.001
		|| lastViewName != viewName)
	{
		CCL::MutableCString s;
		s.appendFormat ("%sonGesture \"%s\" %f, %f", viewName, CCL::Gesture::getGestureName (event.getType () | event.getState ()), event.whereF.x, event.whereF.y);
		if(event.getType() == CCL::GestureEvent::kZoom || event.getType () == CCL::GestureEvent::kRotate)
			s.appendFormat (" amount: %f, %f", event.amountX, event.amountY);

		CCL_PRINTLN (s.str ())
		lastEvent = event;
		lastViewName = viewName;
	}
}
#define LOG_GESTURE(event) logGesture (event, WINDOW_STR);
#else
#define LOG_GESTURE(event)
#endif

using namespace CCL;

//************************************************************************************************
// DoubleTapHandler
//************************************************************************************************

class DoubleTapHandler: public GestureHandler
{
public:
	DECLARE_CLASS_ABSTRACT (DoubleTapHandler, GestureHandler)
	
	DoubleTapHandler (View* view)
	: GestureHandler (view, GestureEvent::kDoubleTap)
	{}
};

DEFINE_CLASS_ABSTRACT_HIDDEN (DoubleTapHandler, GestureHandler)

//************************************************************************************************
// Gesture::Candidate
//************************************************************************************************

class Gesture::Candidate: public Object
{
public:
	PROPERTY_SHARED_AUTO (ITouchHandler, handler, Handler)
	PROPERTY_SHARED_AUTO (View, view, View)
	PROPERTY_VARIABLE (int, gestureType, GestureType) // can have constraints
	PROPERTY_VARIABLE (int, priority, Priority)

	Candidate (ITouchHandler* handler, View* view, int gestureType, int priority);

	bool matches (const GestureEvent& event);
};

//************************************************************************************************
// Gesture::Candidate
//************************************************************************************************

Gesture::Candidate::Candidate (ITouchHandler* handler, View* view, int gestureType, int priority)
: gestureType (gestureType), priority (priority)
{
	setHandler (handler);
	setView (view);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Gesture::Candidate::matches (const GestureEvent& event)
{
	int constraints = getGestureType () & GestureEvent::kConstraintsMask;
	switch(constraints)
	{
	case GestureEvent::kHorizontal:
		return ccl_abs (event.amountX) > ccl_abs (event.amountY);

	case GestureEvent::kVertical:
		return ccl_abs (event.amountY) > ccl_abs (event.amountX);
	}
	return true;
}

//************************************************************************************************
// Gesture
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (Gesture, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

Gesture::Gesture (int type)
: GestureInfo (type),
  lastEvent (0),
  done (false),
  shadow (false),
  exclusiveTouch (false)
{
	ASSERT ((type & GestureEvent::kConstraintsMask) == 0)
	candidates.objectCleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Gesture::addCandidate (ITouchHandler* handler, View* view, int gestureType, int priority)
{
	ForEach (candidates, Candidate, candidate)
		if(candidate->getGestureType () == gestureType)
		{
			// existing candidate with exact same type
			if(priority <= candidate->getPriority ())
				return false;

			// replace existing
			if(!alternativeHandler && candidate->getHandler () != handler)
				setAlternativeHandler (candidate->getHandler ()); // keep lower prio handler as alternative
			candidates.remove (candidate);
			candidate->release ();
			candidates.add (NEW Candidate (handler, view, gestureType, priority));
			return true;
		}
		else if((candidate->getGestureType () & GestureEvent::kConstraintsMask) == 0)
		{
			// new candidate is a special case of the non-constrained existing candidate
			if(priority <= candidate->getPriority ())
				return false;

			// split existing candidate into a remaining case and a case to be replaced
			int remainingContraints = (~gestureType) & GestureEvent::kConstraintsMask;
			
			Candidate* remaining = NEW Candidate (*candidate);
			remaining->setGestureType (candidate->getGestureType () | remainingContraints);
			candidates.add (remaining);

			candidates.remove (candidate);
			candidate->release ();
			candidates.add (NEW Candidate (handler, view, gestureType, priority));
			return true;
		}
		else if((gestureType & GestureEvent::kConstraintsMask) == 0)
		{
			// existing candidate is a special case of the non-constrained new candidate: split into constrained candidates
			bool r1 = addCandidate (handler, view, gestureType | GestureEvent::kHorizontal, priority);
			bool r2 = addCandidate (handler, view, gestureType | GestureEvent::kVertical, priority);
			return r1 || r2;
		}
	EndFor

	candidates.add (NEW Candidate (handler, view, gestureType, priority));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Gesture::checkCandidates (const GestureEvent& event)
{
	if(!getHandler ())
	{
		ForEach (candidates, Candidate, candidate)
			if(candidate->matches (event))
			{
				setHandler (candidate->getHandler ());
				break;
			}
		EndFor
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITouchHandler* Gesture::getSingleCandidate () const
{
	// check if there is only one handler
	ITouchHandler* handler = nullptr;

	ForEach (candidates, Candidate, candidate)
		if(handler == nullptr)
			handler = candidate->getHandler ();
		else if(candidate->getHandler () != handler)
			return nullptr;
	EndFor
	return handler;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Gesture::getCandidateHandlers (IUnknownList& handlers) const
{
	ForEach (candidates, Candidate, candidate)
		if(ITouchHandler* handler = candidate->getHandler ())
			if(!handlers.contains (handler))
				handlers.add (handler, true);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* Gesture::getViewForHandler (ITouchHandler* handler) const
{
	for(auto candidate : iterate_as<Candidate> (candidates))
		if(candidate->getHandler () == handler)
			return candidate->getView ();

	return nullptr;
}

//************************************************************************************************
// TouchInputState::DeferredGesture
//************************************************************************************************

struct TouchInputState::DeferredGesture: public Unknown
{
	GestureEvent event;
	SharedPtr<Gesture> gesture;
};

//************************************************************************************************
// TouchInputState::ZoomOffsetHelper
/** Helper for adjusting the offset of a zoom gesture when a touch is added / removed while zooming. */
//************************************************************************************************

class TouchInputState::ZoomOffsetHelper
{
public:
	ZoomOffsetHelper (const TouchInputState& touchInput, Gesture& gesture)
	: touchInput (touchInput), gesture (gesture)
	{
		if(gesture.getType () == GestureEvent::kZoom)
			setOldState (gesture);
	}

	void setOldState (Gesture& gesture)
	{
		oldCenter = touchInput.calculateTouchCenter (gesture);
	}

	void update (bool touchAdded)
	{
		if(gesture.getType () == GestureEvent::kZoom && gesture.getLastEvent ().getState () > 0 && gesture.getTouchIDs ().count () > (touchAdded ? 1 : 0))
		{
			PointF newCenter (touchInput.calculateTouchCenter (gesture));
			gesture.setOffset (gesture.getOffset () + oldCenter - newCenter);
			CCL_PRINTF ("zoom gesture: oldCenter (%f, %f) newCenter (%f, %f) -> new offset (%f, %f)\n", oldCenter.x, oldCenter.y, newCenter.x, newCenter.y, gesture.getOffset ().x, gesture.getOffset ().y)
		}
	}

private:
	const TouchInputState& touchInput;
	Gesture& gesture;
	PointF oldCenter;
};

//************************************************************************************************
// TouchInputState::SimpleTouchEvent
//************************************************************************************************

class TouchInputState::SimpleTouchEvent: public TouchEvent
{
public:
	SimpleTouchEvent (const TouchInfo& touch, const TouchEventData& data)
	: TouchEvent (touchCollection, data.eventType, data.keys, data.inputDevice)
	{ 
		touchCollection.add (touch);

		touchID = touch.id;
		eventTime = touch.time * 0.001;
		penInfo = data.penInfo;
	}

private:
	TouchCollection touchCollection;
};

//************************************************************************************************
// TouchInputState::TouchItem
//************************************************************************************************

class TouchInputState::TouchItem: public Object
{
public:
	PROPERTY_VARIABLE (TouchID, id, ID)
	PROPERTY_OBJECT (PointF, position, Position)
	PROPERTY_SHARED_AUTO (View, view, View)
	PROPERTY_SHARED_AUTO (ITouchHandler, handler, Handler)
	PROPERTY_VARIABLE (double, lastEventTime, LastEventTime)
	PROPERTY_VARIABLE (KeyState, lastKeys, LastKeys)
	PROPERTY_FLAG (flags, 1<<0, hasMouseCandidate)
	PROPERTY_FLAG (flags, 1<<1, hasContact)
	PROPERTY_FLAG (flags, 1<<2, suppressesContextMenu)
	PROPERTY_FLAG (flags, 1<<3, isDiscarded)

	TouchItem (TouchID id, const TouchEvent& event);

	const TouchEvent& getFirstEvent () const;
	void setFirstEvent (const TouchEvent& event);
	bool isOnView (const View* view) const;

	bool getDistanceFromFirstEvent (const TouchEvent& event, Coord& distance) const;

	class Candidate;

	const ObjectList& getCandidates () const;
	Candidate* getCandidate (ITouchHandler* handler) const;
	void addCandidate (View* view, ITouchHandler* handler);
	void removeCandidate (ITouchHandler* handler);
	bool acceptCandidate (ITouchHandler* handler);
	void reset ();

private:
	TouchCollection firstTouches;
	TouchEvent firstEvent;
	ObjectList candidates;
	int flags;
};

//************************************************************************************************
// TouchInputState::TouchItem::Candidate
//************************************************************************************************

class TouchInputState::TouchItem::Candidate: public Object
{
public:
	PROPERTY_SHARED_AUTO (View, view, View)
	PROPERTY_SHARED_AUTO (ITouchHandler, handler, Handler)
};

//************************************************************************************************
// TouchInputState::TouchItem
//************************************************************************************************

TouchInputState::TouchItem::TouchItem (TouchID id, const TouchEvent& event)
: id (id),
  flags (0),
  lastEventTime (0),
  firstEvent (firstTouches)
{
	setFirstEvent (event);
	
	candidates.objectCleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchInputState::TouchItem::reset ()
{
	candidates.removeAll ();
	hasMouseCandidate (false);

	setView (nullptr);
	setHandler (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TouchInputState::TouchItem::isOnView (const View* view) const
{
	if(view)
	{
		Point where (pointFToInt (position));
		view->windowToClient (where);
		return view->getSize ().pointInside (where);
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchInputState::TouchItem::setFirstEvent (const TouchEvent& event)
{
	firstTouches.copyFrom (event.touches);

	firstEvent.eventClass = event.eventClass;
	firstEvent.eventType = event.eventType;
	firstEvent.eventTime = event.eventTime;
	firstEvent.touchID = event.touchID;
	firstEvent.keys = event.keys;
	firstEvent.inputDevice = event.inputDevice;
	firstEvent.penInfo = event.penInfo;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const TouchEvent& TouchInputState::TouchItem::getFirstEvent () const
{
	return firstEvent;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TouchInputState::TouchItem::getDistanceFromFirstEvent (const TouchEvent& event, Coord& distance) const
{
	const TouchInfo* touch1 = firstEvent.touches.getTouchInfoByID (firstEvent.touchID);
	const TouchInfo* touch2 = event.touches.getTouchInfoByID (event.touchID);
	if(touch1 && touch2)
	{
		Point diff (touch1->where - touch2->where);
		distance = ccl_max (ccl_abs (diff.x), ccl_abs (diff.y));
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline const ObjectList& TouchInputState::TouchItem::getCandidates () const
{
	return candidates;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TouchInputState::TouchItem::Candidate* TouchInputState::TouchItem::getCandidate (ITouchHandler* handler) const
{
	ListForEachObject (candidates, Candidate, c)
		if(c->getHandler () == handler)
			return c;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchInputState::TouchItem::addCandidate (View* view, ITouchHandler* handler)
{
	Candidate* candidate = NEW Candidate;
	candidate->setView (view);
	candidate->setHandler (handler);
	candidates.add (candidate);

	CCL_PRINTF ("touch handler candidate: %s\n", view->myClass ().getPersistentName ())
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchInputState::TouchItem::removeCandidate (ITouchHandler* handler)
{
	if(Candidate* candidate = getCandidate (handler))
	{
		candidates.remove (candidate);
		candidate->release ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TouchInputState::TouchItem::acceptCandidate (ITouchHandler* handler)
{
	ASSERT (getHandler () == nullptr)

	if(Candidate* candidate = getCandidate (handler))
	{
		setHandler (handler);
		setView (candidate->getView ());
		candidates.removeAll ();
		return true;
	}
	return false;
}

//************************************************************************************************
// TouchInputState
//************************************************************************************************

bool TouchInputState::inGestureEvent = false;
const double TouchInputState::kContextMenuDelay = 0.5; ///< less than the long press delay, so it can be triggered without starting to drag
const Coord TouchInputState::kContextMenuMaxDistance = 5; ///< touch must stay inside this tolerance for a longPress context menu

Configuration::IntValue TouchInputState::longPressDelay ("GUI.Touch", "LongPressDelay", 500);
Configuration::BoolValue TouchInputState::longPressContextMenu ("GUI.Touch", "LongPressContextMenu", true);
Configuration::BoolValue TouchInputState::penBarrelButtonGesture ("GUI.Touch", "PenBarrelButtonGesture", true); ///< trigger kPenPrimary gesture on barrel button press

//////////////////////////////////////////////////////////////////////////////////////////////////

TouchInputState::TouchInputState (View& rootView)
: rootView (rootView),
  delegatingGesture (nullptr),
  contextMenuPending (false)
{
	touchItems.objectCleanup ();
	gestures.objectCleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TouchInputState::~TouchInputState ()
{
	cancelSignals ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchInputState::setGestureManager (IGestureManager* manager)
{
	gestureManager = manager;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TouchInputState::TouchItem* TouchInputState::getTouchItem (TouchID id) const
{
	ForEach (touchItems, TouchItem, item)
		if(item->getID () == id)
			return item;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TouchID TouchInputState::getFirstTouchID () const
{
	TouchItem* item = (TouchItem*)touchItems.getFirst ();
	return item ? item->getID () : 	TouchEvent::kNoTouchId;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TouchInputState::hasTouch (TouchID id) const
{
	return getTouchItem (id) != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TouchInputState::getTouchPosition (Point& p, TouchID id) const
{
	PointF pos;
	if(!getTouchPosition (pos, id))
		return false;

	p = pointFToInt (pos);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TouchInputState::getTouchPosition (PointF& p, TouchID id) const
{
	TouchItem* item = getTouchItem (id);
	if(!item)
		return false;

	p = item->getPosition ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TouchInputState::hasTouchAtPosition (PointRef p) const
{
	return hasTouchAtPosition (pointIntToF (p));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TouchInputState::hasTouchAtPosition (PointFRef p) const
{
	RectF area (p, p);
	area.expand (10.f);
	
	ForEach (touchItems, TouchItem, item)
		if(area.pointInside (item->getPosition ()))
			return true;
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TouchInputState::hasTouchHandlerInViewArea (const View& view) const
{
	Point pos;
	Rect area;
	view.getClientRect (area).moveTo (view.clientToWindow (pos));
	
	ForEach (touchItems, TouchItem, item)
		if(item->getHandler ())
			if(View* v = item->getView ())
			{
				Rect rect;
				Point pos;
				v->getClientRect (rect).moveTo (v->clientToWindow (pos));
				if(rect.intersect (area))
					return true;
			}
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchInputState::discardTouchesForView (const View& view, bool keepItems)
{
	// an active drag session that was started from a contained view must be canceled
	if(auto* dragSession = DragSession::getActiveSession ())
	{
		auto* sourceView = unknown_cast<View> (dragSession->getSource ());
		if(sourceView && view.isChild (sourceView, true))
		{
			GestureEvent event (GestureEvent::kFailed, dragSession->getDragImagePosition ()); // (last position)
			tryDragGesture (event);
		}
	}

	// Discard all TouchItems that somehow refer to the view or a deep child.
	// This is important because a TouchItem, TouchHandler or Candidate might hold a reference count on the view,
	// which in turn refers to underrlying application components, which might be about to be removed soon.
	ForEach (touchItems, TouchItem, item)
		bool mustDiscard = item->isOnView (&view); // first check based on position
		if(!mustDiscard)
		{
			if(View* v = item->getView ())
				mustDiscard = v == &view || view.isChild (v, true);

			if(!mustDiscard)
			{
				// check views of candidates (in case no handler / view was chosen yet for this item)
				for(auto* candidate : iterate_as<TouchItem::Candidate> (item->getCandidates ()))
					if(candidate->getView () && (candidate->getView () == &view || view.isChild (candidate->getView (), true)))
					{
						mustDiscard = true;
						break;
					}
			}
		}

		if(mustDiscard)
			discardTouchItem (item, keepItems);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchInputState::setTouchDiscarded (TouchItem& item)
{
	item.isDiscarded (true);

	for(auto gesture : iterate_as<Gesture> (gestures))
		if(gesture->getTouchIDs ().contains (item.getID ()))
			gesture->setDone (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchInputState::discardTouchItem (TouchItem* item, bool keep)
{
	setTouchDiscarded (*item);

	if(keep)
		return;

	TouchInfo info (TouchEvent::kLeave, item->getID (), item->getPosition (), System::GetSystemTicks ());
	TouchEventData data (TouchEvent::kLeave, KeyState (), item->getFirstEvent ().inputDevice);
	processTouchLeave (info, data);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchInputState::discardTouches (bool deferred, bool keepItems)
{
	for(auto item : iterate_as<TouchItem> (touchItems))
		setTouchDiscarded (*item);

	if(keepItems)
		return;

	if(deferred)
		(NEW Message ("discardTouches"))->post (this);
	else
	{
		ForEach (touchItems, TouchItem, item)
			discardTouchItem (item);
		EndFor
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchInputState::discardTouchesForEvent (const TouchEvent& event)
{
	for(int i = 0, num = event.touches.getTouchCount (); i < num; i++)
		if(TouchItem* item = getTouchItem (event.touches.getTouchInfo (i).id))
			discardTouchItem (item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchInputState::discardHoverTouches ()
{
	ForEach (touchItems, TouchItem, item)
		if(!item->hasContact ())
			discardTouchItem (item);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TouchInputState::notify (ISubject*, MessageRef msg)
{
	if(msg == "discardTouches")
		discardTouches (false);
	else if(msg == "deferGesture")
	{
		DeferredGesture* deferredGesture = static_cast<DeferredGesture*> (msg[0].asUnknown ());
		onGesture (deferredGesture->event, *deferredGesture->gesture);
	}
	else if(msg == "popupContextMenu")
	{
		if(getOtherPopup ()) // discard if a popup / dialog has been opened meanwhile
		{
			contextMenuPending = false;
			return;
		}

		if(DragSession::getActiveSession ())
			(NEW Message (msg))->post (this, 100); // defer until drag session is really over
		else if(Window* window = rootView.getWindow ())
		{
			Point where (msg[0], msg[1]);
			window->popupContextMenu (where);
			contextMenuPending = false;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TouchInputState::TouchItem* TouchInputState::addTouchItem (const TouchEvent& event, const TouchInfo& touch)
{
	#if DEBUG_LOG
	if(!touchItems.isEmpty ())
	{
		MutableCString s ("existing items: ");
		for(auto i : iterate_as<TouchItem> (touchItems))
			s.appendFormat ("%s (%s) ", TOUCHID_STR (i->getID ()), i->hasContact () ? "contact" : "hover");
		CCL_PRINTLN (s)
	}
	#endif

	TouchItem* item = NEW TouchItem (event.touchID, event);
	item->setPosition (touch.whereF);
	ASSERT (event.eventTime > 0.)
	touchItems.add (item);
	return item;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchInputState::removeTouchItem (TouchItem* item)
{
	if(touchItems.remove (item))
	{
		CCL_PRINTF ("%sremoveTouchItem: %s (%s)\n", WINDOW_STR, TOUCHID_STR (item->getID ()), MutableCString ().appendFormat (touchItems.isEmpty () ? "last" : "%d remaining", touchItems.count ()).str ())
		item->release ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Gesture* TouchInputState::getGesture (int gestureType, TouchItem& touchItem, bool add)
{
	int plainType = gestureType & GestureEvent::kTypeMask;

	ForEach (gestures, Gesture, gesture)
		if(gesture->getType () == plainType)
			if(gesture->getTouchIDs ().contains (touchItem.getID ()))
				return gesture;
	EndFor

	if(add)
	{
		//CCL_PRINTF ("add gesture: \"%s\"\n", Gesture::getGestureName (plainType))
		Gesture* gesture = NEW Gesture (plainType);
		gestures.add (gesture);
		return gesture;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Gesture* TouchInputState::getGesture (int gestureType, TouchID touchID)
{
	int plainType = gestureType & GestureEvent::kTypeMask;

	ForEach (gestures, Gesture, gesture)
		if(gesture->getType () == plainType)
			if(gesture->getTouchIDs ().contains (touchID))
				return gesture;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchInputState::removeGesture (Gesture* gesture, bool isAborted)
{
	CCL_PRINTF ("remove gesture: \"%s\"\n", Gesture::getGestureName (gesture->getType ()))
	if(gesture->isContinuous () && gesture->getHandler ())
	{
		// send a final kEnd event for continuous gestures, if it's still missing
		int lastState = gesture->getLastEvent ().getState ();
		if(lastState == GestureEvent::kBegin || lastState == GestureEvent::kChanged)
		{
			GestureEvent e (gesture->getLastEvent ());
			e.eventType = (e.eventType & ~GestureEvent::kStatesMask) | GestureEvent::kEnd;
			if(gesture->isShadow () || isAborted)
				e.amountX = e.amountY = 1;
			gesture->getHandler ()->onGesture (e);
		}
	}
	gestures.remove (gesture);
	gesture->setDone ();

	if(gestureManager)
		gestureManager->stopRecognizing (gesture);

	gesture->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchInputState::updateGestureRecognizers ()
{
	// try to create gesture recognizers for new gestures to be detected
	if(gestureManager)
	{
		ForEach (gestures, Gesture, gesture)
			if(!gestureManager->isRecognizing (gesture))
			   gestureManager->startRecognizing (gesture);
		EndFor
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TouchInputState::checkPenButtons (const TouchItem& item, const TouchEvent& touchEvent, bool isNewItem)
{
	if(penBarrelButtonGesture)
	{
		// translate a barrel button press (is pressed and was not pressed before) to a kPenPrimary gesture
		if(touchEvent.inputDevice == TouchEvent::kPenInput
			&& touchEvent.keys.isSet (KeyState::kPenBarrel)
			&& (!item.getLastKeys ().isSet (KeyState::kPenBarrel) || isNewItem))
		{
			GestureEvent event (GestureEvent::kPenPrimary | GestureEvent::kBegin);
			return rootView.onGesture (event);
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchInputState::processTouches (const TouchEvent& event)
{
	if(event.eventType == TouchEvent::kCancel)
	{
		discardTouchesForEvent (event);
		return;
	}

	if(event.eventType == TouchEvent::kEnd)
	{
		ForEach (touchItems, TouchItem, item)
			if(touchItems.contains (item))
			{
				TouchEvent touchEvent (event);
				touchEvent.touchID = item->getID ();
				onTouchEnded (*item, touchEvent);

				removeTouchItem (item);
			}
		EndFor
		return;
	}
	
	if(!getOtherPopup ()) // while in modal loop, do not remove touch events from other windows
	{
		// check for ended touches
		ForEach (touchItems, TouchItem, item)
			if(touchItems.contains (item))
			{
				const TouchInfo* touchInfo = event.touches.getTouchInfoByID (item->getID ());
				if(!touchInfo || isEndingEvent (touchInfo->type))
				{
					TouchEvent touchEvent (event);
					if(!touchInfo)
						touchEvent.eventType = TouchEvent::kEnd;
					touchEvent.touchID = item->getID ();
					onTouchEnded (*item, touchEvent);

					removeTouchItem (item);
				}
			}
		EndFor
	}
	
	// process existing and new touches
	for(int i = 0, num = event.touches.getTouchCount (); i < num; i++)
	{
		// ignore additional touches while a drag session is active
		if(i == 1 && DragSession::getActiveSession ())
			break;

		const TouchInfo& touch = event.touches.getTouchInfo (i);

		TouchEvent touchEvent (event);
		touchEvent.touchID = touch.id;
		touchEvent.eventType = touch.type;
		if(TouchItem* item = getTouchItem (touch.id))
		{
			if(item->isDiscarded ())
				continue;

			checkPenButtons (*item, touchEvent);

			if(touchEvent.eventType == TouchEvent::kBegin && !item->hasContact ())
				onTouchBegan (*item, touchEvent, touch);
			else
				onTouchChanged (*item, touchEvent, touch);
		}
		else if(!isEndingEvent (touchEvent.eventType)) // don't add new touch item if the touch is already ending
		{
			// must start with begin or enter for a new touch
			if(isHoverEvent (touchEvent.eventType))
				touchEvent.eventType = TouchEvent::kEnter;
			else
				touchEvent.eventType = TouchEvent::kBegin;

			if(TouchItem* item = addTouchItem (touchEvent, touch))
			{
				checkPenButtons (*item, touchEvent, true);

				if(isHoverEvent (touchEvent.eventType))
					onTouchEnter (*item, touchEvent, touch);
				else
					onTouchBegan (*item, touchEvent, touch);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchInputState::processTouch (const TouchInfo& touch, const TouchEventData& data)
{
	SimpleTouchEvent touchEvent (touch, data);

	TouchItem* item = getTouchItem (touch.id);
	if(item)
	{
		if(item->isDiscarded ())
			return;

		checkPenButtons (*item, touchEvent);

		if(touchEvent.eventType == TouchEvent::kBegin)
			onTouchBegan (*item, touchEvent, touch);
		else
			onTouchChanged (*item, touchEvent, touch);
	}
	else if(!isEndingEvent (touchEvent.eventType)) // don't add new touch item if the touch is already ending
	{
		// must start with begin or enter for a new touch
		if(isHoverEvent (touchEvent.eventType))
			touchEvent.eventType = TouchEvent::kEnter;
		else
			touchEvent.eventType = TouchEvent::kBegin;

		item = addTouchItem (touchEvent, touch);
		if(item)
		{
			checkPenButtons (*item, touchEvent, true);

			if(isHoverEvent (touchEvent.eventType))
				onTouchEnter (*item, touchEvent, touch);
			else
				onTouchBegan (*item, touchEvent, touch);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchInputState::processTouchEnd (const TouchInfo& touch, const TouchEventData& data)
{
	ASSERT (data.eventType == TouchEvent::kEnd)

	TouchItem* item = getTouchItem (touch.id);
	if(item)
	{
		item->setPosition (touch.whereF);

		SimpleTouchEvent touchEvent (touch, data);
		onTouchEnded (*item, touchEvent);

		removeTouchItem (item);
	}
	else
	{
		CCL_PRINTF ("%sprocessTouchEnd: item %s does not exist\n", WINDOW_STR, TOUCHID_STR (touch.id));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchInputState::processTouchLeave (const TouchInfo& touch, const TouchEventData& data)
{
	ASSERT (data.eventType == TouchEvent::kLeave)

	TouchItem* item = getTouchItem (touch.id);
	if(item)
	{
		item->setPosition (touch.whereF);

		// if leave happens before end (e.g. on Windows for a touch that opens a popup), we must perform 'end' first
		if(item->hasContact ())
		{
			ScopedVar<int> scope1 (ccl_const_cast (touch).type, TouchEvent::kEnd);
			ScopedVar<int> scope2 (ccl_const_cast (data).eventType, TouchEvent::kEnd);
			processTouchEnd (touch, data);
		}
		else
		{
			SimpleTouchEvent touchEvent (touch, data);
			triggerHoverCandidates (*item, touchEvent);

			removeTouchItem (item);
		}
	}
	else
	{
		CCL_PRINTF ("%sprocessTouchLeave: item %s does not exist\n", WINDOW_STR, TOUCHID_STR (touch.id));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TouchInputState::collectTouchHandlers (View& view, PointRef where, TouchItem& touchItem, const TouchEvent& event)
{
	// try child views first
	if(!view.suppressesChildTouch ())
		ForEachViewFastReverse (view, v)
			if(v->isEnabled () && v->getSize ().pointInside (where))
			{			
				Point where2 (where);
				where2.offset (-v->getSize ().left, -v->getSize ().top);

				if(collectTouchHandlers (*v, where2, touchItem, event))
					return true;
			}
	EndFor

	AutoPtr<ITouchHandler> touchHandler = view.createTouchHandler (event);
	if(!touchHandler && !touchItem.hasMouseCandidate ())
	{
		// try to create a mouse handler and wrap it
		MouseEvent mouseEvent (TouchMouseHandler::makeMouseEvent (MouseEvent::kMouseDown, event, view));
		AutoPtr<MouseHandler> mouseHandler = view.createMouseHandler (mouseEvent);
		if(mouseHandler)
			touchHandler = NEW TouchMouseHandler (mouseHandler, mouseHandler->getView ());
		else if(&view == &rootView)
			touchHandler = NEW ViewTouchHandler (&rootView); // sends mouseDown/up to the view

		if(mouseEvent.doubleClicked == 1)
		{
			// workaround: some view already detected a doubleclick (and likely handled it):
			touchItem.reset ();
			return true;
		}

		if(touchHandler)
			touchItem.hasMouseCandidate (true);
	}

	if(touchHandler)
		touchItem.addCandidate (&view, touchHandler);

	// add candidate for double tap to reset a control
	if(Control* control = ccl_cast<Control> (&view))
		if(control->canHandleDoubleTap () && !ccl_strict_cast<EditBox> (control)) // makes no sense for editbox
			touchItem.addCandidate (&view, AutoPtr<ITouchHandler> (NEW DoubleTapHandler (&view)));

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TouchInputState::collectTouchHandlers (TouchItem& touchItem, const TouchEvent& event, const TouchInfo& touch)
{
	Point where (touch.where);
	rootView.windowToClient (where);

	// collect touch handler candidates
	return collectTouchHandlers (rootView, where, touchItem, event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TouchInputState::acceptTouchHandler (TouchItem& touchItem, ITouchHandler* handler)
{
	View* view = touchItem.getView ();
	if(view && !view->isAttached ())
		return false;

	if(!touchItem.acceptCandidate (handler))
	{
		touchItem.setHandler (handler);
		touchItem.setView (nullptr);
	}

	view = touchItem.getView ();
	if(view && view->isAttached ())
	{
		if(!unknown_cast<DoubleTapHandler> (handler) // don't focus if it's a double tap (suppress native text control)
			&& !ccl_cast<ComboBox> (view)) // don't focus a ComboBox (might have hit the menu button - suppress native text control)
			view->takeFocus ();
		
		CCL_PRINTF ("accept touch handler: %s %s\n", TOUCHID_STR (touchItem.getID ()), touchItem.getView () ? touchItem.getView ()->myClass ().getPersistentName () : "")
		handler->begin (touchItem.getFirstEvent ());
		return true;
	}
	else
		return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchInputState::giveUpTouchHandler (TouchItem& touchItem, ITouchHandler* handler)
{
	touchItem.removeCandidate (handler);
	CCL_PRINTF ("touch handler failed: %s\n", touchItem.getView ()->myClass ().getPersistentName ())
	
	// todo: accept other candidate if no other gestures pending
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchInputState::updateHoverCandidates (TouchItem& item, const TouchEvent& event, const TouchInfo& touch)
{
	// trigger left candidates one last time so they can react
	ForEachReverse (item.getCandidates (), TouchItem::Candidate, candidate)
		View* view = candidate->getView ();
		Point where (touch.where);
		if(!view->isInsideClient (view->windowToClient (where)))
			candidate->getHandler ()->trigger (event);
	EndFor

	item.reset ();

	if(!touchItems.isMultiple ()) // collect touch handlers only for the first touch
		collectTouchHandlers (item, event, touch);

	triggerHoverCandidates (item, event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchInputState::triggerHoverCandidates (TouchItem& item, const TouchEvent& event)
{
	// feed hover event to all touch handler candidates
	ForEach (item.getCandidates (), TouchItem::Candidate, candidate)
		ITouchHandler* handler = candidate->getHandler ();
		handler->trigger (event);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchInputState::onTouchEnter (TouchItem& item, const TouchEvent& event, const TouchInfo& touch)
{
	GUI.trackUserInput (event);

	updateHoverCandidates (item, event, touch);

	item.setLastKeys (event.keys);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchInputState::onTouchBegan (TouchItem& item, const TouchEvent& event, const TouchInfo& touch)
{
	if(Window* popupWindow = getOtherPopup ())
	{
		popupWindow->onActivate (false);
		
		// abort if a modal dialog is active
		if(ccl_cast<Dialog> (popupWindow))
			return;

		// check whether a non-modal popup swallows the event
		if(Window* window = ccl_cast<Window> (&rootView))
			if(NonModalPopupSelectorWindow::processForeignEvent (event, window))
			{
				item.isDiscarded (true); // prevent registering gestures for the "swallowed" touch
				return;
			}
	}

	CCL_PRINTF ("%sonTouchBegan %s (%d)\n", WINDOW_STR, TOUCHID_STR (item.getID ()), event.eventType)

	item.hasContact (true);
	item.setFirstEvent (event);

	// now that we have a touch in contact, remove pending hover touches to avoid gesture complications
	discardHoverTouches ();
	ASSERT (touchItems.contains (&item))

	if(!touchItems.isMultiple ())
	{
		Point mousePos (pointFToInt (item.getPosition ()));
		GUI.setLastMousePos (rootView.clientToScreen (mousePos));
	}

	// discard touches originating at the edge of the screen (operating system swipe gestures)
	Rect screen;
	Desktop.getVirtualScreenSize (screen, false);
	Point screenPos = touch.where;
	rootView.clientToScreen (rootView.windowToClient (screenPos));
	static const Rect kEdgeArea = { 5, 0, 5, 5 };

	if(screenPos.x - screen.left < kEdgeArea.left || (screen.right - screenPos.x - 1) < kEdgeArea.right || 
		screenPos.y - screen.top < kEdgeArea.top || (screen.bottom - screenPos.y - 1) < kEdgeArea.bottom)
	{
		item.isDiscarded (true);
		return;
	}

	GUI.trackUserInput (event);

	// add touch to gestures that require more touches
	ForEach (gestures, Gesture, gesture)
		ZoomOffsetHelper zoomHelper (*this, *gesture);

		if(gesture->needsMoreTouches () && gesture->addTouch (item.getID ()))
		{
			zoomHelper.update (true);

			if(gestureManager)
				gestureManager->updateTouchesForGesture (gesture);
		}
	EndFor

	// try to add the touch to an existing touch handler, if it's on the same view
	ForEach (touchItems, TouchItem, existingItem)
		if(existingItem == &item)
			continue;

		View* view = existingItem->getView ();
		ITouchHandler* touchHandler = existingItem->getHandler ();
		if(touchHandler && view)
		{
			Point where2 (touch.where);
			view->windowToClient (where2);
			if(view->getSize ().pointInside (where2))
			{
				if(touchHandler->addTouch (event))
				{
					item.setHandler (touchHandler);
					item.setView (view);

					item.suppressesContextMenu (true); // multiple touches for the same handler: no context menu
					existingItem->suppressesContextMenu (true);

					if(gestureManager)
						gestureManager->onTouchBegan (touch);
					return;
				}
			}
		}
	EndFor

	if(Gesture* shadowGesture = gestures.findIf<Gesture> ([&] (const Gesture& g) { return g.isShadow (); }))
	{
		if(gestureManager)
			gestureManager->onTouchBegan (touch);
		return;
	}

	// try to create a new touch handler, if not already done in onTouchEnter
	if(!item.getHandler () && item.getCandidates ().isEmpty ())
		collectTouchHandlers (item, event, touch);

	// determine the gestures required by touch handler candidates
	bool gesturesRequired = false;
	ForEach (item.getCandidates (), TouchItem::Candidate, candidate)
		ITouchHandler* handler = candidate->getHandler ();
		int gestureType = 0;
		int priority = 0;
		int i = 0;
		while(handler->getRequiredGesture (gestureType, priority, i++))
		{
			Gesture* gesture = getGesture (gestureType, item, true);
			if(gestureType & GestureEvent::kExclusiveTouch)
			{
				gesture->setExclusiveTouch (true);
				gestureType &= ~GestureEvent::kExclusiveTouch;
			}
			if(gestureType & GestureEvent::kSuppressContextMenu)
			{
				// note: could add the flag to TouchItem::Candidate and only apply it when the handler is accepted,
				// but that wouldn't help for cases where application code provides no touchhandler for some gestures (-> ViewTouchHandler wraps MouseHandler)
				item.suppressesContextMenu (true);
				gestureType &= ~GestureEvent::kSuppressContextMenu;
			}

			bool added = gesture->addCandidate (handler, candidate->getView (), gestureType, priority);
			if(added)
			{
				CCL_PRINTF ("gesture: \"%s\" handled by %s (Prio: %d) [%s]\n", Gesture::getGestureName (gestureType), candidate->getView ()->myClass ().getPersistentName (), priority, TOUCHID_STR (item.getID ()))
				gesture->addTouch (item.getID ());
				gesturesRequired = true;
			}
		}
	EndFor

	if(!gesturesRequired)
	{
		// if no candidate requires a specific gesture, accept the first one
		if(!item.getCandidates ().isEmpty ())
			acceptTouchHandler (item, ((TouchItem::Candidate*)item.getCandidates ().getFirst ())->getHandler ());
	}
	else
	{
		// if there is only one handler for all gestures, accept it immediately
		ITouchHandler* commonHandler = nullptr;
		ForEach (gestures, Gesture, gesture)
			if(gesture->getTouchIDs ().count () == 1 && gesture->getTouchIDs ().at (0) == touch.id)
			{
				ITouchHandler* handler = gesture->getSingleCandidate ();
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
			acceptTouchHandler (item, commonHandler);
		else
		{
			// no handler accepted yet (waiting for gesture recognition), but we already send an early preliminary event to all handlers (would be cleaner with an "Unknown" gesture type)
			UnknownList handlers;
			for(auto gesture : iterate_as<Gesture> (gestures))
				if(gesture->getTouchIDs ().count () == 1 && gesture->getTouchIDs ().at (0) == touch.id)
					gesture->getCandidateHandlers (handlers);

			GestureEvent preliminaryEvent (GestureEvent::kSwipe|GestureEvent::kPossible, item.getPosition (), 1., event.keys);
			preliminaryEvent.eventTime = event.eventTime;

			ForEachUnknown (handlers, unk)
				UnknownPtr<ITouchHandler> handler (unk);
				if(handler)
					handler->onGesture (preliminaryEvent);
			EndFor
		}
	}

	updateGestureRecognizers ();

	if(gestureManager)
		gestureManager->onTouchBegan (touch);

	item.setLastKeys (event.keys);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchInputState::onTouchChanged (TouchItem& item, const TouchEvent& event, const TouchInfo& touch)
{
	GUI.trackUserInput (event);

	item.setPosition (touch.whereF);

	if(!touchItems.isMultiple ())
	{
		Point mousePos (pointFToInt (item.getPosition ()));
		GUI.setLastMousePos (rootView.clientToScreen (mousePos));
	}

	if(ITouchHandler* handler = item.getHandler ())
	{
		// limit events per second
		double now = System::GetProfileTime ();
		if(event.eventType == TouchEvent::kMove || event.eventType == TouchEvent::kHover)
			if(now - item.getLastEventTime () < 0.02f)
				if(item.getLastKeys () == event.keys)
					return;

		item.setLastEventTime (now);
		item.setLastKeys (event.keys);

		tbool result = handler->trigger (event);
		if(!result)
		{
			// todo: cancel handler
		}
	}
	else if(event.eventType == TouchEvent::kHover)
		updateHoverCandidates (item, event, touch);

	if(longPressContextMenu && !item.suppressesContextMenu () && !item.isDiscarded ())
	{
		// suppress context menu if touch has moved outside the tolerance area
		Coord distance;
		if(item.getDistanceFromFirstEvent (event, distance) && distance >= kContextMenuMaxDistance)
			item.suppressesContextMenu (true);
	}

	if(gestureManager && !isHoverEvent (event.eventType))
		gestureManager->onTouchChanged (touch);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchInputState::onTouchEnded (TouchItem& touchItem, const TouchEvent& event)
{
	CCL_PRINTF ("%sonTouchEnded %s\n", WINDOW_STR, TOUCHID_STR (touchItem.getID ()))
	GUI.trackUserInput (event);

	SharedPtr<TouchItem> guard (&touchItem);
	if(gestureManager)
	{
		TouchInfo touchInfo (Core::kTouchEnd, touchItem.getID (), touchItem.getPosition ());
		if(const TouchInfo* t = event.touches.getTouchInfoByID (touchItem.getID ()))
			touchInfo.time = t->time;

		gestureManager->onTouchEnded (touchInfo);
	}

	touchItem.hasContact (false);

	if(ITouchHandler* handler = touchItem.getHandler ())
	{
		View* view = touchItem.getView ();
		if(view && !view->isAttached ())
			setTouchDiscarded (touchItem);
		else
			handler->finish (event, touchItem.isDiscarded ());
	}
	else if(event.eventType == TouchEvent::kLeave)
		triggerHoverCandidates (touchItem, event);

	bool isLast = touchItems.count () == 1;
	
	// check if a double tap gesture is pending for this touch
	Gesture* doubleTap = getGesture (GestureEvent::kDoubleTap, touchItem, false);
	if(doubleTap)
	{
		if(doubleTap->canRemove ())
			doubleTap = nullptr; // already done
		else
			isLast = false;
	}

	// remove touch from gestures, remove obsolete gestures
	// note: some gestures (e.g. double tap) must survive touches
	ForEach (gestures, Gesture, gesture)
		ZoomOffsetHelper zoomHelper (*this, *gesture);

		if(gesture->getTouchIDs ().remove (touchItem.getID ()))
		{
			zoomHelper.update (false);

			bool isWaitingForDoubleTap = doubleTap && gesture->getType () == GestureEvent::kSingleTap;

			// the first tap is over: a second one (double tap) might happen later
			// send other handlers a "double tap possible" event to allow some preliminary action
			// (similar to actions that are performed in mouseDown, before detectDoubleClick)
			if(isWaitingForDoubleTap)
			{
				UnknownList handlers;
				gesture->getCandidateHandlers (handlers);

				GestureEvent preliminaryEvent (GestureEvent::kDoubleTap|GestureEvent::kPossible, touchItem.getPosition (), 1., event.keys);
				preliminaryEvent.eventTime = event.eventTime;

				NonModalPopupSelectorWindow::processForeignEvent (event, ccl_cast<Window> (&rootView));

				ForEachUnknown (handlers, unk)
					UnknownPtr<ITouchHandler> handler (unk);
					if(handler)
						handler->onGesture (preliminaryEvent);
				EndFor
			}

			if(isWaitingForDoubleTap)
			{
				CCL_PRINTLN ("keep Single Tap...")
				gesture->setTouchItem (&touchItem);
			}

			// a single tap gesture must survive it's touch if a double tap is still pending (might be triggered via double tap timeout)
			if(gesture->canRemove () && !isWaitingForDoubleTap)
				removeGesture (gesture);
			else
				isLast = false;
		}
	EndFor

	if(isLast)
		ForEach (gestures, Gesture, gesture)
			removeGesture (gesture);
		EndFor

	ASSERT (event.eventTime > 0.)
	if(longPressContextMenu && !touchItem.suppressesContextMenu () && !touchItem.isDiscarded ())
	{
		// trigger context menu if it's a "long press" (gesture object not required) and has not moved outside a tolerance area
		double now = event.eventTime;
		if(now - touchItem.getFirstEvent ().eventTime >= kContextMenuDelay && !EditBox::isAnyEdtiting () && !contextMenuPending)
		{
			Coord distance;
			if(touchItem.getDistanceFromFirstEvent (event, distance))
				if(distance < kContextMenuMaxDistance)
				{
					contextMenuPending = true;
					(NEW Message ("popupContextMenu", touchItem.getPosition ().x, touchItem.getPosition ().y))->post (this);
				}
		}
	}

	touchItem.setLastKeys (event.keys);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PointF TouchInputState::calculateTouchCenter (Gesture& gesture) const
{
	PointF result;
	int numTouches = 0;
	for(auto id : gesture.getTouchIDs ())
	{
		TouchItem* item = getTouchItem (id);
		ASSERT (item)
		if(item)
		{
			result += item->getPosition ();
			numTouches++;
		}
	}

	if(numTouches > 0)
		result *= (1.f / numTouches);

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int TouchInputState::countRemainingShadowTouches (const Gesture& gesture) const
{
	int count = 0;
	for(auto touchId : gesture.getShadowTouches ())
	{
		TouchItem* item = getTouchItem (touchId);
		if(item && item->hasContact () && !item->isDiscarded ())
			count++;
	}
	return count;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TouchInputState::tryDragGesture (const GestureEvent& event)
{
	if(event.getType () == GestureEvent::kLongPress || event.getType () == GestureEvent::kSwipe)
	{
		Window* window = rootView.getWindow ();
		// on platforms with 2 separate session objects for source and target, we need the target session
		DragSession* dragSession = window ? DragSession::getActiveSession (true) : nullptr;
		if(dragSession)
		{
			dragSession->setDragImagePosition (event.where);
			KeyState keys (event.keys);
			keys.keys |= KeyState::kLButton;

			switch(event.getState ())
			{
			case GestureEvent::kBegin:
					dragSession->showNativeDragImage (!dragSession->hasVisualFeedback ());
				break;
				
			case GestureEvent::kChanged:
				{
					DragEvent dragEvent (*dragSession, DragEvent::kDragOver, event.where, keys);
					window->onDragOver (dragEvent);
					dragSession->showNativeDragImage (!dragSession->hasVisualFeedback ());
				}
				break;
				
			case GestureEvent::kEnd:
				if(dragSession->getResult () != IDragSession::kDropNone)
				{
					DragEvent dragEvent (*dragSession, DragEvent::kDrop, event.where, keys);
					window->onDrop (dragEvent);

					// must cleanup:
					dragEvent.eventType = DragEvent::kDragLeave;
					window->onDragLeave (dragEvent);
					break;
				} // otherwise fall through to onDragLeave
				
			case GestureEvent::kFailed:
				{
					DragEvent dragEvent (*dragSession, DragEvent::kDragLeave, event.where, keys);
					dragSession->setCanceled (true);
					window->onDragLeave (dragEvent);
				}
				break;
			}
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TouchInputState::tryDelegateGesture (const GestureEvent& event, Gesture& gesture)
{
	if(gesture.isExclusiveTouch () && gesture.getType () == GestureEvent::kSwipe)
	{
		// if swipe gesture begins with multiple touches, but is restricted to a single touch: find zoom gesture to delegate to it (until recognition detects zoom)
		if(event.getState () == GestureEvent::kBegin)
			if(Gesture* delegateGesture = getGesture (GestureEvent::kZoom, gesture.getTouchIDs ().first ()))
				if(delegateGesture->getTouchIDs ().count () >= 2)
				{
					gesture.setDelegateGesture (delegateGesture);
					delegateGesture->setDelegateGesture (nullptr);
				}

		Gesture* delegateGesture = gesture.getDelegateGesture ();
		if(delegateGesture && delegateGesture->getType () == GestureEvent::kZoom)
		{
			// ignore swipe, handle a zoom event instead
			GestureEvent e (event);
			e.eventType = (e.eventType & ~GestureEvent::kTypeMask) | GestureEvent::kZoom;
			e.amountX = e.amountY = 1; // neutral zoom factor when translated from swipe

			ScopedVar<Gesture*> scope (delegatingGesture, &gesture);
			onGesture (e, *delegateGesture); // recursion!
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchInputState::deferGesture (const GestureEvent& event, Gesture& gesture)
{
	AutoPtr<DeferredGesture> deferredGesture = NEW DeferredGesture;
	deferredGesture->event = event;
	deferredGesture->gesture = &gesture;
	(NEW Message ("deferGesture", Variant (deferredGesture, true)))->post (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TouchInputState::isSameEvent (const GestureEvent& event1, const GestureEvent& event2)
{
	return event1.eventType == event2.eventType
		&& event1.whereF == event2.whereF
		&& event1.keys == event2.keys
		&& event1.amountX == event2.amountX
		&& event1.amountY == event2.amountY;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchInputState::onGesture (const GestureEvent& _event, Gesture& gesture)
{
	GestureEvent event (_event);
	
	if(event.eventTime == 0)
		event.eventTime = System::GetProfileTime ();

	if(gesture.getType () == GestureEvent::kZoom)
		event.setPosition (event.whereF + gesture.getOffset ());

	// ignore successive kChanged events with the same data
	if(event.getState () == GestureEvent::kChanged && isSameEvent (event, gesture.getLastEvent ()))
		return;

	if(gesture.isDone ())
		return;

	// ignore events of shadow gestures, except begin (which ends the shadow state)
	if(gesture.isShadow () && event.getState () != GestureEvent::kBegin)
		return;

	LOG_GESTURE (event)
	GUI.trackUserInput (event);

	SharedPtr<Object> holder (&gesture); // gesture might get removed (e.g. acceptTouchHandler () => handler->begin () => show dialog / popup)
	ScopedVar<bool> guard (inGestureEvent, true);

	if(tryDragGesture (event))
		return;

	if(tryDelegateGesture (event, gesture))
		return;

	switch(event.getState ())
	{
	case GestureEvent::kBegin:
		if(gesture.isShadow () && gesture.getType () == GestureEvent::kSwipe)
		{
			ITouchHandler* handler = gesture.getHandler ();
			if(!handler)
				handler = gesture.getSingleCandidate ();

			// if shadow gesture has the same handler, use an alternative handler (with lower priority) instead
			if(handler && gestures.findIf<Gesture> ([&] (const Gesture& g)
			{
				return &g != &gesture
					&& g.getHandler () == handler
					&& g.getLastEvent ().getState () < GestureEvent::kEnd
					&& g.getTouchIDs ().containsAnyOf (gesture.getShadowTouches ());
			}))
			{
				if(gesture.getAlternativeHandler ())
				{
					gesture.setHandler (gesture.getAlternativeHandler ());
					gesture.setAlternativeHandler (nullptr);
				}
			}
		}
		gesture.checkCandidates (event);
		gesture.setShadow (false);

		// remove original gesture when the anticipated delegate gesture was finally detected
		if(!delegatingGesture)
			if(Gesture* originalGesture = gestures.findIf<Gesture> ([&] (const Gesture& g) { return g.getDelegateGesture () == &gesture; }))
				removeGesture (originalGesture, true);

		if(ITouchHandler* touchHandler = gesture.getHandler ())
		{
			// beginning of gesture: accept pending candidates of associated touches

			if(gesture.getTouchIDs ().isEmpty ())
			{
				// when none of the gesture's touches is present anymore, use the touch item referenced by gesture
				TouchItem* touchItem = static_cast<TouchItem*> (gesture.getTouchItem ());
				if(touchItem && touchItem->getHandler () == nullptr)
				{
					bool didBegin = acceptTouchHandler (*touchItem, touchHandler);

					// finish touchHandler (ITouchHandler::begin is called in acceptTouchHandler)
					if(didBegin && touchHandler)
					{
						TouchEvent touchEvent (touchItem->getFirstEvent ());
						touchEvent.eventType = TouchEvent::kEnd;
						touchHandler->finish (touchEvent);
					}

					gesture.setTouchItem (nullptr);
				}
			}

			ObjectList endGestures;

			VectorForEachFast (gesture.getTouchIDs (), TouchID, id)
				SharedPtr<TouchItem> touchItem = getTouchItem (id);
				if(touchItem && touchItem->getHandler () != touchHandler)
				{
					if(ITouchHandler* oldHandler = touchItem->getHandler ())
					{
						// finish old handler
						TouchInfo info (TouchEvent::kEnd, touchItem->getID (), touchItem->getPosition (), System::GetSystemTicks ());
						TouchEventData data (TouchEvent::kEnd, KeyState (), touchItem->getFirstEvent ().inputDevice);
						SimpleTouchEvent touchEvent (info, data);
						oldHandler->finish (touchEvent);

						// end gesture with old handler
						Gesture* otherGesture = gestures.findIf<Gesture> ([&] (const Gesture& g)
							{ return g.getHandler () == oldHandler && g.getType () != gesture.getType (); });
						if(otherGesture)
							endGestures.addOnce (otherGesture);

						event.setPosition (event.whereF + gesture.getOffset ());
						gesture.setOffset (PointF ());
					}

					// touch item might have removed candidates when handler for previous gesture was accepted: bring back handler of gesture
					if(touchItem->getCandidates ().isEmpty ())
						if(View* view = gesture.getViewForHandler (touchHandler))
							touchItem->addCandidate (view, touchHandler);

					touchItem->setHandler (nullptr);
					if(acceptTouchHandler (*touchItem, touchHandler))
					{
						#if 0 // suppresses in cases where not desired
						if(gesture.getType () == GestureEvent::kLongPress)
							touchItem->suppressesContextMenu (true);
						#endif
					}
					else
						continue;
					
					// try to add other pending touches on the same view to the accepted handler
					ForEach (touchItems, TouchItem, otherItem)
						if(otherItem != touchItem && !otherItem->getHandler ()
							&& otherItem->isOnView (touchItem->getView ())
							&& touchHandler->addTouch (otherItem->getFirstEvent ()))
							{
								otherItem->setHandler (touchHandler);
								otherItem->setView (touchItem->getView ());
								
								 // also add touch to gesture: will cancel other gestures below
								gesture.addTouch (otherItem->getID ());
							}
					EndFor
				}
				#if 0 // suppresses in cases where not desired
				else if(touchItem && gesture.getType () == GestureEvent::kLongPress) // handler might have already been accepted in onTouchBegan if there were no competing gestures
					touchItem->suppressesContextMenu (true);
				#endif
			EndFor

			for(auto otherGesture : iterate_as<Gesture> (endGestures))
			{
				GestureEvent endEvent (otherGesture->getLastEvent ());
				endEvent.eventType = (endEvent.eventType & ~GestureEvent::kStatesMask) | GestureEvent::kFailed;
				CCL_PRINT ("End other Gesture: ") LOG_GESTURE (endEvent)
				otherGesture->getHandler ()->onGesture (endEvent);
			}

			if(touchHandler)
			{
				auto markShadowGesture = [&] (int shadowType)
				{
					if(touchHandler->allowsCompetingGesture (shadowType))
						if(Gesture* otherGesture = getGesture (shadowType, gesture.getTouchIDs ().first ()))
						{
							otherGesture->setShadow (true); // keep detecting gesture (see below)
							otherGesture->setShadowTouches (gesture.getTouchIDs ());
						}
				};

				if(gesture.getType () == GestureEvent::kSwipe)
					markShadowGesture (GestureEvent::kZoom);
				else if(gesture.getType () == GestureEvent::kZoom)
					markShadowGesture (GestureEvent::kSwipe);
			}

			// remove other pending gestures with touches from this gesture
			ForEach (gestures, Gesture, otherGesture)
				if(otherGesture != &gesture && !otherGesture->isShadow () && otherGesture != delegatingGesture)
					VectorForEachFast (gesture.getTouchIDs (), TouchID, id)
						if(otherGesture->getTouchIDs ().contains (id))
						{
							removeGesture (otherGesture, true);
							break;
						}
					EndFor
			EndFor

			// suppress longpress context menu for touches of a multi-touch gesture
			if(gesture.hadMultipleTouches ())
				for(auto id : gesture.getTouchIDs ())
					if(TouchItem* touchItem = getTouchItem (id))
						touchItem->suppressesContextMenu (true);
		}
		break;

	case GestureEvent::kChanged:
		break;

	case GestureEvent::kEnd:
		break;

	case GestureEvent::kFailed:
		// gesture recognition failed
		VectorForEachFast (gesture.getTouchIDs (), TouchID, id)
			TouchItem* touchItem = getTouchItem (id);
			if(touchItem && touchItem->getHandler () == nullptr)
				giveUpTouchHandler (*touchItem, gesture.getHandler ());
		EndFor
		break;
	}

	auto getHandlerForGesture = [&](Gesture& gesture, View*& view) -> ITouchHandler*
	{
		// try handers assigned to touch items of gesture
		VectorForEachFast (gesture.getTouchIDs (), TouchID, id)
			if(TouchItem* touchItem = getTouchItem (id))
				if(ITouchHandler* touchHandler = touchItem->getHandler ())
				{
					if(touchItem->getView () && !touchItem->getView ()->isAttached ())
						continue;

					view = touchItem->getView ();
					return touchHandler; // should be the same handler for multiple touches
				}
		EndFor
		
		// tap gestures might be detected after the (first) tap is already gone (double tap timeout for single tap, different second touch id for double tap)
		if(gesture.isTap ())
			return gesture.getHandler ();

		return nullptr;
	};

	// feed gesture event to active touch handler
	View* view = nullptr;
	if(ITouchHandler* touchHandler = getHandlerForGesture (gesture, view))
	{
		if(view && !view->isAttached ())
			discardTouchesForView (*view); // discard touch (and handler) if view was removed
		else
		{
			touchHandler->onGesture (event);
			gesture.setLastEvent (event);

			// notify the window about the processed gesture
			if(Window* window = ccl_cast<Window> (&rootView))
			{
				window->onGestureProcessed (event, view);

				if(NonModalPopupSelectorWindow::processForeignEvent (event, window))
				{
					// prevent further processing of touch / gesture that closed the popup
					for(TouchID id : gesture.getTouchIDs ())
						if(TouchItem* touchItem = getTouchItem (id))
							setTouchDiscarded (*touchItem);
				}
			}
		}
	}

	// remove succeeded tap gestures when touch is already gone
	if(event.getState () != GestureEvent::kFailed && gesture.isTap () && gesture.getTouchIDs ().isEmpty ())
	{
		// hmm, would not work for multiple people tapping on a large screen (e.g. entangled double taps)
		ForEach (gestures, Gesture, g)
			if(g->isTap () && g->getTouchIDs ().isEmpty ())
				removeGesture (g);
		EndFor
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Window* TouchInputState::getOtherPopup () const
{
	if(Desktop.isPopupActive ())
	{
		Window* popupWindow = Desktop.getTopWindow (kPopupLayer);
		if(popupWindow && popupWindow != rootView.getWindow () && !popupWindow->isInDestroyEvent ())
			return popupWindow;
	}
	return nullptr;
}

//************************************************************************************************
// TouchInputManager
//************************************************************************************************

DEFINE_IID_ (IGestureManager, 0x0A028585, 0xD30A, 0x469A, 0xA6, 0x31, 0xE6, 0x87, 0x28, 0x79, 0x4C, 0xCB)

DEFINE_SINGLETON_CLASS (TouchInputManager, Object)
DEFINE_CLASS_UID (TouchInputManager, 0x6da36ba4, 0xb839, 0x440b, 0x98, 0x31, 0xea, 0x71, 0x5d, 0x11, 0x7f, 0xfe)
DEFINE_SINGLETON (TouchInputManager)

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API TouchInputManager::processTouches (IWindow* _window, const TouchEvent& event)
{
	Window* window = unknown_cast<Window> (_window);
	if(window == nullptr)
		return kResultInvalidArgument;

	// if no (platform) gesture manager was set, add our own
	if(!window->getTouchInputState ().getGestureManager ())
		window->getTouchInputState ().setGestureManager (NEW CustomGestureManager (*window));

	window->getTouchInputState ().processTouches (event);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult TouchInputManager::setGestureManager (IWindow* _window, IGestureManager* manager)
{
	Window* window = unknown_cast<Window> (_window);
	if(window == nullptr)
		return kResultInvalidArgument;

	window->getTouchInputState ().setGestureManager (manager);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGestureManager* TouchInputManager::getGestureManager (IWindow* _window) const
{
	if(Window* window = unknown_cast<Window> (_window))
		return window->getTouchInputState ().getGestureManager ();

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API TouchInputManager::discardTouches (IWindow* _window)
{
	Window* window = unknown_cast<Window> (_window);
	if(window == nullptr)
		return kResultInvalidArgument;
		
	window->getTouchInputState ().discardTouches (false);
	
	return kResultOk;
}
