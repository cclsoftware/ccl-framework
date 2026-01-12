//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2025 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/network/web/websocket.h
// Description : WebSocket class
//
//************************************************************************************************

#ifndef _ccl_websocket_h
#define _ccl_websocket_h

#include "ccl/base/storage/url.h"

#include "ccl/public/base/iprogress.h"
#include "ccl/public/network/web/iwebsocket.h"

namespace CCL {
namespace Web {

class WebSocketClient;

//************************************************************************************************
// WebSocket
//************************************************************************************************

class WebSocket: public Object,
				 public IWebSocket
{
public:
	DECLARE_CLASS (WebSocket, Object)
	DECLARE_METHOD_NAMES (WebSocket)
	DECLARE_PROPERTY_NAMES (WebSocket)

	WebSocket ();
	~WebSocket ();

	static void cancelOnExit ();

	// IWebSocket
	ReadyState CCL_API getReadyState () const override;
	uint32 CCL_API getBufferedAmount () const override;
	StringRef CCL_API getExtensions () const override;
	StringRef CCL_API getProtocol () const override;
	UrlRef CCL_API getUrl () const override;
	tresult CCL_API open (UrlRef url, VariantRef protocols = 0) override;
	tresult CCL_API close (int code = 0, StringRef reason = nullptr) override;
	tresult CCL_API send (VariantRef data) override;

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	CLASS_INTERFACE (IWebSocket, Object)

protected:
	class CancelHelper: public Object,
						public AbstractProgressNotify
	{
	public:
		void setCanceled (bool state) { canceled = state; }
		tbool CCL_API isCanceled () override; // IProgressNotify
		CLASS_INTERFACE (IProgressNotify, Object)
	protected:
		bool canceled = false;		
	};

	static bool exiting;

	CancelHelper cancelHelper;
	WebSocketClient* client;
	ReadyState readyState;
	Url url;

	void setState (ReadyState newState);

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

} // naemspace Web
} // namespace CCL

#endif // _ccl_websocket_h
