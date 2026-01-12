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
// Filename    : core/network/coresocket.h
// Description : Socket class
//
//************************************************************************************************

#ifndef _coresocket_h
#define _coresocket_h

#include "core/platform/corefeatures.h"

#if CORE_SOCKET_IMPLEMENTATION == CORE_PLATFORM_IMPLEMENTATION
	#include CORE_PLATFORM_IMPLEMENTATION_HEADER (coresocket)
#elif CORE_SOCKET_IMPLEMENTATION == CORE_EXTERNAL_PLATFORM_IMPLEMENTATION
	#include CORE_EXTERNAL_PLATFORM_IMPLEMENTATION_HEADER (coresocket)
#elif CORE_SOCKET_IMPLEMENTATION == CORE_FEATURE_IMPLEMENTATION_POSIX
	#include "core/platform/shared/posix/coresocket.posix.h"
#elif CORE_SOCKET_IMPLEMENTATION == CORE_FEATURE_IMPLEMENTATION_LWIP
	#include "core/platform/shared/lwip/coresocket.lwip.h"
#endif

#include "core/network/corenetwork.h"

namespace Core {
namespace Sockets {

typedef Platform::SocketID SocketID;
typedef Platform::SocketIDSet SocketIDSet;

//************************************************************************************************
// Socket
/** Network socket class wrapping a BSD-style descriptor. */
//************************************************************************************************

class Socket
{
public:
	Socket (SocketID socket);
	Socket (AddressFamily addressFamily, SocketType type, ProtocolType protocol);

	SocketID getDescriptor () const;

	bool connect (const SocketAddress& address);
	bool disconnect ();
	bool isConnected () const;	
	
	bool bind (const SocketAddress& address);
	bool listen (int maxConnections);
	SocketID accept ();

	bool getPeerAddress (SocketAddress& address) const;
	bool getLocalAddress (SocketAddress& address) const;

	bool setOption (int option, int value);	
	bool getOption (int& value, int option) const;
	bool joinMulticastGroup (const IPAddress& groupAddress, const IPAddress& adapterAddress);
	bool leaveMulticastGroup (const IPAddress& groupAddress, const IPAddress& adapterAddress);
	
	bool isReadable (int timeout = 0) const;
	bool isWritable (int timeout = 0) const;
	bool isAnyError (int timeout = 0) const;
	
	int send (const void* buffer, int size, int flags = 0);
	int sendAll (const void* buffer, int size, int flags = 0);	
	int receive (void* buffer, int size, int flags = 0);
	int getBytesAvailable (int& bytesAvailable);

	int sendTo (const void* buffer, int size, const SocketAddress& address, int flags = 0);
	int receiveFrom (void* buffer, int size, SocketAddress& address, int flags = 0);

	int getErrorCode () const;
	bool wouldBlockOperation (bool writeDirection) const;

	static int select (SocketID highestSocket, SocketIDSet* readList, SocketIDSet* writeList, SocketIDSet* errorList, int timeout);

protected:
	Platform::Socket platformSocket;
};

//************************************************************************************************
// Socket implementation
//************************************************************************************************

inline Socket::Socket (SocketID socket)
: platformSocket (socket)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline Socket::Socket (AddressFamily addressFamily, SocketType type, ProtocolType protocol)
: platformSocket (addressFamily, type, protocol)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline SocketID Socket::getDescriptor () const
{ return platformSocket.getDescriptor (); }

inline bool Socket::connect (const SocketAddress& address)
{ return platformSocket.connect (address); }

inline bool Socket::disconnect ()
{ return platformSocket.disconnect (); }

inline bool Socket::isConnected () const
{ return platformSocket.isConnected (); }

inline bool Socket::bind (const SocketAddress& address)
{ return platformSocket.bind (address); }

inline bool Socket::listen (int maxConnections)
{ return platformSocket.listen (maxConnections); }

inline SocketID Socket::accept ()
{ return platformSocket.accept (); }

inline bool Socket::getPeerAddress (SocketAddress& address) const
{ return platformSocket.getPeerAddress (address); }

inline bool Socket::getLocalAddress (SocketAddress& address) const
{ return platformSocket.getLocalAddress (address); }

inline bool Socket::setOption (int option, int value)
{ return platformSocket.setOption (option, value); }

inline bool Socket::getOption (int& value, int option) const
{ return platformSocket.getOption (value, option); }

inline bool Socket::joinMulticastGroup (const IPAddress& groupAddress, const IPAddress& adapterAddress)
{ return platformSocket.joinMulticastGroup (groupAddress, adapterAddress); }

inline bool Socket::leaveMulticastGroup (const IPAddress& groupAddress, const IPAddress& adapterAddress)
{ return platformSocket.leaveMulticastGroup (groupAddress, adapterAddress); }

inline bool Socket::isReadable (int timeout) const
{ return platformSocket.isReadable (timeout); }

inline bool Socket::isWritable (int timeout) const
{ return platformSocket.isWritable (timeout); }

inline bool Socket::isAnyError (int timeout) const
{ return platformSocket.isAnyError (timeout); }

inline int Socket::send (const void* buffer, int size, int flags)
{ return platformSocket.send (buffer, size, flags); }

inline int Socket::sendAll (const void* buffer, int size, int flags)
{ return platformSocket.sendAll (buffer, size, flags); }

inline int Socket::receive (void* buffer, int size, int flags)
{ return platformSocket.receive (buffer, size, flags); }

inline int Socket::getBytesAvailable (int& bytesAvailable)
{ return platformSocket.getBytesAvailable (bytesAvailable); }

inline int Socket::sendTo (const void* buffer, int size, const SocketAddress& address, int flags)
{ return platformSocket.sendTo (buffer, size, address, flags); }

inline int Socket::receiveFrom (void* buffer, int size, SocketAddress& address, int flags)
{ return platformSocket.receiveFrom (buffer, size, address, flags); }

inline int Socket::getErrorCode () const
{ return platformSocket.getErrorCode (); }

inline bool Socket::wouldBlockOperation (bool writeDirection) const
{ return platformSocket.wouldBlockOperation (writeDirection); }

inline int Socket::select (SocketID highestSocket, SocketIDSet* readList, SocketIDSet* writeList, SocketIDSet* errorList, int timeout)
{ return Platform::SocketSets::select (highestSocket, readList, writeList, errorList, timeout); }

} // namespace Sockets
} // namespace Core

#endif // _coresocket_h
