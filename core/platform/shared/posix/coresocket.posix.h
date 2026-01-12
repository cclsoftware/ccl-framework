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
// Filename    : core/platform/shared/posix/coresocket.posix.h
// Description : Posix Socket Functions
//
//************************************************************************************************

#ifndef _coresocket_posix_h
#define _coresocket_posix_h

#include "core/platform/shared/coreplatformsocket.h"

#include "core/network/corenetwork.h"

namespace Core {
namespace Platform {

//************************************************************************************************
// PosixSocketHelper
//************************************************************************************************

namespace PosixSocketSets
{
	int select (SocketID highestSocket, fd_set* readList, fd_set* writeList, fd_set* errorList, int timeout);
}

//************************************************************************************************
// PosixSocket
//************************************************************************************************

class PosixSocket: public ISocket
{
public:
	PosixSocket (SocketID socket);
	PosixSocket (Sockets::AddressFamily addressFamily, Sockets::SocketType type, Sockets::ProtocolType protocol);
	~PosixSocket ();

	// ISocket
	SocketID getDescriptor () const override;

	bool connect (const Sockets::SocketAddress& address) override;
	bool disconnect () override;
	bool isConnected () const override;

	bool bind (const Sockets::SocketAddress& address) override;
	bool listen (int maxConnections) override;
	SocketID accept () override;

	bool getPeerAddress (Sockets::SocketAddress& address) const override;
	bool getLocalAddress (Sockets::SocketAddress& address) const override;

	bool setOption (int option, int value) override;
	bool getOption (int& value, int option) const override;
	bool joinMulticastGroup (const Sockets::IPAddress& groupAddress, const Sockets::IPAddress& adapterAddress) override;
	bool leaveMulticastGroup (const Sockets::IPAddress& groupAddress, const Sockets::IPAddress& adapterAddress) override;

	bool isReadable (int timeout) const override;
	bool isWritable (int timeout) const override;
	bool isAnyError (int timeout) const override;

	int send (const void* buffer, int size, int flags) override;
	int sendAll (const void* buffer, int size, int flags) override;
	int receive (void* buffer, int size, int flags) override;
	int getBytesAvailable (int& bytesAvailable) override;

	int sendTo (const void* buffer, int size, const Sockets::SocketAddress& address, int flags) override;
	int receiveFrom (void* buffer, int size, Sockets::SocketAddress& address, int flags) override;

	int getErrorCode () const override;
	bool wouldBlockOperation (bool writeDirection) const override;

protected:
	SocketID socket;
	bool connected;

	virtual int setNonBlockingMode (bool state);
	virtual int setMulticastMembership (uint32 address, bool state);

	enum CheckHint { kReadable, kWritable, kAnyError };
	virtual int checkState (CheckHint hint, int timeout) const;
	virtual bool checkForError () const;
};

#if CORE_SOCKET_IMPLEMENTATION == CORE_FEATURE_IMPLEMENTATION_POSIX
typedef PosixSocket Socket;
#endif

//************************************************************************************************
// PosixSocketIDSet
//************************************************************************************************

class PosixSocketIDSet: public ISocketIDSet
{
public:
	PosixSocketIDSet (const fd_set& p)
	: p (p)
	{}
	
	operator fd_set& () { return p; }
	fd_set* getSet () { return &p; }

	void set (SocketID socket) override { FD_SET (socket, &p); }
	void clear (SocketID socket) override { FD_CLR (socket, &p); }
	bool isSet (SocketID socket) override { return FD_ISSET (socket, &p); }
	void zero () override { FD_ZERO (&p); }

protected:
	fd_set p;
};

#if CORE_SOCKET_IMPLEMENTATION == CORE_FEATURE_IMPLEMENTATION_POSIX
typedef PosixSocketIDSet SocketIDSet;
#endif

} // namespace Platform
} // namespace Core

#endif // _coresocket_posix_h
