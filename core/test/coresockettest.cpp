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
// Filename    : core/test/coresockettest.cpp
// Description : Core Socket Tests
//
//************************************************************************************************

#include "coresockettest.h"

#include "core/network/corenetwork.h"
#include "core/network/coresocket.h"
#include "core/system/corethread.h"

using namespace Core;
using namespace Sockets;
using namespace Test;

const char text[] = "This is a test string\n";

//************************************************************************************************
// ServerThread
//************************************************************************************************

class ServerThread: public Threads::Thread
{
public:
	ServerThread (ITestContext& testContext, bool& succeeded)
	: Thread ("Server Thread"),
	  testContext (testContext),
	  succeeded (succeeded)
	{}

	// Thread
	int threadEntry ()
	{
		Socket socket (kInternet, kStream, kTCP);
		
		IPAddress address;
		address.setIP (127, 0, 0, 1, 50001);
		succeeded &= socket.bind (address);
		if(!succeeded)
			CORE_TEST_FAILED ("Failed to connect server socket to loopback address.")

		socket.listen (SocketOption::kMaxConnections);
		if(!succeeded)
			CORE_TEST_FAILED ("Server failed to listen.")
		SocketID clientSocketID = socket.accept ();
		if(clientSocketID > 0)
			CORE_TEST_MESSAGE ("Server accepted incoming connection.")
		Socket clientSocket (clientSocketID);

		// read string
		char receiveBuffer[sizeof(text)] = {0};
		int bytesRead = 0;
		
		while(bytesRead < sizeof(text))
		{
			int bytesAvailable = 0;
			clientSocket.getBytesAvailable (bytesAvailable);
			int result = clientSocket.receive (receiveBuffer + bytesRead, bytesAvailable);
			if(result > 0)
				bytesRead += result;
		}

		if(bytesRead != sizeof(text))
		{
			succeeded = false;
			CORE_TEST_FAILED ("Number of bytes received did not match string length.")
		}
		
		if(strcmp (receiveBuffer, text) != 0)
		{
			succeeded = false;
			CORE_TEST_FAILED ("Received text does not match sent text.")
		}

		socket.disconnect ();

		return succeeded ? 0 : 1;
	}

private:
	ITestContext& testContext;
	bool& succeeded;
};

//************************************************************************************************
// ClientThread
//************************************************************************************************

class ClientThread: public Threads::Thread
{
public:
	ClientThread (ITestContext& testContext, bool& succeeded)
	: Thread ("Client Thread"),
	  testContext (testContext),
	  succeeded (succeeded)
	{}

	// Thread
	int threadEntry ()
	{
		Socket socket (kInternet, kStream, kTCP);
		
		IPAddress address;
		address.setIP (127, 0, 0, 1, 50001);
		succeeded &= socket.connect (address);
		if(!succeeded)
			CORE_TEST_FAILED ("Failed to connect client socket to loopback address.")

		// send string
		int bytesSent = socket.send (text, sizeof(text));
		if(bytesSent != sizeof(text))
		{
			succeeded = false;
			CORE_TEST_FAILED ("Number of bytes sent does not match string length.")
		}

		socket.disconnect ();

		return succeeded ? 0 : 1;
	}

private:
	ITestContext& testContext;
	bool& succeeded;
};

//************************************************************************************************
// SocketTest
//************************************************************************************************

CORE_REGISTER_TEST (SocketTest)

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr SocketTest::getName () const
{
	return "Core Socket";
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SocketTest::run (ITestContext& testContext)
{
	Network::startup ();
	
	static bool succeeded;
	succeeded = true;

	{
		ServerThread* serverThread = NEW ServerThread (testContext, succeeded);
		serverThread->start ();

		ClientThread* clientThread = NEW ClientThread (testContext, succeeded);
		clientThread->start ();

		if(!serverThread->join (5000))
		{
			serverThread->terminate ();
			succeeded = false;
			CORE_TEST_FAILED ("Server thread did not terminate in time.")
		}

		if(!clientThread->join (5000))
		{
			clientThread->terminate ();
			succeeded = false;
			CORE_TEST_FAILED ("Client thread did not terminate in time.")
		}
		
		delete serverThread;
		delete clientThread;
	}

	Network::shutdown ();

	return succeeded;
}
