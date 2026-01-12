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
// Filename    : ccl/gui/keyevent.cpp
// Description : Keyboard Event
//
//************************************************************************************************

#include "ccl/gui/keyevent.h"

#include "ccl/base/storage/storage.h"

#include "ccl/public/text/translation.h"

namespace CCL {

//************************************************************************************************
// Key Names (parsed by String Extractor)
//************************************************************************************************

static const char* keyNames[VKey::kNumVirtualKeys] =
{
	#define _K(s) s

/* BEGIN_XSTRINGS "VirtualKey" */

/* XSTRING */ _K ("Backspace"),		// kBackspace
/* XSTRING */ _K ("Tab"),			// kTab
/* XSTRING */ _K ("Caps Lock"),		// kCapsLock
/* XSTRING */ _K ("Enter"),			// kEnter
/* XSTRING */ _K ("Return"),		// kReturn

/* XSTRING */ _K ("Shift"),			// kShift
/* XSTRING */ _K ("Command"),		// kCommand
/* XSTRING */ _K ("Option"),		// kOption
/* XSTRING */ _K ("Control"),		// kControl

/* XSTRING */ _K ("Esc"),			// kEscape
/* XSTRING */ _K ("Space"),			// kSpace
/* XSTRING */ _K ("Home"),			// kHome
/* XSTRING */ _K ("End"),			// kEnd

/* XSTRING */ _K ("Left Arrow"),	// kLeft
/* XSTRING */ _K ("Up Arrow"),		// kUp
/* XSTRING */ _K ("Right Arrow"),	// kRight
/* XSTRING */ _K ("Down Arrow"),	// kDown

/* XSTRING */ _K ("Page Up"),		// kPageUp
/* XSTRING */ _K ("Page Down"),		// kPageDown

/* XSTRING */ _K ("Ins"),			// kInsert
/* XSTRING */ _K ("Del"),			// kDelete

/* XSTRING */ _K ("NumPad0"),		// kNumPad0
/* XSTRING */ _K ("NumPad1"),		// kNumPad1
/* XSTRING */ _K ("NumPad2"),		// kNumPad2
/* XSTRING */ _K ("NumPad3"),		// kNumPad3
/* XSTRING */ _K ("NumPad4"),		// kNumPad4
/* XSTRING */ _K ("NumPad5"),		// kNumPad5
/* XSTRING */ _K ("NumPad6"),		// kNumPad6
/* XSTRING */ _K ("NumPad7"),		// kNumPad7
/* XSTRING */ _K ("NumPad8"),		// kNumPad8
/* XSTRING */ _K ("NumPad9"),		// kNumPad9

/* XSTRING */ _K ("NumPad*"),		// kMultiply
/* XSTRING */ _K ("NumPad+"),		// kAdd
/* XSTRING */ _K ("NumPad-"),		// kSubtract
/* XSTRING */ _K ("NumPad."),		// kDecimal
/* XSTRING */ _K ("NumPad/"),		// kDivide

/* XSTRING */ _K ("F1"),			// kF1
/* XSTRING */ _K ("F2"),			// kF2
/* XSTRING */ _K ("F3"),			// kF3
/* XSTRING */ _K ("F4"),			// kF4
/* XSTRING */ _K ("F5"),			// kF5
/* XSTRING */ _K ("F6"),			// kF6
/* XSTRING */ _K ("F7"),			// kF7
/* XSTRING */ _K ("F8"),			// kF8
/* XSTRING */ _K ("F9"),			// kF9
/* XSTRING */ _K ("F10"),			// kF10
/* XSTRING */ _K ("F11"),			// kF11
/* XSTRING */ _K ("F12"),			// kF12
/* XSTRING */ _K ("F13"),			// kF13
/* XSTRING */ _K ("F14"),			// kF14
/* XSTRING */ _K ("F15"),			// kF15
/* XSTRING */ _K ("F16"),			// kF16
/* XSTRING */ _K ("F17"),			// kF17
/* XSTRING */ _K ("F18"),			// kF18
/* XSTRING */ _K ("F19"),			// kF19
/* XSTRING */ _K ("F20"),			// kF20
/* XSTRING */ _K ("F21"),			// kF21
/* XSTRING */ _K ("F22"),			// kF22
/* XSTRING */ _K ("F23"),			// kF23
/* XSTRING */ _K ("F24"),			// kF24

/* XSTRING */ _K ("Volume Mute"),  // kVolumeMute
/* XSTRING */ _K ("Volume Up"),    // kVolumeUp
/* XSTRING */ _K ("Volume Down"),  // kVolumeDown
/* XSTRING */ _K ("Stop"),         // kStop
/* XSTRING */ _K ("Play Pause"),   // kPlayPause
/* XSTRING */ _K ("Pause"),        // kPause
/* XSTRING */ _K ("Record"),       // kRecord
/* XSTRING */ _K ("Forward"),      // kForward
/* XSTRING */ _K ("Rewind"),       // kRewind
/* XSTRING */ _K ("Channel Up"),   // kChannelUp
/* XSTRING */ _K ("Channel Down"), // kChannelDown

/* XSTRING */ _K ("^"),				// kCircumflex
/* XSTRING */ _K ("~"),				// kTilde
/* XSTRING */ _K ("&#180;"),		// kAcute
/* XSTRING */ _K ("&#96;"),			// kGrave
/* XSTRING */ _K ("&#168;")			// kDiaeresis

// XSTRING "Ctrl"
// XSTRING "Alt"

/* END_XSTRINGS */

	#undef _K
};

//************************************************************************************************
// System Key Mapping
//************************************************************************************************

VirtualKey VKey::fromSystemKey (int sysKey)
{
	int count = getKeyMappingSize ();
	for(int i = 0; i < count; i++)
		if(keyMap[i].sysKey == sysKey)
			return keyMap[i].vKey;
	return kUnknown;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int VKey::toSystemKey (VirtualKey key)
{
	int count = getKeyMappingSize ();
	for(int i = 0; i < count; i++)
		if(keyMap[i].vKey == key)
			return keyMap[i].sysKey;
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const char* VKey::getKeyName (VirtualKey key)
{
	#if CCL_PLATFORM_WINDOWS || CCL_PLATFORM_LINUX || CCL_PLATFORM_ANDROID
	if(key == kCommand)
		return "Ctrl";
	if(key == kOption)
		return "Alt";
	#endif
	return key > kUnknown && key < VKey::kNumVirtualKeys ? keyNames[key] : "";
}

//////////////////////////////////////////////////////////////////////////////////////////////////

VirtualKey VKey::getKeyByName (const char* name)
{
	for(int i = 0; i < kNumVirtualKeys; i++)
		if(CString (keyNames[i]) == name)
			return (VirtualKey)i;

	if(CSTR ("NumPad,") == name) // accept as alias (used to be stored in this german form)
		return kDecimal;

	if(CString (name) == "Ctrl")
		return kCommand;
	if(CString (name) == "Alt")
		return kOption;
	return kUnknown;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String VKey::getLocalizedKeyName (VirtualKey key)
{
	LocalString::BeginScope beginScope ("VirtualKey");
	String text = LocalString (getKeyName (key));
	LocalString::EndScope endScope;
	return text;
}

//************************************************************************************************
// Boxed::KeyEvent
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (Boxed::KeyEvent, Object, "KeyEvent")
DEFINE_CLASS_NAMESPACE (Boxed::KeyEvent, NAMESPACE_CCL)

//////////////////////////////////////////////////////////////////////////////////////////////////

Boxed::KeyEvent::KeyEvent (const CCL::KeyEvent& e)
: CCL::KeyEvent (e)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Boxed::KeyEvent::toString (String& string, int flags) const
{
	CCL::KeyEvent::toString (string);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Boxed::KeyEvent::equals (const Object& obj) const
{
	const Boxed::KeyEvent* k = ccl_cast<Boxed::KeyEvent> (&obj);
	return k ? *this == *k : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Boxed::KeyEvent::load (const Storage& storage)
{
	Attributes& a = storage.getAttributes ();

	String name;
	a.get (name, "name");
	return fromString (name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Boxed::KeyEvent::save (const Storage& storage) const
{
	Attributes& a = storage.getAttributes ();

	String name;
	CCL::KeyEvent::toString (name);
	a.set ("name", name);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (Boxed::KeyEvent)
	DEFINE_METHOD_ARGR ("toString", "translated: bool = false", "String")
END_METHOD_NAMES (Boxed::KeyEvent)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Boxed::KeyEvent::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "toString")
	{
		bool translated = msg[0].asBool ();
		String keyString;
		toString (keyString, translated);
		returnValue = keyString;
		returnValue.share ();
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}

} // namespace CCL
