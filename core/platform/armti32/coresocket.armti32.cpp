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
// Filename    : core/platform/armti32/coresocket.armti32.cpp
// Description : TI32 ARM (OMAP-L138) Socket Functions
//
//************************************************************************************************

#include "coresocket.armti32.h"

#include <signal.h>

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
using namespace Sockets;
using namespace Platform;

//************************************************************************************************
// TI32Socket
//************************************************************************************************

TI32Socket::TI32Socket (SocketID socket)
: PosixSocket (socket)
{
	disableSigpipe ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TI32Socket::TI32Socket (AddressFamily addressFamily, SocketType type, ProtocolType protocol)
: PosixSocket (addressFamily, type, protocol)
{
	disableSigpipe ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TI32Socket::disableSigpipe () const
{
	// don't generate a SIGPIPE when a socket gets
	// disconnected, but still written to...
	signal (SIGPIPE, SIG_IGN);
}
