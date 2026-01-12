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
// Filename    : ccl/gui/guihelper.cpp
// Description : GUI Helper
//
//************************************************************************************************

#include "ccl/gui/guihelper.h"
#include "ccl/gui/keyevent.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/text/cclstring.h"
#include "ccl/public/guiservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// GUI Service APIs
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT Internal::IGUIHelper& CCL_API System::CCL_ISOLATED (GetGUIHelper) ()
{
	static GUIHelper theHelper;
	return theHelper;
}

//************************************************************************************************
// GUIHelper
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (GUIHelper, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API GUIHelper::KeyState_fromString (KeyState& This, StringRef string)
{
	This.keys = 0;

	MutableCString asciiString (string);

	if(asciiString.contains ("Command") || asciiString.contains ("Ctrl"))
		This.keys |= KeyState::kCommand;

	if(asciiString.contains ("Shift"))
		This.keys |= KeyState::kShift;

	if(asciiString.contains ("Option") || asciiString.contains ("Alt"))
		This.keys |= KeyState::kOption;

	#if CCL_PLATFORM_MAC
	if(asciiString.contains ("Control"))
		This.keys |= KeyState::kControl;
	#endif
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static void appendModifierName (String& string, VirtualKey key, tbool translated)
{
	if(!string.isEmpty ())
		string.appendASCII ("+");

	if(translated)
		string.append (VKey::getLocalizedKeyName (key));
	else
		string.appendASCII (VKey::getKeyName (key));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API GUIHelper::KeyState_toString (const KeyState& This, String& string, tbool translated)
{
	string.empty ();

	if(This.isSet (KeyState::kCommand))
		appendModifierName (string, VKey::kCommand, translated);

	if(This.isSet (KeyState::kShift))
		appendModifierName (string, VKey::kShift, translated);

	if(This.isSet (KeyState::kOption))
		appendModifierName (string, VKey::kOption, translated);

	#if CCL_PLATFORM_MAC
	if(This.isSet (KeyState::kControl))
		appendModifierName (string, VKey::kControl, translated);
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API GUIHelper::KeyEvent_fromString (KeyEvent& This, StringRef string)
{
	if(string.isEmpty ())
		return false;

	String keyName (string);
	
	// special case for [+] keys...
	if(string.lastChar () == '+')
	{
		if(string.contains (String (VKey::getKeyName (VKey::kAdd))))
		{
			This.vKey = VKey::kAdd;
			This.character = 0;
			This.state.fromString (string);
			return true;
		}
		else 
		{
			This.vKey = VKey::kUnknown;
			This.character = '+';
			This.state.fromString (string);
			return true;
		}
	}

	int index = string.lastIndex (CCLSTR ("+"));
	if(index != -1) // modifier present?
	{
		keyName.remove (0, index + 1);
		This.state.fromString (string);
	}
	else
		This.state.keys = 0;
	
	MutableCString asciiKeyName (keyName);
	This.vKey = VKey::getKeyByName (asciiKeyName);
	if(This.vKey == VKey::kUnknown)
	{
		ASSERT (keyName.length () == 1)
		This.character = keyName[0];
	}
	else
		This.character = 0;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API GUIHelper::KeyEvent_toString (const KeyEvent& This, String& string, tbool translated)
{
	This.state.toString (string, translated != 0);

	if(!string.isEmpty ())
		string.appendASCII ("+");

	if(This.vKey != VKey::kUnknown)
	{
		if(translated)
			string.append (VKey::getLocalizedKeyName (This.vKey));
		else
			string.appendASCII (VKey::getKeyName (This.vKey));
	}
	else
	{
		uchar temp[2] = {This.character, 0};
		string.append (temp);
	}
}
