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
// Filename    : ccl/platform/wasm/gui/systemevent.wasm.cpp
// Description : WebAssembly System Events
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/wasm/gui/systemevent.wasm.h"

#include "ccl/public/base/variant.h"
#include "ccl/public/text/cclstdstring.h"

#include <emscripten/val.h>
#include <emscripten/bind.h>

namespace CCL {
namespace Wasm {

//////////////////////////////////////////////////////////////////////////////////////////////////
// JS Event Handler
//////////////////////////////////////////////////////////////////////////////////////////////////

emscripten::val CCLHandleJSEvent (std::string eventId, std::string targetId, emscripten::val _arg1, emscripten::val _arg2, emscripten::val _arg3);

} // namespace Wasm
} // namespace CCL

using namespace CCL;

//************************************************************************************************
// SystemEventDelegate
//************************************************************************************************

DEFINE_SINGLETON (SystemEventDelegate)

//////////////////////////////////////////////////////////////////////////////////////////////////

void SystemEventDelegate::registerEventHandler (CStringRef id, SystemEventHandler& handler)
{
	items.add ({id, &handler});
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SystemEventDelegate::unregisterEventHandler (SystemEventHandler& handler)
{
	items.removeIf ([&handler] (const EventHandlerItem& item) 
	{ 
		return item.handler == &handler;
	});
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EventResult SystemEventDelegate::handleEvent (CStringRef targetId, SystemEvent& e)
{
	for(EventHandlerItem& item : items)
	{
		if(targetId == "*")
			item.handler->handleEvent (e);
		else if(targetId.contains (item.id))
			return item.handler->handleEvent (e);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// JS Event Handler
//////////////////////////////////////////////////////////////////////////////////////////////////

EMSCRIPTEN_BINDINGS (ccl_js_events)
{
	emscripten::function ("CCLHandleJSEvent", &Wasm::CCLHandleJSEvent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

emscripten::val Wasm::CCLHandleJSEvent (std::string _eventId, std::string _targetId, emscripten::val _arg1, emscripten::val _arg2, emscripten::val _arg3)
{
	Variant arg1;
	Variant arg2;
	Variant arg3;
	toVariant (arg1, _arg1);
	toVariant (arg2, _arg2);
	toVariant (arg3, _arg3);

	CString eventId (_eventId.c_str ());
	CString targetId (_targetId.c_str ());

	Variant result;
	SystemEvent event (result, eventId, arg1, arg2, arg3);
	SystemEventDelegate::instance ().handleEvent (targetId, event);

	emscripten::val resultVal;
	fromVariant (resultVal, result);
	return resultVal;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Value conversion
//////////////////////////////////////////////////////////////////////////////////////////////////

void Wasm::toVariant (Variant& result, const emscripten::val& v)
{
	if(v.isNull () || v.isUndefined ())
		return;
	if(v.isString ())
		result = Variant (fromStdString (v.as<std::string> (), Text::kUTF8), true);
	else if(v.isNumber ())
		result = v.as<double> ();
	else if(v.typeOf ().as<std::string> () == "boolean")
	{
		result = v.as<bool> ();
		result.setBoolFormat (true);
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

void Wasm::fromVariant (emscripten::val& result, VariantRef variant)
{
	ASSERT (!variant.isObject ())

	if(!variant.isValid () || variant.isObject ())
		result = emscripten::val::null();
	else if(variant.isInt ())
	{
		if(variant.isBoolFormat ())
			result = emscripten::val (variant.asBool ());
		else
			result = emscripten::val (variant.asLargeInt ());
	}
	else if(variant.isFloat ())
		result = emscripten::val (variant.asDouble ());
	else if(variant.isString ())
		result = emscripten::val (toStdString (variant.asString (), Text::kUTF8));
};