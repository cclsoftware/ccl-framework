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
// Filename    : core/platform/shared/posix/coreatomic.posix.h
// Description : POSIX Atomic Primitives
//
//************************************************************************************************

#ifndef _coreatomic_posix_h
#define _coreatomic_posix_h

#include "core/public/coretypes.h"

#include <stdint.h>

namespace Core {
namespace Platform {

//************************************************************************************************
// Atomic Primitives
//************************************************************************************************

INLINE void MemoryFence ()
{
	__sync_synchronize ();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

INLINE int32 AtomicAdd (int32 volatile& variable, int32 value)
{
	// https://gcc.gnu.org/onlinedocs/gcc-4.1.2/gcc/Atomic-Builtins.html
	return __sync_fetch_and_add_4 (&variable, value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

INLINE int32 AtomicSet (int32 volatile& variable, int32 value)
{
	variable = value;
	__sync_synchronize ();
	return value;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

INLINE int32 AtomicGet (const int32 volatile& variable)
{
	__sync_synchronize ();
	return variable;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

INLINE bool AtomicTestAndSet (int32 volatile& variable, int32 value, int32 comperand)
{
	return __sync_bool_compare_and_swap_4 ((int32_t*)&variable, comperand, value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

INLINE void* AtomicSetPtr (void* volatile& variable, void* value)
{
	variable = value;
	__sync_synchronize ();
	return value;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

INLINE void* AtomicGetPtr (void* const volatile& variable)
{
	__sync_synchronize ();
	return variable;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

INLINE bool AtomicTestAndSetPtr (void* volatile& variable, void* value, void* comperand)
{
	return __sync_bool_compare_and_swap (&variable, comperand, value);
}

} // namespace Platform
} // namespace Core

#endif // _coreatomic_posix_h
