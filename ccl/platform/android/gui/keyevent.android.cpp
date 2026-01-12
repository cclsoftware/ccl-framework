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
// Filename    : ccl/platform/android/gui/keyevent.android.cpp
// Description : Platform-specific Key Code stuff
//
//************************************************************************************************

#include "ccl/gui/keyevent.h"
#include "ccl/public/text/cclstring.h"

#include <android/keycodes.h>

namespace CCL {
namespace VKey {

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Android
{
	// selected modifer flags from android.view.KeyEvent (java)
	static constexpr int META_SHIFT_ON	= 0x1;
	static constexpr int META_ALT_ON	= 0x02;
	static constexpr int META_CTRL_ON	= 0x1000;

	// flags in unicode character codes, from android.view.KeyCharacterMap (java)
	static constexpr int COMBINING_ACCENT		= 0x80000000;
	static constexpr int COMBINING_ACCENT_MASK	= 0x7FFFFFFF;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Android key mapping
//////////////////////////////////////////////////////////////////////////////////////////////////

KeyMapping keyMap[] =
{
	{AKEYCODE_DEL,			kBackspace},
	{AKEYCODE_TAB,			kTab},
	{AKEYCODE_CAPS_LOCK,	kCapsLock},
	{AKEYCODE_ENTER,		kEnter},
	// (no kReturn)

	{AKEYCODE_SHIFT_LEFT,	kShift},	// note: modifier keys have separate codes for left / right
	{AKEYCODE_SHIFT_RIGHT,	kShift},
	{AKEYCODE_ALT_LEFT,		kOption},
	{AKEYCODE_ALT_RIGHT,	kOption},
	{AKEYCODE_CTRL_LEFT,	kControl},
	{AKEYCODE_CTRL_RIGHT,	kControl},

	{AKEYCODE_ESCAPE,		kEscape},
	{AKEYCODE_SPACE,		kSpace},
	{AKEYCODE_MOVE_HOME,	kHome},
	{AKEYCODE_MOVE_END,		kEnd},

	{AKEYCODE_DPAD_LEFT,	kLeft},
	{AKEYCODE_DPAD_UP,		kUp},
	{AKEYCODE_DPAD_RIGHT,	kRight},
	{AKEYCODE_DPAD_DOWN,	kDown},

	{AKEYCODE_PAGE_UP,		kPageUp},
	{AKEYCODE_PAGE_DOWN,	kPageDown},

	{AKEYCODE_INSERT,		kInsert},
	{AKEYCODE_FORWARD_DEL,	kDelete},

	{AKEYCODE_NUMPAD_0,		kNumPad0},
	{AKEYCODE_NUMPAD_1,		kNumPad1},
	{AKEYCODE_NUMPAD_2,		kNumPad2},
	{AKEYCODE_NUMPAD_3,		kNumPad3},
	{AKEYCODE_NUMPAD_4,		kNumPad4},
	{AKEYCODE_NUMPAD_5,		kNumPad5},
	{AKEYCODE_NUMPAD_6,		kNumPad6},
	{AKEYCODE_NUMPAD_7,		kNumPad7},
	{AKEYCODE_NUMPAD_8,		kNumPad8},
	{AKEYCODE_NUMPAD_9,		kNumPad9},

	{AKEYCODE_NUMPAD_MULTIPLY,		kMultiply},
	{AKEYCODE_NUMPAD_ADD,			kAdd},
	{AKEYCODE_NUMPAD_SUBTRACT,		kSubtract},
	{AKEYCODE_NUMPAD_DOT,			kDecimal},
	{AKEYCODE_NUMPAD_DIVIDE,		kDivide},

	{AKEYCODE_F1,					kF1},
	{AKEYCODE_F2,					kF2},
	{AKEYCODE_F3,					kF3},
	{AKEYCODE_F4,					kF4},
	{AKEYCODE_F5,					kF5},
	{AKEYCODE_F6,					kF6},
	{AKEYCODE_F7,					kF7},
	{AKEYCODE_F8,					kF8},
	{AKEYCODE_F9,					kF9},
	{AKEYCODE_F10,					kF10},
	{AKEYCODE_F11,					kF11},
	{AKEYCODE_F12,					kF12},

	{AKEYCODE_VOLUME_MUTE,			kVolumeMute},
	{AKEYCODE_VOLUME_UP,			kVolumeUp},
	{AKEYCODE_VOLUME_DOWN,			kVolumeDown},
	{AKEYCODE_MEDIA_STOP,			kStop},
	{AKEYCODE_MEDIA_PLAY_PAUSE,		kPlayPause},
	{AKEYCODE_MEDIA_PAUSE,			kPause},
	{AKEYCODE_MEDIA_RECORD,			kRecord},
	{AKEYCODE_MEDIA_FAST_FORWARD,	kForward},
	{AKEYCODE_MEDIA_REWIND,			kRewind},
	{AKEYCODE_CHANNEL_UP,			kChannelUp},
	{AKEYCODE_CHANNEL_DOWN,			kChannelDown}
};

int getKeyMappingSize ()
{
	return ARRAY_COUNT (keyMap);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

constexpr KeyMapping deadKeys[] =
{
	{ 0x5e,     kCircumflex },
	{ 0xb4,		kAcute },
	{ 0x2cb,	kGrave }
};

//////////////////////////////////////////////////////////////////////////////////////////////////

bool tryDeadCharacter (KeyEvent& keyEvent, int character)
{
	for(auto& deadKey : deadKeys)
		if(character == deadKey.sysKey)
		{
			keyEvent.vKey = deadKey.vKey;
			keyEvent.state.keys &= ~KeyState::kShift;
			return true;
		}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void fromSystemModifiers (KeyState& keyState, unsigned int systemKeys)
{	
	keyState.keys = 0;

	if(systemKeys & Android::META_CTRL_ON)
		keyState.keys |= KeyState::kCommand;
	if(systemKeys & Android::META_SHIFT_ON)
		keyState.keys |= KeyState::kShift;
	if(systemKeys & Android::META_ALT_ON)
		keyState.keys |= KeyState::kOption;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void makeKeyEvent (KeyEvent& keyEvent, int keyCode, int character, int modifiers, bool isRepeat)
{
	fromSystemModifiers (keyEvent.state, modifiers);
	if(isRepeat)
		keyEvent.state.keys |= KeyState::kRepeat;

	keyEvent.vKey = fromSystemKey (keyCode);
	if(keyEvent.vKey == kUnknown)
		if(character & Android::COMBINING_ACCENT || character == 0x5e)
			tryDeadCharacter (keyEvent, character & Android::COMBINING_ACCENT_MASK);

	keyEvent.character = character > 0 ? character : 0;

	// TODO: copied from windows implementation, should be unified (different on ios/cocoa)
	// remove shift modifier on non-letter keys with character, but with some exceptions:
	if(keyEvent.character
		&& keyEvent.vKey != VKey::kTab // not for tab
		&& keyEvent.vKey != VKey::kBackspace // not for backspace
		&& keyEvent.vKey != VKey::kEnter
		&& keyEvent.vKey != VKey::kReturn
		&& (keyEvent.vKey < VKey::kNumPad0 || keyEvent.vKey > kDivide) // not on numPad
		&& !Unicode::isAlpha (keyEvent.character))
	{
		keyEvent.state.keys &= ~KeyState::kShift;
	}
}

} // namespace VKey
} // namespace CCL
