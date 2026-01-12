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
// Filename    : core/platform/win/corenetwork.win.cpp
// Description : Windows Network Functions
//
//************************************************************************************************

#include "corenetwork.win.h"

#include "core/public/coretypes.h"
#include "core/public/corevector.h"

#include <ws2tcpip.h> // includes sockaddr_in6

#pragma comment (lib, "Iphlpapi.lib")
#pragma comment (lib, "Ws2_32.lib")

namespace Core {
namespace Platform {

typedef ::ADDRESS_FAMILY sa_family_t;

} // namespace Platform
} // namespace Core

using namespace Core;
using namespace Sockets;
using namespace Platform;

//************************************************************************************************
// Network
//************************************************************************************************

INetwork& Network::instance ()
{
	static Win32Network theNetwork;
	return theNetwork;
}

//************************************************************************************************
// Win32Network
//************************************************************************************************

bool Win32Network::startup ()
{
	WSADATA wsaData = {0};
	if(::WSAStartup (MAKEWORD (2, 2), &wsaData) != 0)
		return false;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Win32Network::shutdown ()
{
	::WSACleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Win32Network::getLocalHostname (CString256& hostname)
{
	return ::gethostname (hostname.getBuffer (), hostname.getSize ()) == 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Win32Network::getLocalIPAddress (IPAddress& address)
{
	CString256 hostname;
	getLocalHostname (hostname);
	return getAddressByHost (address, hostname);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Win32Network::getInterfaceNameForIP (CString32& interfaceName, const IPAddress& ip)
{
	Win32AdapterIterator iter;
	const Win32AdapterIterator::Entry* entry;
	while((entry = iter.next ()) != nullptr)
		if(iter.matches (entry))
		{
			IPAddress address;
			if(iter.getIPAddress (address, entry) && address == ip)
			{
				interfaceName = entry->AdapterName;
				return true;
			}
		}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Win32Network::getLocalMACAddress (uint8 _mac[6])
{
	Win32AdapterIterator iter;
	const Win32AdapterIterator::Entry* entry;
	while((entry = iter.next ()) != nullptr)
	{
		const uint8* mac = nullptr;
		
		if(iter.matches (entry) && entry->PhysicalAddressLength == 6)
			mac = entry->PhysicalAddress;


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

bool Win32Network::getLocalMACAddress (CString32& address)
{
	uint8 mac[6] = {0};
	if(!getLocalMACAddress (mac))
		return false;
	
	getMACAddressString (address, mac);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Win32Network::getMACAddressString (CString32& address, const uint8 mac[6])
{
	address.empty ();
	MACAddressFormat::append (address, mac);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Win32Network::getAddressByHost (SocketAddress& address, CStringPtr hostname)
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

bool Win32Network::getHostByAddress (CString256& hostname, const SocketAddress& address)
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

bool Win32Network::getAddressString (CString256& string, const SocketAddress& address)
{
	SocketAddressConverter temp (address);
	if(!temp.valid)
		return false;

	// IPv4 or IPv6
	ASSERT (address.family == kInternet || address.family == kInternetV6)

	DWORD length = string.getSize ();
	if(::WSAAddressToStringA (temp.as<NativeSocketAddress> (), temp.size, nullptr, string.getBuffer (), &length) != 0)
		return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Win32Network::getAddressFromString (SocketAddress& address, CStringPtr string)
{
	ASSERT (address.family == kInternet || address.family == kInternetV6)
	if(!(address.family == kInternet || address.family == kInternetV6))
		return false;

	SocketAddressConverter temp (address); // init size

	INT length = temp.size;
	if(::WSAStringToAddressA ((LPSTR)string, address.family, nullptr, temp.as<NativeSocketAddress> (), &length) != 0)
		return false;

	return temp.toAddress (address);
}

//************************************************************************************************
// SocketAddressConverter
//************************************************************************************************

bool SocketAddressConverter::toSocketAddress (SocketAddress& dst)
{
	const NativeSocketAddress& src = *reinterpret_cast<NativeSocketAddress*> (buffer);

	// Internet-style address
	if(src.sa_family == AF_INET || src.sa_family == AF_INET6)
	{
		ASSERT (dst.byteSize == sizeof(IPAddress))
		if(dst.byteSize != sizeof(IPAddress))
			return false;

		IPAddress& dstIP = (IPAddress&)dst;

		// IPv4
		if(src.sa_family == AF_INET)
		{
			dstIP.family = kInternet;
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
			dstIP.family = kInternetV6;
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

bool SocketAddressConverter::fromSocketAddress (const SocketAddress& src)
{
	NativeSocketAddress& dst = *reinterpret_cast<NativeSocketAddress*> (buffer);

	// Internet-style address
	if(src.family == kInternet || src.family == kInternetV6)
	{
		if(src.byteSize != sizeof(IPAddress))
			return false;

		const IPAddress& srcIP = (const IPAddress&)src;

		// IPv4
		if(src.family == kInternet)
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

//************************************************************************************************
// Win32AdapterIterator
//************************************************************************************************

Win32AdapterIterator::Win32AdapterIterator ()
: first (nullptr),
  current (nullptr)
{
	ULONG family = AF_INET;
	ULONG bufferSize = 0;
	::GetAdaptersAddresses (family, 0, nullptr, nullptr, &bufferSize);	
	first = (IP_ADAPTER_ADDRESSES*)NEW char[bufferSize];
	DWORD error = ::GetAdaptersAddresses (family, 0, nullptr, first, &bufferSize);
	if(error == NO_ERROR)
		current = first;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Win32AdapterIterator::~Win32AdapterIterator ()
{
	delete [] (char*)first;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Win32AdapterIterator::Entry* Win32AdapterIterator::next ()
{
	Entry* result = current;
	current = current ? current->Next : nullptr;
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Win32AdapterIterator::matches (const Entry* entry) const
{
	return (entry->IfType == IF_TYPE_ETHERNET_CSMACD || entry->IfType == IF_TYPE_IEEE80211) && entry->OperStatus == IfOperStatusUp;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Win32AdapterIterator::getIPAddress (IPAddress& address, const Entry* entry) const
{
	const NativeSocketAddress* src = nullptr;
	int size = 0;

	if(entry->FirstUnicastAddress)
	{
		src = entry->FirstUnicastAddress->Address.lpSockaddr;
		size = entry->FirstUnicastAddress->Address.iSockaddrLength;
	}

	return SocketAddressConverter (src, size).toAddress (address);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Win32AdapterIterator::getIPSubnetMask (IPAddress& address, const Entry* entry) const
{
	const NativeSocketAddress* src = nullptr;
	int size = 0;

	if(entry->FirstUnicastAddress == nullptr)
		return false;

	uint32 mask = 0;
	uint8 prefixLength = entry->FirstUnicastAddress->OnLinkPrefixLength;
	for(int i = 31; i >= 0; i--)
	{
		if(prefixLength-- > 0)
			mask |= 1 << i;
		else
			break;
	}

	sockaddr_in socketAddress;
	socketAddress.sin_family = kInternet;
	socketAddress.sin_addr.s_addr = htonl(mask);
	src = reinterpret_cast<const NativeSocketAddress*>(&socketAddress);
	size = sizeof(NativeSocketAddress);

	return SocketAddressConverter (src, size).toAddress (address);
}
