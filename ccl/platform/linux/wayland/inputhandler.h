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
// Filename    : ccl/platform/linux/wayland/inputhandler.h
// Description : Wayland Input Handling
//
//************************************************************************************************

#ifndef _ccl_linux_inputhandler_h
#define _ccl_linux_inputhandler_h

#include "ccl/platform/linux/wayland/waylandclient.h"

#include "ccl/base/singleton.h"

#include "ccl/gui/keyevent.h"

#include "ccl/public/gui/framework/keycodes.h"
#include "ccl/public/gui/framework/idleclient.h"
#include "ccl/public/collections/vector.h"
#include "ccl/public/collections/linkedlist.h"

struct xkb_context;
struct xkb_state;
struct xkb_keymap;
struct xkb_compose_table;
struct xkb_compose_state;

namespace CCL {	
namespace Linux {
class Surface;

//************************************************************************************************
// InputHandler
//************************************************************************************************

class InputHandler: public Object,
					public IdleClient,
					public Singleton<InputHandler>
{
public:
	InputHandler ();

	struct KeyboardEvent
	{
		xkb_state* state;
		xkb_context* context;
		xkb_keymap* keymap;
		wl_surface* focus;
		
		KeyboardEvent ()
		: state (nullptr),
		  context (nullptr),
		  keymap (nullptr),
		  focus (nullptr)
		{}
	};
	
	enum PointerEventMask
	{
		kPointerEnter = 1 << 0,
		kPointerLeave = 1 << 1,
		kPointerMotion = 1 << 2,
		kPointerButton = 1 << 3,
		kPointerAxis = 1 << 4,
		kPointerAxisSource = 1 << 5,
		kPointerAxisStop = 1 << 6,
		kPointerAxisDiscrete = 1 << 7
	};
	
	struct PointerEvent
	{
		uint32_t eventMask;
		wl_fixed_t x;
		wl_fixed_t y;
		wl_fixed_t dx;
		wl_fixed_t dy;
		uint32_t button;
		uint32_t state;
		uint32_t time;
		uint32_t serial;
		struct Axis
		{
			wl_fixed_t value;
			int32_t discrete;
			bool inverted;
			bool valid;
		} axes[2];
		uint32_t axisSource;
		uint32_t buttonState;
		wl_surface* focus;
		wl_surface* oldSurface;
		
		PointerEvent ()
		: eventMask (0),
		  x (0),
		  y (0),
		  dx (0),
		  dy (0),
		  button (0),
		  state (0),
		  time (0),
		  serial (0),
		  axes {{0}},
		  axisSource (0),
		  buttonState (0),
		  focus (nullptr),
		  oldSurface (nullptr)
		{}
	};

	enum TouchEventType
	{
		kTouchDown = 1 << 0,
		kTouchUp = 1 << 1,
		kTouchMotion = 1 << 2,
		kTouchCancel = 1 << 3,
		kTouchShape = 1 << 4,
		kTouchOrientation = 1 << 5
	};

	struct TouchEvent
	{
		uint32_t type;
		uint32_t serial;
		uint32_t time;
		wl_fixed_t x;
		wl_fixed_t y;
		int32_t id;
		wl_surface* focus;
		wl_fixed_t major;
		wl_fixed_t minor;
		wl_fixed_t orientation;

		TouchEvent ()
		: type (0),
		  serial (0),
		  time (0),
		  x (0),
		  y (0),
		  id (0),
		  focus (nullptr),
		  major (0),
		  minor (0),
		  orientation (0)
		{}
	};

	struct TouchStatus
	{
		int32_t id;
		double lastUpdate;
		wl_surface* surface;
		wl_fixed_t x;
		wl_fixed_t y;

		TouchStatus (int32_t id = 0, double lastUpdate = 0, wl_surface* surface = nullptr, wl_fixed_t x = 0, wl_fixed_t y = 0)
		: id (id),
		  lastUpdate (lastUpdate),
		  surface (surface),
		  x (x),
		  y (y)
		{}
	};

	PROPERTY_POINTER (wl_keyboard, keyboard, Keyboard)
	PROPERTY_POINTER (wl_pointer, pointer, Pointer)
	PROPERTY_POINTER (wl_touch, touch, Touch)
	PROPERTY_POINTER (zwp_relative_pointer_v1, relativePointer, RelativePointer)
	PROPERTY_VARIABLE (uint32, serial, Serial)
	PROPERTY_VARIABLE (int32, repeatDelay, RepeatDelay)
	PROPERTY_VARIABLE (int32, repeatRate, RepeatRate)
	PROPERTY_POINTER (Surface, grabbingSurface, GrabbingSurface)
	
	PROPERTY_BOOL (mouseButtonDown, MouseButtonDown)
	
	bool isInitialized () const;
	void initialize ();
	void terminate ();
	
	void registerSurface (Linux::Surface& surface);
	void unregisterSurface (Linux::Surface& surface);
	
	void flushEvents ();

	bool grabPointer (Surface& surface, bool state, PointRef initialPosition = Point ());
	
	// keyboard
	void onKeyEvent (const KeyEvent& event, wl_surface* focus);
	void onFocusChanged (wl_surface* previousFocus, wl_surface* focus);
	void getActiveModifierKeys (KeyState& keyState);
	bool isKeyPressed (VirtualKey key) const;
	
	// mouse
	void onPointerEvent (const PointerEvent& event);

	// touch
	void onTouchEvent (const TouchEvent& event);
	
	// IdleClient
	void onIdleTimer () override;
	
	CLASS_INTERFACE (ITimerTask, Object)
	
protected:
	struct EventItem
	{
		enum Type
		{
			kKeyboardEvent,
			kPointerEvent,
			kTouchEvent
		};
		
		KeyEvent keyboardEvent;
		PointerEvent pointerEvent;
		TouchEvent touchEvent;
		Linux::Surface* surface;
		int type;
		
		EventItem (const KeyEvent& event, Linux::Surface* surface = nullptr)
		: keyboardEvent (event),
		  surface (surface),
		  type (kKeyboardEvent)
		{}
		
		EventItem (const Linux::InputHandler::PointerEvent& event, Linux::Surface* surface = nullptr)
		: pointerEvent (event),
		  surface (surface),
		  type (kPointerEvent)
		{}

		EventItem (const Linux::InputHandler::TouchEvent& event, Linux::Surface* surface = nullptr)
		: touchEvent (event),
		  surface (surface),
		  type (kTouchEvent)
		{}
	};
	LinkedList<EventItem*> collectedEvents;
	
	struct Listener: wl_keyboard_listener,
					 wl_pointer_listener,
					 wl_touch_listener,
					 zwp_relative_pointer_v1_listener
	{
		Listener ();
		~Listener ();
		
		void getActiveModifierKeys (KeyState& keyState);
		
		// keyboard input
		static void onKeymapReceived (void* data, wl_keyboard* keyboard, uint32_t format, int32_t fd, uint32_t size);
		static void onKeyboardFocusEnter (void* data, wl_keyboard* keyboard, uint32_t serial, wl_surface* surface, struct wl_array* keys);
		static void onKeyboardFocusLeave (void* data, wl_keyboard* keyboard, uint32_t serial, wl_surface* surface);
		static void onKey (void* data, wl_keyboard* keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state);
		static void onModifiers (void* data, wl_keyboard* keyboard, uint32_t serial, uint32_t depressedModifiers, uint32_t latchedModifiers, uint32_t lockedModifiers, uint32_t group);
		static void onRepeatInfo (void* data, wl_keyboard* keyboard, int32_t rate, int32_t delay);
		
		// pointer input 
		static void onPointerEnter (void* data, wl_pointer* pointer, uint32_t serial, wl_surface* surface, wl_fixed_t x, wl_fixed_t y);
		static void onPointerLeave (void* data, wl_pointer* pointer, uint32_t serial, wl_surface* surface);
		static void onPointerMotion (void* data, wl_pointer* pointer, uint32_t time, wl_fixed_t x, wl_fixed_t y);
		static void onPointerButton (void* data, wl_pointer* pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state);
		static void onPointerAxis (void* data, wl_pointer* wl_pointer, uint32_t time, uint32_t axis, wl_fixed_t value);
		static void onPointerAxisSource (void* data, wl_pointer* pointer, uint32_t axisSource);
		static void onPointerAxisStop (void* data, wl_pointer* pointer, uint32_t time, uint32_t axis);
		static void onPointerAxisDiscrete (void* data, wl_pointer* pointer, uint32_t axis, int32_t discrete);
		static void onPointerAxis120 (void* data, wl_pointer* pointer, uint32_t axis, int32_t discrete);
		static void onPointerAxisRelativeDirection (void* data, wl_pointer* pointer, uint32_t axis, uint32_t direction);
		static void onPointerFrame (void* data, wl_pointer* pointer);

		// touch input
		static void onTouchDown (void* data, wl_touch* touch, uint32_t serial, uint32_t time, wl_surface* surface, int32_t id, wl_fixed_t x, wl_fixed_t y);
		static void onTouchUp (void* data, wl_touch* touch, uint32_t serial, uint32_t time, int32_t id);
		static void onTouchMotion (void* data, wl_touch* touch, uint32_t time, int32_t id, wl_fixed_t x, wl_fixed_t y);
		static void onTouchCancel (void* data, wl_touch* touch);
		static void onTouchShape (void* data, wl_touch* touch, int32_t id, wl_fixed_t major, wl_fixed_t minor);
		static void onTouchOrientation (void* data, wl_touch* touch, int32_t id, wl_fixed_t orientation);
		static void onTouchFrame (void* data, wl_touch* touch);

		// relative pointer input
		static void onRelativeMotion (void* data, zwp_relative_pointer_v1* pointer, uint32_t timeHigh, uint32_t timeLow, wl_fixed_t dx, wl_fixed_t dy, wl_fixed_t dxUnaccelerated, wl_fixed_t dyUnaccelerated);

		void discardStaleTouchStatuses ();

	protected:
		static constexpr double kTouchStatusDiscardThreshold = 20;
		KeyboardEvent keyboardEvent;
		PointerEvent pointerEvent;

		Vector<TouchStatus> touchStatuses;
		
		xkb_compose_table* composeTable;
		xkb_compose_state* composeState;
	};
	Listener listener;
	
	KeyEvent savedEvent;
	wl_surface* savedFocus;
	FixedSizeVector<bool, VKey::kNumVirtualKeys> pressedKeys;
	Vector<Linux::Surface*> listeners;

	zwp_confined_pointer_v1* confinedPointer;
	TPoint<double> savedPosition;
};

} // namespace Linux
} // namespace CCL

#endif // _ccl_linux_inputhandler_h
