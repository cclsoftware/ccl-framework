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
// Filename    : core/platform/shared/coreplatformsocket.h
// Description : Socket class platform implementation base
//
//************************************************************************************************

#ifndef _coreplatformsocket_h
#define _coreplatformsocket_h

#include "core/public/corestringbuffer.h"
#include "core/public/coresocketaddress.h"
#include "core/platform/corefeatures.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
// Global socket function stub
//////////////////////////////////////////////////////////////////////////////////////////////////

#if CORE_SOCKET_IMPLEMENTATION == CORE_FEATURE_UNIMPLEMENTED

#define INVALID_SOCKET -1

inline int socket (int addressFamily, int type, int protocol) { return INVALID_SOCKET; }

#endif // CORE_SOCKET_IMPLEMENTATION == CORE_FEATURE_UNIMPLEMENTED

namespace Core {
namespace Platform {

typedef UIntPtr SocketID;

//************************************************************************************************
// ISocket
//************************************************************************************************

struct ISocket
{
	virtual ~ISocket () {}

	virtual SocketID getDescriptor () const = 0;

	virtual bool connect (const Sockets::SocketAddress& address) = 0;
	virtual bool disconnect () = 0;
	virtual bool isConnected () const = 0;
	virtual int getConnectStatus (int timeout) const = 0;

	virtual bool bind (const Sockets::SocketAddress& address) = 0;
	virtual bool listen (int maxConnections) = 0;
	virtual SocketID accept () = 0;

	virtual bool getPeerAddress (Sockets::SocketAddress& address) const = 0;
	virtual bool getLocalAddress (Sockets::SocketAddress& address) const = 0;

	virtual bool setOption (int option, int value) = 0;	
	virtual bool getOption (int& value, int option) const = 0;
	virtual bool joinMulticastGroup (const Sockets::IPAddress& groupAddress, const Sockets::IPAddress& adapterAddress) = 0;
	virtual bool leaveMulticastGroup (const Sockets::IPAddress& groupAddress, const Sockets::IPAddress& adapterAddress) = 0;
	
	virtual bool isReadable (int timeout = 0) const = 0;
	virtual bool isWritable (int timeout = 0) const = 0;
	virtual bool isAnyError (int timeout = 0) const = 0;
	
	virtual int send (const void* buffer, int size, int flags = 0) = 0;
	virtual int sendAll (const void* buffer, int size, int flags = 0) = 0;	
	virtual int receive (void* buffer, int size, int flags = 0) = 0;
	virtual int getBytesAvailable (int& bytesAvailable) = 0;

	virtual int sendTo (const void* buffer, int size, const Sockets::SocketAddress& address, int flags = 0) = 0;
	virtual int receiveFrom (void* buffer, int size, Sockets::SocketAddress& address, int flags = 0) = 0;

	virtual int getErrorCode () const = 0;
	virtual bool wouldBlockOperation (bool writeDirection) const = 0;
};

//************************************************************************************************
// ISocketIDSet
//************************************************************************************************

struct ISocketIDSet
{
	virtual ~ISocketIDSet () {}

	virtual void set (SocketID index) = 0;
	virtual void clear (SocketID index) = 0;
	virtual bool isSet (SocketID index) = 0;
	virtual void zero () = 0;
};

//************************************************************************************************
// SocketHelper
//************************************************************************************************

namespace SocketSets
{
	int select (SocketID highestSocket, ISocketIDSet* readList, ISocketIDSet* writeList, ISocketIDSet* errorList, int timeout);
}

#if CORE_SOCKET_IMPLEMENTATION == CORE_FEATURE_UNIMPLEMENTED

#define SOCKET_ERROR -1

#define ENETDOWN 100
#define ENETUNREACH 101
#define EHOSTDOWN 112
#define EADDRINUSE 98
#define ENETRESET 102
#define ECONNRESET 104
#define ECONNABORTED 103
#define ENOTCONN 107
#define ESHUTDOWN 108
#define ETIMEDOUT 110
#define ECONNREFUSED 111
#define EISCONN 106
#define ENOBUFS 105

//************************************************************************************************
// SocketStub
//************************************************************************************************

class SocketStub: public ISocket
{
public:
	SocketStub (SocketID socket) {}
	SocketStub (Sockets::AddressFamily addressFamily, Sockets::SocketType type, Sockets::ProtocolType protocol) {}

	// ISocket
	SocketID getDescriptor () const override { return INVALID_SOCKET; }
	bool connect (const Sockets::SocketAddress& address) override { return false; }
	bool disconnect () override { return false; }
	bool isConnected () const override { return false; }
	int getConnectStatus (int timeout) const override { return SOCKET_ERROR; }
	bool bind (const Sockets::SocketAddress& address) override { return false; }
	bool listen (int maxConnections) override { return false; }
	SocketID accept () override { return INVALID_SOCKET; }
	bool getPeerAddress (Sockets::SocketAddress& address) const override { return false; }
	bool getLocalAddress (Sockets::SocketAddress& address) const override { return false; }
	bool setOption (int option, int value) override { return false; }
	bool getOption (int& value, int option) const override { return false; }
	bool joinMulticastGroup (const Sockets::IPAddress& groupAddress, const Sockets::IPAddress& adapterAddress) override { return false; }
	bool leaveMulticastGroup (const Sockets::IPAddress& groupAddress, const Sockets::IPAddress& adapterAddress) override { return false; }
	bool isReadable (int timeout = 0) const override { return false; }
	bool isWritable (int timeout = 0) const override { return false; }
	bool isAnyError (int timeout = 0) const override { return false; }
	int send (const void* buffer, int size, int flags = 0) override { return SOCKET_ERROR; }
	int sendAll (const void* buffer, int size, int flags = 0) override { return SOCKET_ERROR; }
	int receive (void* buffer, int size, int flags = 0) override { return SOCKET_ERROR; }
	int getBytesAvailable (int& bytesAvailable) override { return SOCKET_ERROR; }
	int sendTo (const void* buffer, int size, const Sockets::SocketAddress& address, int flags = 0) override { return SOCKET_ERROR; }
	int receiveFrom (void* buffer, int size, Sockets::SocketAddress& address, int flags = 0) override { return SOCKET_ERROR; }
	int getErrorCode () const override { return 0; }
	bool wouldBlockOperation (bool writeDirection) const override { return false; }
};

typedef SocketStub Socket;

//************************************************************************************************
// SocketIDSetStub
//************************************************************************************************

class SocketIDSetStub: public ISocketIDSet
{
public:
	struct DummySet { int dummy; };

	SocketIDSetStub (const DummySet&) {}

	// ISocketIDSet
	void set (SocketID index) override {}
	void clear (SocketID index) override {}
	bool isSet (SocketID index) override { return false; }
	void zero () override {}
};

typedef SocketIDSetStub SocketIDSet;

//************************************************************************************************
// SocketSets stub implementation
//************************************************************************************************

namespace SocketSets
{
	inline int select (SocketID highestSocket, ISocketIDSet* readList, ISocketIDSet* writeList, ISocketIDSet* errorList, int timeout) { return SOCKET_ERROR; }
}

#endif // CORE_SOCKET_IMPLEMENTATION == CORE_FEATURE_UNIMPLEMENTED

} // namespace Platform
} // namespace Core

#endif // _coreplatformsocket_h
