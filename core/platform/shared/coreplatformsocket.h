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

} // namespace Platform
} // namespace Core

#endif // _coreplatformsocket_h
