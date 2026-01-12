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
// Filename    : core/platform/shared/lwip/corenetwork.lwip.cpp
// Description : Lightweight IP Network Functions
//
//************************************************************************************************

#include "corenetwork.lwip.h"

#include "core/public/coretypes.h"
#include "core/public/corevector.h"

using namespace Core;
using namespace Sockets;
using namespace Platform;

typedef uint8_t sa_family_t;

//************************************************************************************************
// Network
//************************************************************************************************

INetwork& Network::instance ()
{
	static LwIPNetwork theNetwork;
	return theNetwork;
}

//************************************************************************************************
// LwIPNetwork
//************************************************************************************************

bool LwIPNetwork::startup ()
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LwIPNetwork::shutdown ()
{}
    
//////////////////////////////////////////////////////////////////////////////////////////////////

bool LwIPNetwork::getLocalHostname (CString256& hostname)
{
	AdapterIterator iter;
	const AdapterIterator::Entry* entry;
	while((entry = iter.next ()) != 0)
	{
		if(iter.matches (entry))
		{
			strcpy (hostname.getBuffer (), netif_get_hostname (entry));
			return true;
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LwIPNetwork::getLocalIPAddress (IPAddress& address)
{
	AdapterIterator iter;
	const AdapterIterator::Entry* entry;
	while((entry = iter.next ()) != 0)
		if(iter.matches (entry))
		{
			 if(iter.getIPAddress (address, entry))
				return true;
		}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LwIPNetwork::getInterfaceNameForIP (CString32& interfaceName, const IPAddress& ip)
{
	LwIPAdapterIterator iter;
	const LwIPAdapterIterator::Entry* entry;
	while((entry = iter.next ()) != 0)
		if(iter.matches (entry))
		{
			IPAddress address;
			if(iter.getIPAddress (address, entry) && address == ip)
			{
				interfaceName = netif_get_hostname (entry);
				return true;
			}
		}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LwIPNetwork::getLocalMACAddress (uint8 _mac[6])
{
	::memset (_mac, 0, 6);
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LwIPNetwork::getLocalMACAddress (CString32& address)
{
	uint8 mac[6] = {0};
	if(!getLocalMACAddress (mac))
		return false;
	
	getMACAddressString (address, mac);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LwIPNetwork::getMACAddressString (CString32& address, const uint8 mac[6])
{
	address.empty ();
	MACAddressFormat::append (address, mac);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LwIPNetwork::getAddressByHost (SocketAddress& address, CStringPtr hostname)
{
	addrinfo* info = 0;
	int result = ::getaddrinfo (hostname, 0, 0, &info);

	if(result != 0)
		return false;

	bool converted = false;
	for(int run = 1; run <= 2; run++)
	{
		if(converted)
			break;
		for(const addrinfo* ptr = info; ptr != 0; ptr = ptr->ai_next)
		{
			int size = static_cast<int> (ptr->ai_addrlen);
			converted = SocketAddressConverter (ptr->ai_addr, size).toAddress (address);
			if(converted)
			{
				if(run == 1 && address.family != kInternet) // prefer IPv4 address
				{
					converted = false;
					continue;
				}
				break;
			}
		}
	}

	::freeaddrinfo (info);
	return converted;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LwIPNetwork::getHostByAddress (CString256& hostname, const SocketAddress& address)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LwIPNetwork::getAddressString (CString256& string, const SocketAddress& address)
{
	SocketAddressConverter temp (address);
	if(!temp.valid)
		return false;

	// IPv4 or IPv6
	ASSERT (address.family == kInternet || address.family == kInternetV6)

	void* src =  (void*)&temp.as<sockaddr_in> ()->sin_addr;
	if(ipaddr_ntoa_r ((ip_addr*)src, string.getBuffer (), string.getSize ()) == nullptr)
	{
		return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LwIPNetwork::getAddressFromString (SocketAddress& address, CStringPtr string)
{
	ASSERT (address.family == kInternet || address.family == kInternetV6)
	if(!(address.family == kInternet || address.family == kInternetV6))
		return false;

	SocketAddressConverter temp (address); // init size

	void* dst = (void*)&temp.as<sockaddr_in> ()->sin_addr;
	if(ipaddr_aton (string, (ip_addr*)dst) == 0)
	{
		return false;
	}

	return temp.toAddress (address);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// SocketAddressConverter
//////////////////////////////////////////////////////////////////////////////////////////////////

bool SocketAddressConverter::toSocketAddress (SocketAddress& dst)
{
	const NativeSocketAddress& src = *reinterpret_cast<NativeSocketAddress*> (buffer);
	
	// Internet-style address
	if(src.sa_family == kInternet || src.sa_family == kInternetV6)
	{
		ASSERT (dst.byteSize == sizeof(IPAddress))
		if(dst.byteSize != sizeof(IPAddress))
			return false;

		IPAddress& dstIP = (IPAddress&)dst;
		dstIP.family = src.sa_family;

		// IPv4
		if(src.sa_family == kInternet)
		{
			ASSERT (size >= static_cast<int> (sizeof(sockaddr_in)))
			if(size < static_cast<int> (sizeof(sockaddr_in)))
				return false;

			sockaddr_in& srcIP = (sockaddr_in&)src;
			dstIP.port = ntohs (srcIP.sin_port);
			::memcpy (dstIP.ip.address, &srcIP.sin_addr.s_addr, 4);
		}
		// IPv6
		else
		{
			// not implemented
		}

		return true; // success ;-)
	}

	// no other address families implemented!
	ASSERT (0)
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SocketAddressConverter::fromSocketAddress (const SocketAddress& src)
{
	NativeSocketAddress& dst = *reinterpret_cast<NativeSocketAddress*> (buffer);
	
	// Internet-style address
	if(src.family == kInternet || src.family == kInternetV6)
	{
		if(src.byteSize != sizeof(IPAddress))
			return false;

		const IPAddress& srcIP = (const IPAddress&)src;
		dst.sa_family = (sa_family_t)src.family;

		// IPv4
		if(src.family == kInternet)
		{
			ASSERT (size >= static_cast<int> (sizeof(sockaddr_in)))
			if(size < static_cast<int> (sizeof(sockaddr_in)))
				return false;

			sockaddr_in& dstIP = (sockaddr_in&)dst;
			dstIP.sin_port = htons (srcIP.port);
			::memcpy (&dstIP.sin_addr.s_addr, srcIP.ip.address, 4);

			size = static_cast<int>(sizeof(sockaddr_in));
		}
		// IPv6
		else
		{
			// not implemented
		}

		return true; // success ;-)
	}

	// no other address families implemented!
	ASSERT (0)
	return false;
}

//************************************************************************************************
// LwIPAdapterIterator
//************************************************************************************************

LwIPAdapterIterator::LwIPAdapterIterator ()
: first (0),
  current (0)
{
	first = netif_list;
	current = first;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LwIPAdapterIterator::~LwIPAdapterIterator ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

const LwIPAdapterIterator::Entry* LwIPAdapterIterator::next ()
{
	Entry* result = current;
	current = current ? current->next : 0;
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LwIPAdapterIterator::matches (const Entry* entry) const
{
	return netif_is_up (entry);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LwIPAdapterIterator::getIPAddress (IPAddress& address, const Entry* entry) const
{
	const NativeSocketAddress* src = 0;
	int size = 0;

	sockaddr_in socketAddress;
	socketAddress.sin_len = sizeof(NativeSocketAddress);
	socketAddress.sin_family = kInternet;
	socketAddress.sin_addr.s_addr = entry->ip_addr.addr;
	src = (const NativeSocketAddress*)&socketAddress;
	size = sizeof(NativeSocketAddress);

	return SocketAddressConverter (src, size).toAddress (address);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LwIPAdapterIterator::getIPSubnetMask (IPAddress& address, const Entry* entry) const
{
	const NativeSocketAddress* src = 0;
	int size = 0;

	sockaddr_in socketAddress;
	socketAddress.sin_len = sizeof(NativeSocketAddress);
	socketAddress.sin_family = kInternet;
	socketAddress.sin_addr.s_addr = entry->netmask.addr;
	src = reinterpret_cast<const NativeSocketAddress*>(&socketAddress);
	size = sizeof(NativeSocketAddress);

	return SocketAddressConverter (src, size).toAddress (address);
}
