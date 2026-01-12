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
// Filename    : ccl/public/gui/framework/guievent.cpp
// Description : GUI Events
//
//************************************************************************************************

#include "ccl/public/gui/framework/guievent.h"
#include "ccl/public/gui/framework/iguihelper.h"

using namespace CCL;

//************************************************************************************************
// KeyState
//************************************************************************************************

bool KeyState::fromString (StringRef string)
{
	return System::GetGUIHelper ().KeyState_fromString (*this, string) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void KeyState::toString (String& string, bool translated) const
{
	System::GetGUIHelper ().KeyState_toString (*this, string, translated);
}

//************************************************************************************************
// KeyEvent
//************************************************************************************************

bool KeyEvent::isSimilar (const KeyEvent& e) const
{
	return (state.getModifiers () == e.state.getModifiers ()) && 
		   ((isVKeyValid () || e.isVKeyValid ()) ? vKey == e.vKey : character == e.character);	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool KeyEvent::fromString (StringRef string)
{
	return System::GetGUIHelper ().KeyEvent_fromString (*this, string) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void KeyEvent::toString (String& string, bool translated) const
{
	System::GetGUIHelper ().KeyEvent_toString (*this, string, translated);
}
