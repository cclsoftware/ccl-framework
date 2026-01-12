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
// Filename    : ccl/platform/android/gui/keyevent.android.h
// Description : Platform-specific Key Code stuff
//
//************************************************************************************************

#ifndef _ccl_keyevent_android_h
#define _ccl_keyevent_android_h

namespace CCL {

struct KeyEvent;

namespace VKey 
{
	void makeKeyEvent (KeyEvent& keyEvent, int keyCode, int character, int modifiers, bool isRepeat);
}

} // namespace CCL

#endif // _ccl_keyevent_android_h
