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
// Filename    : ccl/platform/win/gui/touchhelper.cpp
// Description : Windows Touch API Helpers
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/win/gui/touchhelper.h"
#include "ccl/platform/win/gui/screenscaling.h"
#include "ccl/platform/win/gui/activex.h"

#include "ccl/gui/windows/nativewindow.h"
#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/touch/touchinput.h"
#include "ccl/gui/touch/touchcollection.h"
#include "ccl/gui/system/dragndrop.h"
#include "ccl/gui/gui.h"

#include "ccl/base/storage/configuration.h"

#include "ccl/public/math/mathprimitives.h"
#include "ccl/public/gui/framework/imultitouch.h"
#include "ccl/public/gui/framework/idleclient.h"
#include "ccl/public/systemservices.h"

#include "core/system/coretime.h"

#include <manipulations_i.c>
#include <manipulations.h>

#pragma comment(lib, "Ninput.lib")

using namespace CCL;
using namespace Win32;

#define OWN_DRAG_LOOP 0
#define DEFER_LONGPRESS 1
#define SIMULATE_LEFTBUTTON 0

#define LOG_TOUCHES 1
#define LOG_MANIPULATION 0

//////////////////////////////////////////////////////////////////////////////////////////////////
// Debug logging
//////////////////////////////////////////////////////////////////////////////////////////////////

#if LOG_TOUCHES
#define PRINT_POINTER(msg,id,p) CCL_PRINTF ("[%d] %s (%d, %d) %s\n", id, msg, p.x, p.y, IS_POINTER_INCONTACT_WPARAM (e.wParam) ? "Contact" : "Hover")
#define PRINT_TOUCH(msg,id,p) CCL_PRINTF ("[%d] %s (%d, %d)\n", id, msg, p.x, p.y)
#else
#define PRINT_POINTER(msg,id,p)
#define PRINT_TOUCH(msg,id,p)
#endif

#if LOG_MANIPULATION
#define PRINT_MANIPULATION(s, ...) CCL_PRINTF (s, __VA_ARGS__)
#else
#define PRINT_MANIPULATION(s, ...)
#endif

#if DEBUG_LOG
inline MutableCString getInteractionFlagName (const INTERACTION_CONTEXT_OUTPUT& output)
{
	MutableCString str;
	if(output.interactionFlags & INTERACTION_FLAG_BEGIN)
		str = "BEGIN";
	if(output.interactionFlags & INTERACTION_FLAG_END)
		str += " END";
	if(output.interactionFlags & INTERACTION_FLAG_CANCEL)
		str += " CANCEL";
	if(output.interactionFlags & INTERACTION_FLAG_INERTIA)
		str += " INERTIA";

	return str;
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_INIT_LEVEL (TouchHelper, kFrameworkLevelSecond)
{
	bool multiTouchEnabled = false;
	Configuration::Registry::instance ().getValue (multiTouchEnabled, "CCL.Win32", "MultiTouchEnabled");
	
	if(multiTouchEnabled == true)
		TouchHelper::initialize ();
	return true;
}

//************************************************************************************************
// TouchHelper::GestureRecognizer
//************************************************************************************************

class TouchHelper::GestureRecognizer: public Object
{
public:
	GestureRecognizer (Window* window);

	bool hasGestures () const								{ return !gestures.isEmpty (); }
	bool handlesGesture (const GestureInfo* gesture) const	{ return gestures.contains (const_cast<GestureInfo*> (gesture)); }
	bool handlesTouch (TouchID touchID) const				{ return touchIds.contains (touchID); }

	bool canHandleAdditionalGesture (const GestureInfo& otherGesture) const;

	void addGesture (GestureInfo* gesture);
	void removeGesture (GestureInfo* gesture);
	void updateTouches (GestureInfo* gesture);

	virtual void onTouchAdded (TouchID touchId);
	virtual void onTouchEnded (TouchID touchId);

protected:
	Window* window;
	Vector<TouchID> touchIds;
	LinkedList<GestureInfo*> gestures;
	GestureInfo* activeGesture;
	bool mustCheckSwitchGesture;

	GestureInfo* findGesture (int type) const;
	GestureInfo* chooseGesture (int type);
	GestureInfo* chooseManipulationGesture (float translationX, float translationY, float expansion, float rotation);
	GestureInfo* determineManipulationGesture (float translationX, float translationY, float expansion, float rotation);

	enum
	{
		kDoubleClickTimeout = 600
	};
};

//************************************************************************************************
// TouchHelper::PositionChangeTracker
/** Helper for tracking the last change time of a position per coordinate. */
//************************************************************************************************

class TouchHelper::PositionChangeTracker
{
public:
	void init (PointFRef position, int64 time)
	{
		positions.empty ();
		positions.add ({ time, position });
	}

	void trackPosition (PointFRef position, int64 time)
	{
		// clear positions older than kMoveTimeOut, except for the last
		for(int i = positions.count () - 1; i > 0; i--)
			if(time - positions[i].time > kMoveTimeOut)
				positions.removeAt (i - 1);

		positions.add ({ time, position });
	}

	bool hasMovedX (PointFRef position, int64 time) const { return fabs (position.x - getReferencePosition (time).x) > kTolerance; }
	bool hasMovedY (PointFRef position, int64 time) const { return fabs (position.y - getReferencePosition (time).y) > kTolerance; }

	static constexpr CoordF kTolerance = 12;	// moves inside this tolerance are ignored
	static constexpr int64 kMoveTimeOut = 100;	// in ms: changes faster than this are considered a move

private:
	struct PositionAtTime
	{
		int64 time;
		PointF position;
	};

	Vector<PositionAtTime> positions;

	PointF getReferencePosition (int64 time) const
	{
		if(positions.isEmpty ())
			return PointF ();

		PointF referencePosition = positions[0].position;
		for(int i = 1; i < positions.count (); i++)
		{
			if(time - positions[i].time < kMoveTimeOut)
				break;

			referencePosition = positions[i].position;
		}
		return referencePosition;
	}
};

//************************************************************************************************
// TouchHelper::PointerGestureRecognizer
/** Gesture recognition with WM_POINTER messages using InteractionContext. */
//************************************************************************************************

class TouchHelper::PointerGestureRecognizer: public TouchHelper::GestureRecognizer,
											 public IdleClient
{
public:
	PointerGestureRecognizer (Window* window);
	~PointerGestureRecognizer ();

	void processPointerFrames (const POINTER_INFO& pointerInfo);

	CLASS_INTERFACE (ITimerTask, GestureRecognizer)

private:
	HINTERACTIONCONTEXT interactionContext;
	UINT32 frameId;
	bool initialized;
	bool didBegin;
	GestureEvent pendingSingleTap;
	TouchID prolongedLongPressTouchID;
	int64 initialTime;
	Point initalPos;
	PositionChangeTracker positionTracker; // used for swipe only

	struct ManipulationData
	{
		float translationX = 0;
		float translationY = 0;
		float scale = 1;
		float expansion = 0;
		float rotation = 0;

		ManipulationData ()
		{}

		ManipulationData (const MANIPULATION_TRANSFORM& data)
		: translationX (data.translationX),
		  translationY (data.translationY),
		  scale (data.scale),
		  expansion (data.expansion),
		  rotation (data.rotation)
		{}

		ManipulationData& operator -= (const ManipulationData& other)
		{
			translationX -= other.translationX;
			translationY -= other.translationY;
			scale /= other.scale;
			expansion -= other.expansion;
			rotation -= other.rotation;
			return *this;
		}
	};
	ManipulationData manipulationStart;

	void init ();
	GestureEvent makeGestureEvent (const INTERACTION_CONTEXT_OUTPUT& output, int eventType);
	GestureEvent makeGestureEvent (const INTERACTION_CONTEXT_OUTPUT& output, int eventType, int state);
	GestureEvent makeGestureEvent (PointFRef pos, int eventType, int state);
	void checkPendingTap ();

	static void CALLBACK interactionOutputCallback (void* clientData, const INTERACTION_CONTEXT_OUTPUT* output);
	void onInteractionOutput (const INTERACTION_CONTEXT_OUTPUT& output);

	// GestureRecognizer
	void onTouchAdded (TouchID touchID) override;

	// IdleClient
	void onIdleTimer () override;
};

//************************************************************************************************
// TouchHelper
//************************************************************************************************

int TouchHelper::lastTouchMessageTime = 0;
Point TouchHelper::lastTouchPosition;
bool TouchHelper::touchDragging = false;
TouchID TouchHelper::lastTouchID = 0;
KeyState TouchHelper::lastKeys;
Configuration::BoolValue TouchHelper::usePenAsMouse ("CCL.Win32", "UsePenAsMouse", true);

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchHelper::initialize ()
{
	lastTouchMessageTime = ::GetTickCount ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchHelper::onPlatformStarted (bool ownProcess)
{
	/** This function tells Windows if we want to handle mouse input as WM_POINTER messages instead of WM_MOUSEMOVE, WM_BUTTONDOWN, etc.
		Only the first call per process succeeds and changes the state.
		If we run as the main application, nail this down to mouse messages, to prevent changes by a plugin (QT 5.12 does this). */
	if(ownProcess)
		::EnableMouseInPointer (false);
}
 
//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchHelper::prepareWindow (Window& window)
{
	window.getTouchInputState ().setGestureManager (NEW RecognizerManager (&window));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TouchHelper::RecognizerManager* TouchHelper::getRecognizerManager (Window& window)
{
	return unknown_cast<TouchHelper::RecognizerManager> (window.getTouchInputState ().getGestureManager ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TouchHelper::didHandleButtonMessage (Window& window, PointRef where)
{
	return didHandleCurrentMessage () && (isButtonMessageFromTouch () || window.getTouchInputState ().hasTouchAtPosition (where));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 TouchHelper::getTouchTime (uint64 time)
{
	return Core::SystemClock::toMilliseconds (time);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TouchHelper::processGestureEvent (Window& window, SystemEvent& e)
{
	if(e.msg == WM_GESTURENOTIFY) // nothing here
		return false;

	ASSERT (e.msg == WM_GESTURE)

	HWND hwnd = (HWND)window.getSystemWindow ();

	GESTUREINFO gestureInfo = {0};
	gestureInfo.cbSize = sizeof(GESTUREINFO);

	bool handled = true;
	if(::GetGestureInfo ((HGESTUREINFO)e.lParam, &gestureInfo))
	{
		Point where (gestureInfo.ptsLocation.x, gestureInfo.ptsLocation.y);
		Win32Window::cast (&window)->screenPixelToClientCoord (where);

		int eventType = 0;
		switch(gestureInfo.dwID)
		{
		case GID_BEGIN	: eventType = GestureEvent::kBegin; handled = false; break;
		case GID_END	: eventType = GestureEvent::kBegin; handled = false; break;
		case GID_ZOOM	: eventType = GestureEvent::kZoom; break;
		case GID_PAN	: eventType = GestureEvent::kSwipe; break; // ???
		case GID_ROTATE	: eventType = GestureEvent::kRotate; break;
		}

		if(eventType != 0)
			window.onGesture (GestureEvent (eventType, where));
	}

	if(handled)
		::CloseGestureInfoHandle ((HGESTUREINFO)e.lParam);
	return handled;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchHelper::setTouchDragging (bool state)
{
	if(touchDragging != state)
	{
		CCL_PRINTF ("setTouchDragging (%d)\n", state)
		touchDragging = state;
		if(touchDragging)
		{
			MouseEvent e0 (MouseEvent::kMouseMove, lastTouchPosition);
			GUI.simulateEvent (e0);
			MouseEvent e1 (MouseEvent::kMouseDown, lastTouchPosition, SIMULATE_LEFTBUTTON ? KeyState::kLButton : KeyState::kRButton);
			GUI.simulateEvent (e1);
		}
		else
		{
			MouseEvent e (MouseEvent::kMouseUp, lastTouchPosition, SIMULATE_LEFTBUTTON ? KeyState::kLButton : KeyState::kRButton);
			GUI.simulateEvent (e);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TouchHelper::isTouchDragging ()
{
	return touchDragging;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TouchHelper::processPointerEvent (Window& window, SystemEvent& e)
{
	POINTER_INFO pointerInfo = {};
	if(::GetPointerInfo (GET_POINTERID_WPARAM (e.wParam), &pointerInfo))
	{
		ASSERT (GET_POINTERID_WPARAM (e.wParam) == pointerInfo.pointerId)

		TouchInputState::TouchEventData eventData;
		GUI.getKeyState (eventData.keys);

		if(pointerInfo.pointerType == PT_TOUCH)
			eventData.inputDevice = TouchEvent::kTouchInput;
		else if(pointerInfo.pointerType == PT_PEN)
		{
			eventData.inputDevice = TouchEvent::kPenInput;

			POINTER_PEN_INFO pointerPenInfo = {};
			if(::GetPointerPenInfo (pointerInfo.pointerId, &pointerPenInfo))
			{
				if(pointerPenInfo.penMask & PEN_MASK_TILT_X)
					eventData.penInfo.tiltX = float(pointerPenInfo.tiltX);
				if(pointerPenInfo.penMask & PEN_MASK_TILT_Y)
					eventData.penInfo.tiltY = float(pointerPenInfo.tiltY);
				if(pointerPenInfo.penMask & PEN_MASK_ROTATION)
					eventData.penInfo.twist = float(pointerPenInfo.rotation);
				if(pointerPenInfo.penMask & PEN_MASK_PRESSURE)
					eventData.penInfo.pressure = float(pointerPenInfo.pressure) / 1024.f;

				if(pointerPenInfo.penFlags & PEN_FLAG_BARREL)
					eventData.keys.keys |= KeyState::kPenBarrel;
				if(pointerPenInfo.penFlags & PEN_FLAG_ERASER)
					eventData.keys.keys |= KeyState::kPenEraser;
				if(pointerPenInfo.penFlags & PEN_FLAG_INVERTED)
					eventData.keys.keys |= KeyState::kPenInverted;
			}
		}

		if(pointerInfo.pointerType == PT_MOUSE) // ignore mouse input, Windows will send mouse messages afterwards (e.g. when running as plug-in in a host that handles mouse as pointer)
			return false;

		// if demanded by configuration, ignore pen input (fallback to mouse handling; pen has mouseover-like behavior, sends WM_POINTERUPDATE before WM_POINTERDOWN)
		if(usePenAsMouse && pointerInfo.pointerType == PT_PEN)
			return false;

		Point screenPoint (pointerInfo.ptPixelLocation.x, pointerInfo.ptPixelLocation.y);
		Win32::gScreens->toCoordPoint (screenPoint);
		Point point (screenPoint);

		lastTouchPosition = point;

		window.screenToClient (point);

		lastTouchID = pointerInfo.pointerId;
		lastTouchMessageTime = GetMessageTime ();

		// these do not seem to reflect the keyboard modifiers:
		if(pointerInfo.dwKeyStates & POINTER_MOD_SHIFT)
			eventData.keys.keys |= KeyState::kShift;
		if(pointerInfo.dwKeyStates & POINTER_MOD_CTRL)
			eventData.keys.keys |= KeyState::kCommand;

		lastKeys = eventData.keys;

		TouchInfo touch (TouchEvent::kMove, pointerInfo.pointerId, point, getTouchTime (pointerInfo.PerformanceCount));

		switch(e.msg)
		{
		case WM_POINTERDOWN :
			PRINT_POINTER ("WM_POINTERDOWN", touch.id, point)
			eventData.eventType = touch.type = TouchEvent::kBegin;
			GUI.setMousePosition (screenPoint);
			GUI.resetDoubleClick ();
			window.getTouchInputState ().processTouch (touch, eventData);

			if(RecognizerManager* recMan = getRecognizerManager (window))
				recMan->processPointerFrames (pointerInfo);
			return true;

		case WM_POINTERUPDATE :
			PRINT_POINTER ("WM_POINTERUPDATE", touch.id, point)
			eventData.eventType = touch.type = IS_POINTER_INCONTACT_WPARAM (e.wParam) ? TouchEvent::kMove : TouchEvent::kHover;
			window.getTouchInputState ().processTouch (touch, eventData);

			if(touch.type == TouchEvent::kMove)
				if(RecognizerManager* recMan = getRecognizerManager (window))
					recMan->processPointerFrames (pointerInfo);

			#if !OWN_DRAG_LOOP
			if(isTouchDragging ())
			{
				GUI.simulateEvent (MouseEvent (MouseEvent::kMouseMove, lastTouchPosition));
				return false; // (not handled)
			}
			#endif
			return true;

		case WM_POINTERUP :
			PRINT_POINTER ("WM_POINTERUP", touch.id, point)

			if(RecognizerManager* recMan = getRecognizerManager (window))
				recMan->processPointerFrames (pointerInfo);

			#if !OWN_DRAG_LOOP
			if(isTouchDragging ())
			{
				setTouchDragging (false);
				
				if(Gesture* gesture = window.getTouchInputState ().getGesture (GestureEvent::kLongPress, touch.id))
				{
					GestureEvent event (GestureEvent::kLongPress|GestureEvent::kEnd, point);
					event.keys = eventData.keys;
					window.getTouchInputState ().onGesture (event, *static_cast<Gesture*> (gesture));
				}
			}
			#endif

			eventData.eventType = touch.type = TouchEvent::kEnd;
			window.getTouchInputState ().processTouchEnd (touch, eventData);
			return true;
		
		case WM_POINTERENTER:
			PRINT_POINTER ("WM_POINTERENTER", touch.id, point)
			eventData.eventType = touch.type = TouchEvent::kEnter;
			window.getTouchInputState ().processTouch (touch, eventData);
			break;

		case WM_POINTERLEAVE:
			PRINT_POINTER ("WM_POINTERLEAVE", touch.id, point)
			eventData.eventType = touch.type = TouchEvent::kLeave;
			window.getTouchInputState ().processTouchLeave (touch, eventData);
			break;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TouchHelper::runDragLoop (DragSession& session)
{
	#if !OWN_DRAG_LOOP
	return false;
	#endif

	Window* window = Desktop.getActiveWindow ();
	if(window)
	{
		TouchID touchId = lastTouchID;
		Point p;
		window->getTouchInputState ().getTouchPosition (p, touchId);

		DragEvent dragEvent (session, DragEvent::kDragEnter, p, KeyState::kLButton);
		dragEvent.eventTime = System::GetProfileTime ();
		dragEvent.keys = lastKeys;
		window->onDragEnter (dragEvent);

		while(window->getTouchInputState ().getTouchPosition (p, touchId))
		{
			if(GUI.isKeyPressed (VKey::kEscape))
				session.setCanceled ();

			if(session.wasCanceled () || session.isDropped ())
				break;

			if(p != dragEvent.where)
			{
				session.setDragImagePosition (p);
				session.showNativeDragImage (!session.hasVisualFeedback ());

				dragEvent.eventType = DragEvent::kDragOver;
				dragEvent.eventTime = System::GetProfileTime ();
				dragEvent.where = p;
				dragEvent.keys = lastKeys;
				window->onDragOver (dragEvent);
			}

			MSG msg;
			if(::GetMessage (&msg, nullptr, 0, 0))
			{
				::TranslateMessage (&msg);
				::DispatchMessage (&msg);
			}
		}

		if(!session.wasCanceled () && session.getResult () != DragSession::kDropNone)
			session.setDropped (true);

		dragEvent.eventTime = System::GetProfileTime ();
		if(session.isDropped ())
		{
			dragEvent.eventType = DragEvent::kDrop;
			dragEvent.keys = lastKeys;
			window->onDrop (dragEvent);
		}
		else
		{
			dragEvent.eventType = DragEvent::kDragLeave;
			window->onDragLeave (dragEvent);
		}
	}
	return true;
}

//************************************************************************************************
// RecognizerManager
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (TouchHelper::RecognizerManager, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

TouchHelper::RecognizerManager::RecognizerManager (Window* window)
: window (window)
{
	recognizers.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TouchHelper::GestureRecognizer* TouchHelper::RecognizerManager::findRecognizer (const GestureInfo* gesture) const
{
	ListForEachObject (recognizers, GestureRecognizer, recognizer)
		if(recognizer->handlesGesture (gesture))
			return recognizer;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TouchHelper::GestureRecognizer* TouchHelper::RecognizerManager::findRecognizerForTouches (const GestureInfo* gesture) const
{
	ListForEachObject (recognizers, GestureRecognizer, recognizer)
		if(!recognizer->canHandleAdditionalGesture (*gesture))
			continue;

		bool handlesAll = true;
		VectorForEachFast (gesture->getTouchIDs (), TouchID, touchID)
			if(!recognizer->handlesTouch (touchID))
			{
				handlesAll = false;
				break;
			}
		EndFor
		if(handlesAll)
			return recognizer;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchHelper::RecognizerManager::processPointerFrames (const POINTER_INFO& pointerInfo)
{
	ForEach (recognizers, PointerGestureRecognizer, recognizer)
		if(recognizer->handlesTouch (pointerInfo.pointerId))
			recognizer->processPointerFrames (pointerInfo);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool TouchHelper::RecognizerManager::isRecognizing (const GestureInfo* gesture) const
{
	return findRecognizer (gesture) != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchHelper::RecognizerManager::startRecognizing (GestureInfo* gesture)
{
	// try to find a reconizer for the same set of touches
	GestureRecognizer* recognizer = findRecognizerForTouches (gesture);
	if(!recognizer)
	{
		recognizer = NEW PointerGestureRecognizer (window);
		recognizers.append (recognizer);
	}
	CCL_PRINTF ("[%s] startRecognizing \"%s\" %s\n", Debugger::ObjectID (recognizer).str, Gesture::getGestureName (gesture), window->myClass ().getPersistentName ())
	recognizer->addGesture (gesture);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchHelper::RecognizerManager::stopRecognizing (GestureInfo* gesture)
{
	GestureRecognizer* recognizer = findRecognizer (gesture);
	SOFT_ASSERT (recognizer, "stopRecognizing")
	CCL_PRINTF ("[%s] stopRecognizing \"%s\" %s\n", Debugger::ObjectID (recognizer).str, Gesture::getGestureName (gesture), window->myClass ().getPersistentName ())
	if(recognizer)
	{
		recognizer->removeGesture (gesture);

		if(!recognizer->hasGestures ())
			if(recognizers.remove (recognizer))
				deferDestruction (recognizer); // can't destroy recognizer while he's in it's process... call
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchHelper::RecognizerManager::updateTouchesForGesture (GestureInfo* gesture)
{
	if(GestureRecognizer* recognizer = findRecognizer (gesture))
		recognizer->updateTouches (gesture);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchHelper::RecognizerManager::onTouchEnded (const TouchInfo& touchInfo)
{
	for(auto recognizer : iterate_as<GestureRecognizer> (recognizers))
		if(recognizer->handlesTouch (touchInfo.id))
			recognizer->onTouchEnded (touchInfo.id);
}

//************************************************************************************************
// TouchHelper::GestureRecognizer
//************************************************************************************************

TouchHelper::GestureRecognizer::GestureRecognizer (Window* window)
: window (window),
  activeGesture (nullptr),
  mustCheckSwitchGesture (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

GestureInfo* TouchHelper::GestureRecognizer::findGesture (int type) const
{
	ListForEach (gestures, GestureInfo*, gesture)
		if(gesture->getType () == type)
			return gesture;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TouchHelper::GestureRecognizer::canHandleAdditionalGesture (const GestureInfo& otherGesture) const
{
	// don't combine recognition of continuous gestures (swipe, zoom, etc.) with tap or double tap (double tap accepts any other touch as second touch, but continuous recognizers must be separate)
	GestureInfo* ownGesture = gestures.getFirst ();
	return !ownGesture || ownGesture->isContinuous () == otherGesture.isContinuous ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchHelper::GestureRecognizer::addGesture (GestureInfo* gesture)
{
	gestures.append (gesture);

	updateTouches (gesture);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchHelper::GestureRecognizer::removeGesture (GestureInfo* gesture)
{
	if(gesture == activeGesture)
		activeGesture = nullptr;

	gestures.remove (gesture);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchHelper::GestureRecognizer::updateTouches (GestureInfo* gesture)
{
	ASSERT (gestures.contains (gesture))

	VectorForEachFast (gesture->getTouchIDs (), TouchID, touchID)
		if(!touchIds.contains (touchID))
		{
			touchIds.add (touchID);
			onTouchAdded (touchID);
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchHelper::GestureRecognizer::onTouchAdded (TouchID touchId)
{
	if(touchIds.count () == 2)
		mustCheckSwitchGesture = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchHelper::GestureRecognizer::onTouchEnded (TouchID touchId)
{
	touchIds.remove (touchId);

	if(touchIds.count () == 1)
		mustCheckSwitchGesture = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

GestureInfo* TouchHelper::GestureRecognizer::chooseGesture (int type)
{
	if(!activeGesture)
	{
		activeGesture = findGesture (type);
		CCL_PRINTF ("chooseGesture: %s\n", activeGesture ? Gesture::getGestureName (activeGesture) : "-")
	}

	return activeGesture && activeGesture->getType () == type ? activeGesture : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

GestureInfo* TouchHelper::GestureRecognizer::determineManipulationGesture (float translationX, float translationY, float expansion, float rotation)
{
	struct ManipulationGesture
	{
		GestureInfo* gesture;
		float amount;
			
		ManipulationGesture (GestureInfo* gesture = nullptr, float amount = 0)
		: gesture (gesture), amount (amount) {}
			
		bool operator > (const ManipulationGesture& g) const
		{ return amount > g.amount; }
	};

	Vector<ManipulationGesture> manipulations (3);

	// these factors are meant as minimum values of a noticable manipulation (not as a required minimum for detection)
	// we use them to compare the amount of translation / expansion / rotation in a common scale
	const float minTranslation	= 5.f;
	const float minExpansion	= 2.f;
	const float minRotation		= 0.03f;

	float normTranslation = (ccl_abs (translationX) + ccl_abs (translationY)) / minTranslation;
	float normExpansion = ccl_abs (expansion) / minExpansion;
	float normRotation = ccl_abs (rotation) / minRotation;

	if(GestureInfo* swipe = findGesture (GestureEvent::kSwipe))
		manipulations.addSorted (ManipulationGesture (swipe, normTranslation));

	if(GestureInfo* zoom = findGesture (GestureEvent::kZoom))
		manipulations.addSorted (ManipulationGesture (zoom, normExpansion));

	if(GestureInfo* rotate = findGesture (GestureEvent::kRotate))
		manipulations.addSorted (ManipulationGesture (rotate, normRotation));

	if(!manipulations.isEmpty ())
	{
		GestureInfo* gesture = manipulations.last ().gesture;
		CCL_PRINTF ("determineManipulationGesture: translate: %f, expand: %f, rotate: %f  => %s (%d touches: %d)\n", normTranslation, normExpansion, normRotation, Gesture::getGestureName (gesture), gesture->getTouchIDs ().count (), gesture->getTouchIDs ().at (0) )
		return gesture;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

GestureInfo* TouchHelper::GestureRecognizer::chooseManipulationGesture (float translationX, float translationY, float expansion, float rotation)
{
	if(!activeGesture)
	{
		// delay decision until something happens
		if(translationX == 0 && translationY == 0 && expansion == 0 && rotation == 0)
			return nullptr;

		activeGesture = determineManipulationGesture (translationX, translationY, expansion, rotation);
	}

	if(activeGesture && gestures.contains (activeGesture)) // safety check if gesture is still alive
		switch(activeGesture->getType ())
		{
		case GestureEvent::kSwipe:
		case GestureEvent::kZoom:
		case GestureEvent::kRotate:
			return activeGesture;
		}

	return nullptr;
}

//************************************************************************************************
// TouchHelper::PointerGestureRecognizer
//************************************************************************************************

void CALLBACK TouchHelper::PointerGestureRecognizer::interactionOutputCallback (void* clientData, const INTERACTION_CONTEXT_OUTPUT* output)
{
	PointerGestureRecognizer* r = reinterpret_cast<PointerGestureRecognizer*>(clientData);
	r->onInteractionOutput (*output);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TouchHelper::PointerGestureRecognizer::PointerGestureRecognizer (Window* window)
: GestureRecognizer (window),
  frameId (0),
  prolongedLongPressTouchID (-1),
  initialized (false),
  initialTime (0),
  didBegin (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchHelper::PointerGestureRecognizer::init ()
{
	ASSERT (!initialized)

	HRESULT hr = ::CreateInteractionContext (&interactionContext);
	if(SUCCEEDED (hr))
		hr = ::SetPropertyInteractionContext (interactionContext, INTERACTION_CONTEXT_PROPERTY_FILTER_POINTERS, TRUE);

	if(SUCCEEDED (hr))
	{
		Vector<INTERACTION_CONTEXT_CONFIGURATION> configuration;
		
		INTERACTION_CONTEXT_CONFIGURATION manipulation = {}; // must be one structure for all manipulations (translate, scale, rotate)
		INTERACTION_CONTEXT_CONFIGURATION tap = {};

		ListForEach (gestures, GestureInfo*, gesture)
			// setup recognition for this gesture
			INTERACTION_CONTEXT_CONFIGURATION config = {};
			switch(gesture->getType ())
			{
			case GestureEvent::kSwipe:
				manipulation.interactionId = INTERACTION_ID_MANIPULATION;
				manipulation.enable |=
					INTERACTION_CONFIGURATION_FLAG_MANIPULATION |
					INTERACTION_CONFIGURATION_FLAG_MANIPULATION_TRANSLATION_X |
					INTERACTION_CONFIGURATION_FLAG_MANIPULATION_TRANSLATION_Y;
					//INTERACTION_CONFIGURATION_FLAG_MANIPULATION_TRANSLATION_INERTIA
				continue;

			case GestureEvent::kZoom:
				manipulation.interactionId = INTERACTION_ID_MANIPULATION;
				manipulation.enable |= INTERACTION_CONFIGURATION_FLAG_MANIPULATION_SCALING;
					//INTERACTION_CONFIGURATION_FLAG_MANIPULATION_SCALING_INERTIA
				continue;

			case GestureEvent::kRotate:
				manipulation.interactionId = INTERACTION_ID_MANIPULATION;
				manipulation.enable |= INTERACTION_CONFIGURATION_FLAG_MANIPULATION_ROTATION;
					//INTERACTION_CONFIGURATION_FLAG_MANIPULATION_ROTATION_INERTIA
				continue;

			case GestureEvent::kLongPress:
				config.interactionId = INTERACTION_ID_HOLD;
				config.enable = INTERACTION_CONFIGURATION_FLAG_HOLD;
				break;

			case GestureEvent::kSingleTap:
				tap.interactionId = INTERACTION_ID_TAP;
				tap.enable |= INTERACTION_CONFIGURATION_FLAG_TAP;
				continue;

			case GestureEvent::kDoubleTap:
				tap.interactionId = INTERACTION_ID_TAP;
				tap.enable |= INTERACTION_CONFIGURATION_FLAG_TAP_DOUBLE;
				continue;

			default:
				continue;
			}		
			ASSERT (config.interactionId)
			if(config.interactionId)
				configuration.add (config);
		EndFor

		if(manipulation.interactionId)
			configuration.add (manipulation);

		if(tap.interactionId)
			configuration.add (tap);
		
		ASSERT (!configuration.isEmpty ())
		if(!configuration.isEmpty ())
			hr = ::SetInteractionConfigurationInteractionContext (interactionContext, configuration.count (), configuration);
	}

	if(SUCCEEDED (hr))
	{
		hr = ::RegisterOutputCallbackInteractionContext (interactionContext, interactionOutputCallback, this);

		VectorForEachFast (touchIds, TouchID, touchID)
			::AddPointerInteractionContext (interactionContext, (UINT32)touchID);
		EndFor
	}
	initialized = true;
	initialTime = System::GetSystemTicks ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TouchHelper::PointerGestureRecognizer::~PointerGestureRecognizer ()
{
	if(initialized)
		::DestroyInteractionContext (interactionContext);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchHelper::PointerGestureRecognizer::onTouchAdded (TouchID touchID)
{
	GestureRecognizer::onTouchAdded (touchID);

	// add pointer if added after initialisation
	if(initialized)
		::AddPointerInteractionContext (interactionContext, (UINT32)touchID);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchHelper::PointerGestureRecognizer::processPointerFrames (const POINTER_INFO& pointerInfo)
{
	if(pointerInfo.frameId != frameId)
	{
		if(!initialized)
		{
			init ();
			initalPos (pointerInfo.ptPixelLocation.x, pointerInfo.ptPixelLocation.y);
		}

		if(prolongedLongPressTouchID == pointerInfo.pointerId && !isTouchDragging ())
		{
			// send a gesture event for the persisting long press touch
			Point p;
			Gesture* gesture = window->getTouchInputState ().getGesture (GestureEvent::kLongPress, prolongedLongPressTouchID);
			if(gesture && window->getTouchInputState ().getTouchPosition (p, prolongedLongPressTouchID))
			{
				int state = (pointerInfo.pointerFlags & POINTER_FLAG_UP) ? GestureEvent::kEnd : GestureEvent::kChanged;
				GestureEvent event (GestureEvent::kLongPress|state, p);
				window->getTouchInputState ().onGesture (event, *gesture);
			}
		}

		if(!activeGesture)
		{
			// check if long press gesture should start
			int now = System::GetSystemTicks ();
			if(now - initialTime > TouchInputState::getLongPressDelay ()
				&& ccl_abs (initalPos.x - pointerInfo.ptPixelLocation.x) < 2
				&& ccl_abs (initalPos.y - pointerInfo.ptPixelLocation.y) < 2)
			{
				if(activeGesture = findGesture (GestureEvent::kLongPress))
				{
					PointF where (pointerInfo.ptPixelLocation.x, pointerInfo.ptPixelLocation.y);
					GestureEvent event (makeGestureEvent (where, GestureEvent::kLongPress, GestureEvent::kBegin));

					prolongedLongPressTouchID = activeGesture->getTouchIDs ().first ();
					window->getTouchInputState ().deferGesture (event, *static_cast<Gesture*> (activeGesture));
				}
			}
		}

		// new frame to process
		frameId = pointerInfo.frameId;

		// determine pointer count and frame history length
		UINT uEntriesCount = 0;
		UINT uPointerCount = 0;
		if(::GetPointerFrameInfoHistory (pointerInfo.pointerId, &uEntriesCount, &uPointerCount, nullptr))
		{
			POINTER_INFO* frameHistory = NEW POINTER_INFO[uEntriesCount * uPointerCount];
			if(frameHistory != nullptr)
			{
				// retrieve frame history
				if(::GetPointerFrameInfoHistory (pointerInfo.pointerId, &uEntriesCount, &uPointerCount, frameHistory))
				{
					// process frame history
					::ProcessPointerFramesInteractionContext (interactionContext, uEntriesCount, uPointerCount, frameHistory);
				}
				delete [] frameHistory;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

GestureEvent TouchHelper::PointerGestureRecognizer::makeGestureEvent (const INTERACTION_CONTEXT_OUTPUT& output, int eventType)
{
	int state = GestureEvent::kChanged;

	if(output.interactionFlags & INTERACTION_FLAG_BEGIN)
		state = GestureEvent::kBegin;
	else if(output.interactionFlags & INTERACTION_FLAG_END)
		state = GestureEvent::kEnd;
	else if(output.interactionFlags & INTERACTION_FLAG_CANCEL)
		state = GestureEvent::kFailed;

	return makeGestureEvent (output, eventType, state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

GestureEvent TouchHelper::PointerGestureRecognizer::makeGestureEvent (const INTERACTION_CONTEXT_OUTPUT& output, int eventType, int state)
{
	if(output.interactionFlags & INTERACTION_FLAG_INERTIA)
	{
		CCL_PRINTLN ("INTERACTION_FLAG_INERTIA")
	}

	ManipulationData manipulation (output.arguments.manipulation.cumulative);
	manipulation -= manipulationStart;

	GestureEvent event (makeGestureEvent (PointF (output.x, output.y), eventType, state));

	switch(event.getType ())
	{
	case GestureEvent::kSwipe:
		{
			event.amountX = output.arguments.manipulation.velocity.velocityX * 1000;
			event.amountY = output.arguments.manipulation.velocity.velocityY * 1000;

			auto getPosition = [&] ()
			{
				PointF touchPosition;
				window->getTouchInputState ().getTouchPosition (touchPosition, touchIds.first ());
				return touchPosition;
			};

			switch(state)
			{
			case GestureEvent::kBegin:
				positionTracker.init (getPosition (), System::GetSystemTicks ());
				break;

			case GestureEvent::kChanged:
				positionTracker.trackPosition (getPosition (), System::GetSystemTicks ());
				break;

			case GestureEvent::kEnd:
				{
					int64 now = System::GetSystemTicks ();
					PointF position = getPosition ();

					if(!positionTracker.hasMovedX (position, now))
						event.amountX = 0;

					if(!positionTracker.hasMovedY (position, now))
						event.amountY = 0;
				}
				break;
			}
		}
		break;

	case GestureEvent::kZoom:
		event.amountX = manipulation.scale;
		event.amountY = event.amountX;
		break;

	case GestureEvent::kRotate:
		event.amountX = manipulation.rotation;
		event.amountY = event.amountX;
		break;

	case GestureEvent::kSingleTap:
	case GestureEvent::kDoubleTap:
		event.eventType = eventType | GestureEvent::kBegin;
		break;
	}

	return event;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

GestureEvent TouchHelper::PointerGestureRecognizer::makeGestureEvent (PointFRef pos, int eventType, int state)
{
	PointF where (pos.x, pos.y);
	Win32Window::cast (window)->screenPixelToClientCoord (where);

	GestureEvent event (eventType|state, where);
	event.keys = lastKeys;

	if(!didBegin && event.getType () >= GestureEvent::kSwipe && event.getType () <= GestureEvent::kLongPress)
	{
		if(state == GestureEvent::kChanged)
		{
			event.eventType = eventType | GestureEvent::kBegin;
			didBegin = true;
		}
		else if(state == GestureEvent::kBegin)
			didBegin = true;
	}
	return event;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchHelper::PointerGestureRecognizer::onInteractionOutput (const INTERACTION_CONTEXT_OUTPUT& output)
{
	switch(output.interactionId)
	{
	case INTERACTION_ID_MANIPULATION:
		{
			const MANIPULATION_TRANSFORM& cumulative = output.arguments.manipulation.cumulative;

			ManipulationData manipulation (cumulative);
			manipulation -= manipulationStart;

			PRINT_MANIPULATION ("Manipulation %s move: %f, %f, scale: %f, expansion: %f, rotate: %f (velocity: %f, %f)\n", getInteractionFlagName (output).str (),
				manipulation.translationX, manipulation.translationY, manipulation.scale, manipulation.expansion, manipulation.rotation,
				output.arguments.manipulation.velocity.velocityX, output.arguments.manipulation.velocity.velocityY)

			if(mustCheckSwitchGesture && activeGesture && activeGesture->isContinuous () && gestures.isMultiple ())
			{
				auto checkSwitchgesture = [&] (int oldType, int newType)
				{
					GestureInfo* newGesture = findGesture (newType);
					if(newGesture)
					{
						activeGesture = newGesture;

						// report start of new gesture (old will be ended by TouchInputState)
						manipulationStart = ManipulationData (cumulative);
						GestureEvent beginEvent (makeGestureEvent (output, newGesture->getType (), GestureEvent::kBegin));
						if(beginEvent.getType () == GestureEvent::kZoom)
							beginEvent.setPosition (window->getTouchInputState ().calculateTouchCenter (*static_cast<Gesture*> (newGesture)));

						CCL_PRINTF ("\nPointerGestureRecognizer: switch gesture %d (%d, %d)\n", beginEvent.getType (), beginEvent.where.x, beginEvent.where.y);
						window->getTouchInputState ().onGesture (beginEvent, *static_cast<Gesture*> (newGesture));
						return true;
					}
					return false;
				};

				mustCheckSwitchGesture = false;

				if(touchIds.count () == 2)
					if(checkSwitchgesture (GestureEvent::kSwipe, GestureEvent::kZoom))
						return;

				if(touchIds.count () == 1)
					if(checkSwitchgesture (GestureEvent::kZoom, GestureEvent::kSwipe))
						return;
			}

			if(GestureInfo* gesture = chooseManipulationGesture (manipulation.translationX, manipulation.translationY, manipulation.expansion, manipulation.rotation))
			{
				GestureEvent event (makeGestureEvent (output, gesture->getType ()));
				window->getTouchInputState ().onGesture (event, *static_cast<Gesture*> (gesture));
			}
		}
		break;

	case INTERACTION_ID_TAP:
		CCL_PRINTF ("Tap %s (count: %d)\n", getInteractionFlagName (output).str (), output.arguments.tap.count)
		if(output.arguments.tap.count == 1)
		{
			// first tap: check if also interested in double tap
			GestureEvent singleTapEvent (makeGestureEvent (output, GestureEvent::kSingleTap));
			GestureInfo* doubleTap = findGesture (GestureEvent::kDoubleTap);

			// if doubleTap gesture has 2 taps already but was not detected, we have to deliver 2 single taps now: the pending one first, the second as normal single tap (below)
			if(doubleTap && doubleTap->canRemove () && isTimerEnabled ())
				checkPendingTap ();

			if(doubleTap && !doubleTap->canRemove ())
			{
				// start timer for delayed delivery of this event, if no second tap appears
				pendingSingleTap = singleTapEvent;
				startTimer (kDoubleClickTimeout, false);
			}
			else if(GestureInfo* gesture = chooseGesture (GestureEvent::kSingleTap))
			{
				window->getTouchInputState ().onGesture (singleTapEvent, *static_cast<Gesture*> (gesture));
				stopTimer ();
			}
		}
		else if(output.arguments.tap.count == 2)
		{
			stopTimer ();
			if(GestureInfo* gesture = chooseGesture (GestureEvent::kDoubleTap))
			{
				GestureEvent event (makeGestureEvent (output, GestureEvent::kDoubleTap));
				window->getTouchInputState ().onGesture (event, *static_cast<Gesture*> (gesture));
			}
		}
		break;

	case INTERACTION_ID_SECONDARY_TAP:
		CCL_PRINTF ("Secondary Tap %s (count: %d)\n", getInteractionFlagName (output).str (), output.arguments.tap.count)
		break;

	case INTERACTION_ID_HOLD:
		CCL_PRINTF ("Hold %s\n", getInteractionFlagName (output).str ())
		if(GestureInfo* gesture = chooseGesture (GestureEvent::kLongPress))
		{
			if(didBegin && prolongedLongPressTouchID == gesture->getTouchIDs ().first ())
				break;

			if(output.interactionFlags & INTERACTION_FLAG_END)
				prolongedLongPressTouchID = gesture->getTouchIDs ().first (); // the "hold" gesture ends when moved out of a small tolerance area, we want our long press to continue until the touch ends
			else
				#if DEFER_LONGPRESS // (avoid starting a DragSession from this callback)
				window->getTouchInputState ().deferGesture (makeGestureEvent (output, GestureEvent::kLongPress), *static_cast<Gesture*> (gesture));
				#else
				window->getTouchInputState ().onGesture (makeGestureEvent (output, GestureEvent::kLongPress), *static_cast<Gesture*> (gesture));
				#endif
		}
		break;

	case INTERACTION_ID_DRAG:
		CCL_PRINTF ("Drag %s\n", getInteractionFlagName (output).str ())
		break;

	default:
		break;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchHelper::PointerGestureRecognizer::checkPendingTap ()
{
	// deliver single tap event if no second tap detected during double tap timeout
	if(GestureInfo* gesture = chooseGesture (GestureEvent::kSingleTap))
	{
		CCL_PRINTLN ("deliver pending single tap")
		window->getTouchInputState ().onGesture (pendingSingleTap, *static_cast<Gesture*> (gesture));
	}

	stopTimer ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchHelper::PointerGestureRecognizer::onIdleTimer ()
{
	checkPendingTap ();
}
