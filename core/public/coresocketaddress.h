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
// Filename    : core/public/coresocketaddress.h
// Description : Socket Address
//
//************************************************************************************************

#ifndef _coresocketaddress_h
#define _coresocketaddress_h

#include "core/public/coretypes.h"

namespace Core {
namespace Sockets {
namespace SocketTypes {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Constants
//////////////////////////////////////////////////////////////////////////////////////////////////

/** Address family. \ingroup core */
DEFINE_ENUM (AddressFamily)
{
	kInternet = 2,		///< IP version 4 (AF_INET)
	kInternetV6 = 30	///< IP version 6 (AF_INET6)
};

/** Socket type. \ingroup core */
DEFINE_ENUM (SocketType)
{
	kStream = 1,		///< stream socket (SOCK_STREAM)
	kDatagram = 2		///< datagram socket (SOCK_DGRAM)
};

/** Protocol type. \ingroup core */
DEFINE_ENUM (ProtocolType)
{
	kIPv4 = 4,			///< IPv4 (IPPROTO_IPV4)
	kTCP = 6,			///< TCP (IPPROTO_TCP)
	kUDP = 17,			///< UDP (IPPROTO_UDP)
	kIPv6 = 41			///< IPv6 (IPPROTO_IPV6)
};

/** Port number. \ingroup core */
typedef uint16 PortNumber;

//////////////////////////////////////////////////////////////////////////////////////////////////
/** Socket Options \ingroup core  */
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace SocketOption
{
	enum Limits 
	{
		kMaxConnections = 5,	///< Maximum queue length specifiable by listen() method

		kTimeoutInfinite = -1	///< Block indefinitely, use with socket select().
	};

	enum Options
	{
		kNonBlocking = 100,		///< set non-blocking mode (O_NONBLOCK)
		kBroadcast,				///< allow UDP socket to send to broadcast address (SO_BROADCAST)
		kReuseAddress,			///< allow reuse of address
		kReusePort,				///< allow reuse of port
		kSendBufferSize,		///< buffer size reserved for sends (SO_SNDBUF)
		kReceiveBufferSize,		///< buffer size reserved for receives (SO_RCVBUF)
		kTCPNoDelay				///< enable/disable the Nagle algorithm (TCP_NODELAY)
	};
}

//************************************************************************************************
// SocketAddress
/** Basic socket address description. \ingroup core */
//************************************************************************************************

struct SocketAddress
{
	uint32 byteSize;		///< byte size
	AddressFamily family;	///< address family

	SocketAddress ()
	: byteSize (0),
	  family (0)
	{}
};

//************************************************************************************************
// IPAddress
/** Internet-style address (IPv4 or IPv6). \ingroup core */
//************************************************************************************************

struct IPAddress: SocketAddress
{
	PortNumber port;			///< port number
	union
	{
		struct 
		{
			uint8 address[4];	///< 32 bit IPv4 address
		} ip;

		struct
		{
			uint8 address[16];	///< 128 bit IPv6 address
			uint32 flowinfo;	///< flow information
			uint32 scopeid;		///< scope identifier
		} ipv6;
	};

	IPAddress (AddressFamily _family = kInternet)
	{
		::memset (this, 0, sizeof(IPAddress));
		byteSize = sizeof(IPAddress);
		family = _family;
	}

	/** Cast socket address to IP address. */
	static inline IPAddress* cast (SocketAddress* addr)
	{
		return addr && addr->byteSize == sizeof(IPAddress) ? reinterpret_cast<IPAddress*> (addr) : nullptr;
	}

	/** Check if IP address is null. */
	bool isNull () const;

	/** Check if IP address is the loopback IP. */
	bool isLoopback () const;

	/** Check if this IP addresses is equal to another one. */
	bool isEqual (const IPAddress& other, const IPAddress& mask) const;

	/** Assign IPv4 address. */
	IPAddress& setIP (uint8 a, uint8 b, uint8 c, uint8 d, PortNumber port = 0);

	/** Assign IPv4 address. */
	IPAddress& setIP (uint32 value, PortNumber port = 0);

	/** Get IPv4 address as 32 bit integer. */
	uint32 getIPv4 () const;
	
	bool operator == (const IPAddress& other) const;
	bool operator != (const IPAddress& other) const;
};

//************************************************************************************************
// MACAddressFormat
/** MAC address string conversion. */
//************************************************************************************************

struct MACAddressFormat
{
	enum Format
	{
		kStandard,	///< standard MAC address format
		kSystem,	///< system MAC address format
		kCompact	///< compact MAC address format
	};

	/** Get format string. */
	static CStringPtr getFormatString (Format format)
	{
		static CStringPtr standardFormat = "%02X:%02X:%02X:%02X:%02X:%02X";
		static CStringPtr systemFormat = "%02X-%02X-%02X-%02X-%02X-%02X";
		static CStringPtr compactFormat = "%02X%02X%02X%02X%02X%02X";
		return format == kStandard ? standardFormat : format == kSystem ? systemFormat : compactFormat;
	}

	/** Append MAC address to string, works with CCL string classes. */
	template <typename StringType>
	static void append (StringType& s, const uint8 mac[6], Format format = kStandard)
	{
		s.appendFormat (getFormatString (format), 
						int(mac[0]), int(mac[1]), int(mac[2]), 
						int(mac[3]), int(mac[4]), int(mac[5]));
	}

	/** Scan MAC address from C-string. */
	static bool scan (uint8 mac[6], CStringPtr cString, Format format = kStandard)
	{
		int v[6] = {0};
		int result = ::sscanf (cString, getFormatString (format), &v[0], &v[1], &v[2], &v[3], &v[4], &v[5]);
		if(result != 6)
			return false;
		for(int i = 0; i < 6; i++)
			mac[i] = (uint8)(v[i] & 0xFF);
		return true;
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// IPAddress inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool IPAddress::isNull () const
{
	if(family == kInternet)
		return ip.address[0] == 0 && ip.address[1] == 0 && ip.address[2] == 0 && ip.address[3] == 0;
	else for(int i = 0; i < 16; i++)
		if(ipv6.address[i] != 0)
			return false;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool IPAddress::isLoopback () const
{
	if(family == kInternet)
		return ip.address[0] == 127 && ip.address[1] == 0 && ip.address[2] == 0 && ip.address[3] == 1;
	else
	{
		for(int i = 0; i < 15; i++)
			if(ipv6.address[i] != 0)
				return false;
		if(ipv6.address[15] != 1)
			return false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool IPAddress::isEqual (const IPAddress& other, const IPAddress& mask) const
{
	if(family != other.family || family != mask.family)
		return false;

	if(family == kInternet)
	{
		for(int i = 0; i < 4; i++)
		{
			if((ip.address[i] & mask.ip.address[i]) != (other.ip.address[i] & mask.ip.address[i]))
				return false;
		}
	}
	else
	{
		for(int i = 0; i < 16; i++)
		{
			if((ipv6.address[i] & mask.ipv6.address[i]) != (other.ipv6.address[i] & mask.ipv6.address[i]))
				return false;
		}
	}

	return true;
}

	
//////////////////////////////////////////////////////////////////////////////////////////////////

inline IPAddress& IPAddress::setIP (uint8 a, uint8 b, uint8 c, uint8 d, PortNumber _port)
{
	family = kInternet;
	port = _port;
	ip.address[0] = a;
	ip.address[1] = b;
	ip.address[2] = c;
	ip.address[3] = d;
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline IPAddress& IPAddress::setIP (uint32 value, PortNumber port)
{
	return setIP ((uint8)(value>>24), (uint8)(value>>16), (uint8)(value>>8), (uint8)value, port);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline uint32 IPAddress::getIPv4 () const
{
	return family == kInternet ? (ip.address[0]<<24)|(ip.address[1]<<16)|(ip.address[2]<<8)|ip.address[3] : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool IPAddress::operator == (const IPAddress& other) const
{
	if(family != other.family)
		return false;
	if(family == kInternet)
		return getIPv4 () == other.getIPv4 ();
	else
		return ::memcmp (ipv6.address, other.ipv6.address, 16) == 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool IPAddress::operator != (const IPAddress& other) const
{
	return !(*this == other);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace SocketTypes

using namespace SocketTypes;

} // namespace Sockets
} // namespace Core

#endif // _coresocketaddress_h
