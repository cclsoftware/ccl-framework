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
// Filename    : core/platform/shared/lwip/corenetwork.lwip.h
// Description : Lightweight IP Network Functions
//
//************************************************************************************************

#ifndef _corenetwork_lwip_h
#define _corenetwork_lwip_h

#include "core/platform/shared/coreplatformnetwork.h"

#define LWIP_COMPAT_SOCKETS 0

#include "lwip/sockets.h"
#include "lwip/netif.h"
#include "lwip/netdb.h"

#define SD_BOTH SHUT_RDWR

#ifndef SOCKET_ERROR
#define SOCKET_ERROR -1
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET (unsigned int)(~0)
#endif

#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif

#ifndef IFF_RUNNING
#define IFF_RUNNING 0x40
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////
// POSIX compatibility
//////////////////////////////////////////////////////////////////////////////////////////////////

inline int getaddrinfo (const char* nodename, const char* servname, const struct addrinfo* hints, struct addrinfo** res) { return lwip_getaddrinfo (nodename, servname, hints, res); }
inline void freeaddrinfo (struct addrinfo* ai) { lwip_freeaddrinfo (ai); }

namespace Core {
namespace Platform {

typedef sockaddr NativeSocketAddress;

//************************************************************************************************
// LwIPNetwork
//************************************************************************************************

class LwIPNetwork: public INetwork
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
// LwIPAdapterIterator
//************************************************************************************************

class LwIPAdapterIterator: public IAdapterIterator<netif>
{
public:
	LwIPAdapterIterator ();
	~LwIPAdapterIterator ();
	
	// IAdapterIterator
	const Entry* next () override;
	bool matches (const Entry* entry) const override;
	bool getIPAddress (Sockets::IPAddress& address, const Entry* entry) const override;
	bool getIPSubnetMask (Sockets::IPAddress& address, const Entry* entry) const override;

protected:
	Entry* first;
	Entry* current;
};

typedef LwIPAdapterIterator AdapterIterator;

} // namespace Platform
} // namespace Core

#endif // _corenetwork_lwip_h
