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
// Filename    : ccl/network/web/internal/webengine.cpp
// Description : Internal Web Engine
//
//************************************************************************************************

#include "ccl/network/web/internal/httpclient.h"
#include "ccl/network/web/internal/httpserver.h"
#include "ccl/network/web/internal/wsclient.h"

#include "ccl/base/message.h"

namespace CCL {
namespace Web {

//************************************************************************************************
// InternalWebEngine
//************************************************************************************************

class InternalWebEngine: public WebEngine
{
public:
	// WebEngine
	void startup () override;
	void shutdown () override;
	tresult setUserAgent (StringRef userAgent) override;
	IWebClient* createHTTPClient (bool useSSL) override;
	IWebServer* createHTTPServer () override;
	WebSocketClient* createWebSocketClient (IObserver* owner) override;
};

} // namespace Web
} // namespace CCL

using namespace CCL;
using namespace Web;

//************************************************************************************************
// InternalWebEngine
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (WebEngine, InternalWebEngine)

//////////////////////////////////////////////////////////////////////////////////////////////////

void InternalWebEngine::startup ()
{
	// make sure singleton exists
	HTTP::ConnectionManager::instance ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void InternalWebEngine::shutdown ()
{
	// make sure persistent connections are closed
	HTTP::ConnectionManager::instance ().terminate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult InternalWebEngine::setUserAgent (StringRef userAgent)
{
	ASSERT (HTTP::Client::getUserAgent (false).isEmpty ())
	if(!HTTP::Client::getUserAgent (false).isEmpty ()) // must be called only once!
		return kResultFailed;

	HTTP::Client::setUserAgent (MutableCString (userAgent, Text::kUTF8));
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IWebClient* InternalWebEngine::createHTTPClient (bool useSSL)
{
	return NEW HTTP::Client (useSSL);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IWebServer* InternalWebEngine::createHTTPServer ()
{
	return NEW HTTP::Server;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WebSocketClient* InternalWebEngine::createWebSocketClient (IObserver* owner)
{
	return NEW InternalWSClient (owner);
}
