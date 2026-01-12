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
// Filename    : core/system/coreatomic.h
// Description : Atomic primitives
//
//************************************************************************************************

#ifndef _coreatomic_h
#define _coreatomic_h

#include "core/platform/corefeatures.h"

#if CORE_ATOMIC_IMPLEMENTATION == CORE_PLATFORM_IMPLEMENTATION
	#include CORE_PLATFORM_IMPLEMENTATION_HEADER (coreatomic)
	#define CORE_HAS_ATOMICS 1
#elif CORE_ATOMIC_IMPLEMENTATION == CORE_EXTERNAL_PLATFORM_IMPLEMENTATION
	#include CORE_EXTERNAL_PLATFORM_IMPLEMENTATION_HEADER (coreatomic)
	#define CORE_HAS_ATOMICS 1
#elif CORE_ATOMIC_IMPLEMENTATION == CORE_FEATURE_IMPLEMENTATION_POSIX
	#include "core/platform/shared/posix/coreatomic.posix.h"
	#define CORE_HAS_ATOMICS 1
#else
	#define CORE_HAS_ATOMICS 0
#endif

namespace Core {

#if CORE_HAS_ATOMICS

/**
* \fn void Platform::MemoryFence ()
* \brief A full memory barrier synchronizing data in all threads
*/
using Platform::MemoryFence;

/**
* \fn int32 Platform::AtomicAdd (int32 volatile& variable, int32 value)
* \brief Atomic addition
* \param[in,out] variable First addend and variable to store the result
* \param[in] value Second addend
* \return The initial value of \a variable
*/
using Platform::AtomicAdd;

/**
* \fn int32 Platform::AtomicSet (int32 volatile& variable, int32 value)
* \brief Atomic assignment
* \param[in,out] variable Variable to assign the value to
* \param[in] value The value to assign
* \return The initial value of \a variable
*/
using Platform::AtomicSet;

/**
* \fn int32 Platform::AtomicGet (const int32 volatile& variable)
* \brief Get the value of a variable
* \param[in] variable Variable to get the value from
* \return The value of \a variable
*/
using Platform::AtomicGet;

/**
* \fn bool Platform::AtomicTestAndSet (int32 volatile& variable, int32 value, int32 comperand)
* \brief Atomic compare-and-exchange
* \details Compares \variable and \comperand. If both are equal, \value is stored into \variable. Otherwise, no operation is performed.
* \param[in,out] variable Variable to assign the value to
* \param[in] value The value to assign
* \param[in] comperand Value to compare \a variable to
* \return True if \a variable and \a comperand were equal. False otherwise.
*/
using Platform::AtomicTestAndSet;

/**
* \fn void* Platform::AtomicSetPtr (void* volatile& variable, void* value)
* \brief Atomic pointer assignment
* \param[in,out] variable Variable to assign the value to
* \param[in] value The value to assign
* \return The initial value of \a variable
*/
using Platform::AtomicSetPtr;

/**
* \fn void* Platform::AtomicGetPtr (void* const volatile& variable)
* \brief Get the address of a pointer
* \param[in] variable Variable to get the value from
* \return The value of \a variable
*/
using Platform::AtomicGetPtr;

/**
* \fn bool Platform::AtomicTestAndSetPtr (void* volatile& variable, void* value, void* comperand)
* \brief Atomic compare-and-exchange for pointer values
* \details Compares \variable and \comperand. If both are equal, \value is stored into \variable. Otherwise, no operation is performed.
* \param[in,out] variable Variable to assign the value to
* \param[in] value The value to assign
* \param[in] comperand Value to compare \a variable to
* \return True if \a variable and \a comperand were equal. False otherwise.
*/
using Platform::AtomicTestAndSetPtr;

#endif // CORE_HAS_ATOMICS

} // namespace Core

#endif // _coreatomic_h
