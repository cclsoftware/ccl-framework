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
// Filename    : ccl/platform/wasm/gui/systemshell.wasm.cpp
// Description : WebAssembly System Shell
//
//************************************************************************************************

#include "ccl/gui/system/systemshell.h"

#include "ccl/base/storage/url.h"

#include "ccl/public/text/cstring.h"

namespace CCL {
namespace Wasm {

//////////////////////////////////////////////////////////////////////////////////////////////////
// JS Helpers
//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (bool, openUrlInWindow, (CStringPtr url),
{
	return window.open (UTF8ToString (url)) !== null;
});

} // namespace Wasm

//************************************************************************************************
// WasmSystemShell
//************************************************************************************************

class WasmSystemShell: public SystemShell
{
public:
	// SystemShell
	tresult openApplicationSettings () override { return kResultNotImplemented; }
	tresult openNativeUrl (UrlRef url, int flags) override;
};

} // namespace CCL

using namespace CCL;
using namespace Wasm;

//************************************************************************************************
// WasmSystemShell
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (SystemShell, WasmSystemShell)

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult WasmSystemShell::openNativeUrl (UrlRef url, int flags)
{
	return openUrlInWindow (MutableCString (UrlFullString (url, true), Text::kUTF8).str ()) ? kResultOk : kResultFailed;
}