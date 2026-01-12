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
// Filename    : ccl/platform/cocoa/gui/keyevent.ios.mm
// Description : Platform-specific Key Code stuff
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/public/text/cclstring.h"
#include "ccl/gui/keyevent.h"
#include "ccl/gui/system/systemevent.h"

#include "ccl/platform/cocoa/cclcocoa.h"

#include "ccl/public/base/ccldefpush.h"

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////
// iOS key mapping
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace VKey {
KeyMapping keyMap[] =
{
	{UIKeyboardHIDUsageKeyboardDeleteOrBackspace, kBackspace},	// [BACKSPACE]
	{UIKeyboardHIDUsageKeyboardTab, kTab},	// [TAB]
	{UIKeyboardHIDUsageKeyboardCapsLock, kCapsLock},	// [CAPS LOCK]
	{UIKeyboardHIDUsageKeyboardReturnOrEnter, kReturn},	// [ENTER]
	{UIKeyboardHIDUsageKeyboardEscape, kEscape},	// [ESC]
	{UIKeyboardHIDUsageKeyboardSpacebar, kSpace},	// [SPACE]
	{UIKeyboardHIDUsageKeyboardHome, kHome},	// [HOME]
	{UIKeyboardHIDUsageKeyboardEnd, kEnd},	// [END]

	{UIKeyboardHIDUsageKeyboardLeftArrow, kLeft},	// [LEFT ARROW]
	{UIKeyboardHIDUsageKeyboardUpArrow, kUp},	// [UP ARROW]
	{UIKeyboardHIDUsageKeyboardRightArrow, kRight},	// [RIGHT ARROW]
	{UIKeyboardHIDUsageKeyboardDownArrow, kDown},	// [DOWN ARROW]

	{UIKeyboardHIDUsageKeyboardPageUp, kPageUp},	// [PAGE UP]
	{UIKeyboardHIDUsageKeyboardPageDown, kPageDown},	// [PAGE DOWN]

	{UIKeyboardHIDUsageKeyboardInsert, kInsert},	// [INS]
	{UIKeyboardHIDUsageKeyboardDeleteOrBackspace, kDelete},	// [DEL]

	{UIKeyboardHIDUsageKeypad0, kNumPad0},	// 0
	{UIKeyboardHIDUsageKeypad1, kNumPad1},	// 1
	{UIKeyboardHIDUsageKeypad2,	kNumPad2},	// 2
	{UIKeyboardHIDUsageKeypad3, kNumPad3},	// 3
	{UIKeyboardHIDUsageKeypad4,	kNumPad4},	// 4
	{UIKeyboardHIDUsageKeypad5, kNumPad5},	// 5
	{UIKeyboardHIDUsageKeypad6, kNumPad6},	// 6
	{UIKeyboardHIDUsageKeypad7,	kNumPad7},	// 7
	{UIKeyboardHIDUsageKeypad8,	kNumPad8},	// 8
	{UIKeyboardHIDUsageKeypad9,	kNumPad9},	// 9

	{UIKeyboardHIDUsageKeypadAsterisk, kMultiply},	// *
	{UIKeyboardHIDUsageKeypadPlus, kAdd},	// +
	{UIKeyboardHIDUsageKeypadHyphen, kSubtract},	// -
	{UIKeyboardHIDUsageKeypadComma, kDecimal},	// ,
	{UIKeyboardHIDUsageKeypadPeriod, kDecimal},	// ,
	{UIKeyboardHIDUsageKeypadSlash,	kDivide},	// /

	{UIKeyboardHIDUsageKeyboardF1, kF1},	// [F1]
	{UIKeyboardHIDUsageKeyboardF2, kF2},	// [F2]
	{UIKeyboardHIDUsageKeyboardF3, kF3},	// [F3]
	{UIKeyboardHIDUsageKeyboardF4, kF4},	// [F4]
	{UIKeyboardHIDUsageKeyboardF5, kF5},	// [F5]
	{UIKeyboardHIDUsageKeyboardF6, kF6},	// [F6]
	{UIKeyboardHIDUsageKeyboardF7, kF7},	// [F7]
	{UIKeyboardHIDUsageKeyboardF8, kF8},	// [F8]
	{UIKeyboardHIDUsageKeyboardF9, kF9},	// [F9]
	{UIKeyboardHIDUsageKeyboardF10, kF10},	// [F10]
	{UIKeyboardHIDUsageKeyboardF11, kF11},	// [F11]
	{UIKeyboardHIDUsageKeyboardF12, kF12},	// [F12]
	{UIKeyboardHIDUsageKeyboardF13, kF13},	// [F13]
	{UIKeyboardHIDUsageKeyboardF14, kF14},	// [F14]
	{UIKeyboardHIDUsageKeyboardF15, kF15},	// [F15]
	{UIKeyboardHIDUsageKeyboardF16, kF16},	// [F16]
	{UIKeyboardHIDUsageKeyboardF17, kF17},	// [F17]
	{UIKeyboardHIDUsageKeyboardF18, kF18},	// [F18]
	{UIKeyboardHIDUsageKeyboardF19, kF19},	// [F19]
	{UIKeyboardHIDUsageKeyboardF20, kF20},	// [F20]
	{UIKeyboardHIDUsageKeyboardF21, kF21},	// [F21]
	{UIKeyboardHIDUsageKeyboardF22, kF22},	// [F22]
	{UIKeyboardHIDUsageKeyboardF23, kF23},	// [F23]
	{UIKeyboardHIDUsageKeyboardF24, kF24}	// [F24]
};

enum CharacterCodes
{
	kCaret = 0x5E,
	kAccentGrave = 0x60,
	kAccentAcute = 0xB4
};

int getKeyMappingSize ()
{
	return ARRAY_COUNT (keyMap);
}

//************************************************************************************************
// KeyState
//************************************************************************************************

void fromSystemModifiers (KeyState& keyState, unsigned int systemKeys)
{	
	keyState.keys = 0;

	if(systemKeys & UIKeyModifierCommand)
		keyState.keys |= KeyState::kCommand;
	if(systemKeys & UIKeyModifierShift)
		keyState.keys |= KeyState::kShift;
	if(systemKeys & UIKeyModifierAlternate)
		keyState.keys |= KeyState::kOption;
	if(systemKeys & UIKeyModifierControl)
		keyState.keys |= KeyState::kControl;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void toSystemModifiers (KeyState keyState, unsigned int& systemKeys)
{
	systemKeys = 0;

	if(keyState.keys & KeyState::kCommand)
		systemKeys |= UIKeyModifierCommand;
	if(keyState.keys & KeyState::kShift)
		systemKeys |= UIKeyModifierShift;
	if(keyState.keys & KeyState::kOption)
		systemKeys |= UIKeyModifierAlternate;
	if(keyState.keys & KeyState::kControl)
		systemKeys |= UIKeyModifierControl;
}

//************************************************************************************************
// KeyEvent
//************************************************************************************************

void fromSystemEvent (KeyEvent& keyEvent, const SystemEvent& systemEvent)
{
	UIPress* press = (UIPress*)systemEvent.dataRef;
	UIKey* key = [press key];
	UIKeyModifierFlags modifiers = [key modifierFlags];
	UIKeyboardHIDUsage keyCode = [key keyCode];
	
	// special handling for caps lock pressed
	if(keyCode == UIKeyboardHIDUsageKeyboardCapsLock && modifiers & UIKeyModifierAlphaShift)
	{
		keyEvent.vKey = VKey::kCapsLock;
		keyEvent.eventType = KeyEvent::kKeyDown;
		return;
	}
	
	// Get character
	unichar charBuffer[10] = {0};
	NSString* chars = nil;
	if(modifiers & (UIKeyModifierCommand | UIKeyModifierControl | UIKeyModifierAlternate))
		chars = [key charactersIgnoringModifiers];
	else
		chars = [key characters];
	
	NSRange range = {0, 1};
	if([chars length] > 0)
		[chars getCharacters:charBuffer range:range];
	unsigned short charCode = charBuffer[0];
	
	CCL_PRINTF ("Keycode %x Charcode %x Modifiers %x\n", keyCode, charCode, modifiers)
	
	// Detect phase
	switch([press phase])
	{
		case UIPressPhaseBegan :
			keyEvent.eventType = KeyEvent::kKeyDown;
			break;
		case UIPressPhaseEnded :
			keyEvent.eventType = KeyEvent::kKeyUp;
			break;
	}
	
	// Read modifiers
	fromSystemModifiers (keyEvent.state, (unsigned int)modifiers);
	
	// Set virtual key
	keyEvent.vKey = VKey::kUnknown;
	
	// Caret
	// English (US): Keycode 23 + charcode 5E;
	// Latin America: Keycode 34 + charcode 7b + option (e6);
	// French: Keycode 2f + charcode 0 (Dead Key not possible due to ambiguity with spanish latin america accent acute); Keycode 2f + charcode 5E;
	// German:  Keycode 64 + charcode 0 (Dead Key); Keycode 64 + charcode 5E; Keycode 64 + 5e
	if((keyCode == UIKeyboardHIDUsageKeyboard6 && charCode == kCaret) ||
	   (keyCode == UIKeyboardHIDUsageKeyboardQuote && charCode == 0x7B && keyEvent.state.keys == KeyState::kOption) ||
	   (keyCode == UIKeyboardHIDUsageKeyboardOpenBracket && (charCode == kCaret)) ||
	   (keyCode == UIKeyboardHIDUsageKeyboardNonUSBackslash && (charCode == 0 || charCode == kCaret || charCode == kAccentAcute || charCode == kAccentGrave)))
	{
		charCode = kCaret;
		keyEvent.vKey = VKey::kCircumflex;
	}
	
	// Accent Grave
	// English (US): Keycode 35 + charcode 60;
	// Spanish Latin America:  Keycode 31 + charcode 7d + option (e6);
	// French: Keycode 31 + charcode 0 (Dead Key); Keycode 31 + charcode 60;
	// German: Keycode 2e + charcode 0 + shift (Dead Key); Keycode 2e + charcode 60 + shift;
	if((keyCode == UIKeyboardHIDUsageKeyboardGraveAccentAndTilde && charCode == kAccentGrave) ||
	   (keyCode == UIKeyboardHIDUsageKeyboardBackslash && (charCode == 0x7D && keyEvent.state.keys == KeyState::kOption ||
														   charCode == 0 || charCode == kAccentGrave || charCode == kAccentAcute || charCode == kCaret)) ||
	   (keyCode == UIKeyboardHIDUsageKeyboardEqualSign && ((charCode == 0 || charCode == kAccentGrave || charCode == kAccentAcute || charCode == kCaret) &&
														   keyEvent.state.keys == KeyState::kShift)))
	{
		charCode = kAccentGrave;
		keyEvent.vKey = VKey::kGrave;
	}
	
	// Accent Acute:
	// English (US): Keycode 30 + charcode 5d + option;
	// Spanish Latin America: Keycode 2f + charcode 0 (Dead Key not possible due to ambiguity with french caret); Keycode 2f + charcode b4;
	// French: Keycode 21 + charcode 27 + option;
	// German: Keycode 2e + charcode 0 (Dead Key); Keycode 2e + charcode b4;
	if((keyCode == UIKeyboardHIDUsageKeyboardCloseBracket && charCode == 0x5D && keyEvent.state.keys == KeyState::kOption) ||
	   (keyCode == UIKeyboardHIDUsageKeyboardOpenBracket && charCode == kAccentAcute) ||
	   (keyCode == UIKeyboardHIDUsageKeyboard4 && charCode == 0x27 && keyEvent.state.keys == KeyState::kOption) ||
	   (keyCode == UIKeyboardHIDUsageKeyboardEqualSign && (charCode == 0 || charCode == kAccentAcute || charCode == kAccentGrave || charCode == kCaret) &&
		keyEvent.state.keys != KeyState::kShift))
	{
		charCode = kAccentAcute;
		keyEvent.vKey = VKey::kAcute;
	}
	
	// Detect modifier
	switch(keyCode)
	{
	case UIKeyboardHIDUsageKeyboardLeftShift : keyEvent.vKey = VKey::kShift; break;
	case UIKeyboardHIDUsageKeyboardRightShift : keyEvent.vKey = VKey::kShift; break;
	case UIKeyboardHIDUsageKeyboardLeftGUI : keyEvent.vKey = VKey::kCommand; break;
	case UIKeyboardHIDUsageKeyboardRightGUI : keyEvent.vKey = VKey::kCommand; break;
	case UIKeyboardHIDUsageKeyboardLeftAlt :	keyEvent.vKey = VKey::kOption; break;
	case UIKeyboardHIDUsageKeyboardRightAlt : keyEvent.vKey = VKey::kOption; break;
	case UIKeyboardHIDUsageKeyboardLeftControl : keyEvent.vKey = VKey::kControl; break;
	case UIKeyboardHIDUsageKeyboardRightControl : keyEvent.vKey = VKey::kControl; break;
	}
	
	// Detect other keys
	if(keyEvent.vKey == VKey::kUnknown)
	{
		for(int i = 0; i < ARRAY_COUNT (VKey::keyMap); i++)
		{
			VKey::KeyMapping& mapping = VKey::keyMap[i];
			if(mapping.sysKey == keyCode)
			{
				keyEvent.vKey = mapping.vKey;
				break;
			}
		}
	}
	
	keyEvent.character = charCode;
	
	// Remove shift for certain combinations
	if((keyEvent.vKey == VKey::kUnknown ||
		keyEvent.vKey == VKey::kCircumflex ||
		keyEvent.vKey == VKey::kGrave ||
		keyEvent.vKey == VKey::kAcute) &&
	   Unicode::isAlpha (keyEvent.character) == false)
	{
		keyEvent.state.keys &= ~KeyState::kShift;
		keyEvent.state.keys &= ~KeyState::kOption;
	}
}

} // namespace VKey
} // namespace CCL
