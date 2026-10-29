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
// Filename    : ccl/network/web/webabstraction.h
// Description : Web Implementation Abstraction
//
//************************************************************************************************

#ifndef _ccl_webabstraction_h
#define _ccl_webabstraction_h

#include "ccl/base/singleton.h"

#include "ccl/public/base/memorystream.h"

namespace CCL {

interface IProgressNotify;

namespace Web {

interface IWebClient;
interface IWebServer;

//************************************************************************************************
// WebSocketMessage
//************************************************************************************************

class WebSocketMessage: public Object
{
public:
	DECLARE_CLASS (WebSocketMessage, Object)

	static constexpr int kSmallPayloadSize = 125; // max. payload length 7 Bit

	WebSocketMessage ();

	PROPERTY_BOOL (text, Text)
	PROPERTY_SHARED_AUTO (IMemoryStream, largePayload, LargePayload)

	void setSmallPayload (const void* data, int length);
	void setPayload (const void* data, int length);

	int getPayloadLength () const;
	const void* getPayloadData () const;

protected:
	uint8 smallPayload[kSmallPayloadSize] = {};
	int smallPayloadLength = 0;
};

//************************************************************************************************
// WebSocketClient
//************************************************************************************************

class WebSocketClient: public Object
{
public:
	DECLARE_CLASS_ABSTRACT (WebSocketClient, Object)

	WebSocketClient (IObserver* owner);

	static constexpr uint64 kMaxPayloadLength = 8 * 1024 * 1024; // 8 MB limit
	static constexpr int kReadWriteTimeout = 5 * 1000; // Don't block longer than this on read/write operations

	virtual tresult connect (UrlRef url, VariantRef protocols, IProgressNotify* progress) = 0;
	virtual tresult process () = 0;
	virtual void queueMessage (WebSocketMessage* message) = 0;
	virtual uint32 getBufferedAmount () const = 0;
	virtual void flushAll () = 0;
	virtual void disconnect () = 0;

	void signalConnected (tresult result);
	void signalReceived (WebSocketMessage* message);
	void signalError ();

	DECLARE_STRINGID_MEMBER (kConnectResult)
	DECLARE_STRINGID_MEMBER (kProcessingError)
	DECLARE_STRINGID_MEMBER (kMessageReceived)

protected:
	IObserver* owner;
};

//************************************************************************************************
// WebEngine
//************************************************************************************************

class WebEngine: public Object,
				 public ExternalSingleton<WebEngine>
{
public:
	DECLARE_CLASS_ABSTRACT (WebEngine, Object)

	virtual void startup () = 0;

	virtual void shutdown () = 0;

	virtual tresult setUserAgent (StringRef userAgent) = 0;

	virtual IWebClient* createHTTPClient (bool useSSL) = 0;

	virtual IWebServer* createHTTPServer () = 0;

	virtual WebSocketClient* createWebSocketClient (IObserver* owner) = 0;
};

} // namespace Web
} // namespace CCL

#endif // _ccl_webabstraction_h

