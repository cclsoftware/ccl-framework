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
// Filename    : ccl/public/network/isocket.h
// Description : Network Socket Interface
//
//************************************************************************************************

#ifndef _ccl_isocket_h
#define _ccl_isocket_h

#include "ccl/public/base/iunknown.h"

#include "core/public/coresocketaddress.h"

namespace CCL {
interface IProgressNotify;

namespace Net {

using namespace Core::Sockets::SocketTypes;

//////////////////////////////////////////////////////////////////////////////////////////////////
/** Network result codes */
//////////////////////////////////////////////////////////////////////////////////////////////////

constexpr tresult kResultNetworkUnreachable = static_cast<tresult>(0x80010051L);  ///< Network unreachable (WSAENETUNREACH)
constexpr tresult kResultAddressInUse       = static_cast<tresult>(0x80010048L);  ///< Address is already used (WSAEADDRINUSE)
constexpr tresult kResultConnectionReset    = static_cast<tresult>(0x80010052L);  ///< Network dropped connection on reset (WSAENETRESET)
constexpr tresult kResultConnectionAborted  = static_cast<tresult>(0x80010053L);  ///< Local connection error (WSAECONNABORTED)
constexpr tresult kResultConnectionTimeOut  = static_cast<tresult>(0x80010060L);  ///< Connection failed after waiting too long (WSAETIMEDOUT)
constexpr tresult kResultConnectionRefused  = static_cast<tresult>(0x80010061L);  ///< Attempt to connect failed (WSAECONNREFUSED)
constexpr tresult kResultSocketError        = static_cast<tresult>(0x80010000L);  ///< An unspecified socket error

//************************************************************************************************
// ISocket
/** Network socket interface. */
//************************************************************************************************

interface ISocket: IUnknown
{
	//////////////////////////////////////////////////////////////////////////////////////////////
	// Client
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Establish connection to specified host. */
	virtual tresult CCL_API connect (const SocketAddress& address) = 0;

	/** Close socket connection. */
	virtual tresult CCL_API disconnect () = 0;

	/** Returns true if socket is currently connected. */
	virtual tbool CCL_API isConnected () = 0;

	/** Retrieve address of peer this socket is connected to. */
	virtual tresult CCL_API getPeerAddress (SocketAddress& address) = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Server
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Bind socket to specified local address. */
	virtual tresult CCL_API bind (const SocketAddress& address) = 0;

	/** Place socket into listening state, waiting for incoming connections. */
	virtual tresult CCL_API listen (int maxConnections) = 0;

	/** Permit incoming connection attempt. Returned socket must be released! */
	virtual ISocket* CCL_API accept () = 0;

	/** Retrieve local address of this socket. */
	virtual tresult CCL_API getLocalAddress (SocketAddress& address) = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Client/Server
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Set socket option. */
	virtual tresult CCL_API setOption (int option, VariantRef value) = 0;

	/** Check if socket is readable (data available, incoming connection, connection closed). */
	virtual tbool CCL_API isReadable (int timeout = 0) = 0;

	/** Check if socket is writable (connection succeeded, data can be sent). */
	virtual tbool CCL_API isWritable (int timeout = 0) = 0;

	/** Check for socket errors (connection attempt failed). */
	virtual tbool CCL_API isAnyError (int timeout = 0) = 0;

	/** Send data to socket. */
	virtual int CCL_API send (const void* buffer, int size, int flags = 0) = 0;

	/** Receive data from socket. */
	virtual int CCL_API receive (void* buffer, int size, int flags = 0) = 0;

	/** Send data to specified destination, used for connectionless sockets. */
	virtual int CCL_API sendTo (const void* buffer, int size, const SocketAddress& address, int flags = 0) = 0;
        
	/** Receive data from socket, used for connectionless sockets. */
	virtual int CCL_API receiveFrom (void* buffer, int size, SocketAddress& address, int flags = 0) = 0;
    
	/** Returns true if last socket operation exited because it would block. */
	virtual tbool CCL_API wouldBlockOperation (tbool writeDirection) = 0;

	DECLARE_IID (ISocket)
};

DEFINE_IID (ISocket, 0xca64f7ac, 0x7736, 0x4dfb, 0x87, 0x75, 0x8c, 0xd6, 0x99, 0x22, 0x8, 0x15)

//************************************************************************************************
// IMulticastSocket
/** Additonal socket interface to join (UDP) multicast groups. */
//************************************************************************************************

interface IMulticastSocket: IUnknown
{
	/** Join given multicast group, adapter address can be null (any). */
	virtual tresult CCL_API joinMulticastGroup (const IPAddress& groupAddress, const IPAddress& adapterAddress) = 0;
	
	/** Leave given multicast group, adapter address can be null (any). */
	virtual tresult CCL_API leaveMulticastGroup (const IPAddress& groupAddress, const IPAddress& adapterAddress) = 0;

	DECLARE_IID (IMulticastSocket)
};

DEFINE_IID (IMulticastSocket, 0x95a59a0a, 0x5ded, 0x42ba, 0x9b, 0x67, 0x77, 0xb2, 0x57, 0x91, 0xab, 0x39)

//************************************************************************************************
// INetworkStream
/** Network stream interface (extends IStream). */
//************************************************************************************************

interface INetworkStream: IUnknown
{
	/** Get underlying socket. */
	virtual ISocket* CCL_API getSocket () = 0;

	/** Set timeout in milliseconds. */
	virtual void CCL_API setTimeout (int timeout) = 0;

	/** Set pseudo blocking behavior. */
	virtual void CCL_API setPseudoBlocking (tbool state) = 0;

	/** Set callback interface for cancelation. */
	virtual void CCL_API setCancelCallback (IProgressNotify* callback) = 0;

	DECLARE_IID (INetworkStream)
};

DEFINE_IID (INetworkStream, 0xc62c37c8, 0x5e9e, 0x47f7, 0x8a, 0x77, 0x3c, 0x45, 0xf4, 0x6b, 0x7a, 0xc0)

} // namespace Net
} // namespace CCL

#endif // _ccl_isocket_h
