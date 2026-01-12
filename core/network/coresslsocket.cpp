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
// Filename    : core/network/coresslsocket.cpp
// Description : SSL Socket class
//
//************************************************************************************************

#include "core/network/coresslsocket.h"

#if DEBUG
#include "core/system/coredebug.h"
#endif

using namespace Core;
using namespace Sockets;

//************************************************************************************************
// SSLSocket
//************************************************************************************************

SSLSocket::SSLSocket (AddressFamily addressFamily)
: Socket (addressFamily, kStream, kTCP)
{
	ssl.setIOHandler (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SSLSocket::setPeerName (CStringPtr peerName)
{
	ssl.setPeerName (peerName);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SSLResult SSLSocket::handshake ()
{
	return ssl.handshake ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SSLResult SSLSocket::close ()
{
	return ssl.close ();
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

SSLResult SSLSocket::sendSSL (const void* buffer, int size, int& bytesSend)
{
	return ssl.write (buffer, size, bytesSend);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SSLResult SSLSocket::receiveSSL (void* buffer, int size, int& bytesReceived)
{
	return ssl.read (buffer, size, bytesReceived);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SSLResult SSLSocket::write (const void* buffer, int size, int& bytesWritten)
{
	int result = Socket::send (buffer, size);
	if(result == SOCKET_ERROR)
	{
		bytesWritten = 0;
		if(wouldBlockOperation (true))
			return kSSLWouldBlock;
		else
		{
			#if DEBUG
			DebugPrintf ("SSL write failed on raw socket with %d!\n", getErrorCode ());
			#endif			
			return kSSLFailed;
		}
	}

	bytesWritten = result;
	return kSSLSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SSLResult SSLSocket::read (void* buffer, int size, int& bytesRead)
{
	#if CORE_PLATFORM_WINDOWS
	int bytesAvailable = 0;
	if(getBytesAvailable (bytesAvailable) == SOCKET_ERROR)
		return kSSLFailed;
	if(bytesAvailable <= 0) // avoid to get stuck in receive call for blocking socket
		return kSSLWouldBlock;
	#endif

	int result = Socket::receive (buffer, size);
	if(result == SOCKET_ERROR)
	{
		bytesRead = 0;
		if(wouldBlockOperation (false))
			return kSSLWouldBlock;
		else
		{
			#if DEBUG
			DebugPrintf ("SSL read failed on raw socket with %d!\n", getErrorCode ());
			#endif			
			return kSSLFailed;
		}
	}

	bytesRead = result;
	return kSSLSuccess;
}
