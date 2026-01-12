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
// Filename    : core/public/coreplatform.h
// Description : Platform definitions
//
//************************************************************************************************

#ifndef _coreplatform_h
#define _coreplatform_h

/*
	Common platform switches:
	
	* CORE_PLATFORM_DESKTOP : Defined for desktop platforms (Windows, macOS, Linux) 
	* CORE_PLATFORM_MOBILE  : Defined for mobile platforms (iOS, Android) 
	* CORE_PLATFORM_RTOS    : Platform is an RTOS (Zephyr, Little Kernel, etc.)
	* CORE_PLATFORM_64BIT   : Platform is 64 bit (x86-64 or Arm64)
	* CORE_PLATFORM_ARM     : Arm platform architecture
	* CORE_PLATFORM_ARM64EC : Arm64EC platform architecture (Windows only)
	* CORE_PLATFORM_INTEL   : Intel x86 platform architecture (i386 or x86-64)
*/

//////////////////////////////////////////////////////////////////////////////////////////////////
// Platform
//////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32
	/** We are on Windows platform. */
	#define CORE_PLATFORM_WINDOWS 1
	#define CORE_PLATFORM_DESKTOP 1

#elif defined(__APPLE__) && defined(__MACH__)
	#include <TargetConditionals.h>
	#if TARGET_OS_OSX == 1
		#define CORE_PLATFORM_MAC 1
		#define CORE_PLATFORM_DESKTOP 1
	#elif TARGET_OS_IPHONE == 1 || TARGET_OS_SIMULATOR == 1
		#define CORE_PLATFORM_IOS 1
		#define CORE_PLATFORM_MOBILE 1
	#else
		#error Core does not support this Apple platform
	#endif

#elif DSP_TI32
	/** OMAP DSP Core defined here. */
	#define CORE_PLATFORM_TI32 1
	#define CORE_PLATFORM_RTOS 1

#elif ARM_TI32
	/** OMAP ARM Core defined here. */
	#define CORE_PLATFORM_TI32 1
	#define CORE_PLATFORM_RTOS 1

#elif DSP_SHARC
	/** SHARC DSP Core defined here. */
	#define CORE_PLATFORM_RTOS 1

#elif defined(ANDROID) || defined(__ANDROID__)
	/** We are on Android platform */
	#define CORE_PLATFORM_ANDROID 1
	#define CORE_PLATFORM_MOBILE 1

#elif CTL_RTOS
	/** Crossworks Tasking Library */
	#define CORE_PLATFORM_RTOS 1
    // Disable support for certain features not supported by CrossWorks' distribution of GCC
    #undef __cpp_initializer_lists

#elif __ZEPHYR__
	/** Zephyr OS **/
	#define CORE_PLATFORM_RTOS 1

#elif CORE_PLATFORM_CMSIS
	/** CMSIS platform **/
	#define CORE_PLATFORM_RTOS 1

#elif __linux__
	/** Linux defined here. */
	#define CORE_PLATFORM_LINUX 1
	#define CORE_PLATFORM_DESKTOP 1

#elif LITTLEKERNEL
	/** Little Kernel*/
	#define CORE_PLATFORM_RTOS 1

#elif AZURE
	#define CORE_PLATFORM_RTOS 1

#else	
	// This is an unknown platform. Try to include a header with platform-specific definitions
	#ifndef CORE_PLATFORM_NAME
		#error Core does not support this platform
	#endif
	#include "corebasicmacros.h"
	#define CORE_EXTERNAL_PLATFORM_HEADER STRINGIFY (coreplatform.CORE_PLATFORM_NAME.h)
	#include CORE_EXTERNAL_PLATFORM_HEADER
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////
// Architecture
//////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(_WIN64) || defined(__LP64__) || defined(__x86_64__) || defined(_M_X64) || defined(__ARM_ARCH_ISA_A64) || defined(_M_ARM64)
	/** Define for 64 Bit platforms. */
	#define CORE_PLATFORM_64BIT 1
#endif

#if defined(_M_ARM64EC)
	/** Define both for ARM64EC as it is compatible with x86-64 code. */
	#define CORE_PLATFORM_ARM64EC 1
	#define CORE_PLATFORM_INTEL 1

#elif defined(__arm__) || defined(__TARGET_ARCH_ARM) || defined(__ARM_ARCH) || defined(_M_ARM) || defined(_M_ARM64)
	#define CORE_PLATFORM_ARM 1

#elif defined(__x86_64__) || defined(_M_X64) || defined(i386) || defined(__i386__) || defined(__i386) || defined(_M_IX86)
	#define CORE_PLATFORM_INTEL 1
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////
// Configurations
//////////////////////////////////////////////////////////////////////////////////////////////////

#if CORE_PLATFORM_WINDOWS
	#undef DEBUG
	#undef RELEASE

	#ifdef _DEBUG
		/** Define for debug builds. */
		#define DEBUG 1
		/** Define for release builds. */
		#define RELEASE 0
	#else
		#define DEBUG 0
		#define RELEASE 1
	#endif

#else

	#if defined(CORE_PLATFORM_TI32)
		#if RELEASE // we define this in our make files to mean debug off
			#undef DEBUG
		#endif
	#endif

	#undef RELEASE
	#ifdef DEBUG
		#undef DEBUG
		#define DEBUG 1
		#define RELEASE 0
	#else
		#define DEBUG 0
		#define RELEASE 1
	#endif
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////
// C Library includes
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "coreclibrary.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
// Debugging
//////////////////////////////////////////////////////////////////////////////////////////////////

#undef ASSERT
#if DEBUG
	#if defined(DSP_TI32) || defined(DSP_SHARC)
		void ASSERTDSP (int val);
		#define ASSERT(expr)	ASSERTDSP(expr);
	#elif CORE_PLATFORM_MAC || CORE_PLATFORM_IOS
		#define ASSERT(expr)	if(!(expr)) __builtin_debugtrap ();
	#elif CORE_PLATFORM_LINUX || CORE_PLATFORM_ANDROID
		#include <signal.h>
		#define ASSERT(expr)	if(!(expr)) ::raise (SIGTRAP);
    #else
		#define ASSERT(expr)	assert (expr);
	#endif
#else
	#define ASSERT(expr)
#endif

#undef NEW
#define NEW new

//////////////////////////////////////////////////////////////////////////////////////////////////
// API definitions
//////////////////////////////////////////////////////////////////////////////////////////////////

#undef INLINE
#undef CORE_ALIGN
#undef CORE_ALIGN_AS
#undef CORE_EXPORT

#if _MSC_VER
	/** Define for inline functions. */
	#define INLINE					__forceinline
	/** Alignement macros. */
	#define CORE_ALIGN_AS(size)		__declspec(align(size))
	#define CORE_ALIGN(type, size)	CORE_ALIGN_AS (size) type
	/** Exported functions. */
	#define CORE_EXPORT				extern "C"

#elif defined(__GNUC__)
	#define INLINE					__attribute__((always_inline)) inline
	#define CORE_ALIGN_AS(size)		__attribute__((__aligned__(size)))
	#define CORE_ALIGN(type, size)	type CORE_ALIGN_AS (size)
	#define CORE_EXPORT				extern "C" __attribute__((visibility("default")))

#elif defined(DSP_TI32) || defined(DSP_SHARC)
	#define INLINE					inline
	#define CORE_ALIGN_AS(size)
	#define CORE_ALIGN 				DATA_ALIGN
	#define CORE_EXPORT

#else
	#define INLINE
	#define CORE_ALIGN_AS(size)
	#define CORE_ALIGN(type, size)
	#define CORE_EXPORT
#endif	

#if DSP_TI32
	/** DSP compiler doesn't support static declarations in a template. */
	#define CORE_STATIC_TEMPLATE_MEMBER
#else
	#define CORE_STATIC_TEMPLATE_MEMBER static
#endif

/* Hint for compiler to optimize hot function. */
#if CTL_RTOS
	#define CORE_HOT_FUNCTION __attribute__ ((section(".fast")))
#elif CORE_PLATFORM_CMSIS
	#define CORE_HOT_FUNCTION __attribute__ ((section(".text.ITCM")))
#elif !defined (CORE_HOT_FUNCTION)
	#define CORE_HOT_FUNCTION
#endif
           
//////////////////////////////////////////////////////////////////////////////////////////////////
// Warnings
//////////////////////////////////////////////////////////////////////////////////////////////////
     
/* Hint for compiler to silence warnings about intentional fall-throughs in switch statements. */
#if defined __has_cpp_attribute
    #if __has_cpp_attribute (fallthrough)
        #define CORE_FALLTHROUGH [[fallthrough]];
    #else
        #define CORE_FALLTHROUGH
    #endif
#else
    #define CORE_FALLTHROUGH
#endif
       
//////////////////////////////////////////////////////////////////////////////////////////////////
// Byte order
//////////////////////////////////////////////////////////////////////////////////////////////////

/* Byte order definition as preprocessor macros to allow usage in #if blocks. */
#define CORE_LITTLE_ENDIAN 0	///< little endian (Intel)
#define CORE_BIG_ENDIAN    1	///< big endian (Motorola)

/* Native platform byte order */
#if CORE_PLATFORM_MAC || CORE_PLATFORM_IOS
	#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        #define CORE_NATIVE_BYTEORDER CORE_LITTLE_ENDIAN
	#else
        #define CORE_NATIVE_BYTEORDER CORE_BIG_ENDIAN
	#endif
#else
	#define CORE_NATIVE_BYTEORDER CORE_LITTLE_ENDIAN	///< Windows Intel, OMAP-L138, etc.
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////
// Other
//////////////////////////////////////////////////////////////////////////////////////////////////

#undef EndFor
/** End of iteration macros. */
#define EndFor }}

// include our own memory allocator (can be disabled within coremalloc.h)
#include "core/public/coremalloc.h"

#endif // _coreplatform_h
