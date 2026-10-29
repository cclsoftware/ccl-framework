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
// Filename    : ccl/platform/wasm/network/webengine.wasm.h
// Description : WebAssembly WebEngine implementation
//
//************************************************************************************************

#ifndef _ccl_wasm_webengine_h
#define _ccl_wasm_webengine_h

#include "ccl/network/web/webabstraction.h"

namespace CCL {
namespace Web {

//************************************************************************************************
// WasmWebEngine
//************************************************************************************************

class WasmWebEngine: public WebEngine
{
public:
	DECLARE_CLASS (WasmWebEngine, WebEngine)

	// WebEngine
	void startup () override {}
	void shutdown () override {}
	tresult setUserAgent (StringRef userAgent) override;
	IWebClient* createHTTPClient (bool useSSL) override;
	IWebServer* createHTTPServer () override;
	WebSocketClient* createWebSocketClient (IObserver* owner) override;
};

} // namespace Web
} // namespace CCL

#endif // _ccl_wasm_webengine_h
