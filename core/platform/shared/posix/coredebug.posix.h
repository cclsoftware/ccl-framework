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
// Filename    : core/platform/shared/posix/coredebug.posix.h
// Description : Debugging Functions POSIX implementation
//
//************************************************************************************************

#ifndef _coredebug_posix_h
#define _coredebug_posix_h

#include "core/platform/shared/coreplatformdebug.h"
  
#include "core/system/corethread.h"
namespace Core {
namespace Platform {

//************************************************************************************************
// Debugging Functions
//************************************************************************************************

inline void Debug::print (CStringPtr string)
{
	static Threads::Lock theLock;
	Threads::ScopedLock scopedLock (theLock);

	::printf (string);
	::fflush (stdout);
}

} // namespace Platform
} // namespace Core

#endif // _coredebug_posix_h
