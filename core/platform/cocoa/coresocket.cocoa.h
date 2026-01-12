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
// Filename    : core/platform/cocoa/coresocket.cocoa.h
// Description : Cocoa Socket Functions
//
//************************************************************************************************

#ifndef _coresocket_cocoa_h
#define _coresocket_cocoa_h

#include "core/platform/shared/posix/coresocket.posix.h"

#include <netinet/tcp.h>

namespace Core {
namespace Platform {

//************************************************************************************************
// CocoaSocket
//************************************************************************************************

class CocoaSocket: public PosixSocket
{
public:
	CocoaSocket (SocketID socket);
	CocoaSocket (Sockets::AddressFamily addressFamily, Sockets::SocketType type, Sockets::ProtocolType protocol);

	// PosixSocket
	bool setOption (int option, int value) override;
	int setMulticastMembership (uint32 address, bool state) override;
	int checkState (CheckHint hint, int timeout) const override;
	
protected:	
	void disableSigpipe () const;
};

typedef CocoaSocket Socket;

//************************************************************************************************
// SocketIDSet
//************************************************************************************************

typedef PosixSocketIDSet SocketIDSet;

} // namespace Platform
} // namespace Core

#endif // _coresocket_cocoa_h
