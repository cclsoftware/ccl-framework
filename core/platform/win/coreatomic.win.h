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
// Filename    : core/platform/win/coreatomic.win.h
// Description : Windows Atomic Primitives
//
//************************************************************************************************

#ifndef _coreatomic_win_h
#define _coreatomic_win_h

#include "core/public/coretypes.h"

#include <windows.h>

namespace Core {
namespace Platform {

//************************************************************************************************
// Atomic Primitives
//************************************************************************************************

INLINE void MemoryFence ()
{
	MemoryBarrier ();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

INLINE int32 AtomicAdd (int32 volatile& variable, int32 value)
{
	return InterlockedExchangeAdd ((LONG*)&variable, value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

INLINE int32 AtomicSet (int32 volatile& variable, int32 value)
{
	return InterlockedExchange ((LONG*)&variable, value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

INLINE int32 AtomicGet (const int32 volatile& variable)
{
	MemoryBarrier ();
	return variable;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

INLINE bool AtomicTestAndSet (int32 volatile& variable, int32 value, int32 comperand)
{
	return InterlockedCompareExchange ((LONG*)&variable, value, comperand) == comperand;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

INLINE void* AtomicSetPtr (void* volatile& variable, void* value)
{
	return InterlockedExchangePointer (&variable, value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

INLINE void* AtomicGetPtr (void* const volatile& variable)
{
	MemoryBarrier ();
	return variable;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

INLINE bool AtomicTestAndSetPtr (void* volatile& variable, void* value, void* comperand)
{
	return InterlockedCompareExchangePointer (&variable, value, comperand) == comperand;
}

} // namespace Platform
} // namespace Core

#endif // _coreatomic_win_h
