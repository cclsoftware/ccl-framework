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
// Filename    : core/network/coresslsocket.h
// Description : SSL Socket class
//
//************************************************************************************************

#ifndef _coresslsocket_h
#define _coresslsocket_h

#include "core/network/coresocket.h"

#include "core/network/coresslcontext.h"

namespace Core {
namespace Sockets {

//************************************************************************************************
// SSLSocket
//************************************************************************************************

class SSLSocket: public Socket,
				 private SSLContextIOHandler
{
public:
	SSLSocket (AddressFamily addressFamily = kInternet);

	void setPeerName (CStringPtr peerName);
	
	SSLResult handshake ();
	SSLResult close ();	
	
	SSLResult sendSSL (const void* buffer, int size, int& bytesSend);
	SSLResult receiveSSL (void* buffer, int size, int& bytesReceived);
	
protected:
	SSLContext ssl;
	
	// SSLContextIOHandler
	SSLResult write (const void* buffer, int size, int& bytesWritten) override;
	SSLResult read (void* buffer, int size, int& bytesRead) override;
};

} // namespace Sockets
} // namespace Core

#endif // _coresslsocket_h
