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
// Filename    : ccl/network/netsocket.h
// Description : Socket class
//
//************************************************************************************************

#ifndef _ccl_netsocket_h
#define _ccl_netsocket_h

#include "ccl/public/base/unknown.h"
#include "ccl/public/network/isocket.h"

#include "ccl/public/base/ccldefpush.h"
#include "core/network/coresocket.h"
#include "ccl/public/base/ccldefpop.h"

#define kNetworkTextEncoding Text::kSystemEncoding	///< C-String encoding for host names, etc.

namespace CCL {
namespace Net {

using Core::Sockets::SocketID;
using Core::Sockets::SocketIDSet;

//************************************************************************************************
// BaseSocket
//************************************************************************************************

class BaseSocket: public Unknown,
				  public ISocket
{
public:
	CLASS_INTERFACE (ISocket, Unknown)

protected:
	static CCL::tresult handleError (Core::Sockets::Socket& coreSocket, const char* debugMessage);
	static CCL::tresult translateErrorCode (int nativeError);
	static CCL::String getErrorString (int nativeError);
};

//************************************************************************************************
// Socket
//************************************************************************************************

class Socket: public BaseSocket,
			  public IMulticastSocket
{
public:
	static Socket* createSocket (AddressFamily addressFamily, SocketType type, ProtocolType protocol);

	SocketID getDescriptor () const;

	// ISocket
	tresult CCL_API connect (const SocketAddress& address) override;
	tresult CCL_API disconnect () override;
	tbool CCL_API isConnected () override;
	tresult CCL_API getPeerAddress (SocketAddress& address) override;
	tresult CCL_API bind (const SocketAddress& address) override;
	tresult CCL_API listen (int maxConnections) override;
	ISocket* CCL_API accept () override;
	tresult CCL_API getLocalAddress (SocketAddress& address) override;
	tresult CCL_API setOption (int option, VariantRef value) override;
	tbool CCL_API isReadable (int timeout = 0) override;
	tbool CCL_API isWritable (int timeout = 0) override;
	tbool CCL_API isAnyError (int timeout = 0) override;
	int CCL_API send (const void* buffer, int size, int flags = 0) override;
	int CCL_API receive (void* buffer, int size, int flags = 0) override;
	int CCL_API sendTo (const void* buffer, int size, const SocketAddress& address, int flags = 0) override;
	int CCL_API receiveFrom (void* buffer, int size, SocketAddress& address, int flags = 0) override;
	tbool CCL_API wouldBlockOperation (tbool writeDirection) override;

	// IMulticastSocket
	tresult CCL_API joinMulticastGroup (const IPAddress& groupAddress, const IPAddress& adapterAddress) override;
	tresult CCL_API leaveMulticastGroup (const IPAddress& groupAddress, const IPAddress& adapterAddress) override;

	// for casting IUnknown to Socket, only used internally
	DECLARE_IID (Socket)

	CLASS_INTERFACE2 (Socket, IMulticastSocket, BaseSocket)

protected:
	Socket (SocketID descriptor);

	Core::Sockets::Socket coreSocket;

	CCL::tresult handleError (const char* debugMessage);
};

} // namespace Net
} // namespace CCL

#endif // _ccl_netsocket_h
