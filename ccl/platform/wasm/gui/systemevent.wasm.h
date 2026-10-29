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
// Filename    : ccl/platform/wasm/gui/systemevent.wasm.h
// Description : WebAssembly System Events
//
//************************************************************************************************

#ifndef _ccl_wasm_systemevent_h
#define _ccl_wasm_systemevent_h

#include "ccl/gui/system/systemevent.h"

#include "ccl/base/singleton.h"
#include "ccl/public/collections/vector.h"
#include "ccl/public/text/cstring.h"

namespace emscripten {
class val; }

namespace CCL {
namespace Wasm {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Value conversion
//////////////////////////////////////////////////////////////////////////////////////////////////

void toVariant (Variant& result, const emscripten::val& value);
void fromVariant (emscripten::val& result, VariantRef variant);

} // namespace Wasm

//************************************************************************************************
// SystemEventDelegate
//************************************************************************************************

class SystemEventDelegate: public Object,
						   public Singleton<SystemEventDelegate>
{
public:
	void registerEventHandler (CStringRef id, SystemEventHandler& handler);
	void unregisterEventHandler (SystemEventHandler& handler);
	EventResult handleEvent (CStringRef targetId, SystemEvent& e);

private:
	struct EventHandlerItem
	{
		MutableCString id;
		SystemEventHandler* handler = nullptr;
	};
	Vector<EventHandlerItem> items;
};

} // namespace CCL

#endif // _ccl_wasm_systemevent_h
