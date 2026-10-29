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
// Filename    : core/public/coreconfiguration.h
// Description : Framework configuration
//
//************************************************************************************************

#ifndef _coreconfiguration_h
#define _coreconfiguration_h

#ifndef _coreplatform_h
#error Do not include this file directly, it will be included by coreplatform.h!
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////
// CORE_HOT_FUNCTION
/** Hint for compiler to optimize hot function. */
//////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CORE_HOT_FUNCTION
	#if CTL_RTOS
		#define CORE_HOT_FUNCTION __attribute__ ((section(".fast")))
	#elif CORE_PLATFORM_CMSIS
		#define CORE_HOT_FUNCTION __attribute__ ((section(".text.ITCM")))
	#else
		#define CORE_HOT_FUNCTION
	#endif
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////
// CORE_STRING_STACK_MAX
/** Max. size of string buffer on stack. */
//////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CORE_STRING_STACK_MAX
	#if CORE_PLATFORM_RTOS
		#define CORE_STRING_STACK_MAX 512
	#else
		#define CORE_STRING_STACK_MAX 4096
	#endif
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////
// CORE_ATTRIBUTE_ID_LENGTH
/** Max. length of an Attribute's identifier stored inline. */
//////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CORE_ATTRIBUTE_ID_LENGTH
	#if CORE_PLATFORM_RTOS
		#define CORE_ATTRIBUTE_ID_LENGTH 32
	#else
		#define CORE_ATTRIBUTE_ID_LENGTH 64
	#endif
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////
// CORE_DOUBLE_AS_FLOAT
/** Define as 1 if the platform doesn't support double precision, i.e. sizeof(double) equals sizeof(float). */
//////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CORE_DOUBLE_AS_FLOAT
	#if CTL_RTOS
		#define CORE_DOUBLE_AS_FLOAT 1
	#else
		#define CORE_DOUBLE_AS_FLOAT 0
	#endif
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////
// CORE_MEMORYPOOL_LAZY_INITIALIZATION
/** Enable lazy memory pool initialization (deferred until first use). */
//////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CORE_MEMORYPOOL_LAZY_INITIALIZATION
	#if CTL_RTOS
		#define CORE_MEMORYPOOL_LAZY_INITIALIZATION 1
	#else
		#define CORE_MEMORYPOOL_LAZY_INITIALIZATION 0
	#endif
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////
// CORE_MEMORYPOOL_COUNT_MAX
/** Max. number of memory pools that can be registered for lazy initialization. */
//////////////////////////////////////////////////////////////////////////////////////////////////

#if CORE_MEMORYPOOL_LAZY_INITIALIZATION
	#ifndef CORE_MEMORYPOOL_COUNT_MAX
		#define CORE_MEMORYPOOL_COUNT_MAX 16
	#endif
#endif

#endif // _coreconfiguration_h
