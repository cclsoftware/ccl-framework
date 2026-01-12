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
// Filename    : ccl/gui/keyevent.h
// Description : Keyboard Event
//
//************************************************************************************************

#ifndef _ccl_keyevent_h
#define _ccl_keyevent_h

#include "ccl/base/object.h"

#include "ccl/public/gui/framework/guievent.h"

namespace CCL {

//************************************************************************************************
// Virtual Key Conversion
//************************************************************************************************

namespace VKey
{
	/** Map system key. */
	VirtualKey fromSystemKey (int sysKey);

	/** Get sytem key. */
	int toSystemKey (VirtualKey key);

	/** Get key name. */
	const char* getKeyName (VirtualKey key);

	/** Get key by name. */
	VirtualKey getKeyByName (const char* name);

	/** Get localized key name. */
	String getLocalizedKeyName (VirtualKey key);

	/** Map system modifiers. */
	void fromSystemModifiers (KeyState& keyState, unsigned int systemKeys);

	/** Map system modifiers. */
	void toSystemModifiers (KeyState keyState, unsigned int& systemKeys);

	/** Map system keyboard event. */
	void fromSystemEvent (KeyEvent& keyEvent, const SystemEvent& systemEvent);

	struct KeyMapping
	{
		int sysKey;
		VirtualKey vKey;
	};

	extern KeyMapping keyMap[];
	extern int getKeyMappingSize ();
}

//************************************************************************************************
// Boxed::KeyEvent
//************************************************************************************************

namespace Boxed
{
	class KeyEvent: public Object,
					public CCL::KeyEvent
	{
	public:
		DECLARE_CLASS (KeyEvent, Object)
		DECLARE_METHOD_NAMES (KeyEvent)

		KeyEvent (const CCL::KeyEvent& e = CCL::KeyEvent ());

		using CCL::KeyEvent::toString;

		// Object
		bool toString (String& string, int flags) const override;
		bool equals (const Object& obj) const override;
		bool load (const Storage& storage) override;
		bool save (const Storage& storage) const override;
		
	protected:
		// IObject
		tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
	};
}

} // namespace CCL

#endif // _ccl_keyevent_h
