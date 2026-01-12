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
// Filename    : ccl/network/web/http/server.h
// Description : HTTP Server
//
//************************************************************************************************

#ifndef _ccl_httpserver_h
#define _ccl_httpserver_h

#include "ccl/network/web/webserver.h"

namespace CCL {
namespace Web {
namespace HTTP {

//************************************************************************************************
// HTTP::Server
//************************************************************************************************

class Server: public WebServer
{
public:
	DECLARE_CLASS (Server, WebServer)

	Server ();
	~Server ();

	// WebServer
	tresult CCL_API startup (const Net::SocketAddress& address) override;
	tresult CCL_API getAddress (Net::SocketAddress& address) override;
	tresult CCL_API run () override;
	void CCL_API quit () override;

protected:
	Net::ISocket* socket;
	bool quitRequested;
};

} // namespace HTTP
} // namespace Web
} // namespace CCL

#endif // _ccl_httpserver_h
