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
// Filename    : core/platform/shared/lwip/coresocket.lwip.cpp
// Description : Ctl Socket Functions
//
//************************************************************************************************

#include "coresocket.lwip.h"

namespace Core {
namespace Platform {

//************************************************************************************************
// SocketSets
//************************************************************************************************

namespace SocketSets 
{
	fd_set* toFdSet (ISocketIDSet* set)
	{
		return set ? static_cast<SocketIDSet*> (set)->getSet () : 0;
	}

	int select (SocketID highestSocket, ISocketIDSet* readList, ISocketIDSet* writeList, ISocketIDSet* errorList, int timeout_ms)
	{
		return PosixSocketSets::select (highestSocket, toFdSet (readList), toFdSet (writeList), toFdSet (errorList), timeout_ms);
	}
}

} // namespace Platform
} // namespace Core

using namespace Core;
using namespace Platform;
using namespace Sockets;

//************************************************************************************************
// LwIPSocket
//************************************************************************************************

LwIPSocket::LwIPSocket (SocketID socket)
: PosixSocket(socket)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

LwIPSocket::LwIPSocket (AddressFamily addressFamily, SocketType type, ProtocolType protocol)
: PosixSocket (::lwip_socket (addressFamily, type, protocol))
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LwIPSocket::checkForError () const
{
	int value = 0;
	socklen_t size = sizeof(value);

	// accept() returns EWOULDBLOCK with no connection ready.  This is normal operation
	// and the presence of this socket condition is not indicative of "error".
	int errorCode = ::getsockopt (socket, SOL_SOCKET, SO_ERROR, (char*)&value, &size);
	if((errorCode == EAGAIN) || (errorCode == EWOULDBLOCK) || (errorCode == 0))
		return false;
	else
		return true;
}
