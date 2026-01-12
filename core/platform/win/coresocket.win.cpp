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
// Filename    : core/platform/win/coresocket.win.cpp
// Description : Windows Socket Functions
//
//************************************************************************************************

#include "coresocket.win.h"

#include "core/system/corethread.h"
#include "core/network/corenetwork.h"

namespace Core {
namespace Platform {

//************************************************************************************************
// SocketSets
//************************************************************************************************

namespace SocketSets
{
	fd_set* toFdSet (ISocketIDSet* set)
	{
		return set ? static_cast<SocketIDSet*> (set)->getSet () : nullptr;
	}

	int select (SocketID highestSocket, ISocketIDSet* readList, ISocketIDSet* writeList, ISocketIDSet* errorList, int timeout_ms)
	{
		return PosixSocketSets::select (highestSocket, toFdSet (readList), toFdSet (writeList), toFdSet (errorList), timeout_ms);
	}
}

} // namespace Platform
} // namespace Core

using namespace Core;
using namespace Sockets;
using namespace Platform;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Missing POSIX functions
//////////////////////////////////////////////////////////////////////////////////////////////////

int close (SocketID socket)
{
	return ::closesocket (socket);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ioctl (Core::Platform::SocketID, unsigned long request, ...)
{
	return EINVAL;
}

//************************************************************************************************
// Win32Socket
//************************************************************************************************

Win32Socket::Win32Socket (SocketID socket)
: PosixSocket (socket)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Win32Socket::Win32Socket (AddressFamily addressFamily, SocketType type, ProtocolType protocol)
: PosixSocket (addressFamily, type, protocol)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Win32Socket::setNonBlockingMode (bool state)
{
	u_long nonBlocking = state ? 1 : 0;
	return ::ioctlsocket (socket, FIONBIO, &nonBlocking);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Win32Socket::getBytesAvailable (int& bytesAvailable)
{
	u_long arg = 0;
	int result = ::ioctlsocket (socket, FIONREAD, &arg);
	bytesAvailable = arg;
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Win32Socket::wouldBlockOperation (bool writeDirection) const
{
	int errorCode = getErrorCode ();
	return errorCode == WSAEWOULDBLOCK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Win32Socket::getErrorCode () const
{
	return ::WSAGetLastError ();
}
