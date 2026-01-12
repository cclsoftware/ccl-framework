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
// Filename    : core/platform/android/coresocket.android.h
// Description : Android Socket Functions
//
//************************************************************************************************

#ifndef _coresocket_android_h
#define _coresocket_android_h

#include "core/platform/shared/posix/coresocket.posix.h"

#include <linux/tcp.h>

namespace Core {
namespace Platform {

//************************************************************************************************
// AndroidSocket
//************************************************************************************************

class AndroidSocket: public PosixSocket
{
public:
	AndroidSocket (SocketID socket);
	AndroidSocket (Sockets::AddressFamily addressFamily, Sockets::SocketType type, Sockets::ProtocolType protocol);

	// PosixSocket
	int setMulticastMembership (uint32 address, bool state) override;
};

typedef AndroidSocket Socket;

//************************************************************************************************
// SocketIDSet
//************************************************************************************************

typedef PosixSocketIDSet SocketIDSet;

} // namespace Platform
} // namespace Core

#endif // _coresocket_android_h
