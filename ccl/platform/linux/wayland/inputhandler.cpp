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
// Filename    : ccl/platform/linux/wayland/inputhandler.cpp
// Description : Wayland Input Handling
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/linux/wayland/inputhandler.h"
#include "ccl/platform/linux/wayland/surface.h"
#include "ccl/platform/linux/interfaces/iinputlocale.h"

#include "ccl/public/system/ilocalemanager.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/text/cclstring.h"

#include <sys/mman.h>
#include <unistd.h>
#include <linux/input.h>

#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-names.h>
#include <xkbcommon/xkbcommon-compose.h>

using namespace CCL;
using namespace Linux;

//************************************************************************************************
// InputHandler
//************************************************************************************************

DEFINE_SINGLETON (InputHandler)

//////////////////////////////////////////////////////////////////////////////////////////////////

InputHandler::InputHandler ()
: keyboard (nullptr),
  pointer (nullptr),
  touch (nullptr),
  serial (0),
  repeatDelay (0),
  repeatRate (1),
  savedFocus (nullptr),
  mouseButtonDown (false),
  relativePointer (nullptr),
  confinedPointer (nullptr),
  grabbingSurface (nullptr)
{
	pressedKeys.setCount (pressedKeys.getCapacity ());
	pressedKeys.zeroFill ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void InputHandler::terminate ()
{
	stopTimer ();

	while(EventItem* event = collectedEvents.removeFirst ())
		delete event;
		
	if(getKeyboard ())
	{
		wl_keyboard_release (getKeyboard ());
		setKeyboard (nullptr);
	}
	if(getRelativePointer ())
	{
		zwp_relative_pointer_v1_destroy (getRelativePointer ());
		setRelativePointer (nullptr);
	}
	if(getPointer ())
	{
		wl_pointer_destroy (getPointer ());
		setPointer (nullptr);
	}
	if(getTouch ())
	{
		wl_touch_destroy (getTouch ());
		setTouch (nullptr);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool InputHandler::isInitialized () const
{
	return getKeyboard () != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void InputHandler::initialize ()
{
	WaylandClient& client = WaylandClient::instance ();

	// keyboard
	if(client.hasKeyboardInput ())
 	{
 		setKeyboard (wl_seat_get_keyboard (client.getSeat ()));
 		if(getKeyboard ())
 			wl_keyboard_add_listener (getKeyboard (), &listener, &listener);
 	}
 	else if(getKeyboard ())
 	{
 		wl_keyboard_release (getKeyboard ());
 		setKeyboard (nullptr);
 	}
 	
 	// mouse
 	if(client.hasPointerInput ())
	{
		setPointer (wl_seat_get_pointer (client.getSeat ()));
		if(getPointer ())
		{
			wl_pointer_add_listener (getPointer (), &listener, &listener);

			zwp_relative_pointer_manager_v1* relativePointerManager = client.getRelativePointerManager ();
			if(relativePointerManager)
				setRelativePointer (zwp_relative_pointer_manager_v1_get_relative_pointer (relativePointerManager, getPointer ()));
			if(getRelativePointer ())
				zwp_relative_pointer_v1_add_listener (getRelativePointer (), &listener, &listener);
		}
	}
	else if(getPointer ())
	{
		if(getRelativePointer ())
			zwp_relative_pointer_v1_destroy (getRelativePointer ());
		setRelativePointer (nullptr);
		wl_pointer_release (getPointer ());
		setPointer (nullptr);
	}

	// touch
	if(client.hasTouchInput ())
	{
		setTouch (wl_seat_get_touch (client.getSeat ()));
		if(getTouch ())
			wl_touch_add_listener (getTouch (), &listener, &listener);
	}
	else if (getTouch ())
	{
		wl_touch_release (getTouch ());
		setTouch (nullptr);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void InputHandler::registerSurface (Surface& surface)
{
	if(!listeners.contains (&surface))
		listeners.add (&surface);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void InputHandler::unregisterSurface (Surface& surface)
{
	if(&surface == grabbingSurface)
		grabPointer (surface, false);
	listeners.remove (&surface);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool InputHandler::grabPointer (Surface& surface, bool state, PointRef initialPosition)
{
	if(state == true)
	{
		ASSERT (listeners.contains (&surface))
		if(!listeners.contains (&surface))
			return false;

		if(relativePointer == nullptr)
			return false;

		grabbingSurface = &surface;
		savedPosition.x = initialPosition.x;
		savedPosition.y = initialPosition.y;

		return true;
	}
	else
	{
		if(confinedPointer)
			zwp_confined_pointer_v1_destroy (confinedPointer);
		confinedPointer = nullptr;
		grabbingSurface = nullptr;

		return true;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void InputHandler::flushEvents ()
{
	auto sendEvent = [this] (const EventItem* event, Surface* surface) 
	{
		if(surface == nullptr || surface->suppressInput ())
			return;
		
		switch(event->type)
		{
		case EventItem::kKeyboardEvent :
			surface->handleKeyboardEvent (event->keyboardEvent);
			break;
		case EventItem::kPointerEvent :
			mouseButtonDown = get_flag<uint32_t> (event->pointerEvent.buttonState, KeyState::kLButton);
			surface->handlePointerEvent (event->pointerEvent);
			break;
		case EventItem::kTouchEvent :
			surface->handleTouchEvent (event->touchEvent);
			break;
		}
	};
	
	while(EventItem* event = collectedEvents.removeFirst ())
	{
		if(event->surface == nullptr)
		{
			for(Surface* surface : listeners)
				sendEvent (event, surface);
		}
		else if(listeners.contains (event->surface))
		{
			sendEvent (event, event->surface);
		}
		delete event;
	}

	listener.discardStaleTouchStatuses ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void InputHandler::onKeyEvent (const KeyEvent& event, wl_surface* focus)
{
	savedFocus = nullptr;
	for(Surface* surface : listeners)
	{
		if(focus == nullptr || surface->getWaylandSurface () == focus)
		{
			if(event.eventType == KeyEvent::kKeyDown)
			{
				savedEvent = event;
				set_flag<uint32> (savedEvent.state.keys, KeyState::kRepeat);
				savedFocus = focus;
				if(!isTimerEnabled () && repeatRate > 0)
					startTimer (repeatDelay, true);
				
				if(event.vKey >= 0 && event.vKey < pressedKeys.count ())
					pressedKeys[event.vKey] = true;
				
				collectedEvents.append (NEW EventItem (event, surface));
			}
			else
			{
				stopTimer ();
				if(event.vKey >= 0 && event.vKey < pressedKeys.count ())
					pressedKeys[event.vKey] = false;
				
				collectedEvents.append (NEW EventItem (event, surface));
			}
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void InputHandler::onFocusChanged (wl_surface* previousFocus, wl_surface* focus)
{
	stopTimer ();
	for(Surface* surface : listeners)
	{
		if(surface->getWaylandSurface () == previousFocus)
		{
			surface->handleFocus (FocusEvent::kKillFocus);
		}
		if(surface->getWaylandSurface () == focus)
		{
			surface->handleFocus (FocusEvent::kSetFocus);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void InputHandler::getActiveModifierKeys (KeyState& keyState)
{
	listener.getActiveModifierKeys (keyState);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool InputHandler::isKeyPressed (VirtualKey key) const
{
	if(key >= 0 && key < pressedKeys.count ())
		return pressedKeys[key];
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void InputHandler::onPointerEvent (const PointerEvent& event)
{
	if(grabbingSurface != nullptr)
	{
		savedPosition.x += wl_fixed_to_double (event.dx);
		savedPosition.y += wl_fixed_to_double (event.dy);

		EventItem* lastItem = collectedEvents.getLast ();
		if(lastItem && lastItem->type == EventItem::kPointerEvent 
			&& lastItem->surface == grabbingSurface && lastItem->pointerEvent.eventMask == event.eventMask)
		{
			lastItem->pointerEvent.x = wl_fixed_from_double (savedPosition.x);
			lastItem->pointerEvent.y = wl_fixed_from_double (savedPosition.y);
		}
		else
		{
			EventItem* item = NEW EventItem (event, grabbingSurface);
			item->pointerEvent.x = wl_fixed_from_double (savedPosition.x);
			item->pointerEvent.y = wl_fixed_from_double (savedPosition.y);
			item->pointerEvent.focus = grabbingSurface->getWaylandSurface ();
			set_flag<uint32_t> (item->pointerEvent.eventMask, kPointerEnter, false);
			set_flag<uint32_t> (item->pointerEvent.eventMask, kPointerLeave, false);
			collectedEvents.append (item);
		}

		return;
	}
	
	for(Surface* surface : listeners)
	{
		if((event.focus == surface->getWaylandSurface () && surface->getWaylandSurface () != nullptr)
				|| (event.oldSurface == surface->getWaylandSurface () && surface->getWaylandSurface () != nullptr)
				|| (event.focus == nullptr && event.oldSurface == nullptr))
			collectedEvents.append (NEW EventItem (event, surface));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void InputHandler::onTouchEvent (const TouchEvent& event)
{
	for(Surface* surface : listeners)
	{
		if(event.focus == surface->getWaylandSurface ())
		{
			collectedEvents.append (NEW EventItem (event, surface));
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void InputHandler::onIdleTimer ()
{
	if(savedFocus)
	{
		int64 expectedDelay = -1;
		if(repeatRate > 0)
			expectedDelay = 1000 / repeatRate;
		if(delay != expectedDelay)
		{
			stopTimer ();
			if(repeatRate > 0)
				startTimer (expectedDelay, true);
		}
		else
			onKeyEvent (savedEvent, savedFocus);
	}
	else
		stopTimer ();
}

//************************************************************************************************
// InputHandler::Listener
//************************************************************************************************

InputHandler::Listener::Listener ()
: composeTable (nullptr),
  composeState (nullptr)
{
	wl_keyboard_listener::enter = onKeyboardFocusEnter;
	wl_keyboard_listener::leave = onKeyboardFocusLeave;
	wl_keyboard_listener::keymap = onKeymapReceived;
	wl_keyboard_listener::key = onKey;
	wl_keyboard_listener::modifiers = onModifiers;
	wl_keyboard_listener::repeat_info = onRepeatInfo;

	wl_pointer_listener::enter = onPointerEnter;
	wl_pointer_listener::leave = onPointerLeave;
	wl_pointer_listener::motion = onPointerMotion;
	wl_pointer_listener::button = onPointerButton;
	wl_pointer_listener::axis = onPointerAxis;
	wl_pointer_listener::axis_source = onPointerAxisSource;
	wl_pointer_listener::axis_stop = onPointerAxisStop;
	wl_pointer_listener::axis_discrete = onPointerAxisDiscrete;
	#ifdef WL_POINTER_AXIS_VALUE120_SINCE_VERSION
	wl_pointer_listener::axis_value120 = onPointerAxis120;
	#endif
	#ifdef WL_POINTER_AXIS_RELATIVE_DIRECTION_SINCE_VERSION
	wl_pointer_listener::axis_relative_direction = onPointerAxisRelativeDirection;
	#endif
	wl_pointer_listener::frame = onPointerFrame;

	wl_touch_listener::down = onTouchDown;
	wl_touch_listener::up = onTouchUp;
	wl_touch_listener::motion = onTouchMotion;
	wl_touch_listener::cancel = onTouchCancel;
	wl_touch_listener::shape = onTouchShape;
	wl_touch_listener::orientation = onTouchOrientation;
	wl_touch_listener::frame = onTouchFrame;

	zwp_relative_pointer_v1_listener::relative_motion = onRelativeMotion;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

InputHandler::Listener::~Listener ()
{
	if(keyboardEvent.state)
		xkb_state_unref (keyboardEvent.state);
	if(keyboardEvent.keymap)
		xkb_keymap_unref (keyboardEvent.keymap);
	if(keyboardEvent.context)
		xkb_context_unref (keyboardEvent.context);
	if(composeState)
		xkb_compose_state_unref (composeState);
	if(composeTable)
		xkb_compose_table_unref (composeTable);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void InputHandler::Listener::onKeymapReceived (void* data, wl_keyboard* keyboard, uint32_t format, int32_t fd, uint32_t size)
{
	ASSERT (format == WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1)
	if(format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1)
		return;

	Listener* This = static_cast<Listener*> (data);
	
	if(keyboard != InputHandler::instance ().getKeyboard ())
		return;
	
	char* map = static_cast<char*> (::mmap (nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0));
	ASSERT (map != MAP_FAILED)
	if(map == MAP_FAILED || map == nullptr)
		return;
	
	if(This->keyboardEvent.context == nullptr)
		This->keyboardEvent.context = xkb_context_new (XKB_CONTEXT_NO_FLAGS);
	
	if(This->keyboardEvent.keymap)
		xkb_keymap_unref (This->keyboardEvent.keymap);
	This->keyboardEvent.keymap = xkb_keymap_new_from_string (This->keyboardEvent.context, map, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
	
	if(This->keyboardEvent.state)
		xkb_state_unref (This->keyboardEvent.state);
	This->keyboardEvent.state = xkb_state_new (This->keyboardEvent.keymap);
	
	::munmap (map, size);
	::close (fd);
	
	UnknownPtr<IInputLocale> inputLocale (&System::GetLocaleManager ());
	if(inputLocale)
	{
		inputLocale->setKeyMap (This->keyboardEvent.keymap);
	
		CStringPtr locale = inputLocale->getInputLocale ();
		if(locale)
		{
			if(This->composeTable)
				xkb_compose_table_unref (This->composeTable);
			This->composeTable = xkb_compose_table_new_from_locale (This->keyboardEvent.context, locale, XKB_COMPOSE_COMPILE_NO_FLAGS);

			if(This->composeState)
				xkb_compose_state_unref (This->composeState);
			This->composeState = xkb_compose_state_new (This->composeTable, XKB_COMPOSE_STATE_NO_FLAGS);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void InputHandler::Listener::onKeyboardFocusEnter (void* data, wl_keyboard* keyboard, uint32_t serial, wl_surface* surface, struct wl_array* keys)
{
	Listener* This = static_cast<Listener*> (data);

	if(keyboard != InputHandler::instance ().getKeyboard ())
		return;
	
	wl_surface* previousFocus = This->keyboardEvent.focus;
	This->keyboardEvent.focus = surface;
	
	InputHandler::instance ().onFocusChanged (previousFocus, This->keyboardEvent.focus);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void InputHandler::Listener::onKeyboardFocusLeave (void* data, wl_keyboard* keyboard, uint32_t serial, wl_surface* surface)
{
	Listener* This = static_cast<Listener*> (data);
	
	if(keyboard != InputHandler::instance ().getKeyboard ())
		return;
	
	InputHandler::instance ().onFocusChanged (This->keyboardEvent.focus, nullptr);
	
	This->keyboardEvent.focus = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void InputHandler::Listener::onKey (void* data, wl_keyboard* keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
{
	Listener* This = static_cast<Listener*> (data);
	
	if(keyboard != InputHandler::instance ().getKeyboard ())
		return;
	
	if(This->keyboardEvent.state == nullptr)
		return;
	
	InputHandler::instance ().setSerial (serial);
	
	double now = time / 1000.;
	
	xkb_keysym_t keysym = xkb_state_key_get_one_sym (This->keyboardEvent.state, key + 8);
	
	VirtualKey vKey = VKey::fromSystemKey (keysym);
	KeyState keyState;
	This->getActiveModifierKeys (keyState);
	
	#ifdef WL_KEYBOARD_KEY_STATE_REPEATED_SINCE_VERSION
	if(state == WL_KEYBOARD_KEY_STATE_REPEATED)
		keyState.keys |= KeyState::kRepeat;
	#endif

	uchar composedCharacter = 0;
	if(This->composeState != nullptr && state == WL_KEYBOARD_KEY_STATE_PRESSED)
	{
		xkb_compose_state_feed (This->composeState, keysym);
		xkb_compose_status status = xkb_compose_state_get_status (This->composeState);
		switch(status)
		{
		case XKB_COMPOSE_COMPOSING :
			break;
		
		case XKB_COMPOSE_COMPOSED :
			{
				char buffer[8] = { 0 };
				int length = xkb_compose_state_get_utf8 (This->composeState, buffer, ARRAY_COUNT (buffer));
				xkb_compose_state_reset (This->composeState);
				String characterString;
				characterString.appendCString (Text::kUTF8, buffer, length);
				if(!characterString.isEmpty ())
					composedCharacter = characterString[0];
				else 
				{
					xkb_keysym_t keysym = xkb_compose_state_get_one_sym (This->composeState);
					composedCharacter = xkb_keysym_to_utf32 (keysym);
				}
			}
			break;
		case XKB_COMPOSE_CANCELLED :
			xkb_compose_state_reset (This->composeState);
			break;
		}
	}

	if(composedCharacter == 0)
		composedCharacter = xkb_state_key_get_utf32 (This->keyboardEvent.state, key + 8);
	
	uchar character;
	xkb_state* shiftState = xkb_state_new (This->keyboardEvent.keymap);
	if(xkb_state_mod_name_is_active (shiftState, XKB_MOD_NAME_SHIFT, XKB_STATE_MODS_EFFECTIVE))
		xkb_state_update_key (shiftState, KEY_LEFTSHIFT + 8, XKB_KEY_DOWN);
	character = xkb_state_key_get_utf32 (shiftState, key + 8);
	if(character > 0 && character < ' ')
	{
		// non-printable character, try again without modifiers
		character = xkb_keysym_to_utf32 (keysym);
	}
	if(composedCharacter == 0)
		composedCharacter = character;
	
	xkb_state_unref (shiftState);

	InputHandler::instance ().onKeyEvent (KeyEvent (state == WL_KEYBOARD_KEY_STATE_RELEASED ? KeyEvent::kKeyUp : KeyEvent::kKeyDown, vKey, character, composedCharacter, keyState, now), This->keyboardEvent.focus);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void InputHandler::Listener::onModifiers (void* data, wl_keyboard* keyboard, uint32_t serial, uint32_t depressedModifiers, uint32_t latchedModifiers, uint32_t lockedModifiers, uint32_t group)
{
	Listener* This = static_cast<Listener*> (data);
	
	if(keyboard != InputHandler::instance ().getKeyboard ())
		return;
	
	if(This->keyboardEvent.state == nullptr)
		return;
	
	xkb_state_update_mask (This->keyboardEvent.state, depressedModifiers, latchedModifiers, lockedModifiers, 0, 0, group);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void InputHandler::Listener::onRepeatInfo (void* data, wl_keyboard* keyboard, int32_t rate, int32_t delay)
{
	if(keyboard != InputHandler::instance ().getKeyboard ())
		return;

	InputHandler::instance ().setRepeatRate (rate);
	InputHandler::instance ().setRepeatDelay (delay);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void InputHandler::Listener::getActiveModifierKeys (KeyState& keyState)
{
	set_flag<uint32> (keyState.keys, KeyState::kModifierMask, false);
	
	if(keyboardEvent.state == nullptr)
		return;
	
	if(xkb_state_mod_name_is_active (keyboardEvent.state, XKB_MOD_NAME_SHIFT, XKB_STATE_MODS_EFFECTIVE))
		set_flag<uint32> (keyState.keys, KeyState::kShift);
	if(xkb_state_mod_name_is_active (keyboardEvent.state, XKB_MOD_NAME_ALT, XKB_STATE_MODS_EFFECTIVE))
		set_flag<uint32> (keyState.keys, KeyState::kOption);
	if(xkb_state_mod_name_is_active (keyboardEvent.state, XKB_MOD_NAME_CTRL, XKB_STATE_MODS_EFFECTIVE))
		set_flag<uint32> (keyState.keys, KeyState::kCommand);
	keyState.keys |= pointerEvent.buttonState;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void InputHandler::Listener::onPointerEnter (void* data, wl_pointer* pointer, uint32_t serial, wl_surface* surface, wl_fixed_t x, wl_fixed_t y)
{
	Listener* This = static_cast<Listener*> (data);

	if(pointer != InputHandler::instance ().getPointer ())
		return;
	
	set_flag<uint32_t> (This->pointerEvent.eventMask, kPointerEnter);
	This->pointerEvent.x = x;
	This->pointerEvent.y = y;
	This->pointerEvent.focus = surface;

	if(This->pointerEvent.oldSurface == This->pointerEvent.focus)
	{
		set_flag<uint32_t> (This->pointerEvent.eventMask, kPointerEnter, false);
		set_flag<uint32_t> (This->pointerEvent.eventMask, kPointerLeave, false);
	}
	
	WaylandClient::instance ().setEnterSerial (serial);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void InputHandler::Listener::onPointerLeave (void* data, wl_pointer* pointer, uint32_t serial, wl_surface* surface)
{
	Listener* This = static_cast<Listener*> (data);

	if(pointer != InputHandler::instance ().getPointer ())
		return;

	This->pointerEvent.oldSurface = surface;
	if(This->pointerEvent.buttonState == 0)
	{
		set_flag<uint32_t> (This->pointerEvent.eventMask, kPointerLeave);
		This->pointerEvent.focus = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void InputHandler::Listener::onPointerMotion (void* data, wl_pointer* pointer, uint32_t time, wl_fixed_t x, wl_fixed_t y)
{
	Listener* This = static_cast<Listener*> (data);

	if(pointer != InputHandler::instance ().getPointer () || This->pointerEvent.focus == nullptr)
		return;
	
	set_flag<uint32_t> (This->pointerEvent.eventMask, kPointerMotion);
	This->pointerEvent.time = time;
	This->pointerEvent.x = x;
	This->pointerEvent.y = y;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void InputHandler::Listener::onPointerButton (void* data, wl_pointer* pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state)
{
	Listener* This = static_cast<Listener*> (data);

	if(pointer != InputHandler::instance ().getPointer ())
		return;
	
	set_flag<uint32_t> (This->pointerEvent.eventMask, kPointerButton);
	This->pointerEvent.time = time;
	This->pointerEvent.button = button;
	This->pointerEvent.state = state;
	
	unsigned int flag = 0;
	switch(button)
	{
	case BTN_LEFT:
		flag = KeyState::kLButton;
		break;
	case BTN_MIDDLE:
		flag = KeyState::kMButton;
		break;
	case BTN_RIGHT:
		flag = KeyState::kRButton;
		break;
	}
	if(state == WL_POINTER_BUTTON_STATE_PRESSED)
	{
		This->pointerEvent.serial = serial;
		set_flag<uint32_t> (This->pointerEvent.buttonState, flag, true);
	}
	else
		set_flag<uint32_t> (This->pointerEvent.buttonState, flag, false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void InputHandler::Listener::onPointerAxis (void* data, wl_pointer* pointer, uint32_t time, uint32_t axis, wl_fixed_t value)
{
	Listener* This = static_cast<Listener*> (data);

	if(pointer != InputHandler::instance ().getPointer ())
		return;
	
	if(axis >= ARRAY_COUNT (This->pointerEvent.axes))
		return;
	
	set_flag<uint32_t> (This->pointerEvent.eventMask, kPointerAxis);
	This->pointerEvent.time = time;
	This->pointerEvent.axes[axis].value += value;
	This->pointerEvent.axes[axis].valid = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void InputHandler::Listener::onPointerAxisSource (void* data, wl_pointer* pointer, uint32_t axisSource)
{
	Listener* This = static_cast<Listener*> (data);

	if(pointer != InputHandler::instance ().getPointer ())
		return;
	
	set_flag<uint32_t> (This->pointerEvent.eventMask, kPointerAxisSource);
	This->pointerEvent.axisSource = axisSource;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void InputHandler::Listener::onPointerAxisStop (void* data, wl_pointer* pointer, uint32_t time, uint32_t axis)
{
	Listener* This = static_cast<Listener*> (data);

	if(pointer != InputHandler::instance ().getPointer ())
		return;
	
	if(axis >= ARRAY_COUNT (This->pointerEvent.axes))
		return;
	
	set_flag<uint32_t> (This->pointerEvent.eventMask, kPointerAxisStop);
	This->pointerEvent.time = time;
	This->pointerEvent.axes[axis].valid = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void InputHandler::Listener::onPointerAxisDiscrete (void* data, wl_pointer* pointer, uint32_t axis, int32_t discrete)
{
	onPointerAxis120 (data, pointer, axis, discrete * 120);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void InputHandler::Listener::onPointerAxis120 (void* data, wl_pointer* pointer, uint32_t axis, int32_t discrete)
{
	Listener* This = static_cast<Listener*> (data);

	if(pointer != InputHandler::instance ().getPointer ())
		return;
	
	if(axis >= ARRAY_COUNT (This->pointerEvent.axes))
		return;
	
	set_flag<uint32_t> (This->pointerEvent.eventMask, kPointerAxisDiscrete);
	This->pointerEvent.axes[axis].discrete += discrete;
	This->pointerEvent.axes[axis].valid = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void InputHandler::Listener::onPointerAxisRelativeDirection (void* data, wl_pointer* pointer, uint32_t axis, uint32_t direction)
{
	Listener* This = static_cast<Listener*> (data);

	if(pointer != InputHandler::instance ().getPointer ())
		return;
	
	if(axis >= ARRAY_COUNT (This->pointerEvent.axes))
		return;
	
	#ifdef WL_POINTER_AXIS_RELATIVE_DIRECTION_SINCE_VERSION
	This->pointerEvent.axes[axis].inverted = (direction == WL_POINTER_AXIS_RELATIVE_DIRECTION_INVERTED);
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void InputHandler::Listener::onPointerFrame (void* data, wl_pointer* pointer)
{
	Listener* This = static_cast<Listener*> (data);

	if(pointer != InputHandler::instance ().getPointer ())
		return;

	// When the application looses pointer focus, we expect no button to be pressed
	ASSERT (This->pointerEvent.oldSurface == nullptr || (This->pointerEvent.buttonState == 0 || This->pointerEvent.focus != nullptr))
	
	InputHandler::instance ().setSerial (This->pointerEvent.serial);

	PointerEvent event (This->pointerEvent);
	
	This->pointerEvent.eventMask = 0;
	This->pointerEvent.oldSurface = nullptr;
	for(int i = 0; i < ARRAY_COUNT (This->pointerEvent.axes); i++)
	{
		This->pointerEvent.axes[i].valid = false;
		This->pointerEvent.axes[i].value = 0;
		This->pointerEvent.axes[i].discrete = 0;
		This->pointerEvent.axes[i].inverted = false;
	}
	This->pointerEvent.time = 0;
	This->pointerEvent.dx = 0;
	This->pointerEvent.dy = 0;
		
	InputHandler::instance ().onPointerEvent (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void InputHandler::Listener::onTouchDown (void* data, wl_touch* touch, uint32_t serial, uint32_t time, wl_surface* surface, int32_t id, wl_fixed_t x, wl_fixed_t y)
{
	Listener* This = static_cast<Listener*> (data);

	if(touch != InputHandler::instance ().getTouch ())
		return;

	InputHandler::instance ().setSerial (serial);

	double lastUpdate = System::GetProfileTime ();
	auto touchStatus = This->touchStatuses.findIf ([id] (const InputHandler::TouchStatus& t) { return t.id == id; });
	if(!touchStatus)
	{
		if(This->touchStatuses.add (TouchStatus (id, lastUpdate, surface, x, y)))
			touchStatus = &This->touchStatuses.last ();
		else
			return;
	}
	else
	{
		touchStatus->x = x;
		touchStatus->y = y;
		touchStatus->lastUpdate = lastUpdate;
	}

	TouchEvent event;
	event.type = kTouchDown;
	event.time = time;
	event.id = id;
	event.x = x;
	event.y = y;
	event.focus = surface;

	InputHandler::instance ().onTouchEvent (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void InputHandler::Listener::onTouchUp (void* data, wl_touch* touch, uint32_t serial, uint32_t time, int32_t id)
{
	Listener* This = static_cast<Listener*> (data);

	if(touch != InputHandler::instance ().getTouch ())
		return;

	int touchStatusIdx = 0;
	for(; touchStatusIdx < This->touchStatuses.count (); touchStatusIdx++)
	{
		if(This->touchStatuses[touchStatusIdx].id == id)
			break;
	}
	if(touchStatusIdx >= This->touchStatuses.count ())
		return;

	auto& touchStatus = This->touchStatuses[touchStatusIdx];
	TouchEvent event;
	event.type = kTouchUp;
	event.time = time;
	event.id = id;
	event.serial = serial;
	event.focus = touchStatus.surface;
	event.x = touchStatus.x;
	event.y = touchStatus.y;
	This->touchStatuses.removeAt (touchStatusIdx);

	InputHandler::instance ().onTouchEvent (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void InputHandler::Listener::onTouchMotion (void* data, wl_touch* touch, uint32_t time, int32_t id, wl_fixed_t x, wl_fixed_t y)
{
	Listener* This = static_cast<Listener*> (data);

	if(touch != InputHandler::instance ().getTouch ())
		return;

	auto touchStatus = This->touchStatuses.findIf ([id] (const InputHandler::TouchStatus& t) { return t.id == id; });
	if(!touchStatus)
		return;

	touchStatus->x = x;
	touchStatus->y = y;
	touchStatus->lastUpdate = System::GetProfileTime ();

	TouchEvent event;
	event.type = kTouchMotion;
	event.time = time;
	event.focus = touchStatus->surface;
	event.id = id;
	event.x = x;
	event.y = y;
	InputHandler::instance ().onTouchEvent (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void InputHandler::Listener::onTouchCancel (void* data, wl_touch* touch)
{
	Listener* This = static_cast<Listener*> (data);

	if(touch != InputHandler::instance ().getTouch ())
		return;
	
	This->touchStatuses.removeAll ();

	TouchEvent event;
	event.type = kTouchCancel;
	InputHandler::instance ().onTouchEvent (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void InputHandler::Listener::onTouchShape (void* data, wl_touch* touch, int32_t id, wl_fixed_t major, wl_fixed_t minor)
{
	Listener* This = static_cast<Listener*> (data);

	if(touch != InputHandler::instance ().getTouch ())
		return;

	auto touchStatus = This->touchStatuses.findIf ([id] (const InputHandler::TouchStatus& t) { return t.id == id; });
	if(!touchStatus)
		return;
	touchStatus->lastUpdate = System::GetProfileTime ();

	TouchEvent event;
	event.type = kTouchShape;
	event.id = id;
	event.major = major;
	event.minor = minor;
	InputHandler::instance ().onTouchEvent (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void InputHandler::Listener::onTouchOrientation (void* data, wl_touch* touch, int32_t id, wl_fixed_t orientation)
{
	Listener* This = static_cast<Listener*> (data);

	if(touch != InputHandler::instance ().getTouch ())
		return;

	auto touchStatus = This->touchStatuses.findIf ([id] (const InputHandler::TouchStatus& t) { return t.id == id; });
	if(!touchStatus)
		return;
	touchStatus->lastUpdate = System::GetProfileTime ();

	TouchEvent event;
	event.type = kTouchOrientation;
	event.id = id;
	event.orientation = orientation;
	InputHandler::instance ().onTouchEvent (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void InputHandler::Listener::onTouchFrame (void* data, wl_touch* touch)
{
	Listener* This = static_cast<Listener*> (data);

	if(touch != InputHandler::instance ().getTouch ())
		return;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void InputHandler::Listener::discardStaleTouchStatuses ()
{
	touchStatuses.removeIf ([] (const TouchStatus& t) { return System::GetProfileTime () > t.lastUpdate + kTouchStatusDiscardThreshold; });
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void InputHandler::Listener::onRelativeMotion (void* data, zwp_relative_pointer_v1* pointer, uint32_t timeHigh, uint32_t timeLow, wl_fixed_t dx, wl_fixed_t dy, wl_fixed_t dxUnaccelerated, wl_fixed_t dyUnaccelerated)
{
	Listener* This = static_cast<Listener*> (data);

	if(pointer != InputHandler::instance ().getRelativePointer ())
		return;

	This->pointerEvent.time = timeHigh / 1000;
	This->pointerEvent.dx = dx;
	This->pointerEvent.dy = dy;
	set_flag<uint32_t> (This->pointerEvent.eventMask, kPointerMotion);
}
