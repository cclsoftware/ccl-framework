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
// Filename    : ccl/network/web/webserver.cpp
// Description : Web Server
//
//************************************************************************************************

#include "ccl/network/web/webserver.h"

using namespace CCL;
using namespace Web;

//************************************************************************************************
// WebServer
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (WebServer, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

WebServer::WebServer ()
: app (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WebServer::setApp (IWebServerApp* app)
{
	this->app = app;
}
