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
// Filename    : ccl/public/system/atomic.h
// Description : Intrinsic Atomic Primitives
//
//************************************************************************************************

#ifndef _ccl_atomic_h
#define _ccl_atomic_h

// Use intrinsics for atomic operations if supported by compiler and operating system,
// use atomic functions exported by framework otherwise.

#include "ccl/public/atomicexports.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
// Windows
//////////////////////////////////////////////////////////////////////////////////////////////////

#if CCL_PLATFORM_WINDOWS

extern "C"
{
	long  __cdecl _InterlockedExchangeAdd (volatile long* Addend, long Value);
	long  __cdecl _InterlockedExchange (volatile long* Addend, long Value);
	long  __cdecl _InterlockedCompareExchange (volatile long* Dest, long Exchange, long Comp);
	long  __cdecl _InterlockedDecrement(long volatile *);
	long  __cdecl _InterlockedIncrement(long volatile *);
}

#pragma intrinsic (_InterlockedExchangeAdd)
#pragma intrinsic (_InterlockedExchange)
#pragma intrinsic (_InterlockedCompareExchange)
#pragma intrinsic (_InterlockedIncrement)
#pragma intrinsic (_InterlockedDecrement)

#define AtomicAddInline(variable, value) \
	_InterlockedExchangeAdd ((long*)&(variable), value)

#define AtomicTestAndSetInline(variable, value, comperand) \
	(_InterlockedCompareExchange ((long*)&(variable), value, comperand) == comperand)

#define AtomicSetInline(variable, value) \
	_InterlockedExchange ((long*)&(variable), value)

//////////////////////////////////////////////////////////////////////////////////////////////////
// Mac and iOS
//////////////////////////////////////////////////////////////////////////////////////////////////

#elif CCL_PLATFORM_MAC || CCL_PLATFORM_IOS

//#define OSATOMIC_USE_INLINED 1
#include <libkern/OSAtomic.h>

#define AtomicAddInline(variable, value) \
	(OSAtomicAdd32Barrier (value, &variable) - value)

#define AtomicTestAndSetInline(variable, value, comperand) \
	CCL::System::AtomicTestAndSet (variable, value, comperand)

#define AtomicSetInline(variable, value) \
	CCL::System::AtomicSet (variable, value)

//////////////////////////////////////////////////////////////////////////////////////////////////
// Other Platforms and Compilers
//////////////////////////////////////////////////////////////////////////////////////////////////

#elif CCL_PLATFORM_LINUX || CCL_PLATFORM_ANDROID

#define AtomicAddInline(variable, value) \
	__sync_fetch_and_add_4 (&variable, value)

#define AtomicTestAndSetInline(variable, value, comperand) \
	__sync_bool_compare_and_swap_4 ((int32_t*)&variable, comperand, value)

#define AtomicSetInline(variable, value) \
	CCL::System::AtomicSet (variable, value)

#else

#define AtomicAddInline(variable, value) \
	CCL::System::AtomicAdd (variable, value)

#define AtomicTestAndSetInline(variable, value, comperand) \
	CCL::System::AtomicTestAndSet (variable, value, comperand)

#define AtomicSetInline(variable, value) \
	CCL::System::AtomicSet (variable, value)

#endif

//////////////////////////////////////////////////////////////////////////////////////////////////
// Non-intrinsic
//////////////////////////////////////////////////////////////////////////////////////////////////

#define AtomicSetPtrInline(variable, value) \
	CCL::System::AtomicSetPtr (variable, value)

#define AtomicGetPtrInline(variable) \
	CCL::System::AtomicGetPtr (variable)

#define AtomicGetInline(variable) \
	CCL::System::AtomicGet (variable)

#define AtomicTestAndSetPtrInline(variable, value, comperand) \
	CCL::System::AtomicTestAndSetPtr (variable, value, comperand)

#endif // _ccl_atomic_h
