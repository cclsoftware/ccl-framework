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
// Filename    : ccl/network/netsocket.cpp
// Description : Socket class
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/network/netsocket.h"

#include "ccl/public/base/variant.h"
#include "ccl/public/system/cclerror.h"

#if 0 // LATER TODO: cclnet does not support translations!
	#include "ccl/public/text/translation.h"
#else
	#define BEGIN_XSTRINGS(x)
	#define END_XSTRINGS
	#define XSTRING(name, text) static const char* name = text;
	#define XSTR(name) String (name)
#endif

using namespace CCL;
using namespace Net;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("Network")
	XSTRING (NetworkUnreachable, "Network is unreachable.")
	XSTRING (AddressInUse, "Socket bind error, address is already in use.")
	XSTRING (ConnectionReset, "The network connection was reset by peer.")
	XSTRING (ConnectionAborted, "The network connection was aborted by client.")
	XSTRING (ConnectionTimeOut, "The network connection timed out.")
	XSTRING (ConnectionRefused, "The network connection was refused.")
	XSTRING (OutOfMemory, "Network error, out of memory.")
	XSTRING (SockedError, "Unspecified socket error, system error code %(1).")
END_XSTRINGS

//************************************************************************************************
// Socket
//************************************************************************************************

Socket* Socket::createSocket (AddressFamily addressFamily, SocketType type, ProtocolType protocol)
{
	SocketID descriptor = ::socket (addressFamily, type, protocol);
	ASSERT (descriptor != INVALID_SOCKET)
	return descriptor != INVALID_SOCKET ? NEW Socket (descriptor) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_IID_ (Socket, 0xc59eeaee, 0x6ecd, 0x4f77, 0x85, 0x3d, 0x18, 0x86, 0x50, 0x3f, 0xb8, 0x7c)

//////////////////////////////////////////////////////////////////////////////////////////////////

Socket::Socket (SocketID descriptor)
: coreSocket (descriptor)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Socket::connect (const SocketAddress& address)
{
	if(!coreSocket.connect (address))
		return handleError ("Socket connect failed");
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Socket::disconnect ()
{
	if(!coreSocket.disconnect ())
		return handleError ("Socket disconnect failed");
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Socket::isConnected ()
{
	return coreSocket.isConnected ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Socket::getPeerAddress (SocketAddress& address)
{
	if(!coreSocket.getPeerAddress (address))
		return handleError ("Get peer name failed");
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Socket::bind (const SocketAddress& address)
{
	if(!coreSocket.bind (address))
		return handleError ("Socket bind failed");
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Socket::listen (int maxConnections)
{
	if(!coreSocket.listen (maxConnections))
		return handleError ("Socket listen failed");
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ISocket* CCL_API Socket::accept ()
{
	SocketID descriptor = coreSocket.accept ();
	if(descriptor == INVALID_SOCKET)
	{
		handleError ("Socket accept failed");
		return nullptr;
	}
	return NEW Socket (descriptor);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Socket::getLocalAddress (SocketAddress& address)
{
	if(!coreSocket.getLocalAddress (address))
		return handleError ("Get local name failed");
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Socket::setOption (int option, VariantRef value)
{
	if(!coreSocket.setOption (option, value.asInt ()))
		return handleError ("Socket option failed");
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Socket::isReadable (int timeout)
{
	return coreSocket.isReadable (timeout);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Socket::isWritable (int timeout)
{
	return coreSocket.isWritable (timeout);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Socket::isAnyError (int timeout)
{
	return coreSocket.isAnyError (timeout);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API Socket::send (const void* buffer, int size, int flags)
{
	int result = coreSocket.send (buffer, size, flags);
	if(result == SOCKET_ERROR && !coreSocket.isConnected ())
		handleError ("Socket send failed");
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API Socket::receive (void* buffer, int size, int flags)
{
	int result = coreSocket.receive (buffer, size, flags);
	if(result == SOCKET_ERROR && !coreSocket.isConnected ())
		handleError ("Socket receive failed");
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API Socket::sendTo (const void* buffer, int size, const SocketAddress& address, int flags)
{
	int result = coreSocket.sendTo (buffer, size, address, flags);
	if(result == SOCKET_ERROR && !coreSocket.isConnected ())
		handleError ("Socket sendTo failed");
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API Socket::receiveFrom (void* buffer, int size, SocketAddress& address, int flags)
{
	int result = coreSocket.receiveFrom (buffer, size, address, flags);
	if(result == SOCKET_ERROR && !coreSocket.isConnected ())
		handleError ("Socket receiveFrom failed");
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Socket::wouldBlockOperation (tbool writeDirection)
{
	return coreSocket.wouldBlockOperation (writeDirection != 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Socket::joinMulticastGroup (const IPAddress& groupAddress, const IPAddress& adapterAddress)
{
	if(!coreSocket.joinMulticastGroup (groupAddress, adapterAddress))
		return handleError ("Socket join multicast group failed");
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Socket::leaveMulticastGroup (const IPAddress& groupAddress, const IPAddress& adapterAddress)
{
	if(!coreSocket.leaveMulticastGroup (groupAddress, adapterAddress))
		return handleError ("Socket leave multicast group failed");
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SocketID Socket::getDescriptor () const
{
    return coreSocket.getDescriptor ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult Socket::handleError (const char* debugMessage)
{
	return BaseSocket::handleError (coreSocket, debugMessage);
}

//************************************************************************************************
// BaseSocket
//************************************************************************************************

tresult BaseSocket::handleError (Core::Sockets::Socket& coreSocket, const char* debugMessage)
{
	int errorCode = coreSocket.getErrorCode ();

	CCL_PRINTF ("%s, errorcode %d\n", debugMessage, errorCode)

	tresult tr = translateErrorCode (errorCode);
	ccl_raise (getErrorString (errorCode), tr);
	return tr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult BaseSocket::translateErrorCode (int nativeError)
{
	#if CCL_PLATFORM_WINDOWS
	switch(nativeError)
	{
	case WSAENETUNREACH:
	case WSAENETDOWN:
	case WSAEHOSTDOWN:
	case WSAEHOSTUNREACH:
		return kResultNetworkUnreachable;
	case WSAEADDRINUSE:
	case WSAEISCONN:
		return kResultAddressInUse;
	case WSAENETRESET:
	case WSAECONNRESET:
		return kResultConnectionReset;
	case WSAECONNABORTED:
	case WSAENOTCONN:
	case WSAESHUTDOWN:
		return kResultConnectionAborted;
	case WSAETIMEDOUT:
		return kResultConnectionTimeOut;
	case WSAECONNREFUSED:
		return kResultConnectionRefused;
	case WSA_NOT_ENOUGH_MEMORY:
		return kResultOutOfMemory;
	}
	#else
	switch(nativeError)
	{
	case ENETDOWN:
	case ENETUNREACH:
	case EHOSTDOWN:
		return kResultNetworkUnreachable;
	case EADDRINUSE:
		return kResultAddressInUse;
	case ENETRESET:
	case ECONNRESET:
		return kResultConnectionReset;
	case ECONNABORTED:
	case ENOTCONN:
	case ESHUTDOWN:
		return kResultConnectionAborted;
	case ETIMEDOUT:
		return kResultConnectionTimeOut;
	case ECONNREFUSED:
	case EISCONN:
		return kResultConnectionRefused;
	case ENOBUFS:
		return kResultOutOfMemory;
	}
	#endif
	return kResultSocketError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String BaseSocket::getErrorString (int nativeError)
{
	switch(translateErrorCode (nativeError))
	{
	case kResultNetworkUnreachable:
		return XSTR (NetworkUnreachable);
	case kResultAddressInUse:
		return XSTR (AddressInUse);
	case kResultConnectionReset:
		return XSTR (ConnectionReset);
	case kResultConnectionAborted:
		return XSTR (ConnectionAborted);
	case kResultConnectionTimeOut:
		return XSTR (ConnectionTimeOut);
	case kResultConnectionRefused:
		return XSTR (ConnectionRefused);
	case kResultOutOfMemory:
		return XSTR (OutOfMemory);
	}
	return String ().appendFormat (XSTR (SockedError), nativeError);
}
