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
// Filename    : core/public/coretypes.h
// Description : Basic datatypes
//
//************************************************************************************************

#ifndef _coretypes_h
#define _coretypes_h

#include "core/public/coreplatform.h"

namespace Core {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Integral types
//////////////////////////////////////////////// //////////////////////////////////////////////////

typedef signed char int8;				///< 8 Bit integer (signed) 
typedef unsigned char uint8;			///< 8 Bit integer (unsigned)

typedef short int16;					///< 16 Bit integer (signed)
typedef unsigned short uint16;			///< 16 Bit integer (unsigned)

typedef int int32;						///< 32 Bit integer (signed)
typedef unsigned int uint32;			///< 32 Bit integer (unsigned)

#if _MSC_VER
typedef __int64 int64;					///< 64 Bit integer (signed)
typedef unsigned __int64 uint64;		///< 64 Bit integer (unsigned)
#else
typedef long long int64;
typedef unsigned long long uint64;
#endif

#if CORE_PLATFORM_64BIT
typedef int64 IntPtr;					///< integer pointer (64 Bit)
typedef uint64 UIntPtr;					///< unsigned integer pointer (64 Bit)
#else
typedef int32 IntPtr;					///< integer pointer (32 Bit)
typedef uint32 UIntPtr;					///< unsigned integer pointer (32 Bit)
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////
// Embedded platform types
//////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(DSP_TI32) 
typedef long int40;						///< 40 bit integer (C6000 MAC)
typedef uint32 abs_time;				///< On DSP, this is 32 bit. On desktop, it is 64 bit
#elif defined(DSP_SHARC)
typedef int64 int40;					///< 40 bit integer (C6000 MAC)
typedef uint32 abs_time;				///< On DSP, this is 32 bit. On desktop, it is 64 bit
#elif defined(LITTLEKERNEL)
typedef int64 int40;
typedef lk_bigtime_t abs_time;
#else
typedef int64 int40;
typedef int64 abs_time;
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////
// Floating-point types
//////////////////////////////////////////////////////////////////////////////////////////////////

/** 32 Bit floating-point type. */
typedef float float32;

/** 64 Bit floating-point type. */
typedef double float64;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Semantic types
//////////////////////////////////////////////////////////////////////////////////////////////////

/** Compiler-independent boolean type. */
typedef uint8 tbool;

#undef DEFINE_ENUM

/** Define compiler-independent enum type. */
#define DEFINE_ENUM(Name) \
typedef Core::int32 Name; \
enum Name##Enum

/** Platform-specific Module Reference. */
typedef void* ModuleRef;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Character types
//////////////////////////////////////////////////////////////////////////////////////////////////

#if _MSC_VER
typedef __wchar_t uchar;	///< 16 Bit Unicode character (UTF-16)
#else
typedef uint16 uchar;
#endif

typedef uint32 uchar32;		///< 32 Bit Unicode character (UTF-32)

//////////////////////////////////////////////////////////////////////////////////////////////////
// String types
//////////////////////////////////////////////////////////////////////////////////////////////////

/** Constant C-string pointer. */
typedef const char* CStringPtr;

/** Constant 16 Bit Unicode string pointer (UTF-16). */
typedef const uchar* UStringPtr;

/** Buffer for C-string output. */
struct StringResult
{
	char* charBuffer;
	int charBufferSize;

	StringResult (char* charBuffer, int charBufferSize)
	: charBuffer (charBuffer),
	  charBufferSize (charBufferSize)
	{}
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// String macros
//////////////////////////////////////////////////////////////////////////////////////////////////

#undef ENDLINE
#undef FORMAT_INT64

/** End of line. */
#if CORE_PLATFORM_WINDOWS
	#define ENDLINE			"\r\n"
#else
	#define ENDLINE			"\n"
#endif

/** printf placeholder for 64 Bit integer. */
#if _MSC_VER
	#define FORMAT_INT64	"I64"
#else
	#define FORMAT_INT64	"ll"
#endif

/** Max. size of string buffer on stack. */
#if defined(CORE_PLATFORM_RTOS)
	#define STRING_STACK_SPACE_MAX 512
#else
	#define STRING_STACK_SPACE_MAX 4096
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////
// ExitCode
/** Process exit code. */
//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_ENUM (ExitCode)
{
	kExitSuccess = 0,	///< process terminated cleanly
	kExitError = 1		///< process terminated with error
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// Severity
/** Logging severity. */
//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_ENUM (Severity)
{
	kSeverityFatal,		///< fatal level
	kSeverityError,		///< error level
	kSeverityWarning,	///< warning level
	kSeverityInfo,		///< info level
	kSeverityDebug,		///< debug level
	kSeverityTrace		///< trace level
};

} // namespace Core

#endif // _coretypes_h
