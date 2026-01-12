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
// Filename    : core/platform/armti32/coresocket.armti32.h
// Description : TI32 ARM (OMAP-L138) Socket Functions
//
//************************************************************************************************

#ifndef _coresocket_armti32_h
#define _coresocket_armti32_h

#include "core/platform/shared/posix/coresocket.posix.h"

#ifndef TCP_NODELAY
#define TCP_NODELAY 1
#endif

namespace Core {
namespace Platform {

//************************************************************************************************
// TI32Socket
//************************************************************************************************

class TI32Socket: public PosixSocket
{
public:
	TI32Socket (SocketID socket);
	TI32Socket (Sockets::AddressFamily addressFamily, Sockets::SocketType type, Sockets::ProtocolType protocol);

protected:	
	void disableSigpipe () const;
};

typedef TI32Socket Socket;

//************************************************************************************************
// SocketIDSet
//************************************************************************************************

typedef PosixSocketIDSet SocketIDSet;

} // namespace Platform
} // namespace Core

#endif // _coresocket_armti32_h
