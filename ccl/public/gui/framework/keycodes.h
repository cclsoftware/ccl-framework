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
// Filename    : ccl/public/gui/framework/keycodes.h
// Description : Virtual Key Codes
//
//************************************************************************************************

#ifndef _ccl_keycodes_h
#define _ccl_keycodes_h

#include "ccl/public/base/platform.h"

namespace CCL {

//************************************************************************************************
// Virtual Key Codes
//************************************************************************************************

/** Virtual key code.
	\ingroup gui */
typedef int VirtualKey;

namespace VKey
{
	enum KeyCodes
	{
		kUnknown = -1,

		kBackspace,					///< [BACKSPACE]
		kTab,						///< [TAB]
		kCapsLock,					///< [CAPS LOCK]
		kEnter,						///< [ENTER]
		kReturn,					///< [RETURN]

		kShift,						///< [SHIFT]
		kCommand,					///< [CTRL] on Windows, [APPLE] on MacOS
		kOption,					///< [ALT] on Windows, [OPTION] on MacOS
		kControl,					///< [CONTROL] on MacOS only

		kEscape,					///< [ESC]
		kSpace,						///< [SPACE]
		kHome,						///< [HOME]
		kEnd,						///< [END]

		kLeft,						///< [LEFT ARROW]
		kUp,						///< [UP ARROW]
		kRight,						///< [RIGHT ARROW]
		kDown,						///< [DOWN ARROW]

		kPageUp,					///< [PAGE UP]
		kPageDown,					///< [PAGE DOWN]

		kInsert,					///< [INS]
		kDelete,					///< [DEL]

		kNumPad0,					///< 0
		kNumPad1,					///< 1
		kNumPad2,					///< 2
		kNumPad3,					///< 3
		kNumPad4,					///< 4
		kNumPad5,					///< 5
		kNumPad6,					///< 6
		kNumPad7, 					///< 7
		kNumPad8, 					///< 8
		kNumPad9,					///< 9

		kMultiply,					///< *
		kAdd,						///< +
		kSubtract,					///< -
		kDecimal,					///< .
		kDivide,					///< /

		kF1,						///< [F1]
		kF2,						///< [F2]
		kF3,						///< [F3]
		kF4,						///< [F4]
		kF5,						///< [F5]
		kF6,						///< [F6]
		kF7,						///< [F7]
		kF8,						///< [F8]
		kF9,						///< [F9]
		kF10,						///< [F10]
		kF11,						///< [F11]
		kF12,						///< [F12]
		kF13,						///< [F13]
		kF14,						///< [F14]
		kF15,						///< [F15]
		kF16,						///< [F16]
		kF17,						///< [F17]
		kF18,						///< [F18]
		kF19,						///< [F19]
		kF20,						///< [F20]
		kF21,						///< [F21]
		kF22,						///< [F22]
		kF23,						///< [F23]
		kF24,						///< [F24]

		kVolumeMute,                ///< Media Keys
		kVolumeUp,
		kVolumeDown,
		kStop,
		kPlayPause,
		kPause,
		kRecord,
		kForward,
		kRewind,
		kChannelUp,
		kChannelDown,

		kCircumflex,
		kTilde,
		kAcute,
		kGrave,
		kDiaeresis,

		kNumVirtualKeys
	};
}

} // namespace CCL

#endif // _ccl_keycodes_h
