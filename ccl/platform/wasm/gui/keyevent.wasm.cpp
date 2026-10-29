//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
//
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/platform/wasm/gui/keyevent.wasm.cpp
// Description : Platform-specific Key Codes
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/wasm/gui/keyevent.wasm.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/text/cclstring.h"

#include <emscripten/key_codes.h>
#include <emscripten/html5.h>

namespace CCL {
namespace Wasm {

//////////////////////////////////////////////////////////////////////////////////////////////////
// JS Helpers
//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (bool, getKeyInLayoutMap, (char* cString, int cStringSize, CStringPtr keyCode, bool capsLock), 
{
	try 
	{
		const code = UTF8ToString (keyCode);
		const result = CCL.findCharacterInLayoutMap (code, capsLock);
		if(result)
		{
			stringToUTF8 (result, cString, cStringSize);
			return true;
		}
	}
	catch(e) 
	{}
	return false;
});


} // namespace Wasm
} // namespace CCL

using namespace CCL;
using namespace Wasm;

//////////////////////////////////////////////////////////////////////////////////////////////////
// WebAssembly key mapping
//////////////////////////////////////////////////////////////////////////////////////////////////

VKey::KeyMapping VKey::keyMap[] =
{
	{DOM_PK_BACKSPACE,		kBackspace},
	{DOM_PK_TAB,			kTab},
	{DOM_PK_CAPS_LOCK,		kCapsLock},
	{DOM_PK_NUMPAD_ENTER,	kEnter},
	{DOM_PK_ENTER,			kReturn},

	{DOM_PK_SHIFT_LEFT,		kShift},
	{DOM_PK_SHIFT_RIGHT,	kShift},
	{DOM_PK_CONTROL_LEFT,	kCommand},
	{DOM_PK_CONTROL_RIGHT,	kCommand},
	{DOM_PK_ALT_LEFT,		kOption},
	{DOM_PK_ALT_RIGHT,		kOption},

	{DOM_PK_ESCAPE,			kEscape},
	{DOM_PK_SPACE,			kSpace},
	{DOM_PK_HOME,			kHome},
	{DOM_PK_END,			kEnd},

	{DOM_PK_ARROW_LEFT,		kLeft},
	{DOM_PK_ARROW_UP,		kUp},
	{DOM_PK_ARROW_RIGHT,	kRight},
	{DOM_PK_ARROW_DOWN,		kDown},

	{DOM_PK_PAGE_UP,		kPageUp},
	{DOM_PK_PAGE_DOWN,		kPageDown},

	{DOM_PK_INSERT,			kInsert},
	{DOM_PK_DELETE,			kDelete},

	{DOM_PK_NUMPAD_0,		kNumPad0},
	{DOM_PK_NUMPAD_1,		kNumPad1},
	{DOM_PK_NUMPAD_2,		kNumPad2},
	{DOM_PK_NUMPAD_3,		kNumPad3},
	{DOM_PK_NUMPAD_4,		kNumPad4},
	{DOM_PK_NUMPAD_5,		kNumPad5},
	{DOM_PK_NUMPAD_6,		kNumPad6},
	{DOM_PK_NUMPAD_7,		kNumPad7},
	{DOM_PK_NUMPAD_8,		kNumPad8},
	{DOM_PK_NUMPAD_9,		kNumPad9},

	{DOM_PK_NUMPAD_MULTIPLY,kMultiply},
	{DOM_PK_NUMPAD_ADD,		kAdd},
	{DOM_PK_NUMPAD_SUBTRACT,kSubtract},
	{DOM_PK_NUMPAD_DECIMAL,	kDecimal},
	{DOM_PK_NUMPAD_DIVIDE,	kDivide},

	{DOM_PK_F1,				kF1},
	{DOM_PK_F2,				kF2},
	{DOM_PK_F3,				kF3},
	{DOM_PK_F4,				kF4},
	{DOM_PK_F5,				kF5},
	{DOM_PK_F6,				kF6},
	{DOM_PK_F7,				kF7},
	{DOM_PK_F8,				kF8},
	{DOM_PK_F9,				kF9},
	{DOM_PK_F10,			kF10},
	{DOM_PK_F11,			kF11},
	{DOM_PK_F12,			kF12},
	{DOM_PK_F13,			kF13},
	{DOM_PK_F14,			kF14},
	{DOM_PK_F15,			kF15},
	{DOM_PK_F16,			kF16},
	{DOM_PK_F17,			kF17},
	{DOM_PK_F18,			kF18},
	{DOM_PK_F19,			kF19},
	{DOM_PK_F20,			kF20},
	{DOM_PK_F21,			kF21},
	{DOM_PK_F22,			kF22},
	{DOM_PK_F23,			kF23},
	{DOM_PK_F24,			kF24},

	{DOM_PK_AUDIO_VOLUME_MUTE,	kVolumeMute},
	{DOM_PK_AUDIO_VOLUME_UP,	kVolumeUp},
	{DOM_PK_AUDIO_VOLUME_DOWN, 	kVolumeDown},
	{DOM_PK_MEDIA_STOP, 		kStop},
	{DOM_PK_MEDIA_PLAY_PAUSE, 	kPlayPause}
};

int VKey::getKeyMappingSize ()
{
	return ARRAY_COUNT (keyMap);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

VirtualKey VKey::fromSystemKey (CStringRef code)
{
	return fromSystemKey (emscripten_compute_dom_pk_code (code));
}

/////////////////////////////////////////////////////////////////////////////////////////////////

uchar VKey::getKeyCharacter (CStringRef key, CStringRef code, bool capsLock)
{
	char result[8] = "";
	if(getKeyInLayoutMap (result, ARRAY_COUNT (result), code, capsLock))
	{
		CCL_PRINTF ("key in layout map: %s -> %s\n", code.str (), result)
		String unicodeResult;
		unicodeResult.appendCString (Text::kUTF8, result);
		return unicodeResult.firstChar ();
	}

	String character;
	if(code.startsWith ("Digit"))
		character = String (Text::kUTF8, code.subString (CString ("Digit").length ()).toLowercase ());
	else if(code.startsWith ("Key"))
		character = String (Text::kUTF8, code.subString (CString ("Key").length ()).toLowercase ());
	else if(code.startsWith ("Numpad"))
		character = String (Text::kUTF8, code.subString (CString ("Numpad").length ()).toLowercase ());
	else
		character = String (Text::kUTF8, key);

	if(character.length () > 1)
		character.empty ();

	if(capsLock)
		character.toUppercase ();

	return character.firstChar ();
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void VKey::fromSystemModifiers (KeyState& state, bool control, bool shift, bool alt, bool repeat, uint16 buttons)
{
	if(repeat)
		state.keys |= KeyState::kRepeat;
	if(control)
		state.keys |= KeyState::kCommand;
	if(shift)
		state.keys |= KeyState::kShift;
	if(alt)
		state.keys |= KeyState::kOption;

	if(buttons & (1 << 0))
		state.keys |= KeyState::kLButton;
	if(buttons & (1 << 1))
		state.keys |= KeyState::kRButton;
	if(buttons & (1 << 2))
		state.keys |= KeyState::kMButton;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void VKey::fromSystemEvent (KeyEvent& event, const EmscriptenKeyboardEvent& keyEvent, int eventType)
{
	event.eventType = eventType == EMSCRIPTEN_EVENT_KEYDOWN ? KeyEvent::kKeyDown : KeyEvent::kKeyUp;
	event.vKey = VKey::fromSystemKey (keyEvent.code);
	event.character = VKey::getKeyCharacter (keyEvent.key, keyEvent.code, keyEvent.shiftKey);
	event.eventTime = keyEvent.timestamp;
	fromSystemModifiers (event.state, keyEvent.ctrlKey, keyEvent.shiftKey, keyEvent.altKey, keyEvent.repeat);

	#if DEBUG
	String keyEventString;
	event.toString (keyEventString, true);
	keyEventString.append (", ").append (event.character);
	CCL_PRINTF ("Key Event: %s\n", MutableCString (keyEventString, Text::kUTF8).str ())
	#endif
}
