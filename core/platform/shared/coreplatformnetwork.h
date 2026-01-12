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
// Filename    : core/platform/shared/coreplatformnetwork.h
// Description : Network Functions platform implementation base
//
//************************************************************************************************

#ifndef _coreplatformnetwork_h
#define _coreplatformnetwork_h

#include "core/public/corestringbuffer.h"
#include "core/public/corevector.h"
#include "core/public/coresocketaddress.h"

namespace Core {
namespace Platform {

//************************************************************************************************
// INetwork
//************************************************************************************************

struct INetwork
{
	virtual bool startup () = 0;
	virtual void shutdown () = 0;
	virtual bool getLocalHostname (CString256& hostname) = 0;
	virtual bool getLocalIPAddress (Sockets::IPAddress& address) = 0;
	virtual bool getInterfaceNameForIP (CString32& interfaceName, const Sockets::IPAddress& ip) = 0;
	virtual bool getLocalMACAddress (uint8 mac[6]) = 0;
	virtual bool getLocalMACAddress (CString32& address) = 0;
	virtual void getMACAddressString (CString32& address, const uint8 mac[6]) = 0;
	virtual bool getAddressByHost (Sockets::SocketAddress& address, CStringPtr hostname) = 0;
	virtual bool getHostByAddress (CString256& hostname, const Sockets::SocketAddress& address) = 0;
	virtual bool getAddressString (CString256& string, const Sockets::SocketAddress& address) = 0;
	virtual bool getAddressFromString (Sockets::SocketAddress& address, CStringPtr string) = 0;
};

//************************************************************************************************
// Network
//************************************************************************************************

namespace Network
{
	INetwork& instance ();
}

//************************************************************************************************
// IAdapterIterator
//************************************************************************************************

template<typename T>
struct IAdapterIterator
{
	typedef T Entry;
	
	virtual ~IAdapterIterator () {}

	virtual const Entry* next () = 0;
	virtual bool matches (const Entry* entry) const = 0;
	virtual bool getIPAddress (Sockets::IPAddress& address, const Entry* entry) const = 0;
	virtual bool getIPSubnetMask (Sockets::IPAddress& address, const Entry* entry) const = 0;
};

//************************************************************************************************
// SocketAddressConverterBase
//************************************************************************************************

struct SocketAddressConverter
{
	char buffer[512];
	int size;
	bool valid;

	SocketAddressConverter ()
	{
		size = sizeof(buffer);
		::memset (buffer, 0, size);
		valid = false;
	}

	template<typename T>
	SocketAddressConverter (const T* address, int addresssize)
	{
		valid = false;
		if(address && addresssize <= 512)
		{
			size = addresssize;
			::memcpy (buffer, address, size);
			valid = true;
		}
	}

	SocketAddressConverter (const Sockets::SocketAddress& address)
	{
		size = sizeof(buffer);
		::memset (buffer, 0, size);
		valid = fromSocketAddress (address);
	}

	bool toAddress (Sockets::SocketAddress& address)
	{
		return valid ? toSocketAddress (address) : false;
	}

	template<typename T>
	bool toNativeAddress (T* address, int& addresssize)
	{
		if(valid == false || address == 0 || addresssize < size)
			return false;
		::memcpy (address, buffer, size);
		addresssize = size;
		return true;
	}
	
	template <typename T> T* as () { return reinterpret_cast<T*> (buffer); }

protected:
	bool toSocketAddress (Sockets::SocketAddress& dst);
	bool fromSocketAddress (const Sockets::SocketAddress& src);
};

} // namespace Platform
} // namespace Core

#endif // _coreplatformnetwork_h
