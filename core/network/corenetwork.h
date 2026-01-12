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
// Filename    : core/network/corenetwork.h
// Description : Network Functions
//
//************************************************************************************************

#ifndef _corenetwork_h
#define _corenetwork_h

#include "core/platform/corefeatures.h"

#if CORE_NETWORK_IMPLEMENTATION == CORE_PLATFORM_IMPLEMENTATION
	#include CORE_PLATFORM_IMPLEMENTATION_HEADER (corenetwork)
#elif CORE_NETWORK_IMPLEMENTATION == CORE_EXTERNAL_PLATFORM_IMPLEMENTATION
	#include CORE_EXTERNAL_PLATFORM_IMPLEMENTATION_HEADER (corenetwork)
#elif CORE_NETWORK_IMPLEMENTATION == CORE_FEATURE_IMPLEMENTATION_POSIX
	#include "core/platform/shared/posix/corenetwork.posix.h"
#elif CORE_NETWORK_IMPLEMENTATION == CORE_FEATURE_IMPLEMENTATION_LWIP
	#include "core/platform/shared/lwip/corenetwork.lwip.h"
#endif

#include "core/public/coresocketaddress.h"

namespace Core {
namespace Sockets {

//************************************************************************************************
// Network
/** Network utility functions. */
//************************************************************************************************

namespace Network
{
	/** Start underlying network APIs (WSAStartup on Windows). */
	bool startup ();
	
	/** Shutdown underlying network APIs. */
	void shutdown ();

	/** Get local host name. */
	bool getLocalHostname (CString256& hostname);
	
	/** Get local IP address. */
	bool getLocalIPAddress (IPAddress& address);
	
	/** Get list of all local IP addresses. */
	void getLocalIPAddressList (Vector<IPAddress>& addressList);
	int getLocalIPAddressList (IPAddress addressList[], int maxCount);

	/** Get list of all local IP addresses. Return unique subnet addresses only. */
	int getLocalIPAddressListUnique (IPAddress addressList[], int maxCount);

	/** Get network adapter name for local IP address. */
	bool getInterfaceNameForIP (CString32& interfaceName, const IPAddress& ip);
	
	/** Get local MAC address. */
	bool getLocalMACAddress (uint8 mac[6]);
	bool getLocalMACAddress (CString32& address);

	/** Convert MAC address to string representation. */
	void getMACAddressString (CString32& address, const uint8 mac[6]);

	/** Resolve hostname to socket address. */
	bool getAddressByHost (SocketAddress& address, CStringPtr hostname);
	
	/** Resolve socket address to host name. */
	bool getHostByAddress (CString256& hostname, const SocketAddress& address);
	
	/** Convert socket address to string representation. */
	bool getAddressString (CString256& string, const SocketAddress& address);
	
	/** Scan socket address from string representation. */
	bool getAddressFromString (SocketAddress& address, CStringPtr string);
}

//************************************************************************************************
// AdapterIterator
//************************************************************************************************

class AdapterIterator
{
public:
	typedef Platform::AdapterIterator::Entry Entry;
	
	const Entry* next ();
	bool matches (const Entry* entry) const;
	bool getIPAddress (IPAddress& address, const Entry* entry) const;
	bool getIPSubnetMask (IPAddress& address, const Entry* entry) const;

protected:
	Platform::AdapterIterator platformIterator;
};

//************************************************************************************************
// Network implementation
//************************************************************************************************

inline bool Network::startup () 
{ return Platform::Network::instance ().startup (); }

inline void Network::shutdown ()
{ Platform::Network::instance ().shutdown (); }

inline bool Network::getLocalHostname (CString256& hostname)
{ return Platform::Network::instance ().getLocalHostname (hostname); }

inline bool Network::getLocalIPAddress (IPAddress& address)
{ return Platform::Network::instance ().getLocalIPAddress (address); }

inline bool Network::getInterfaceNameForIP (CString32& interfaceName, const IPAddress& ip)
{ return Platform::Network::instance ().getInterfaceNameForIP (interfaceName, ip); }

inline bool Network::getLocalMACAddress (uint8 mac[6])
{ return Platform::Network::instance ().getLocalMACAddress (mac); }

inline bool Network::getLocalMACAddress (CString32& address)
{ return Platform::Network::instance ().getLocalMACAddress (address); }

inline void Network::getMACAddressString (CString32& address, const uint8 mac[6])
{ return Platform::Network::instance ().getMACAddressString (address, mac); }

inline bool Network::getAddressByHost (SocketAddress& address, CStringPtr hostname)
{ return Platform::Network::instance ().getAddressByHost (address, hostname); }

inline bool Network::getHostByAddress (CString256& hostname, const SocketAddress& address)
{ return Platform::Network::instance ().getHostByAddress (hostname, address); } 

inline bool Network::getAddressString (CString256& string, const SocketAddress& address)
{ return Platform::Network::instance ().getAddressString (string, address); }

inline bool Network::getAddressFromString (SocketAddress& address, CStringPtr string)
{ return Platform::Network::instance ().getAddressFromString (address, string); }

//************************************************************************************************
// AdapterIterator implementation
//************************************************************************************************

inline const AdapterIterator::Entry* AdapterIterator::next ()
{ return platformIterator.next (); }

inline bool AdapterIterator::matches (const Entry* entry) const
{ return platformIterator.matches (entry); }

inline bool AdapterIterator::getIPAddress (IPAddress& address, const Entry* entry) const
{ return platformIterator.getIPAddress (address, entry); }

inline bool AdapterIterator::getIPSubnetMask (IPAddress& address, const Entry* entry) const
{ return platformIterator.getIPSubnetMask (address, entry); }

} // namespace Sockets
} // namespace Core

#endif // _corenetwork_h

