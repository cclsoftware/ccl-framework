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
// Filename    : core/platform/android/coresocket.android.cpp
// Description : Android Socket Functions
//
//************************************************************************************************

#include "coresocket.android.h"

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
// AndroidSocket
//************************************************************************************************

AndroidSocket::AndroidSocket (SocketID socket)
: PosixSocket (socket)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

AndroidSocket::AndroidSocket (AddressFamily addressFamily, SocketType type, ProtocolType protocol)
: PosixSocket (addressFamily, type, protocol)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

int AndroidSocket::setMulticastMembership (uint32 address, bool state)
{
	int result = SOCKET_ERROR;
	int option = state ? IP_ADD_MEMBERSHIP : IP_DROP_MEMBERSHIP;

	ip_mreq mreq;
	mreq.imr_multiaddr.s_addr = htonl (address);

	// On Android, we might have a cellular connection, which we don't want to be
	// using for multicast.. so, iterate through each IP, trying this until it returns
	// a safe value.

	// we want to try the last ip, first...
	// testing has shown that chances are that's the one we need to use, anyway.

	Vector<IPAddress> ips;
	Sockets::Network::getLocalIPAddressList (ips);
	for(int i = ips.count () - 1; i >= 0; i--)
	{
		mreq.imr_interface.s_addr = htonl (ips[i].getIPv4 ());
		result = ::setsockopt (int(socket), IPPROTO_IP, option, (const char*)&mreq, sizeof(mreq));
		if(result == 0)
			break;
	}

	return result;
}
