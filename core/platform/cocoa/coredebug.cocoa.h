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
// Filename    : core/platform/cocoa/coredebug.cocoa.h
// Description : Debugging Functions Cocoa implementation
//
//************************************************************************************************

#ifndef _coredebug_cocoa_h
#define _coredebug_cocoa_h

#include "core/system/corethread.h"

#include "core/platform/shared/coreplatformdebug.h"

namespace Core {
namespace Platform {

//************************************************************************************************
// Debugging Functions
//************************************************************************************************

inline void Debug::print (CStringPtr string)
{
	static Threads::Lock theLock;
	Threads::ScopedLock scopedLock (theLock);

	::fprintf (stderr, "%s", string);
	::fflush (stderr);
}

} // namespace Platform
} // namespace Core

#endif // _coredebug_cocoa_h
