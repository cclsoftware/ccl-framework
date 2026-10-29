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
// Filename    : ccl/platform/wasm/network/httpclient.wasm.h
// Description : WebAssembly HTTP Client using Emscripten fetch API
//
//************************************************************************************************

#ifndef _ccl_wasm_httpclient_h
#define _ccl_wasm_httpclient_h

#include "ccl/network/web/webclient.h"

namespace CCL {
namespace Web {

//************************************************************************************************
// WasmHTTPClient
//************************************************************************************************

class WasmHTTPClient: public WebClient
{
public:
	DECLARE_CLASS (WasmHTTPClient, WebClient)

	WasmHTTPClient (bool useSSL = false);
	~WasmHTTPClient ();

	// WebClient
	tresult CCL_API downloadData (StringRef remotePath, IStream& localStream, IWebHeaderCollection* headers = nullptr,
								  IProgressNotify* progress = nullptr) override;
	tresult CCL_API uploadData (IWebHeaderCollection* headers, IStream& localStream, StringRef remotePath, IStream& responseStream,
								StringID method = nullptr, IProgressNotify* progress = nullptr) override;

private:
	struct FetchContext;
	struct HttpRequest;
	struct FetchEvent;
	FetchContext* currentContext;

	bool useSSL;

	static void onFetchEvent (void* arg);
	static void onFetchHeaders (void* userData, int status, int contentLength, CStringPtr headersStr);
	static void onFetchProgress (void* userData, int64_t loaded, int64_t total, const uint8_t* data, int dataLength);
	static void onFetchComplete (void* userData, int status);

	static void proxyHeaders (HttpRequest* request, int status, int contentLength, char* headers);
	static void proxyProgress (HttpRequest* request, int loaded, int total, uint8_t* data, int dataLength);
	static void proxyResult (HttpRequest* request, int status);

	static void dispatchFetch (void* arg);
	static void dispatchCancel (void* arg);

	tresult performRequest (StringID method, StringRef remotePath, IStream* uploadStream,
							IStream& downloadStream, IWebHeaderCollection* headers, IProgressNotify* progress);

	void prepareHeaders (MutableCString& result, IWebHeaderCollection* headers) const;
};

} // namespace Web
} // namespace CCL

#endif // _ccl_wasm_httpclient_h
