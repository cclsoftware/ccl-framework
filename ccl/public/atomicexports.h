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
// Filename    : ccl/public/atomicexports.h
// Description : Exported Atomic Primitives
//
//************************************************************************************************

#ifndef _ccl_atomicexports_h
#define _ccl_atomicexports_h

#include "ccl/public/cclexports.h"
#include "ccl/public/base/platform.h"

namespace CCL {
namespace System {

/** \addtogroup ccl_system
@{ */

//////////////////////////////////////////////////////////////////////////////////////////////////
// Atomic Primitives APIs
////////////////////////////////////////////////////////////////////////////////////////////////////

/** Perform atomic integer addition, returns old value. */
CCL_EXPORT int32 CCL_API CCL_ISOLATED (AtomicAdd) (int32 volatile& variable, int32 value);
inline int32 AtomicAdd (int32 volatile& variable, int32 value) { return CCL_ISOLATED (AtomicAdd) (variable, value); }

/** Perform atomic integer assignment, returns old value. */
CCL_EXPORT int32 CCL_API CCL_ISOLATED (AtomicSet) (int32 volatile& variable, int32 value);
inline int32 AtomicSet (int32 volatile& variable, int32 value) { return CCL_ISOLATED (AtomicSet) (variable, value); }

/** Perform read after barrier. */
CCL_EXPORT int32 CCL_API CCL_ISOLATED (AtomicGet) (const int32 volatile& variable);
inline int32 AtomicGet (const int32 volatile& variable) { return CCL_ISOLATED (AtomicGet) (variable); }

/** Atomically assign integer if current value equals comperand, returns old value. */
CCL_EXPORT tbool CCL_API CCL_ISOLATED (AtomicTestAndSet) (int32 volatile& variable, int32 value, int32 comperand);
inline tbool AtomicTestAndSet (int32 volatile& variable, int32 value, int32 comperand) { return CCL_ISOLATED (AtomicTestAndSet) (variable, value, comperand); }

/** Perform atomic pointer assignment, returns old value. */
CCL_EXPORT void* CCL_API CCL_ISOLATED (AtomicSetPtr) (void* volatile& variable, void* value);
inline void* AtomicSetPtr (void* volatile& variable, void* value) { return CCL_ISOLATED (AtomicSetPtr) (variable, value); }

/** Return value after barrier. */
CCL_EXPORT void* CCL_API CCL_ISOLATED (AtomicGetPtr) (void* const volatile& variable);
inline void* AtomicGetPtr (void* const volatile& variable) { return CCL_ISOLATED (AtomicGetPtr) (variable); }

/** Atomically assign pointer if current value equals comperand, returns old value. */
CCL_EXPORT tbool CCL_API CCL_ISOLATED (AtomicTestAndSetPtr) (void* volatile& variable, void* value, void* comperand);
inline tbool AtomicTestAndSetPtr (void* volatile& variable, void* value, void* comperand) { return CCL_ISOLATED (AtomicTestAndSetPtr) (variable, value, comperand); }

//////////////////////////////////////////////////////////////////////////////////////////////////
/** @}*/

} // namespace System
} // namespace CCL

#endif // _ccl_atomicexports_h
