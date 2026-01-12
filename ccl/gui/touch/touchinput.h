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
// Filename    : ccl/gui/touch/touchinput.h
// Description : Touch Input State
//
//************************************************************************************************

#ifndef _ccl_touchinput_h
#define _ccl_touchinput_h

#include "ccl/gui/views/view.h"

#include "core/gui/coregesturerecognition.h"

#include "ccl/public/gui/framework/imultitouch.h"

#include "ccl/base/storage/configuration.h"
#include "ccl/base/singleton.h"

namespace CCL {

class TouchHandler;
class Gesture;
interface IGestureManager;

using Core::GestureInfo;

//************************************************************************************************
// TouchInputManager
//************************************************************************************************

class TouchInputManager: public Object,
						 public ITouchInputManager,
						 public Singleton<TouchInputManager>
{
public:
	DECLARE_CLASS (TouchInputManager, Object)

	tresult setGestureManager (IWindow* window, IGestureManager* gestureManager);
	IGestureManager* getGestureManager (IWindow* window) const;

	// ITouchInputManager
	tresult CCL_API processTouches (IWindow* window, const TouchEvent& event) override;
	tresult CCL_API discardTouches (IWindow* window) override;

	CLASS_INTERFACE (ITouchInputManager, Object)
};

//************************************************************************************************
// TouchInputState
//************************************************************************************************

class TouchInputState: public Object
{
public:
	TouchInputState (View& rootView);
	~TouchInputState ();

	void setGestureManager (IGestureManager* gestureManager);
	IGestureManager* getGestureManager () const;

	struct TouchEventData
	{
		int eventType;
		KeyState keys;
		TouchEvent::InputDevice inputDevice;
		TouchEvent::PenInfo penInfo;

		TouchEventData (int eventType = TouchEvent::kMove, const KeyState& keys = KeyState (), TouchEvent::InputDevice inputDevice = TouchEvent::kPointerInput, const TouchEvent::PenInfo& penInfo = TouchEvent::PenInfo ());
		TouchEventData (const TouchEvent& e);
	};

	void processTouches (const TouchEvent& event);		///< process a collection of all touches
	void processTouch (const TouchInfo& touch, const TouchEventData& data = TouchEventData ());		///< process one new or updated touch
	void processTouchEnd (const TouchInfo& touch, const TouchEventData& data = TouchEventData ());	///< process one ended touch
	void processTouchLeave (const TouchInfo& touch, const TouchEventData& data);

	void onGesture (const GestureEvent& event, Gesture& gesture);
	void deferGesture (const GestureEvent& event, Gesture& gesture);

	void discardTouches (bool deferred, bool keepItems = false); ///< remove all pending touches; keepItems: don't remove, but only ignore them (avoid recreating in successive events)
	void discardTouchesForView (const View& view, bool keepItems = false);	///< remove pending touches assigned to that view or it's children
	void discardTouchesForEvent (const TouchEvent& event);
	void discardHoverTouches ();
	bool hasTouchHandlerInViewArea (const View& view) const;
	bool hasTouchAtPosition (PointRef p) const;
	bool hasTouchAtPosition (PointFRef p) const;
	const Container& getPendingGestures () const;
	PointF calculateTouchCenter (Gesture& gesture) const;
	int countRemainingShadowTouches (const Gesture& gesture) const;

	TouchID getFirstTouchID () const;
	bool hasTouch (TouchID id) const;
	bool getTouchPosition (Point& p, TouchID id) const;
	bool getTouchPosition (PointF& p, TouchID id) const;
	Gesture* getGesture (int gestureType, TouchID touchID);
	Window* getOtherPopup () const;

	static bool isInGestureEvent ();
	static int getLongPressDelay ();

private:
	View& rootView;
	ObjectList touchItems;
	ObjectList gestures;
	AutoPtr<IGestureManager> gestureManager;
	static bool inGestureEvent;
	Gesture* delegatingGesture;
	static const double kContextMenuDelay;
	static const Coord kContextMenuMaxDistance;
	bool contextMenuPending;
	static Configuration::IntValue longPressDelay;
	static Configuration::BoolValue longPressContextMenu;
	static Configuration::BoolValue penBarrelButtonGesture;

	class TouchItem;
	class SimpleTouchEvent;
	class ZoomOffsetHelper;
	struct DeferredGesture;

	TouchItem* getTouchItem (TouchID id) const;
	Gesture* getGesture (int gestureType, TouchItem& touchItem, bool add);
	void removeGesture (Gesture* gesture, bool isAborted = false);
	void updateGestureRecognizers ();
	bool tryDragGesture (const GestureEvent& event);
	bool tryDelegateGesture (const GestureEvent& event, Gesture& gesture);
	void setTouchDiscarded (TouchItem& item);

	TouchItem* addTouchItem (const TouchEvent& event, const TouchInfo& touch);
	void discardTouchItem (TouchItem* item, bool keep = false);
	void removeTouchItem (TouchItem* item);
	bool acceptTouchHandler (TouchItem& touchItem, ITouchHandler* handler);
	void giveUpTouchHandler (TouchItem& touchItem, ITouchHandler* handler);
	void updateHoverCandidates (TouchItem& item, const TouchEvent& event, const TouchInfo& touch);
	void triggerHoverCandidates (TouchItem& item, const TouchEvent& event);
	bool collectTouchHandlers (View& view, PointRef where, TouchItem& touchItem, const TouchEvent& event);
	bool collectTouchHandlers (TouchItem& touchItem, const TouchEvent& event, const TouchInfo& touch);

	void onTouchEnter (TouchItem& item, const TouchEvent& event, const TouchInfo& touch);
	void onTouchBegan (TouchItem& item, const TouchEvent& event, const TouchInfo& touch);
	void onTouchChanged (TouchItem& item, const TouchEvent& event, const TouchInfo& touch);
	void onTouchEnded (TouchItem& item, const TouchEvent& event);

	bool checkPenButtons (const TouchItem& item, const TouchEvent& touchEvent, bool isNewItem = false);

	static bool isHoverEvent (int eventType);
	static bool isEndingEvent (int eventType);
	static bool isSameEvent (const GestureEvent& event1, const GestureEvent& event2);

	// Object
	void CCL_API notify (ISubject*, MessageRef msg) override;
};

//************************************************************************************************
// Gesture
//************************************************************************************************

class Gesture: public Core::GestureInfo,
			   public Object
{
public:
	DECLARE_CLASS_ABSTRACT (Gesture, Object)

	Gesture (int type);

	PROPERTY_SHARED_AUTO (ITouchHandler, handler, Handler)
	PROPERTY_SHARED_AUTO (Object, touchItem, TouchItem)	///< optional; used to keep an already ended touch item for processing a "late" single tap (deferred by a pending double tap)
	PROPERTY_OBJECT (GestureEvent, lastEvent, LastEvent) ///< last event sent to handler
	PROPERTY_OBJECT (PointF, offset, Offset)	///< offset to be added to system gesture positions (used for zoom)
	PROPERTY_BOOL (done, Done)
	PROPERTY_BOOL (shadow, Shadow)
	PROPERTY_BOOL (exclusiveTouch, ExclusiveTouch)
	PROPERTY_SHARED_AUTO (Gesture, delegateGesture, DelegateGesture)
	PROPERTY_OBJECT (TouchVector, shadowTouches, ShadowTouches)
	PROPERTY_SHARED_AUTO (ITouchHandler, alternativeHandler, AlternativeHandler)

	bool addCandidate (ITouchHandler* handler, View* view, int gestureType, int priority);
	void checkCandidates (const GestureEvent& event);
	ITouchHandler* getSingleCandidate () const;
	void getCandidateHandlers (IUnknownList& handlers) const;
	View* getViewForHandler (ITouchHandler* handler) const;

private:
	ObjectList candidates;

	class Candidate;
};

//************************************************************************************************
// IGestureManager
/** Gesture Manager interface. Manages gesture recognition using touch input.
	\ingroup gui */
//************************************************************************************************

interface IGestureManager: IUnknown
{
	/** Touch events for individual touches. */
	virtual void onTouchBegan (const TouchInfo& touchInfo) = 0;
	virtual void onTouchChanged (const TouchInfo& touchInfo) = 0;
	virtual void onTouchEnded (const TouchInfo& touchInfo) = 0;

	/** Manage recognizing specific gestures. */
	virtual tbool isRecognizing (const GestureInfo* gesture) const = 0;
	virtual void startRecognizing (GestureInfo* gesture) = 0;
	virtual void stopRecognizing (GestureInfo* gesture) = 0;

	///** Notification when a touch was added to a gesture. */
	virtual void updateTouchesForGesture (GestureInfo* gesture) = 0;

	DECLARE_IID (IGestureManager)
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline IGestureManager* TouchInputState::getGestureManager () const { return gestureManager; }
inline const Container& TouchInputState::getPendingGestures () const { return gestures; }
inline bool TouchInputState::isInGestureEvent () { return inGestureEvent; }
inline bool TouchInputState::isHoverEvent (int eventType) { return eventType >= TouchEvent::kEnter; }
inline bool TouchInputState::isEndingEvent (int eventType) { return eventType == TouchEvent::kEnd || eventType == TouchEvent::kLeave; }
inline int TouchInputState::getLongPressDelay () { return longPressDelay; }

inline TouchInputState::TouchEventData::TouchEventData (int eventType, const KeyState& keys, TouchEvent::InputDevice inputDevice, const TouchEvent::PenInfo& penInfo)
: eventType (eventType),
  keys (keys),
  inputDevice (inputDevice),
  penInfo (penInfo)
{}

inline TouchInputState::TouchEventData::TouchEventData (const TouchEvent& e)
: eventType (e.eventType),
  keys (e.keys),
  inputDevice (e.inputDevice),
  penInfo (e.penInfo)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_touchinput_h
