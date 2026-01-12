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
// Filename    : ccl/network/web/webserver.h
// Description : Web Server
//
//************************************************************************************************

#ifndef _ccl_webserver_h
#define _ccl_webserver_h

#include "ccl/base/object.h"

#include "ccl/public/network/web/iwebserver.h"

namespace CCL {
namespace Web {

//************************************************************************************************
// WebServer
//************************************************************************************************

class WebServer: public Object,
				 public IWebServer
{
public:
	DECLARE_CLASS_ABSTRACT (WebServer, Object)

	WebServer ();

	// IWebServer
	void CCL_API setApp (IWebServerApp* app) override;

	CLASS_INTERFACE (IWebServer, Object)

protected:
	IWebServerApp* app;
};

} // namespace Web
} // namespace CCL

#endif // _ccl_webserver_h
