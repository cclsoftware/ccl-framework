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
// Filename    : core/platform/zephyr/coreatomic.zephyr.h
// Description : Zephyr Atomic Primitives
//
//************************************************************************************************

#ifndef _coreatomic_zephyr_h
#define _coreatomic_zephyr_h

#include "core/public/coretypes.h"

#include "corezephyr.h"

namespace Core {
namespace Platform {

//************************************************************************************************
// Atomic Primitives
//************************************************************************************************

INLINE void MemoryFence ()
{
	atomic_t t;
	atomic_clear (&t);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

INLINE int32 AtomicAdd (int32 volatile& variable, int32 value)
{
	return atomic_add ((int32*)&variable, value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

INLINE int32 AtomicSet (int32 volatile& variable, int32 value)
{
	return atomic_set ((int32*)&variable, value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

INLINE int32 AtomicGet (const int32 volatile& variable)
{
	return atomic_get ((int32*)&variable);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

INLINE bool AtomicTestAndSet (int32 volatile& variable, int32 value, int32 comperand)
{
	return atomic_cas ((int32*)&variable, comperand, value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

INLINE void* AtomicSetPtr (void* volatile& variable, void* value)
{
	// TODO, see https://github.com/zephyrproject-rtos/zephyr/issues/22887
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

INLINE void* AtomicGetPtr (void* const volatile& variable)
{
	// TODO, see https://github.com/zephyrproject-rtos/zephyr/issues/22887
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

INLINE bool AtomicTestAndSetPtr (void* volatile& variable, void* value, void* comperand)
{
	// TODO, see https://github.com/zephyrproject-rtos/zephyr/issues/22887
	return false;
}

} // namespace Platform
} // namespace Core

#endif // _coreatomic_zephyr_h
