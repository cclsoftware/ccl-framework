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
// Filename    : ccl/network/netsslsocket.cpp
// Description : SSL Socket class
//
//************************************************************************************************

#include "ccl/network/netsslsocket.h"

#include "ccl/public/base/variant.h"
#include "ccl/public/text/cstring.h"

#include "ccl/public/base/iprogress.h"
#include "ccl/public/systemservices.h"

using namespace CCL;
using namespace Net;

//************************************************************************************************
// SSLSocket
//************************************************************************************************

SSLSocket* SSLSocket::createSocket (AddressFamily addressFamily)
{
	return NEW SSLSocket (addressFamily);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SSLSocket::SSLSocket (AddressFamily addressFamily)
: coreSSLSocket (addressFamily),
  lastSSLResult (kSSLSuccess)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

SSLSocket::~SSLSocket ()
{
	if(coreSSLSocket.isConnected ()) // circumvent assert in CoreSocket dtor
		coreSSLSocket.disconnect ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SSLSocket::setPeerName (StringRef peerName)
{
	coreSSLSocket.setPeerName (MutableCString (peerName, kNetworkTextEncoding));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SSLSocket::connect (const SocketAddress& address)
{
	return connect (address, nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult SSLSocket::connect (const SocketAddress& address, CCL::IProgressNotify* progress)
{
	if(!coreSSLSocket.connect (address))
		return handleError ("SSL Socket connect failed");

	while(1)
	{
		SSLResult result = coreSSLSocket.handshake ();
		if(result == kSSLWouldBlock)
		{
			if(progress && progress->isCanceled ())
				return kResultAborted;

			System::ThreadSleep (1);
		}
		else
			return result == kSSLSuccess ? kResultOk : kResultFailed;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SSLSocket::disconnect ()
{
	coreSSLSocket.close ();
	System::ThreadSleep (1);

	if(!coreSSLSocket.disconnect ())
		return handleError ("SSL Socket disconnect failed");

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SSLSocket::isConnected ()
{
	return coreSSLSocket.isConnected ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SSLSocket::getPeerAddress (SocketAddress& address)
{
	if(!coreSSLSocket.getPeerAddress (address))
		return handleError ("Get peer name failed");
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SSLSocket::bind (const SocketAddress& address)
{
	CCL_NOT_IMPL ("Not implemented for SSL Socket!")
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SSLSocket::listen (int maxConnections)
{
	CCL_NOT_IMPL ("Not implemented for SSL Socket!")
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ISocket* CCL_API SSLSocket::accept ()
{
	CCL_NOT_IMPL ("Not implemented for SSL Socket!")
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SSLSocket::getLocalAddress (SocketAddress& address)
{
	if(!coreSSLSocket.getLocalAddress (address))
		return handleError ("Get local name failed");
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SSLSocket::setOption (int option, VariantRef value)
{
	if(!coreSSLSocket.setOption (option, value.asInt ()))
		return handleError ("SSL Socket option failed");
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SSLSocket::isReadable (int timeout)
{
	return coreSSLSocket.isReadable (timeout);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SSLSocket::isWritable (int timeout)
{
	return coreSSLSocket.isWritable (timeout);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SSLSocket::isAnyError (int timeout)
{
	return coreSSLSocket.isAnyError (timeout);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API SSLSocket::send (const void* buffer, int size, int flags)
{
	int bytesSend = 0;
	setLastResult (coreSSLSocket.sendSSL (buffer, size, bytesSend));
	return lastSSLResult == kSSLSuccess ? bytesSend : SOCKET_ERROR;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API SSLSocket::receive (void* buffer, int size, int flags)
{
	int bytesReceived = 0;
	setLastResult (coreSSLSocket.receiveSSL (buffer, size, bytesReceived));
	return lastSSLResult == kSSLSuccess ? bytesReceived : SOCKET_ERROR;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API SSLSocket::sendTo (const void* buffer, int size, const SocketAddress& address, int flags)
{
	CCL_NOT_IMPL ("Not implemented for SSL Socket!")
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API SSLSocket::receiveFrom (void* buffer, int size, SocketAddress& address, int flags)
{
	CCL_NOT_IMPL ("Not implemented for SSL Socket!")
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SSLSocket::wouldBlockOperation (tbool writeDirection)
{
	return lastSSLResult == kSSLWouldBlock;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SSLSocket::setLastResult (SSLResult result)
{
	lastSSLResult = result;
	if(!(result == kSSLSuccess || result == kSSLWouldBlock))
	{
		// TODO: ccl_raise (...)
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult SSLSocket::handleError (const char* debugMessage)
{
	return BaseSocket::handleError (coreSSLSocket, debugMessage);
}
