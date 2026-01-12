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
// Filename    : core/platform/win/corenetwork.win.h
// Description : Windows Network Functions
//
//************************************************************************************************

#ifndef _corenetwork_win_h
#define _corenetwork_win_h

#include "core/platform/shared/coreplatformnetwork.h"

#ifdef _WINSOCKAPI_
#pragma message ("warning: including windows.h before including corenetwork.h might cause symbol redefinition errors.")
#endif

#include <winsock2.h>
#include <iphlpapi.h> // needed for GetAdaptersAddresses and IP_ADAPTER_ADDRESSES

namespace Core {
namespace Platform {

typedef sockaddr NativeSocketAddress;

//************************************************************************************************
// Win32Network
//************************************************************************************************

class Win32Network: public INetwork
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
// Win32AdapterIterator
//************************************************************************************************

class Win32AdapterIterator: public IAdapterIterator<IP_ADAPTER_ADDRESSES>
{
public:
	Win32AdapterIterator ();
	~Win32AdapterIterator ();
	
	// IAdapterIterator
	const Entry* next () override;
	bool matches (const Entry* entry) const override;
	bool getIPAddress (Sockets::IPAddress& address, const Entry* entry) const override;
	bool getIPSubnetMask (Sockets::IPAddress& address, const Entry* entry) const override;

protected:
	Entry* first;
	Entry* current;
};

typedef Win32AdapterIterator AdapterIterator;

} // namespace Platform
} // namespace Core

#endif // _corenetwork_win_h
