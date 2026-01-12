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
// Filename    : ccl/platform/linux/gui/keyevent.linux.cpp
// Description : Platform-specific Key Codes
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/keyevent.h"
#include "ccl/public/text/cclstring.h"

#include <xkbcommon/xkbcommon-keysyms.h>

namespace CCL {
namespace VKey {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Linux key mapping
//////////////////////////////////////////////////////////////////////////////////////////////////

KeyMapping keyMap[] =
{
	{XKB_KEY_BackSpace,	kBackspace},
	{XKB_KEY_Tab,		kTab},
	{XKB_KEY_ISO_Left_Tab, kTab},
	{XKB_KEY_Caps_Lock,	kCapsLock},
	{XKB_KEY_KP_Enter,	kEnter},
	{XKB_KEY_Return,	kReturn},

	{XKB_KEY_Shift_L,	kShift},
	{XKB_KEY_Control_L,	kCommand},
	{XKB_KEY_Alt_L,		kOption},

	{XKB_KEY_Shift_R,	kShift},
	{XKB_KEY_Control_R,	kCommand},
	{XKB_KEY_Alt_R,		kOption},

	{XKB_KEY_Escape,	kEscape},
	{XKB_KEY_space,		kSpace},
	{XKB_KEY_Home,		kHome},
	{XKB_KEY_End,		kEnd},

	{XKB_KEY_Left,		kLeft},
	{XKB_KEY_Up,		kUp},
	{XKB_KEY_Right,		kRight},
	{XKB_KEY_Down,		kDown},

	{XKB_KEY_Page_Up,	kPageUp},
	{XKB_KEY_Page_Down,	kPageDown},

	{XKB_KEY_Insert,	kInsert},
	{XKB_KEY_Delete,	kDelete},

	{XKB_KEY_KP_0,	kNumPad0},
	{XKB_KEY_KP_1,	kNumPad1},
	{XKB_KEY_KP_2,	kNumPad2},
	{XKB_KEY_KP_3,	kNumPad3},
	{XKB_KEY_KP_4,	kNumPad4},
	{XKB_KEY_KP_5,	kNumPad5},
	{XKB_KEY_KP_6,	kNumPad6},
	{XKB_KEY_KP_7,	kNumPad7},
	{XKB_KEY_KP_8,	kNumPad8},
	{XKB_KEY_KP_9,	kNumPad9},

	{XKB_KEY_KP_Multiply,	kMultiply},
	{XKB_KEY_KP_Add,		kAdd},
	{XKB_KEY_KP_Subtract,	kSubtract},
	{XKB_KEY_KP_Decimal,	kDecimal},
	{XKB_KEY_KP_Divide,		kDivide},

	{XKB_KEY_F1,	kF1},
	{XKB_KEY_F2,	kF2},
	{XKB_KEY_F3,	kF3},
	{XKB_KEY_F4,	kF4},
	{XKB_KEY_F5,	kF5},
	{XKB_KEY_F6,	kF6},
	{XKB_KEY_F7,	kF7},
	{XKB_KEY_F8,	kF8},
	{XKB_KEY_F9,	kF9},
	{XKB_KEY_F10,	kF10},
	{XKB_KEY_F11,	kF11},
	{XKB_KEY_F12,	kF12},
	{XKB_KEY_F13,	kF13},
	{XKB_KEY_F14,	kF14},
	{XKB_KEY_F15,	kF15},
	{XKB_KEY_F16,	kF16},
	{XKB_KEY_F17,	kF17},
	{XKB_KEY_F18,	kF18},
	{XKB_KEY_F19,	kF19},
	{XKB_KEY_F20,	kF20},
	{XKB_KEY_F21,	kF21},
	{XKB_KEY_F22,	kF22},
	{XKB_KEY_F23,	kF23},
	{XKB_KEY_F24,	kF24}	
};

int getKeyMappingSize ()
{
	return ARRAY_COUNT (keyMap);
}

} // namespace VKey
} // namespace CCL
