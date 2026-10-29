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
// Filename    : ccl/platform/wasm/network/webengine.wasm.cpp
// Description : WebAssembly WebEngine implementation
//
//************************************************************************************************

#include "ccl/platform/wasm/network/webengine.wasm.h"
#include "ccl/platform/wasm/network/httpclient.wasm.h"
#include "ccl/platform/wasm/network/wsclient.wasm.h"

using namespace CCL;
using namespace Web;

//************************************************************************************************
// WasmWebEngine
//************************************************************************************************

DEFINE_CLASS (WasmWebEngine, WebEngine)
DEFINE_EXTERNAL_SINGLETON (WebEngine, WasmWebEngine)

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult WasmWebEngine::setUserAgent (StringRef userAgentStr)
{
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IWebClient* WasmWebEngine::createHTTPClient (bool useSSL)
{
	return NEW WasmHTTPClient (useSSL);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IWebServer* WasmWebEngine::createHTTPServer ()
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WebSocketClient* WasmWebEngine::createWebSocketClient (IObserver* owner)
{
	return NEW WasmWSClient (owner);
}
