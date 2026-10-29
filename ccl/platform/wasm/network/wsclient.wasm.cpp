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
// Filename    : ccl/platform/wasm/network/wsclient.wasm.cpp
// Description : WebAssembly WebSocket Client using Emscripten WebSocket API
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/wasm/network/wsclient.wasm.h"

#include "ccl/base/storage/url.h"

#include "ccl/public/base/iprogress.h"
#include "ccl/public/base/variant.h"
#include "ccl/public/text/cstring.h"

#include <emscripten/threading.h>

using namespace CCL;
using namespace Web;

//************************************************************************************************
// WasmWSClient
//************************************************************************************************

DEFINE_CLASS (WasmWSClient, WebSocketClient)

//////////////////////////////////////////////////////////////////////////////////////////////////

WasmWSClient::WasmWSClient (IObserver* owner)
: WebSocketClient (owner),
  socket (0),
  connected (false),
  connectComplete (false),
  connectSuccess (false)
{
	sendQueue.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WasmWSClient::~WasmWSClient ()
{
	disconnect ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EM_BOOL WasmWSClient::onOpen (int eventType, const EmscriptenWebSocketOpenEvent* event, void* userData)
{
	// Called on the main thread (see emscripten_websocket_set_onopen_callback_on_thread below).
	WasmWSClient* client = static_cast<WasmWSClient*> (userData);
	if(!client)
		return EM_FALSE;

	client->connected = true;
	CCL_PRINTLN ("WebSocket connected")

	client->connectSuccess = true;
	client->connectComplete = true;
	return EM_TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EM_BOOL WasmWSClient::onMessage (int eventType, const EmscriptenWebSocketMessageEvent* event, void* userData)
{
	// Called on the main thread (see emscripten_websocket_set_onmessage_callback_on_thread below).
	WasmWSClient* client = static_cast<WasmWSClient*> (userData);
	if(!client)
		return EM_FALSE;

	WebSocketMessage* message = NEW WebSocketMessage;
	message->setText (event->isText);

	if(event->numBytes > 0)
	{
		if(event->numBytes <= WebSocketMessage::kSmallPayloadSize)
			message->setSmallPayload (event->data, int(event->numBytes));
		else
			message->setPayload (event->data, int(event->numBytes));
	}

	client->signalReceived (message);
	return EM_TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EM_BOOL WasmWSClient::onError (int eventType, const EmscriptenWebSocketErrorEvent* event, void* userData)
{
	// Called on the main thread (see emscripten_websocket_set_onerror_callback_on_thread below).
	WasmWSClient* client = static_cast<WasmWSClient*> (userData);
	if(!client)
		return EM_FALSE;

	CCL_PRINTLN ("WebSocket error")

	// If we haven't connected yet, this is a handshake failure: resolve connect()'s wait instead
	// of routing it through the generic (post-connect) error signal.
	if(!client->connected)
	{
		client->connectSuccess = false;
		client->connectComplete = true;
	}
	else
		client->signalError ();

	return EM_TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EM_BOOL WasmWSClient::onClose (int eventType, const EmscriptenWebSocketCloseEvent* event, void* userData)
{
	// Called on the main thread (see emscripten_websocket_set_onclose_callback_on_thread below).
	WasmWSClient* client = static_cast<WasmWSClient*> (userData);
	if(!client)
		return EM_FALSE;

	bool wasConnected = client->connected;
	client->connected = false;
	CCL_PRINTF ("WebSocket closed: code=%d, reason=%s\n", event->code, event->reason)

	// A close without a preceding open/error (e.g. handshake rejected outright) still needs to
	// resolve connect()'s wait, so it doesn't sit there until it times out.
	if(!wasConnected)
	{
		client->connectSuccess = false;
		client->connectComplete = true;
	}

	return EM_TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult WasmWSClient::connect (UrlRef url, VariantRef protocols, IProgressNotify* progress)
{
	if(connected || socket != 0)
		return kResultFailed;

	UrlFullString urlStr (url, true);
	MutableCString urlCStr (urlStr, Text::kUTF8);

	EmscriptenWebSocketCreateAttributes attrs;
	emscripten_websocket_init_create_attributes (&attrs);

	attrs.url = urlCStr;
	attrs.protocols = nullptr;

	// Set protocols if provided (comma-separated string)
	MutableCString protocolStr;
	if(protocols.isString ())
	{
		String protocolString;
		protocols.toString (protocolString);
		protocolStr = MutableCString (protocolString, Text::kUTF8);
		attrs.protocols = protocolStr;
	}

	socket = emscripten_websocket_new (&attrs);
	if(socket <= 0)
	{
		CCL_PRINTF ("Failed to create WebSocket: %d\n", socket)
		return kResultFailed;
	}

	// Register callbacks on main browser thread to avoid threading assertions
	emscripten_websocket_set_onopen_callback_on_thread (socket, this, onOpen, pthread_t(EM_CALLBACK_THREAD_CONTEXT_MAIN_RUNTIME_THREAD));
	emscripten_websocket_set_onmessage_callback_on_thread (socket, this, onMessage, pthread_t(EM_CALLBACK_THREAD_CONTEXT_MAIN_RUNTIME_THREAD));
	emscripten_websocket_set_onerror_callback_on_thread (socket, this, onError, pthread_t(EM_CALLBACK_THREAD_CONTEXT_MAIN_RUNTIME_THREAD));
	emscripten_websocket_set_onclose_callback_on_thread (socket, this, onClose, pthread_t(EM_CALLBACK_THREAD_CONTEXT_MAIN_RUNTIME_THREAD));

	connectComplete = false;
	connectSuccess = false;

	CCL_PRINTF ("WebSocket connecting to %s, waiting for handshake...\n", urlCStr.str ())

	int iterations = 0;
	while(!connectComplete)
	{
		emscripten_thread_sleep (kPollInterval);

		if(progress && progress->isCanceled ())
		{
			CCL_PRINTLN ("WebSocket connect canceled")
			disconnect ();
			return kResultFailed;
		}

		if(++iterations * kPollInterval >= kReadWriteTimeout)
		{
			CCL_WARN ("WebSocket connect timed out\n", 0)
			disconnect ();
			return kResultFailed;
		}
	}

	CCL_PRINTF ("WebSocket connect result: %s\n", connectSuccess ? "success" : "failed")
	return connectSuccess ? kResultOk : kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult WasmWSClient::process ()
{
	if(!connected)
		return kResultFailed;

	sendQueuedMessages ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmWSClient::queueMessage (WebSocketMessage* message)
{
	if(!message)
		return;

	Threading::ScopedLock lock (sendQueueLock);
	sendQueue.add (message);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmWSClient::sendQueuedMessages ()
{
	if(!connected || socket <= 0)
		return;

	Threading::ScopedLock lock (sendQueueLock);

	while(!sendQueue.isEmpty ())
	{
		WebSocketMessage* message = static_cast<WebSocketMessage*> (sendQueue.getFirst ());
		if(!message)
			break;

		const void* data = message->getPayloadData ();
		int length = message->getPayloadLength ();

		EMSCRIPTEN_RESULT result;
		if(message->isText ())
			result = emscripten_websocket_send_utf8_text (socket, static_cast<CStringPtr> (data));
		else
			result = emscripten_websocket_send_binary (socket, const_cast<void*> (data), uint32(length));

		if(result != EMSCRIPTEN_RESULT_SUCCESS)
		{
			CCL_WARN ("Failed to send WebSocket message: %d\n", result);
			break;
		}

		sendQueue.removeFirst ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uint32 WasmWSClient::getBufferedAmount () const
{
	if(socket <= 0)
		return 0;

	size_t buffered = 0;
	emscripten_websocket_get_buffered_amount (socket, &buffered);
	return uint32(buffered);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmWSClient::flushAll ()
{
	sendQueuedMessages ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmWSClient::disconnect ()
{
	if(socket > 0)
	{
		emscripten_websocket_close (socket, 1000, "Client disconnect");
		emscripten_websocket_delete (socket);
		socket = 0;
	}

	connected = false;
	sendQueue.removeAll ();
}
