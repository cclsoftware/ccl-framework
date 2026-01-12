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
// Filename    : ccl/public/network/inetwork.h
// Description : Network Interface
//
//************************************************************************************************

#ifndef _ccl_inetwork_h
#define _ccl_inetwork_h

#include "ccl/public/network/isocket.h"

namespace CCL {
interface IStream;
interface IUnknownList;

namespace Net {

//************************************************************************************************
// INetwork
/** Network service interface. */
//************************************************************************************************

interface INetwork: IUnknown
{
	//////////////////////////////////////////////////////////////////////////////////////////////
	// Address utilities
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Retrieve host name of local computer. */
	virtual tresult CCL_API getLocalHostname (String& hostname) = 0;

	/** Get IP address of first active adapter on this computer. */
	virtual tresult CCL_API getLocalIPAddress (IPAddress& address) = 0;

	/** Retrieve address of specified host name. */
	virtual tresult CCL_API getAddressByHost (SocketAddress& address, StringRef hostname) = 0;

	/** Perform name resolution from an address to the host name. */
	virtual tresult CCL_API getHostByAddress (String& hostname, const SocketAddress& address) = 0;

	/** Convert address to string (e.g. to IPv4 or IPv6 dotted notation). */
	virtual tresult CCL_API getAddressString (String& string, const SocketAddress& address) = 0;

	/** Convert string to address (e.g. from IPv4 or IPv6 dotted notation). */
	virtual tresult CCL_API getAddressFromString (SocketAddress& address, StringRef string) = 0;
        
	//////////////////////////////////////////////////////////////////////////////////////////////
	// Sockets/Streams
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Create socket bound to a specific service provider. */
	virtual ISocket* CCL_API createSocket (AddressFamily addressFamily, 
										   SocketType type, 
										   ProtocolType protocol) = 0;

	/** Open network stream. */
	virtual IStream* CCL_API openStream (const SocketAddress& address, 
										 ProtocolType protocol) = 0;

	/** Open SSL stream. */
	virtual IStream* CCL_API openSSLStream (const IPAddress& address, 
											StringRef peerName, 
											IProgressNotify* progress = nullptr) = 0;

	/** Determines the state of multiple sockets. On return, the lists will be filled with the matching sockets only. */
	virtual tresult CCL_API selectSockets (IUnknownList* readList, IUnknownList* writeList, IUnknownList* errorList, int timeout) = 0;

	DECLARE_IID (INetwork)
};

DEFINE_IID (INetwork, 0x9bcd3ede, 0x2a33, 0x4a9e, 0x8e, 0x4f, 0x7a, 0xe0, 0x84, 0xe5, 0xa6, 0x3a)

} // namespace Net
} // namespace CCL

#endif // _ccl_inetwork_h
