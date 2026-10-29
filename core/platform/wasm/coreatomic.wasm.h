//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
//
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : core/platform/wasm/coreatomic.wasm.h
// Description : WebAssembly Atomic Primitives
//
//************************************************************************************************

#ifndef _coreatomic_wasm_h
#define _coreatomic_wasm_h

#include "core/public/coretypes.h"

namespace Core {
namespace Platform {

//************************************************************************************************
// Atomic Primitives
/**
	Uses __atomic_* builtins (lowering to genuine i32.atomic.load/store/rmw wasm instructions)
	rather than the shared POSIX implementation's fence-plus-plain-load/store idiom: the
	WebAssembly threads spec only guarantees interop with JS-side `Atomics.load`/`Atomics.store` on
	the same SharedArrayBuffer for actual atomic wasm instructions, not for a plain load/store next
	to a fence. That distinction only matters here, where wasm memory can be concurrently touched
	by JS code outside this module (e.g. a hand-authored AudioWorkletProcessor) - it doesn't apply
	to native POSIX platforms, where there is no separate runtime with its own atomicity rules
	reading the same process memory.
*/
//************************************************************************************************

INLINE void MemoryFence ()
{
	__atomic_thread_fence (__ATOMIC_SEQ_CST);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

INLINE int32 AtomicAdd (int32 volatile& variable, int32 value)
{
	return __atomic_fetch_add (&variable, value, __ATOMIC_SEQ_CST);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

INLINE int32 AtomicSet (int32 volatile& variable, int32 value)
{
	__atomic_store_n (&variable, value, __ATOMIC_SEQ_CST);
	return value;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

INLINE int32 AtomicGet (const int32 volatile& variable)
{
	return __atomic_load_n (&variable, __ATOMIC_SEQ_CST);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

INLINE bool AtomicTestAndSet (int32 volatile& variable, int32 value, int32 comperand)
{
	return __atomic_compare_exchange_n (&variable, &comperand, value, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

INLINE void* AtomicSetPtr (void* volatile& variable, void* value)
{
	__atomic_store_n (&variable, value, __ATOMIC_SEQ_CST);
	return value;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

INLINE void* AtomicGetPtr (void* const volatile& variable)
{
	return __atomic_load_n (&variable, __ATOMIC_SEQ_CST);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

INLINE bool AtomicTestAndSetPtr (void* volatile& variable, void* value, void* comperand)
{
	return __atomic_compare_exchange_n (&variable, &comperand, value, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

} // namespace Platform
} // namespace Core

#endif // _coreatomic_wasm_h
