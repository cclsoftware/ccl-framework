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
// Filename    : ccl/platform/wasm/gui/keyevent.wasm.h
// Description : Platform-specific Key Codes
//
//************************************************************************************************

#ifndef _ccl_wasm_keyevent_h
#define _ccl_wasm_keyevent_h

#include "ccl/gui/keyevent.h"

struct EmscriptenKeyboardEvent;

namespace CCL {

//************************************************************************************************
// Virtual Key Conversion
//************************************************************************************************

namespace VKey
{
	VirtualKey fromSystemKey (CStringRef code);

	uchar getKeyCharacter (CStringRef key, CStringRef code, bool capsLock);

	void fromSystemModifiers (KeyState& state, bool control, bool shift, bool alt, bool repeat = false, uint16 buttons = 0);

	void fromSystemEvent (KeyEvent& event, const EmscriptenKeyboardEvent& keyEvent, int eventType);
}

} // namespace CCL

#endif // _ccl_wasm_keyevent_h
