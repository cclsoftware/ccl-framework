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
// Filename    : core/platform/shared/posix/corenetwork.posix.h
// Description : POSIX Network Functions
//
//************************************************************************************************

#ifndef _corenetwork_posix_h
#define _corenetwork_posix_h

#include "core/platform/corefeatures.h"

#if CORE_NETWORK_IMPLEMENTATION == CORE_FEATURE_IMPLEMENTATION_POSIX
#include <ifaddrs.h>
#include <sys/fcntl.h>
#include <net/if.h> // For IFF_LOOPBACK, etc

#define CORE_IFADDRS_HAVE_DATA_MEMBER 1
#define CORE_IFADDRS_HAVE_NETMASK_MEMBER 1
#endif

#include "core/platform/shared/coreplatformnetwork.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>

#define SD_BOTH SHUT_RDWR

#ifndef SOCKET_ERROR
#define SOCKET_ERROR -1
#endif
#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif
#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif
#ifndef IFF_RUNNING
#define IFF_RUNNING 0x40
#endif

namespace Core {
namespace Platform {

typedef sockaddr NativeSocketAddress;

//************************************************************************************************
// PosixNetwork
//************************************************************************************************

class PosixNetwork: public INetwork
{
	// INetwork
	bool startup () override;
	void shutdown () override;
	bool getLocalHostname (CString256& hostname) override;
	bool getLocalIPAddress (Sockets::IPAddress& address) override;
	bool getInterfaceNameForIP (CString32& interfaceName, const Sockets::IPAddress& ip) override;
	bool getLocalMACAddress (uint8 mac[6]) override;
	bool getLocalMACAddress (CString32& address) override;
	void getMACAddressString (CString32& address, const uint8 mac[6]) override;
	bool getAddressByHost (Sockets::SocketAddress& address, CStringPtr hostname) override;
	bool getHostByAddress (CString256& hostname, const Sockets::SocketAddress& address) override;
	bool getAddressString (CString256& string, const Sockets::SocketAddress& address) override;
	bool getAddressFromString (Sockets::SocketAddress& address, CStringPtr string) override;
};

//************************************************************************************************
// SocketAddressConverter
/** Helper class to convert from Core-style to native address. */
//************************************************************************************************

#if CORE_NETWORK_IMPLEMENTATION == CORE_FEATURE_IMPLEMENTATION_POSIX
typedef SocketAddressConverter PosixSocketAddressConverter;
#endif

//************************************************************************************************
// PosixAdapterIterator
//************************************************************************************************

class PosixAdapterIterator: public IAdapterIterator<ifaddrs>
{
public:
	PosixAdapterIterator ();
	~PosixAdapterIterator ();
	
	// IAdapterIterator
	const Entry* next () override;
	bool matches (const Entry* entry) const override;
	bool getIPAddress (Sockets::IPAddress& address, const Entry* entry) const override;
	bool getIPSubnetMask (Sockets::IPAddress& address, const Entry* entry) const override;

protected:
	Entry* first;
	Entry* current;
};

#if CORE_NETWORK_IMPLEMENTATION == CORE_FEATURE_IMPLEMENTATION_POSIX
typedef PosixAdapterIterator AdapterIterator;
#endif

} // namespace Platform
} // namespace Core

#endif // _corenetwork_posix_h
