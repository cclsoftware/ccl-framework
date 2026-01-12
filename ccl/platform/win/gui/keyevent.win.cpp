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
// Filename    : ccl/platform/win/gui/keyevent.win.cpp
// Description : Platform-specific Key Code stuff
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/keyevent.h"
#include "ccl/gui/system/systemevent.h"
#include "ccl/public/text/cclstring.h"

#include "ccl/platform/win/cclwindows.h"
#include "ccl/platform/win/gui/windowhelper.h"

namespace CCL {
namespace VKey {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Windows key mapping
//////////////////////////////////////////////////////////////////////////////////////////////////

KeyMapping keyMap[] =
{
	{VK_BACK,		kBackspace},
	{VK_TAB,		kTab},
	{VK_CAPITAL,	kCapsLock},
	{VK_RETURN,		kReturn},

	{VK_SHIFT,		kShift},
	{VK_CONTROL,	kCommand},
	{VK_MENU,		kOption},

	{VK_ESCAPE,		kEscape},
	{VK_SPACE,		kSpace},
	{VK_HOME,		kHome},
	{VK_END,		kEnd},

	{VK_LEFT,		kLeft},
	{VK_UP,			kUp},
	{VK_RIGHT,		kRight},
	{VK_DOWN,		kDown},

	{VK_PRIOR,		kPageUp},
	{VK_NEXT,		kPageDown},

	{VK_INSERT,		kInsert},
	{VK_DELETE,		kDelete},

	{VK_NUMPAD0,	kNumPad0},
	{VK_NUMPAD1,	kNumPad1},
	{VK_NUMPAD2,	kNumPad2},
	{VK_NUMPAD3,	kNumPad3},
	{VK_NUMPAD4,	kNumPad4},
	{VK_NUMPAD5,	kNumPad5},
	{VK_NUMPAD6,	kNumPad6},
	{VK_NUMPAD7,	kNumPad7},
	{VK_NUMPAD8,	kNumPad8},
	{VK_NUMPAD9,	kNumPad9},

	{VK_MULTIPLY,	kMultiply},
	{VK_ADD,		kAdd},
	{VK_SUBTRACT,	kSubtract},
	{VK_DECIMAL,	kDecimal},
	{VK_DIVIDE,		kDivide},

	{VK_F1,			kF1},
	{VK_F2,			kF2},
	{VK_F3,			kF3},
	{VK_F4,			kF4},
	{VK_F5,			kF5},
	{VK_F6,			kF6},
	{VK_F7,			kF7},
	{VK_F8,			kF8},
	{VK_F9,			kF9},
	{VK_F10,		kF10},
	{VK_F11,		kF11},
	{VK_F12,		kF12},
	{VK_F13,		kF13},
	{VK_F14,		kF14},
	{VK_F15,		kF15},
	{VK_F16,		kF16},
	{VK_F17,		kF17},
	{VK_F18,		kF18},
	{VK_F19,		kF19},
	{VK_F20,		kF20},
	{VK_F21,		kF21},
	{VK_F22,		kF22},
	{VK_F23,		kF23},
	{VK_F24,		kF24}
};

int getKeyMappingSize ()
{
	return ARRAY_COUNT (keyMap);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

KeyMapping scanKeyMap[] =
{
	{0x0052,	kNumPad0},
	{0x004F,	kNumPad1},
	{0x0050,	kNumPad2},
	{0x0051,	kNumPad3},
	{0x004B,	kNumPad4},
	{0x004C,	kNumPad5},
	{0x004D,	kNumPad6},
	{0x0047,	kNumPad7},
	{0x0048,	kNumPad8},
	{0x0049,	kNumPad9},

	{0x011C,	kEnter},
	{0x0037,	kMultiply},
	{0x004E,	kAdd},
	{0x004A,	kSubtract},
	{0x0053,	kDecimal},
	{0x0135,	kDivide}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

VirtualKey fromScanCode (int sysKey)
{
	for(int i = 0; i < ARRAY_COUNT(scanKeyMap); i++)
		if(scanKeyMap[i].sysKey == sysKey)
			return scanKeyMap[i].vKey;
	return VKey::kUnknown;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

constexpr KeyMapping deadKeys[] =
{
	{ 0x5e, kCircumflex },
	{ 0x60, kGrave },
	{ 0xb4, kAcute }
};

//////////////////////////////////////////////////////////////////////////////////////////////////

VirtualKey fromDeadCharacter (uchar character)
{
	for(auto& deadKey : deadKeys)
		if(character == deadKey.sysKey)
			return deadKey.vKey;

	return VKey::kUnknown;
}

//************************************************************************************************
// KeyState
//************************************************************************************************

void fromSystemModifiers (KeyState& keyState, unsigned int systemKeys)
{
	keyState.keys = 0;

	#define MAP_KEY(SysMask, LibMask) \
	if(systemKeys & SysMask) keyState.keys |= KeyState::LibMask;

	MAP_KEY (MK_LBUTTON, kLButton)
	MAP_KEY (MK_MBUTTON, kMButton)
	MAP_KEY (MK_RBUTTON, kRButton)
	MAP_KEY (MK_SHIFT,   kShift)
	MAP_KEY (MK_CONTROL, kCommand)

	#undef MAP_KEY

	if(::GetKeyState (VK_MENU) < 0)
		keyState.keys |= KeyState::kOption;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void toSystemModifiers (KeyState keyState, unsigned int& systemKeys)
{
	systemKeys = 0;

	#define MAP_KEY(SysMask, LibMask) \
	if(keyState.keys & KeyState::LibMask) systemKeys |= SysMask;

	MAP_KEY (MK_LBUTTON, kLButton)
	MAP_KEY (MK_MBUTTON, kMButton)
	MAP_KEY (MK_RBUTTON, kRButton)
	MAP_KEY (MK_SHIFT,   kShift)
	MAP_KEY (MK_CONTROL, kCommand)

	#undef MAP_KEY
}

//************************************************************************************************
// KeyEvent
//************************************************************************************************

void fromSystemEvent (KeyEvent& keyEvent, const SystemEvent& systemEvent)
{
	// Modifiers
	keyEvent.state.keys = 0;
	if(GetKeyState (VK_SHIFT) < 0)
		keyEvent.state.keys |= KeyState::kShift;
	if(GetKeyState (VK_CONTROL) < 0)
		keyEvent.state.keys |= KeyState::kCommand;
	if(GetKeyState (VK_MENU) < 0)
		keyEvent.state.keys |= KeyState::kOption;

	BYTE keyState[256] = {0};
	GetKeyboardState (keyState);

	if(HIWORD(systemEvent.lParam) & KF_REPEAT)
		keyEvent.state.keys |= KeyState::kRepeat;

	// Virtual key
	bool isKeyUp = systemEvent.msg == WM_KEYUP || systemEvent.msg == WM_SYSKEYUP;
	keyEvent.eventType = isKeyUp ? KeyEvent::kKeyUp : KeyEvent::kKeyDown;
	keyEvent.vKey = fromScanCode ((IntPtr)systemEvent.lParam >> 16);
	if(keyEvent.vKey == VKey::kUnknown)
		keyEvent.vKey = fromSystemKey ((IntPtr)systemEvent.wParam);

	// Composed character code

	UINT keyCode = (UIntPtr)systemEvent.wParam;
	UINT scanCode = ((UIntPtr)systemEvent.lParam & 0xFF0000) >> 16;
	uchar uniChar[2] = {0};

	// flush internal state of ToUnicode
	BYTE nullKeyState[256] = {0};
	::ToUnicode (VK_SPACE, ::MapVirtualKey (VK_SPACE, MAPVK_VK_TO_VSC), nullKeyState, uniChar, ARRAY_COUNT (uniChar), 0);

	if(!isKeyUp)
	{
		static UINT lastKeyCode = 0;
		static UINT lastScanCode = 0;
		static BYTE lastKeyState[256] = {0};
		static bool lastWasDead = false;

		bool isDead = false;

		bool isModifier = keyEvent.vKey == VKey::kShift
			|| keyEvent.vKey == VKey::kCommand
			|| keyEvent.vKey == VKey::kOption
			|| keyEvent.vKey == VKey::kControl;

		if(!isModifier && lastKeyCode != 0 && lastWasDead)
		{
			// reinject dead keys for proper dead key handling
			::ToUnicode (lastKeyCode, lastScanCode, lastKeyState, uniChar, ARRAY_COUNT (uniChar), 0);
			lastKeyCode = 0;
		}

		int result = ::ToUnicode (keyCode, scanCode, keyState, uniChar, ARRAY_COUNT (uniChar), 0);
		if(result > 1)
		{
			// uniChar is a dead key followed by either a dead key or a printable character. Call ToUnicode again to get the second character.
			result = ::ToUnicode (keyCode, scanCode, keyState, uniChar, ARRAY_COUNT (uniChar), 0);
		}

		if(result == -1)
		{
			// This was a dead key. Flush the internal state of ToUnicode and handle the dead key later.
			isDead = true;
			::ToUnicode (VK_SPACE, ::MapVirtualKey (VK_SPACE, MAPVK_VK_TO_VSC), nullKeyState, uniChar, ARRAY_COUNT (uniChar), 0);
		}

		if(result > 0 || (isDead && lastWasDead && result != 0))
		{
			keyEvent.composedCharacter = uniChar[0];
			lastWasDead = false;
		}
		else if(result != 0)
		{
			lastScanCode = scanCode;
			lastKeyCode = keyCode;
			lastWasDead = isDead;
			::memcpy (lastKeyState, keyState, sizeof(keyState));
		}
	}

	// Simple character code, ignoring all dead keys and all modifiers except 'shift'

	keyState[VK_CONTROL] = 0;    // We don't need special characters at this point
	keyState[VK_MENU] = 0;
	int result = ::ToUnicode (keyCode, scanCode, keyState, uniChar, ARRAY_COUNT (uniChar), 0);
	if(result != 0)
	{
		keyEvent.character = uniChar[0];

		// check for diacritics / dead keys:
		VirtualKey deadKey = fromDeadCharacter (uniChar[0]);
		if(deadKey != VKey::kUnknown)
		{
			keyEvent.vKey = deadKey;
			keyEvent.state.keys &= ~KeyState::kShift;

			if(Win32::GetWindowFromNativeHandle ((HWND)systemEvent.hwnd)) // skip second ::ToUnicode call in a window, but not in a native text control
				return;
		}
	}

	// remove shift modifier on non-letter keys with character, but with some exceptions:
	if(keyEvent.character
		&& keyEvent.vKey != VKey::kTab // not for tab
		&& keyEvent.vKey != VKey::kBackspace // not for backspace
		&& keyEvent.vKey != VKey::kEnter
		&& keyEvent.vKey != VKey::kReturn
		&& (keyEvent.vKey < VKey::kNumPad0 || keyEvent.vKey > kDivide) // not on numPad
		&& Unicode::isAlpha (keyEvent.character) == false)
	{
		keyEvent.state.keys &= ~KeyState::kShift;
	}

	CCL_PRINTF ("MSG   %08x %04x\n", systemEvent.lParam, systemEvent.wParam)
	CCL_PRINTF ("Event %2d %2d %02x \n", keyEvent.vKey, keyEvent.character, keyEvent.state.keys)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace VKey
} // namespace CCL
