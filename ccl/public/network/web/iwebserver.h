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
// Filename    : ccl/public/network/web/iwebserver.h
// Description : Web Server Interface
//
//************************************************************************************************

#ifndef _ccl_iwebserver_h
#define _ccl_iwebserver_h

#include "ccl/public/network/isocket.h"

namespace CCL {
namespace Web {

interface IWebServerApp;
interface IWebRequest;

//************************************************************************************************
// IWebServer
/** Server interface for protocols like HTTP. */
//************************************************************************************************

interface IWebServer: IUnknown
{
	/** Assign application callback interface. */
	virtual void CCL_API setApp (IWebServerApp* app) = 0;

	/** Start server bound to given address. */
	virtual tresult CCL_API startup (const Net::SocketAddress& address) = 0;

	/** Get address this server has been bound to. */
	virtual tresult CCL_API getAddress (Net::SocketAddress& address) = 0;

	/** Run server loop. */
	virtual tresult CCL_API run () = 0;

	/** Request quit of server loop. */
	virtual void CCL_API quit () = 0;

	DECLARE_IID (IWebServer)
};

DEFINE_IID (IWebServer, 0x2368e9fa, 0xae55, 0x4fe5, 0xab, 0xc2, 0xa1, 0xb7, 0xc8, 0xec, 0xb9, 0xf)

//************************************************************************************************
// IWebServerApp
/** Application callback interface for IWebServer. */
//************************************************************************************************

interface IWebServerApp: IUnknown
{
	/** Provide server identity. */
	virtual StringRef CCL_API getServerName () const = 0;

	/** Handle request. */
	virtual tresult CCL_API handleRequest (IWebRequest& request) = 0;
	
	DECLARE_IID (IWebServerApp)
};

DEFINE_IID (IWebServerApp, 0x52fa50e9, 0x2f50, 0x40db, 0x9b, 0x41, 0x8a, 0x1d, 0x4, 0xbe, 0x31, 0xeb)

} // namespace Web
} // namespace CCL

#endif // _ccl_iwebserver_h
