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
// Filename    : ccl/platform/wasm/network/wsclient.wasm.h
// Description : WebAssembly WebSocket Client using Emscripten WebSocket API
//
//************************************************************************************************

#ifndef _ccl_wasm_wsclient_h
#define _ccl_wasm_wsclient_h

#include "ccl/network/web/webabstraction.h"

#include "ccl/base/collections/objectlist.h"

#include "ccl/public/system/threadsync.h"

#include <emscripten/websocket.h>

namespace CCL {
namespace Web {

//************************************************************************************************
// WasmWSClient
//************************************************************************************************

class WasmWSClient: public WebSocketClient
{
public:
	DECLARE_CLASS_ABSTRACT (WasmWSClient, WebSocketClient)

	WasmWSClient (IObserver* owner);
	~WasmWSClient ();

	// WebSocketClient
	tresult connect (UrlRef url, VariantRef protocols, IProgressNotify* progress) override;
	tresult process () override;
	void queueMessage (WebSocketMessage* message) override;
	uint32 getBufferedAmount () const override;
	void flushAll () override;
	void disconnect () override;

protected:
	static constexpr int kPollInterval = 50;

	EMSCRIPTEN_WEBSOCKET_T socket;
	Threading::CriticalSection sendQueueLock;
	ObjectList sendQueue;
	bool connected;
	bool connectComplete;
	bool connectSuccess;

	static EM_BOOL onOpen (int eventType, const EmscriptenWebSocketOpenEvent* event, void* userData);
	static EM_BOOL onMessage (int eventType, const EmscriptenWebSocketMessageEvent* event, void* userData);
	static EM_BOOL onError (int eventType, const EmscriptenWebSocketErrorEvent* event, void* userData);
	static EM_BOOL onClose (int eventType, const EmscriptenWebSocketCloseEvent* event, void* userData);

	void sendQueuedMessages ();
};

} // namespace Web
} // namespace CCL

#endif // _ccl_wasm_wsclient_h
