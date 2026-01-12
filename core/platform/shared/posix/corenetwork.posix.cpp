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
// Filename    : core/platform/shared/posix/corenetwork.posix.cpp
// Description : POSIX Network Functions
//
//************************************************************************************************

#include "core/platform/corefeatures.h"

#if CORE_NETWORK_IMPLEMENTATION == CORE_PLATFORM_IMPLEMENTATION
#include CORE_PLATFORM_IMPLEMENTATION_HEADER (corenetwork)
#elif CORE_NETWORK_IMPLEMENTATION == CORE_FEATURE_IMPLEMENTATION_LWIP
#include "core/platform/shared/lwip/corenetwork.lwip.h"
#else
#include "core/platform/shared/posix/corenetwork.posix.h"
#endif

namespace Core {
namespace Platform {

//************************************************************************************************
// PosixSocketAddressConverter
//************************************************************************************************

bool PosixSocketAddressConverter::toSocketAddress (Sockets::SocketAddress& dst)
{
	const NativeSocketAddress& src = *reinterpret_cast<NativeSocketAddress*> (buffer);

	// Internet-style address
	if(src.sa_family == AF_INET || src.sa_family == AF_INET6)
	{
		ASSERT (dst.byteSize == sizeof(Sockets::IPAddress))
		if(dst.byteSize != sizeof(Sockets::IPAddress))
			return false;

		Sockets::IPAddress& dstIP = (Sockets::IPAddress&)dst;

		// IPv4
		if(src.sa_family == AF_INET)
		{
			dstIP.family = Sockets::kInternet;
		
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
			dstIP.family = Sockets::kInternetV6;
			
			ASSERT (size >= static_cast<int> (sizeof(sockaddr_in6)))
			if(size < static_cast<int> (sizeof(sockaddr_in6)))
				return false;

			sockaddr_in6& srcIP = (sockaddr_in6&)src;
			dstIP.port = ntohs (srcIP.sin6_port);
			memcpy (dstIP.ipv6.address, &srcIP.sin6_addr, 16);
			dstIP.ipv6.flowinfo = ntohl (srcIP.sin6_flowinfo);
			dstIP.ipv6.scopeid = ntohl (srcIP.sin6_scope_id);
		}

		return true; // success ;-)
	}

	// no other address families implemented!
	ASSERT (0)
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixSocketAddressConverter::fromSocketAddress (const Sockets::SocketAddress& src)
{
	NativeSocketAddress& dst = *reinterpret_cast<NativeSocketAddress*> (buffer);

	// Internet-style address
	if(src.family == Sockets::kInternet || src.family == Sockets::kInternetV6)
	{
		if(src.byteSize != sizeof(Sockets::IPAddress))
			return false;

		const Sockets::IPAddress& srcIP = (const Sockets::IPAddress&)src;

		// IPv4
		if(src.family == Sockets::kInternet)
		{
			dst.sa_family = AF_INET;
		
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
			dst.sa_family = AF_INET6;
			
			ASSERT (size >= static_cast<int> (sizeof(sockaddr_in6)))
			if(size < static_cast<int> (sizeof(sockaddr_in6)))
				return false;

			sockaddr_in6& dstIP = (sockaddr_in6&)dst;
			dstIP.sin6_port = htons (srcIP.port);
			::memcpy (&dstIP.sin6_addr, srcIP.ipv6.address, 16);
			dstIP.sin6_flowinfo = htonl (srcIP.ipv6.flowinfo);
			dstIP.sin6_scope_id = htonl (srcIP.ipv6.scopeid);

			size = static_cast<int> (sizeof(sockaddr_in6));
		}

		return true; // success ;-)
	}

	// no other address families implemented!
	ASSERT (0)
	return false;
}

} // namespace Platform
} // namespace Core

using namespace Core;
using namespace Sockets;
using namespace Platform;
	
//************************************************************************************************
// Network
//************************************************************************************************

#if CORE_NETWORK_IMPLEMENTATION == CORE_FEATURE_IMPLEMENTATION_POSIX
INetwork& Network::instance () 
{ 
	static PosixNetwork theNetwork;
	return theNetwork; 
}
#endif

//************************************************************************************************
// PosixNetwork
//************************************************************************************************

bool PosixNetwork::startup ()
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PosixNetwork::shutdown ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixNetwork::getLocalHostname (CString256& hostname)
{
	return ::gethostname (hostname.getBuffer (), hostname.getSize ()) == 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixNetwork::getLocalIPAddress (IPAddress& address)
{
	AdapterIterator iter;
	const AdapterIterator::Entry* entry;
	while((entry = iter.next ()) != nullptr)
		if(iter.matches (entry))
		{
			 if(iter.getIPAddress (address, entry))
				return true;
		}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixNetwork::getInterfaceNameForIP (CString32& interfaceName, const IPAddress& ip)
{
	AdapterIterator iter;
	const AdapterIterator::Entry* entry;
	while((entry = iter.next ()) != nullptr)
		if(iter.matches (entry))
		{
			IPAddress address;
			if(iter.getIPAddress (address, entry) && address == ip)
			{
				interfaceName = entry->ifa_name;
				return true;
			}
		}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixNetwork::getLocalMACAddress (uint8 _mac[6])
{
	AdapterIterator iter;
	const AdapterIterator::Entry* entry;
	while((entry = iter.next ()) != nullptr)
	{
		const uint8* mac = nullptr;

#if CORE_IFADDRS_HAVE_DATA_MEMBER
		struct ifreq req;
		if(entry->ifa_data != nullptr)
		{
			::strcpy (req.ifr_name, entry->ifa_name);
			int sd = ::socket (PF_INET, SOCK_DGRAM, 0);
			if(::ioctl (sd, SIOCGIFHWADDR, &req) != -1)
				mac = (uint8*)req.ifr_ifru.ifru_hwaddr.sa_data;
			::close (sd);
		}
#endif

		if(mac != nullptr)
		{
			::memcpy (_mac, mac, 6);
			return true;
		}
	}

	::memset (_mac, 0, 6);
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixNetwork::getLocalMACAddress (CString32& address)
{
	uint8 mac[6] = {0};
	if(!getLocalMACAddress (mac))
		return false;
	
	getMACAddressString (address, mac);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PosixNetwork::getMACAddressString (CString32& address, const uint8 mac[6])
{
	address.empty ();
	MACAddressFormat::append (address, mac);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixNetwork::getAddressByHost (SocketAddress& address, CStringPtr hostname)
{
	addrinfo* info = nullptr;
	int result = ::getaddrinfo (hostname, nullptr, nullptr, &info);

	if(result != 0)
		return false;

	bool converted = false;
	for(int run = 1; run <= 2; run++)
	{
		if(converted)
			break;
		for(addrinfo* ptr = info; ptr != nullptr; ptr = ptr->ai_next)
		{
			int size = static_cast<int> (ptr->ai_addrlen);
			converted = SocketAddressConverter (ptr->ai_addr, size).toAddress (address);
			if(converted)
			{
				ptr->ai_addrlen = size;
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

bool PosixNetwork::getHostByAddress (CString256& hostname, const SocketAddress& address)
{
	SocketAddressConverter temp (address);
	if(!temp.valid)
		return false;

	int result = ::getnameinfo (temp.as<NativeSocketAddress> (), temp.size, hostname.getBuffer (), hostname.getSize (), nullptr, 0, 0);
	if(result != 0)
		return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixNetwork::getAddressString (CString256& string, const SocketAddress& address)
{
	SocketAddressConverter temp (address);
	if(!temp.valid)
		return false;

	// IPv4 or IPv6
	ASSERT (address.family == kInternet || address.family == kInternetV6)

	void* src = address.family == kInternet ? &temp.as<sockaddr_in> ()->sin_addr : (void*)&temp.as<sockaddr_in6> ()->sin6_addr;
	if(!::inet_ntop (address.family, src, string.getBuffer (), string.getSize ()))
		return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixNetwork::getAddressFromString (SocketAddress& address, CStringPtr string)
{
	ASSERT (address.family == kInternet || address.family == kInternetV6)
	if(!(address.family == kInternet || address.family == kInternetV6))
		return false;

	SocketAddressConverter temp (address); // init size

	void* dst = address.family == kInternet ? &temp.as<sockaddr_in> ()->sin_addr : (void*)&temp.as<sockaddr_in6> ()->sin6_addr;
	if(::inet_pton (address.family, string, dst) != 1)
		return false;

	return temp.toAddress (address);
}

//************************************************************************************************
// PosixAdapterIterator
//************************************************************************************************

PosixAdapterIterator::PosixAdapterIterator ()
: first (nullptr),
  current (nullptr)
{
	if(::getifaddrs (&first) != -1)
		current = first;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PosixAdapterIterator::~PosixAdapterIterator ()
{
	if(first)
		::freeifaddrs (first);	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const PosixAdapterIterator::Entry* PosixAdapterIterator::next ()
{
	Entry* result = current;
	current = current ? current->ifa_next : nullptr;
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixAdapterIterator::matches (const Entry* entry) const
{
	return entry->ifa_addr != nullptr && entry->ifa_addr->sa_family == AF_INET && (entry->ifa_flags & IFF_RUNNING) && !(entry->ifa_flags & IFF_LOOPBACK);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixAdapterIterator::getIPAddress (IPAddress& address, const Entry* entry) const
{
	const NativeSocketAddress* src = nullptr;
	int size = 0;

	if(entry->ifa_addr != nullptr && entry->ifa_addr->sa_family == AF_INET)
	{
		src = entry->ifa_addr;
		size = sizeof(sockaddr_in);
	}

	return SocketAddressConverter (src, size).toAddress (address);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixAdapterIterator::getIPSubnetMask (IPAddress& address, const Entry* entry) const
{
	const NativeSocketAddress* src = nullptr;
	int size = 0;

#if CORE_IFADDRS_HAVE_NETMASK_MEMBER
	if(entry->ifa_netmask != nullptr && entry->ifa_netmask->sa_family == AF_INET)
	{
		src = entry->ifa_netmask;
		size = sizeof(sockaddr_in);
	}
#endif

	return SocketAddressConverter (src, size).toAddress (address);
}
