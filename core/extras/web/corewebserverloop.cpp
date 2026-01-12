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
// Filename    : core/extras/web/corewebserverloop.cpp
// Description : Embedded HTTP Server Run Loop
//
//************************************************************************************************

#include "core/extras/web/corewebserverloop.h"

#include "core/network/coresocket.h"
#include "core/network/corenetstream.h"
#include "core/system/corethread.h"

using namespace Core;
using namespace Portable;
using namespace Threads;
using namespace Sockets;
using namespace HTTP;

//************************************************************************************************
// HTTP::ServerRunLoop::Thread
//************************************************************************************************

class ServerRunLoop::Thread: public Threads::Thread
{
public:
	Thread (ServerRunLoop& server)
	: Threads::Thread ("HTTPServer"),
	  server (server)
	{
		setPriority (Threads::kPriorityBelowNormal);
	}

	// Thread
	int threadEntry () override
	{
		server.run ();
		return 0;
	}

public:
	ServerRunLoop& server;
};

//************************************************************************************************
// HTTP::ServerRunLoop
//************************************************************************************************

ServerRunLoop::ServerRunLoop ()
: requestHandler (0),
  socket (0),
  thread (0),
  quitRequested (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ServerRunLoop::~ServerRunLoop ()
{
	// quit() must be called!
	ASSERT (socket == 0)
	ASSERT (thread == 0)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ServerRunLoop::startup (IRequestHandler* requestHandler, Sockets::IPAddress& address)
{
	this->requestHandler = requestHandler;

	// create socket
	ASSERT (socket == 0)
	socket = NEW Socket (address.family, kStream, kTCP);
	
	// bind to given address
	if(!socket->bind (address))
		return false;
	
	// place into listening state
	if(!socket->listen (SocketOption::kMaxConnections))
		return false;

	// start thread
	ASSERT (thread == 0)
	thread = NEW Thread (*this);
	thread->start ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ServerRunLoop::run ()
{
	ASSERT (socket != 0)
	ASSERT (requestHandler != 0)

	while(!quitRequested)
	{
		SocketID descriptor = socket->accept ();
		if(descriptor && !quitRequested)
		{
			Socket s (descriptor);
			NetworkStream ns (s);
			requestHandler->handleHTTPRequest (ns);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ServerRunLoop::quit ()
{
	quitRequested = true;

	IPAddress ip;
	ASSERT (socket != 0)
	socket->getLocalAddress (ip);

	if(ip.isNull ()) // bound to all adapters, use first address
	{
		PortNumber port = ip.port;
		Network::getLocalIPAddress (ip);
		ip.port = port;
	}

	// Attempt to connect to self to break the run() loop...
	Socket closer (ip.family, kStream, kTCP);
	if(closer.connect (ip))
	{
		char buffer[1] = {1};
		closer.send (buffer, 1);
		Threads::CurrentThread::sleep (100);
		closer.disconnect ();
	}

	// stop thread
	ASSERT (thread != 0)
	if(!thread->join (5000))
		thread->terminate ();
	delete thread;
	thread = 0;

	delete socket;
	socket = 0;
}
