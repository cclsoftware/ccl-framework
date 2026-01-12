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
// Filename    : core/platform/win/coresocket.win.h
// Description : Windows Socket Functions
//
//************************************************************************************************

#ifndef _coresocket_win_h
#define _coresocket_win_h

#include "core/platform/shared/posix/coresocket.posix.h"

#include "core/platform/win/corenetwork.win.h"

#include <ws2tcpip.h>

#define SOCKET_DEFINED 1

//////////////////////////////////////////////////////////////////////////////////////////////////
// Missing POSIX functions
//////////////////////////////////////////////////////////////////////////////////////////////////

int close (Core::Platform::SocketID socket);
int ioctl (Core::Platform::SocketID, unsigned long request, ...);

namespace Core {
namespace Platform {
	
//************************************************************************************************
// Win32Socket
//************************************************************************************************

class Win32Socket: public PosixSocket
{
public:
	Win32Socket (SocketID socket);
	Win32Socket (Sockets::AddressFamily addressFamily, Sockets::SocketType type, Sockets::ProtocolType protocol);

	// PosixSocket
	int getBytesAvailable (int& bytesAvailable) override;
	int getErrorCode () const override;
	bool wouldBlockOperation (bool writeDirection) const override;

protected:
	int setNonBlockingMode (bool state) override;
};

typedef Win32Socket Socket;

//************************************************************************************************
// SocketIDSet
//************************************************************************************************

typedef PosixSocketIDSet SocketIDSet;

} // namespace Platform
} // namespace Core

#endif // _coresocket_win_h
