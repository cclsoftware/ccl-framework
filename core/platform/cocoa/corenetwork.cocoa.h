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
// Filename    : core/platform/cocoa/corenetwork.cocoa.h
// Description : Cocoa Network Functions
//
//************************************************************************************************

#ifndef _corenetwork_cocoa_h
#define _corenetwork_cocoa_h

#include <ifaddrs.h>
#include <sys/fcntl.h>
#import <net/if.h> // For IFF_LOOPBACK, etc
#include <net/if_dl.h>

#include "core/platform/shared/posix/corenetwork.posix.h"

#define CORE_IFADDRS_HAVE_DATA_MEMBER 0
#define CORE_IFADDRS_HAVE_NETMASK_MEMBER 1

namespace Core {
namespace Platform {

//************************************************************************************************
// CocoaNetwork
//************************************************************************************************

class CocoaNetwork: public PosixNetwork
{
	// PosixNetwork
	bool getLocalMACAddress (uint8 mac[6]) override;
};

//************************************************************************************************
// SocketAddressConverter
/** Helper class to convert from Core-style to native address. */
//************************************************************************************************

typedef SocketAddressConverter PosixSocketAddressConverter;

//************************************************************************************************
// AdapterIterator
//************************************************************************************************

typedef PosixAdapterIterator AdapterIterator;

} // namespace Platform
} // namespace Core

#endif // _corenetwork_cocoa_h
