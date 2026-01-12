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
// Filename    : ccl/network/web/http/server.cpp
// Description : HTTP Server
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/network/web/http/server.h"
#include "ccl/network/web/http/request.h"

#include "ccl/network/netstream.h"

#include "ccl/public/text/cclstring.h"
#include "ccl/public/netservices.h"

using namespace CCL;
using namespace Web;
using namespace HTTP;

//************************************************************************************************
// HTTP::Server
//************************************************************************************************

DEFINE_CLASS_HIDDEN (Server, WebServer)

//////////////////////////////////////////////////////////////////////////////////////////////////

Server::Server ()
: socket (nullptr),
  quitRequested (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Server::~Server ()
{
	safe_release (socket);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Server::startup (const Net::SocketAddress& address)
{
	ASSERT (socket == nullptr)
	safe_release (socket);

	// create socket
	socket = System::GetNetwork ().createSocket (address.family, Net::kStream, Net::kTCP);
	ASSERT (socket)
	if(!socket)
		return kResultFailed;

	// bind to given address
	tresult result = socket->bind (address);
	if(result != kResultOk)
		return result;

	// place into listening state
	result = socket->listen (Net::SocketOption::kMaxConnections);
	if(result != kResultOk)
		return result;

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Server::getAddress (Net::SocketAddress& address)
{
	Net::IPAddress* ip = Net::IPAddress::cast (&address);
	ASSERT (ip)
	if(!ip)
		return kResultInvalidArgument;

	ASSERT (socket)
	if(!socket)
		return kResultFailed;

	tresult result = socket->getLocalAddress (*ip);
	if(result != kResultOk)
		return result;

	if(ip->isNull ()) // bound to all adapters, return first address
	{
		Net::PortNumber port = ip->port;
		result = System::GetNetwork ().getLocalIPAddress (*ip);
		ip->port = port;
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Server::run ()
{
	ASSERT (socket)
	if(!socket)
		return kResultUnexpected;

	MutableCString serverName;
	if(app)
		serverName = app->getServerName ();
	if(serverName.isEmpty ())
		serverName = CSTR ("WebServer/1.0");

	while(!quitRequested)
	{
		AutoPtr<Net::ISocket> connection = socket->accept ();
		if(connection && !quitRequested)
		{
			// TODO: push to thread pool...

			AutoPtr<Net::NetworkStream> stream = NEW Net::NetworkStream (connection);
			AutoPtr<Request> request = NEW Request (stream);
			if(request->receive ())
			{
				#if DEBUG_LOG
				request->dump ();
				#endif

				Response& response = request->getResponse ();

				bool handled = false;
				if(request->getHeaders ().getHost ().isEmpty ())
				{
					handled = true;
					response.setStatus (HTTP::kBadRequest);
				}

				if(!handled)
				{
					if(app)
						app->handleRequest (*request);
					response.setStatus (HTTP::kOK);
				}

				// TODO: "Date" value = "Mon, 23 Nov 2009 14:58:11 GMT"
				response.getHeaders ().setServer (serverName);
				response.getHeaders ().setConnection ("close");

				#if DEBUG_LOG
				response.dump ();
				#endif

				bool done = response.send ();
				ASSERT (done == true)
			}
		}
	}
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Server::quit ()
{
	if(quitRequested)
		return;

	quitRequested = true;

	// Attempt connecting to self to break the run() loop...
	Net::IPAddress address;
	getAddress (address);

	AutoPtr<Net::ISocket> closer = System::GetNetwork ().createSocket (address.family, Net::kStream, Net::kTCP);
	ASSERT (closer)
	if(closer)
	{
		closer->connect (address);
		char buffer[1] = {1};
		closer->send (buffer, 1);
		closer->disconnect ();
	}
}
