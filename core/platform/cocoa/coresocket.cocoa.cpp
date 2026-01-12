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
// Filename    : core/platform/cocoa/coresocket.cocoa.cpp
// Description : Cocoa Socket Functions
//
//************************************************************************************************

#include "coresocket.cocoa.h"

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
// CocoaSocket
//************************************************************************************************

CocoaSocket::CocoaSocket (SocketID socket)
: PosixSocket (socket)
{
	disableSigpipe ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CocoaSocket::CocoaSocket (AddressFamily addressFamily, SocketType type, ProtocolType protocol)
: PosixSocket (addressFamily, type, protocol)
{
	disableSigpipe ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaSocket::disableSigpipe () const
{
	// don't generate a SIGPIPE when a socket gets
	// disconnected, but still written to...
	int set = 1;
	::setsockopt ((int)socket, SOL_SOCKET, SO_NOSIGPIPE, (void*)&set, sizeof(int));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CocoaSocket::setOption (int option, int value)
{
	if(option == SocketOption::kReusePort)
		return ::setsockopt ((int)socket, SOL_SOCKET, SO_REUSEPORT, (const char*)&value, sizeof(value)) == 0;
	
	return PosixSocket::setOption (option, value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CocoaSocket::setMulticastMembership (uint32 address, bool state)
{
	// Reportedly there's an OSX 'bug' where although it's legal to join the same group on multiple
	// interfaces, OSX (and iOS?) spits out 'address in use' errors.
	// So, quickly drop the membership first (on this interface only!) and then do the ADD...
	if(state == true)
		setMulticastMembership (address, false);
	
	int result = SOCKET_ERROR;
	int option = state ? IP_ADD_MEMBERSHIP : IP_DROP_MEMBERSHIP;

	ip_mreq mreq;
	mreq.imr_multiaddr.s_addr = htonl (address);

	#if CORE_PLATFORM_IOS
	// On IOS, we might have a cellular connection, which we don't want to be
	// using for multicast.. so, iterate through each IP, trying this until it returns
	// a safe value.
				
	// we want to try the last ip, first...
	// testing has shown that chances are that's the one we need to use, anyway.

	Vector<IPAddress> ips;
	Sockets::Network::getLocalIPAddressList (ips);
	for(int i = ips.count ()-1; i >= 0; i--)
	{
		mreq.imr_interface.s_addr = htonl (ips[i].getIPv4 ());
		result = ::setsockopt ((int)socket, IPPROTO_IP, option, (const char*)&mreq, sizeof(mreq));
		if(result == 0)
			break;
	}

	#else
	mreq.imr_interface.s_addr = htonl (INADDR_ANY);
	result = ::setsockopt ((int)socket, IPPROTO_IP, option, (const char*)&mreq, sizeof(mreq));
	#endif

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CocoaSocket::checkState (CheckHint hint, int timeout) const
{
	pollfd fds[1];
	fds[0].fd = (int)socket;
	switch(hint)
	{
		case kReadable : fds[0].events = POLLIN;	break;
		case kWritable : fds[0].events = POLLOUT;	break;
		case kAnyError : fds[0].events = 0;			break;
	}
	
	int result = ::poll (fds, 1, timeout);
	if(fds[0].revents & (POLLERR | POLLHUP | POLLNVAL))
		return SOCKET_ERROR;

	if(checkForError ())
		return SOCKET_ERROR;
		
	return result;
}
