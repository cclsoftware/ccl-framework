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
// Filename    : ccl/network/network.h
// Description : Network class
//
//************************************************************************************************

#ifndef _ccl_network_h
#define _ccl_network_h

#include "ccl/public/base/unknown.h"

#include "ccl/public/network/inetwork.h"

namespace CCL {
namespace Net {

//************************************************************************************************
// Network
//************************************************************************************************

class Network: public Unknown,
			   public INetwork
{
public:
	bool startup ();
	void shutdown ();

	// INetwork
	tresult CCL_API getLocalHostname (String& hostname) override;
	tresult CCL_API getLocalIPAddress (IPAddress& address) override;
	tresult CCL_API getAddressByHost (SocketAddress& address, StringRef hostname) override;
	tresult CCL_API getHostByAddress (String& hostname, const SocketAddress& address) override;
	tresult CCL_API getAddressString (String& string, const SocketAddress& address) override;
	tresult CCL_API getAddressFromString (SocketAddress& address, StringRef string) override;
	ISocket* CCL_API createSocket (AddressFamily addressFamily, SocketType type, ProtocolType protocol) override;
	IStream* CCL_API openStream (const SocketAddress& address, ProtocolType protocol) override;
	tresult CCL_API selectSockets (IUnknownList* readList, IUnknownList* writeList, IUnknownList* errorList, int timeout) override;
	IStream* CCL_API openSSLStream (const IPAddress& address, StringRef peerName, IProgressNotify* progress = nullptr) override;

	CLASS_INTERFACE (INetwork, Unknown)
};

} // namespace Net
} // namespace CCL

#endif // _ccl_network_h
