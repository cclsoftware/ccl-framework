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
// Filename    : ccl/platform/wasm/gui/clipboard.wasm.cpp
// Description : System Clipboard
//
//************************************************************************************************

#include "ccl/platform/wasm/gui/systemevent.wasm.h"

#include "ccl/gui/system/clipboard.h"

#include "ccl/public/base/variant.h"
#include "ccl/public/text/cstring.h"
#include "ccl/public/text/cclstring.h"

#include <emscripten/html5.h>

namespace CCL {
namespace Wasm { 

//////////////////////////////////////////////////////////////////////////////////////////////////
// JS Helpers
//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (void, setClipboardText, (CStringPtr text), 
{
	try 
	{
		navigator.clipboard.writeText (UTF8ToString (text));
	}
	catch(e)
	{}
});

} // namespace Wasm;

//************************************************************************************************
// WasmClipboard
//************************************************************************************************

class WasmClipboard: public Clipboard,
					 public SystemEventHandler
{
public:
	WasmClipboard ();
	~WasmClipboard ();

	// Clipboard
	bool setNativeText (StringRef text) override;
	bool getNativeText (String& text) const override;
	bool hasNativeContentChanged () override;

	// SystemEventHandler
	EventResult handleEvent (SystemEvent& e) override;

protected:
	String cachedContent;
	bool changed;
};

} // namespace CCL

using namespace CCL;
using namespace Wasm;

//************************************************************************************************
// WasmClipboard
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (Clipboard, WasmClipboard)

//////////////////////////////////////////////////////////////////////////////////////////////////

WasmClipboard::WasmClipboard ()
: changed (false)
{
	SystemEventDelegate::instance ().registerEventHandler ("clipboard", *this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WasmClipboard::~WasmClipboard ()
{
	SystemEventDelegate::instance ().unregisterEventHandler (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WasmClipboard::setNativeText (StringRef text)
{
	setClipboardText (MutableCString (text, Text::kUTF8));
	cachedContent = text;
	changed = true;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WasmClipboard::getNativeText (String& text) const
{
	text = cachedContent;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WasmClipboard::hasNativeContentChanged ()
{
	return changed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EventResult WasmClipboard::handleEvent (SystemEvent& e)
{
	if(e.eventId == "changed")
	{
		cachedContent = e.arg1.asString ();
		changed = true;
	}
	return nullptr;
}
