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
// Filename    : core/platform/cocoa/coreatomic.cocoa.h
// Description : Cocoa Atomic Primitives
//
//************************************************************************************************

#ifndef _coreatomic_cocoa_h
#define _coreatomic_cocoa_h

#include "core/public/coretypes.h"

#define OSATOMIC_USE_INLINED 1
#include <libkern/OSAtomic.h>

namespace Core {
namespace Platform {

//************************************************************************************************
// Atomic Primitives
//************************************************************************************************

INLINE void MemoryFence ()
{
	OSMemoryBarrier ();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

INLINE int32 AtomicAdd (int32 volatile& variable, int32 value)
{
	int32_t	newValue = OSAtomicAdd32Barrier (value, &variable);
	return newValue - value;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

INLINE int32 AtomicSet (int32 volatile& variable, int32 value)
{
	variable = value;
	OSMemoryBarrier ();
	return value;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

INLINE int32 AtomicGet (const int32 volatile& variable)
{
	OSMemoryBarrier ();
	return variable;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

INLINE bool AtomicTestAndSet (int32 volatile& variable, int32 value, int32 comperand)
{
	return OSAtomicCompareAndSwap32Barrier (comperand, value, (int32_t*)&variable);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

INLINE void* AtomicSetPtr (void* volatile& variable, void* value)
{
	variable = value;
	OSMemoryBarrier ();
	return value;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

INLINE void* AtomicGetPtr (void* const volatile& variable)
{
	OSMemoryBarrier ();
	return variable;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

INLINE bool AtomicTestAndSetPtr (void* volatile& variable, void* value, void* comperand)
{
	#if CORE_PLATFORM_64BIT
	return OSAtomicCompareAndSwap64Barrier ((int64_t)comperand, (int64_t)value, (int64_t*)&variable);
	#else
	return OSAtomicCompareAndSwap32Barrier ((int32_t)comperand, (int32_t)value, (int32_t*)&variable);
	#endif
}

} // namespace Platform
} // namespace Core

#endif // _coreatomic_cocoa_h
