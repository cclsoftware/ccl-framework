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
// Filename    : ccl/network/netsslsocket.h
// Description : SSL Socket class
//
//************************************************************************************************

#define DEBUG_SSL 0

#ifndef _ccl_netsslsocket_h
#define _ccl_netsslsocket_h

#include "ccl/network/netsocket.h"

#include "core/network/coresslsocket.h"

using namespace Core::Sockets::SSLTypes;

namespace CCL {
namespace Net {

#if DEBUG_SSL
class CoreSSLSocketDebug : public Core::Sockets::SSLSocket
{
public:
	CoreSSLSocketDebug (AddressFamily addressFamily = kInternet) : SSLSocket (addressFamily) {;}
	virtual void debugMessage (const char* s, int i) {CCL::Debugger::warn (s, i);}
};
#endif

//************************************************************************************************
// SSLSocket
//************************************************************************************************

class SSLSocket: public BaseSocket
{
public:
	static SSLSocket* createSocket (AddressFamily addressFamily);

	void setPeerName (StringRef peerName);

	tresult connect (const SocketAddress& address, CCL::IProgressNotify* progress);

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

protected:
	SSLSocket (AddressFamily addressFamily = kInternet);
	~SSLSocket ();
	
	#if DEBUG_SSL
	CoreSSLSocketDebug coreSSLSocket;
	#else
	Core::Sockets::SSLSocket coreSSLSocket;
	#endif
	SSLResult lastSSLResult;

	void setLastResult (SSLResult result);
	CCL::tresult handleError (const char* debugMessage);
};

} // namespace Net
} // namespace CCL

#endif // _ccl_netsslsocket_h
