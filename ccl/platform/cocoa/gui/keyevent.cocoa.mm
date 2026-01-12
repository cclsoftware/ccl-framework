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
// Filename    : ccl/platform/cocoa/gui/keyevent.cocoa.mm
// Description : Platform-specific Key Code stuff
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/public/text/cclstring.h"
#include "ccl/gui/keyevent.h"
#include "ccl/gui/system/systemevent.h"

#include "ccl/platform/cocoa/cclcocoa.h"
#include "ccl/platform/cocoa/cclcarbon.h"
#include "ccl/platform/cocoa/macutils.h"

#include <CoreServices/CoreServices.h>

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////
// macOS key mapping
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace VKey {
KeyMapping keyMap[] =
{
	{0x00330000,							kBackspace},				// [BACKSPACE]
	{NSTabCharacter,						kTab},						// [TAB]
	{0x00390000,							kCapsLock},					// [CAPS LOCK]
	{NSEnterCharacter,						kEnter},					// [ENTER]
	{NSCarriageReturnCharacter,				kReturn},
	{0x1b,									kEscape},					// [ESC]
	{0x20,									kSpace},					// [SPACE]
	{0xf729,								kHome},						// [HOME]
	{0xf72b,								kEnd},						// [END]

	{NSLeftArrowFunctionKey,				kLeft},						// [LEFT ARROW]
	{NSUpArrowFunctionKey,					kUp},						// [UP ARROW]
	{NSRightArrowFunctionKey,				kRight},					// [RIGHT ARROW]
	{NSDownArrowFunctionKey,				kDown},						// [DOWN ARROW]

	{NSPageUpFunctionKey,					kPageUp},					// [PAGE UP]
	{NSPageDownFunctionKey,					kPageDown},					// [PAGE DOWN]

	{NSInsertFunctionKey,					kInsert},					// [INS]
	{NSDeleteFunctionKey,					kDelete},					// [DEL]

	{'0' + 0x00520000,						kNumPad0},					// 0
	{'1' + 0x00530000,						kNumPad1},					// 1
	{'2' + 0x00540000,						kNumPad2},					// 2
	{'3' + 0x00550000,						kNumPad3},					// 3
	{'4' + 0x00560000,						kNumPad4},					// 4
	{'5' + 0x00570000,						kNumPad5},					// 5
	{'6' + 0x00580000,						kNumPad6},					// 6
	{'7' + 0x00590000,						kNumPad7}, 					// 7
	{'8' + 0x005B0000,						kNumPad8}, 					// 8
	{'9' + 0x005C0000,						kNumPad9},					// 9

	{'*' + 0x00430000,						kMultiply},					// *
	{'+' + 0x00450000,						kAdd},						// +
	{'-' + 0x004E0000,						kSubtract},					// -
	{',' + 0x00410000,						kDecimal},					// ,
	{'.' + 0x00410000,						kDecimal},					// ,
	{'/' + 0x004B0000,						kDivide},					// /

	{NSF1FunctionKey,						kF1},						// [F1]
	{NSF2FunctionKey,						kF2},						// [F2]
	{NSF3FunctionKey,						kF3},						// [F3]
	{NSF4FunctionKey,						kF4},						// [F4]
	{NSF5FunctionKey,						kF5},						// [F5]
	{NSF6FunctionKey,						kF6},						// [F6]
	{NSF7FunctionKey,						kF7},						// [F7]
	{NSF8FunctionKey,						kF8},						// [F8]
	{NSF9FunctionKey,						kF9},						// [F9]
	{NSF10FunctionKey,						kF10},						// [F10]
	{NSF11FunctionKey,						kF11},						// [F11]
	{NSF12FunctionKey,						kF12},						// [F12]
	{NSF13FunctionKey,						kF13},						// [F13]
	{NSF14FunctionKey,						kF14},						// [F14]
	{NSF15FunctionKey,						kF15},						// [F15]
	{NSF16FunctionKey,						kF16},						// [F16]
	{NSF17FunctionKey,						kF17},						// [F17]
	{NSF18FunctionKey,						kF18},						// [F18]
	{NSF19FunctionKey,						kF19},						// [F19]
	{NSF20FunctionKey,						kF20},						// [F20]
	{NSF21FunctionKey,						kF21},						// [F21]
	{NSF22FunctionKey,						kF22},						// [F22]
	{NSF23FunctionKey,						kF23},						// [F23]
	{NSF24FunctionKey,						kF24}						// [F24]
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
	if(systemKeys & NSEventModifierFlagCommand)
		keyState.keys |= KeyState::kCommand;
	if(systemKeys & NSEventModifierFlagShift)
		keyState.keys |= KeyState::kShift;
	if(systemKeys & NSEventModifierFlagOption)
		keyState.keys |= KeyState::kOption;
	if(systemKeys & NSEventModifierFlagControl)
		keyState.keys |= KeyState::kControl;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void toSystemModifiers (KeyState keyState, unsigned int& systemKeys)
{
	systemKeys = 0;
	
	if(keyState.keys & KeyState::kCommand)
		systemKeys |= NSEventModifierFlagCommand;
	if(keyState.keys & KeyState::kShift)
		systemKeys |= NSEventModifierFlagShift;
	if(keyState.keys & KeyState::kOption)
		systemKeys |= NSEventModifierFlagOption;
	if(keyState.keys & KeyState::kControl)
		systemKeys |= NSEventModifierFlagControl;
}

//************************************************************************************************
// KeyEvent
//************************************************************************************************

void fromSystemEvent (KeyEvent& keyEvent, const SystemEvent& systemEvent)
{
	NSEvent* nsEvent = (NSEvent*)systemEvent.dataRef;
	NSUInteger modifiers = [nsEvent modifierFlags];
	UInt16 keyCode = [nsEvent keyCode];
	
	// special handling for caps lock pressed
	if(keyCode == kVK_CapsLock && modifiers & NSEventModifierFlagCapsLock)
	{
		keyEvent.vKey = VKey::kCapsLock;
		keyEvent.eventType = KeyEvent::kKeyDown;
		return;
	}
	
	unichar charBuffer[10] = {0};
	if([nsEvent type] != NSEventTypeFlagsChanged)
	{
		NSString* chars;
		if(modifiers & (NSEventModifierFlagCommand | NSEventModifierFlagControl | NSEventModifierFlagOption))
			chars = [nsEvent charactersIgnoringModifiers];
		else
			chars = [nsEvent characters];

		NSRange range = {0, 1};
		if([chars length] > 0)
			[chars getCharacters:charBuffer range:range];
	}
	unsigned short charCode = charBuffer[0];
	
	CCL_PRINTF ("Keycode %x Charcode %x Modifiers %x\n", keyCode, charCode, modifiers)
	
	keyEvent.vKey = VKey::kUnknown;
	bool repeat = false;
	
	switch ([nsEvent type])
	{
	case NSEventTypeKeyDown :
		keyEvent.eventType = KeyEvent::kKeyDown;
		repeat = [nsEvent isARepeat];
		break;
	case NSEventTypeKeyUp :
		keyEvent.eventType = KeyEvent::kKeyUp;
		repeat = [nsEvent isARepeat];
		break;
	case NSEventTypeFlagsChanged:
		{
			fromSystemModifiers (keyEvent.state, (unsigned int)modifiers);
			int newModifiers = keyEvent.state.keys;

			static int lastModifiers = 0;
			
			// determine which modifier has changed
			for(int m = KeyState::kShift; m <= KeyState::kControl; m*=2)
			{
				if((newModifiers & m) != (lastModifiers & m))
				{
					switch(m)
					{
						case KeyState::kShift:    keyEvent.vKey = VKey::kShift; break;
						case KeyState::kCommand:  keyEvent.vKey = VKey::kCommand; break;
						case KeyState::kOption:   keyEvent.vKey = VKey::kOption; break;
						case KeyState::kControl:  keyEvent.vKey = VKey::kControl; break;
						default:
							ASSERT (false)
							continue;
					}

					// determine if up or down
						if(newModifiers & m)
							keyEvent.eventType = KeyEvent::kKeyDown;
						else if(lastModifiers & m)
							keyEvent.eventType = KeyEvent::kKeyUp;
					}
				}
			lastModifiers = newModifiers;
			return;
		}
	}

	if(keyEvent.vKey == VKey::kUnknown)
	{
		if(keyCode == kVK_Tab)
			keyEvent.vKey = VKey::kTab; // problem was: shift+Tab has a match for the charcode (not keycode) in the loop below
		// translate non-printable ascii chars produced by Control + letter to the actual letter
		else if((modifiers & NSEventModifierFlagControl) && charCode > 0 && charCode < 27 && keyCode != kVK_Tab) // not for tab key (to be reviewed: maybe more exceptions required)
			keyEvent.character = charCode - 1 + 'a';
		else if(charCode == 0 && keyCode == kVK_ISO_Section || charCode == 0x5e) // 0x5e => circumflex
		{
			keyEvent.vKey = VKey::kCircumflex;
			modifiers &= ~NSEventModifierFlagShift; // disable shift
		}
		else if(charCode == 0 && keyCode == kVK_ANSI_Equal)
		{
			if(modifiers & NSEventModifierFlagShift)
			{
				keyEvent.vKey = VKey::kGrave;
				modifiers &= ~NSEventModifierFlagShift; // disable shift
			}
			else
				keyEvent.vKey = VKey::kAcute;
		}
		else if(charCode == 0 && keyCode == kVK_ANSI_Semicolon)
			keyEvent.vKey = VKey::kAcute;
		else if(charCode == 0 && keyCode == kVK_ANSI_Grave || charCode == 0x60) // 0x60 => grave
		{
			keyEvent.vKey = VKey::kGrave;
			modifiers &= ~NSEventModifierFlagShift; // disable shift
		}
		else
		{
			for(int i = 0; i < ARRAY_COUNT (VKey::keyMap); i++)
			{
				VKey::KeyMapping& mapping = VKey::keyMap[i];
				if((mapping.sysKey & 0xFFFF0000) != 0)
				{
					if(((mapping.sysKey & 0xFFFF0000) >> 16) == keyCode)
					{
						keyEvent.vKey = mapping.vKey;
						break;
					}
				}
				else
				{
					if(mapping.sysKey == charCode)
					{
						keyEvent.vKey = mapping.vKey;
						break;
					}
				}
			}
		}
	}
	
	// suppress private use unicode characters for function keys
	if((modifiers & NSEventModifierFlagFunction) == 0)
		keyEvent.character = charCode;
	
	fromSystemModifiers (keyEvent.state, (unsigned int)modifiers);
	if(keyEvent.vKey == VKey::kUnknown && Unicode::isAlpha (keyEvent.character) == false)
		keyEvent.state.keys &= ~KeyState::kShift;

	if(repeat)
		keyEvent.state.keys |= KeyState::kRepeat;
	
	if(keyEvent.eventType == KeyEvent::kKeyDown)
	{
		// get the composed character
		
		static UInt32 deadKeyState = 0;
		
		CFObj<TISInputSourceRef> currentKeyboard = TISCopyCurrentKeyboardInputSource ();
		if(currentKeyboard)
		{
			CFDataRef layoutData = static_cast<CFDataRef> (TISGetInputSourceProperty (currentKeyboard, kTISPropertyUnicodeKeyLayoutData));
			if(layoutData)
			{
				const UCKeyboardLayout* keyboardLayout = reinterpret_cast<const UCKeyboardLayout*> (CFDataGetBytePtr (layoutData));
				
				UInt32 modifierKeyState = (modifiers >> 16) & 0xFF;
				
				UniChar uniChar[4] = {0};
				UniCharCount length = 0;
				
				UInt32 savedDeadKeyState = deadKeyState;
				
				OSStatus status = UCKeyTranslate (keyboardLayout, [nsEvent keyCode], kUCKeyActionDown, modifierKeyState, LMGetKbdType (), 0,
												  &deadKeyState, ARRAY_COUNT (uniChar), &length, uniChar);
				
				if(status == noErr && length > 0 && (deadKeyState == 0 || savedDeadKeyState != deadKeyState))
					keyEvent.composedCharacter = uniChar[0];
			}
		}
	}
}

} // namespace VKey
} // namespace CCL
