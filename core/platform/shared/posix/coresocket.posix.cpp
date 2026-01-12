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
// Filename    : core/platform/shared/posix/coresocket.posix.cpp
// Description : Posix Socket Functions
//
//************************************************************************************************

#include "core/platform/corefeatures.h"

#if CORE_SOCKET_IMPLEMENTATION == CORE_PLATFORM_IMPLEMENTATION
#include CORE_PLATFORM_IMPLEMENTATION_HEADER (coresocket)
#elif CORE_SOCKET_IMPLEMENTATION == CORE_FEATURE_IMPLEMENTATION_LWIP
#include "core/platform/shared/lwip/coresocket.lwip.h"
#else
#include "core/platform/shared/posix/coresocket.posix.h"
#endif

#include "core/system/corethread.h"

#if CORE_SOCKET_IMPLEMENTATION == CORE_FEATURE_IMPLEMENTATION_POSIX
#define TCP_NODELAY 1
#endif

#ifndef SOCKET_DEFINED
#define SOCKET_DEFINED 1
#define SOCKET int
#endif

namespace Core {
namespace Platform {

//************************************************************************************************
// PosixSocketSets
//************************************************************************************************

namespace PosixSocketSets
{
	int select (SocketID highestSocket, fd_set* readList, fd_set* writeList, fd_set* errorList, int timeout_ms)
	{
		timeval timeout = { 0,0 };
		timeval* timeoutArg = nullptr;

		if(timeout_ms >= 0)
		{
			timeout.tv_sec = timeout_ms / 1000;
			timeout.tv_usec = (timeout_ms % 1000) * 1000;
			timeoutArg = &timeout;
		}

		int result = ::select (static_cast<int> (highestSocket + 1), readList, writeList, errorList, timeoutArg);
		return result < 0 ? SOCKET_ERROR : result;
	}
}

//************************************************************************************************
// SocketSets
//************************************************************************************************

#if CORE_SOCKET_IMPLEMENTATION == CORE_FEATURE_IMPLEMENTATION_POSIX
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
#endif

} // namespace Platform
} // namespace Core

using namespace Core;
using namespace Sockets;
using namespace Platform;

//************************************************************************************************
// PosixSocket
//************************************************************************************************

PosixSocket::PosixSocket (SocketID socket)
: socket (socket),
  connected (false)
{
	ASSERT (socket != INVALID_SOCKET)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PosixSocket::PosixSocket (AddressFamily addressFamily, SocketType type, ProtocolType protocol)
: socket (::socket (addressFamily, type, protocol)),
  connected (false)
{
	ASSERT (socket != INVALID_SOCKET)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PosixSocket::~PosixSocket ()
{
	ASSERT (connected == false)
	if(connected)
		disconnect ();

	if(socket != INVALID_SOCKET)
	{
		int result = ::close (static_cast<SOCKET> (socket));
		if(result != 0) // use an if to ignore the unused variable warning
		{
			ASSERT (result == 0)
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SocketID PosixSocket::getDescriptor () const
{
	return socket;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixSocket::connect (const SocketAddress& address)
{
	ASSERT (connected == false)
	if(connected)
		return false;

	Platform::SocketAddressConverter temp (address);
	if(!temp.valid)
		return false;

	int result = ::connect (static_cast<SOCKET> (socket), temp.as<NativeSocketAddress> (), temp.size);
	if(result != 0)
	{
		if(!wouldBlockOperation (true))
			return false;
	}

	connected = true;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixSocket::disconnect ()
{
	if(connected)
	{
		int result = ::shutdown (static_cast<SOCKET> (socket), SD_BOTH);
		if(result != 0)
		{
			if(!wouldBlockOperation (true))
			{
				int errorCode = getErrorCode ();

				// handle socket already disconnected and "host is down" errors
				#if CORE_PLATFORM_WINDOWS
				if(errorCode != WSAENOTCONN && errorCode != WSAEHOSTDOWN)
				#else
				if(errorCode != ENOTCONN && errorCode != EHOSTDOWN)
				#endif
					return false;
			}
		}

		connected = false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixSocket::isConnected () const
{
	return connected;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixSocket::bind (const SocketAddress& address)
{
	Platform::SocketAddressConverter temp (address);
	if(!temp.valid)
		return false;

	int result = ::bind (static_cast<SOCKET> (socket), temp.as<NativeSocketAddress> (), temp.size);
	if(result != 0)
		return false;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixSocket::listen (int maxConnections)
{
	int result = ::listen (static_cast<SOCKET> (socket), maxConnections);
	if(result != 0)
		return false;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SocketID PosixSocket::accept ()
{
	return ::accept (static_cast<SOCKET> (socket), nullptr, nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixSocket::getLocalAddress (SocketAddress& address) const
{
	Platform::SocketAddressConverter temp;
	if(::getsockname (static_cast<SOCKET> (socket), temp.as<NativeSocketAddress> (), (socklen_t*)&temp.size) != 0)
		return false;
	temp.valid = temp.size > 0;

	return temp.toAddress (address);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixSocket::getPeerAddress (SocketAddress& address) const
{
	Platform::SocketAddressConverter temp;
	if(::getpeername (static_cast<SOCKET> (socket), temp.as<NativeSocketAddress> (), (socklen_t*)&temp.size) != 0)
		return false;
	temp.valid = temp.size > 0;

	return temp.toAddress (address);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixSocket::setOption (int option, int value)
{
	int result = SOCKET_ERROR;
	switch(option)
	{
	case SocketOption::kNonBlocking :
		result = setNonBlockingMode (value != 0);
		break;

	case SocketOption::kBroadcast :
		result = ::setsockopt (static_cast<SOCKET> (socket), SOL_SOCKET, SO_BROADCAST, (const char*)&value, sizeof(value));
		break;

	case SocketOption::kReuseAddress :
		result = ::setsockopt (static_cast<SOCKET> (socket), SOL_SOCKET, SO_REUSEADDR, (const char*)&value, sizeof(value));
		break;

	case SocketOption::kReusePort :
		// not available
		break;

	case SocketOption::kSendBufferSize :
		result = ::setsockopt (static_cast<SOCKET> (socket), SOL_SOCKET, SO_SNDBUF, (const char*)&value, sizeof(value));
		break;

	case SocketOption::kReceiveBufferSize :
		result = ::setsockopt (static_cast<SOCKET> (socket), SOL_SOCKET, SO_RCVBUF, (const char*)&value, sizeof(value));
		break;

	case SocketOption::kTCPNoDelay :
		result = ::setsockopt (static_cast<SOCKET> (socket), IPPROTO_TCP, TCP_NODELAY, (const char*)&value, sizeof(value));
		break;

	default :
		ASSERT (0) // Unknown socket option!
		break;
	}
	return result == 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixSocket::getOption (int& value, int option) const
{
	int result = SOCKET_ERROR;
	socklen_t valueSize = sizeof(value);
	switch(option)
	{
	case SocketOption::kSendBufferSize :
		result = ::getsockopt(static_cast<SOCKET> (socket), SOL_SOCKET, SO_SNDBUF, (char*)&value, &valueSize);
		break;

	case SocketOption::kReceiveBufferSize :
		result = ::getsockopt (static_cast<SOCKET> (socket), SOL_SOCKET, SO_RCVBUF, (char*)&value, &valueSize);
		break;

	default :
		ASSERT (0) // Unknown socket option!
		break;
	}
	return result == 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int PosixSocket::setNonBlockingMode (bool state)
{
#ifdef O_NONBLOCK
	int flags = ::fcntl (static_cast<SOCKET> (socket), F_GETFL, 0);
	if(flags == SOCKET_ERROR)
		return flags;

	if(state)
		flags |= O_NONBLOCK;
	else
		flags &= ~O_NONBLOCK;

	return ::fcntl (static_cast<SOCKET> (socket), F_SETFL, flags);
#else
	return SOCKET_ERROR;
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixSocket::joinMulticastGroup (const IPAddress& groupAddress, const IPAddress& adapterAddress)
{
	ASSERT (groupAddress.family == adapterAddress.family)

	int result = SOCKET_ERROR;
	if(groupAddress.family == kInternet)
	{
		if(adapterAddress.isNull ()) // LATER TODO: review original code with platform workarounds!
			result = setMulticastMembership (groupAddress.getIPv4 (), true);
		else
		{
			ip_mreq mreq;
			mreq.imr_multiaddr.s_addr = htonl (groupAddress.getIPv4 ());
			mreq.imr_interface.s_addr = htonl (adapterAddress.getIPv4 ());
			result = ::setsockopt(static_cast<SOCKET> (socket), IPPROTO_IP, IP_ADD_MEMBERSHIP, reinterpret_cast<const char*>(&mreq), sizeof(mreq));
		}
	}
	return result != SOCKET_ERROR;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixSocket::leaveMulticastGroup (const IPAddress& groupAddress, const IPAddress& adapterAddress)
{
	ASSERT (groupAddress.family == adapterAddress.family)

	int result = SOCKET_ERROR;
	if(groupAddress.family == kInternet)
	{
		if(adapterAddress.isNull ()) // LATER TODO: review original code with platform workarounds!
			result = setMulticastMembership (groupAddress.getIPv4 (), false);
		else
		{
			ip_mreq mreq;
			mreq.imr_multiaddr.s_addr = htonl (groupAddress.getIPv4 ());
			mreq.imr_interface.s_addr = htonl (adapterAddress.getIPv4 ());
			result = ::setsockopt (static_cast<SOCKET> (socket), IPPROTO_IP, IP_DROP_MEMBERSHIP, reinterpret_cast<const char*>(&mreq), sizeof(mreq));
		}
	}
	return result != SOCKET_ERROR;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int PosixSocket::setMulticastMembership (uint32 address, bool state)
{
	int result = SOCKET_ERROR;
	int option = state ? IP_ADD_MEMBERSHIP : IP_DROP_MEMBERSHIP;

	ip_mreq mreq;
	mreq.imr_multiaddr.s_addr = htonl (address);

	mreq.imr_interface.s_addr = htonl (INADDR_ANY);
	result = ::setsockopt (static_cast<SOCKET> (socket), IPPROTO_IP, option, (const char*)&mreq, sizeof(mreq));

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixSocket::isReadable (int timeout) const
{
	return checkState (kReadable, timeout) == 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixSocket::isWritable (int timeout) const
{
	return checkState (kWritable, timeout) == 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixSocket::isAnyError (int timeout) const
{
	return checkState (kAnyError, timeout) != 0; // could be 1 or SOCKET_ERROR
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int PosixSocket::send (const void* buffer, int size, int flags)
{
	int result = (int)::send (static_cast<SOCKET> (socket), (const char*)buffer, size, flags);
	if(result == SOCKET_ERROR)
	{
		// reset connection flag to give higher level a chance to handle the situation gracefully
		if(!wouldBlockOperation (true))
            connected = false;
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int PosixSocket::sendAll (const void* buffer, int size, int flags)
{
	int bytesToSend = size;
	const char* charBuffer = (const char*)buffer;
	while(bytesToSend > 0)
	{
		int result = send (charBuffer, bytesToSend, flags);

		if(result == SOCKET_ERROR)
		{		
			if(!wouldBlockOperation (true))
				return SOCKET_ERROR;
			Threads::CurrentThread::sleep (1);
		}
		else
		{
			bytesToSend -= result;
			charBuffer += result;
		}
	}
	return size;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int PosixSocket::receive (void* buffer, int size, int flags)
{
	int result = (int)::recv (static_cast<SOCKET> (socket), (char*)buffer, size, flags);
	if(result == SOCKET_ERROR)
	{
		// reset connection flag to give higher level a chance to handle the situation gracefully
		if(!wouldBlockOperation (false))
            connected = false;
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int PosixSocket::getBytesAvailable (int& bytesAvailable)
{
	return ::ioctl (static_cast<SOCKET> (socket), FIONREAD, &bytesAvailable);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int PosixSocket::sendTo (const void* buffer, int size, const SocketAddress& address, int flags)
{
	Platform::SocketAddressConverter temp (address);
	if(!temp.valid)
		return SOCKET_ERROR;

	int result = (int)::sendto (static_cast<SOCKET> (socket), (const char*)buffer, size, flags, temp.as<NativeSocketAddress> (), temp.size);
	if(result == SOCKET_ERROR)
	{
		// reset connection flag to give higher level a chance to handle the situation gracefully
		if(!wouldBlockOperation (true))
			connected = false;
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int PosixSocket::receiveFrom (void* buffer, int size, SocketAddress& address, int flags)
{
	Platform::SocketAddressConverter temp (address);
	if(!temp.valid)
		return SOCKET_ERROR;

	socklen_t addrSize = temp.size;
	int result = (int)::recvfrom (static_cast<SOCKET> (socket), (char*)buffer, size, flags, temp.as<NativeSocketAddress> (), &addrSize);
	if(result == SOCKET_ERROR)
	{
		// reset connection flag to give higher level a chance to handle the situation gracefully
		if(!wouldBlockOperation (true))
			connected = false;
	}
	temp.toAddress (address);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixSocket::wouldBlockOperation (bool writeDirection) const
{
	int errorCode = getErrorCode ();
	if(errorCode == EAGAIN || errorCode == EWOULDBLOCK || errorCode == EINPROGRESS)
		return true;
	if(writeDirection && errorCode == ETIMEDOUT)
		return true;
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int PosixSocket::checkState (CheckHint hint, int timeout) const
{
	SocketIDSet *readArg = nullptr, *writeArg = nullptr, *errorArg = nullptr;
	
	fd_set readFd, writeFd, errorFd;
	::memset (&readFd, 0, sizeof(readFd));
	::memset (&writeFd, 0, sizeof(writeFd));
	::memset (&errorFd, 0, sizeof(errorFd));

	SocketIDSet readList (readFd), writeList (writeFd), errorList (errorFd);
	
	switch(hint)
	{
	case kReadable: readList.set (socket); readArg = &readList; break;
	case kWritable: writeList.set (socket); writeArg = &writeList; break;
	case kAnyError: errorList.set (socket); errorArg = &errorList; break;
	}

	int result = SocketSets::select (socket, readArg, writeArg, errorArg, timeout);
	if(result < 0)
		return result;

	if(checkForError ())
		return SOCKET_ERROR;

	switch(hint)
	{
	case kReadable: if(readList.isSet (socket)) return 1; break;
	case kWritable: if(writeList.isSet (socket)) return 1; break;
	case kAnyError: if(errorList.isSet (socket)) return 1; break;
	}
	
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixSocket::checkForError () const
{
	int value = 0;
	socklen_t size = sizeof(value);

	return ::getsockopt (static_cast<SOCKET> (socket), SOL_SOCKET, SO_ERROR, (char*)&value, &size) < 0 || value != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int PosixSocket::getErrorCode () const
{
	return errno;
}
