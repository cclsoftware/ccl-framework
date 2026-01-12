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
// Filename    : ccl/public/base/platform.h
// Description : Platform definitions
//
//************************************************************************************************

#ifndef _ccl_platform_h
#define _ccl_platform_h

//////////////////////////////////////////////////////////////////////////////////////////////////
// Import Core Library Definitions
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "core/public/coretypes.h"

#if defined(CORE_PLATFORM_DESKTOP)
	#define CCL_PLATFORM_DESKTOP 1
#endif

#if defined(CORE_PLATFORM_64BIT)
	#define CCL_PLATFORM_64BIT 1
#endif

#if defined(CORE_PLATFORM_INTEL)
	#define CCL_PLATFORM_INTEL 1
#endif

#if defined(CORE_PLATFORM_ARM)
	#define CCL_PLATFORM_ARM 1
#endif

#if defined(CORE_PLATFORM_ARM64EC)
	#define CCL_PLATFORM_ARM64EC 1
#endif

#define CCL_LITTLE_ENDIAN		CORE_LITTLE_ENDIAN
#define CCL_BIG_ENDIAN			CORE_BIG_ENDIAN
#define CCL_NATIVE_BYTEORDER	CORE_NATIVE_BYTEORDER
#define CCL_FALLTHROUGH         CORE_FALLTHROUGH

#if defined(CORE_DEBUG_INTERNAL)
	#define CCL_DEBUG_INTERNAL 1
#endif

namespace CCL {

using Core::int8;
using Core::uint8;
using Core::int16;
using Core::uint16;
using Core::int32;
using Core::uint32;
using Core::int64;
using Core::uint64;
using Core::IntPtr;
using Core::UIntPtr;
using Core::float32;
using Core::float64;
using Core::tbool;
using Core::uchar;
using Core::uchar32;
using Core::CStringPtr;
using Core::UStringPtr;
using Core::StringResult;
using Core::ModuleRef;
using Core::ExitCode;
using Core::kExitSuccess;
using Core::kExitError;
using Core::Severity;
using Core::kSeverityFatal;
using Core::kSeverityError;
using Core::kSeverityWarning;
using Core::kSeverityInfo;
using Core::kSeverityDebug;
using Core::kSeverityTrace;

enum ByteOrder
{
	kLittleEndian = CCL_LITTLE_ENDIAN,
	kBigEndian = CCL_BIG_ENDIAN,
	kNativeByteOrder = CCL_NATIVE_BYTEORDER
};

} // namespace CCL

//////////////////////////////////////////////////////////////////////////////////////////////////
// Platform identifiers
//////////////////////////////////////////////////////////////////////////////////////////////////

#define CCL_PLATFORM_ID_WIN				"win"
#define CCL_PLATFORM_ID_MAC				"mac"
#define CCL_PLATFORM_ID_IOS				"ios"
#define CCL_PLATFORM_ID_ANDROID			"android"
#define CCL_PLATFORM_ID_LINUX			"linux"

#if CCL_PLATFORM_ARM
	#if CCL_PLATFORM_64BIT
		#define CCL_PLATFORM_ARCH "Arm64"
	#else
		#define CCL_PLATFORM_ARCH "Arm"
	#endif
#elif CCL_PLATFORM_ARM64EC
	#define CCL_PLATFORM_ARCH "Arm64EC"
#elif CCL_PLATFORM_INTEL
	#if CCL_PLATFORM_64BIT
		#define CCL_PLATFORM_ARCH "x64"
	#else
		#define CCL_PLATFORM_ARCH "x86"
	#endif
#endif

#if CORE_PLATFORM_WINDOWS
	#define CCL_PLATFORM_WINDOWS		1
	#define CCL_PLATFORM_ID_CURRENT		CCL_PLATFORM_ID_WIN
	#define CCL_OS_NAME					"Windows"
	#define CCL_PLATFORM_STRING			"Win " CCL_PLATFORM_ARCH

#elif CORE_PLATFORM_IOS
	#define CCL_PLATFORM_IOS			1
	#define CCL_PLATFORM_ID_CURRENT		CCL_PLATFORM_ID_IOS
	#define CCL_OS_NAME					"iOS"
	#define CCL_PLATFORM_STRING			"iOS"

#elif CORE_PLATFORM_ANDROID
	#define CCL_PLATFORM_ANDROID		1
	#define CCL_PLATFORM_ID_CURRENT		CCL_PLATFORM_ID_ANDROID
	#define CCL_OS_NAME					"Android"
	#define CCL_PLATFORM_STRING			"Android " CCL_PLATFORM_ARCH

#elif CORE_PLATFORM_MAC
	#define CCL_PLATFORM_MAC			1
	#define CCL_PLATFORM_ID_CURRENT		CCL_PLATFORM_ID_MAC
	#define CCL_OS_NAME					"macOS"
	#define CCL_PLATFORM_STRING			"macOS " CCL_PLATFORM_ARCH
	
#elif CORE_PLATFORM_LINUX
	#define CCL_PLATFORM_LINUX			1
	#define CCL_PLATFORM_ID_CURRENT		CCL_PLATFORM_ID_LINUX
	#define CCL_OS_NAME					"Linux"
	#define CCL_PLATFORM_STRING			"Linux " CCL_PLATFORM_ARCH
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////
// Meta info
//////////////////////////////////////////////////////////////////////////////////////////////////

#define NAMESPACE_CCL "CCL"

//////////////////////////////////////////////////////////////////////////////////////////////////
// API definitions
//////////////////////////////////////////////////////////////////////////////////////////////////

#undef CCL_API
#undef CCL_VCALL
#undef CCL_NOVTABLE
#undef CCL_ALIGN
#undef CCL_EXPORT

/** 16 Byte Alignement macro. */
#define CCL_ALIGN(type) CORE_ALIGN(type,16)

/** Exported functions. */
#define CCL_EXPORT CORE_EXPORT

#if _MSC_VER
	/** Interface calling convention. */
	#define CCL_API			__stdcall
	/** Calling convention for variable arguments. */
	#define CCL_VCALL		__cdecl
	/** Eliminate vfptr initialization for interfaces. */
	#define CCL_NOVTABLE	__declspec(novtable)
#else
	#define CCL_API
	#define CCL_VCALL
	#define CCL_NOVTABLE
#endif

#undef interface
/** Interface keyword alias. */
#define interface struct

//////////////////////////////////////////////////////////////////////////////////////////////////
// Exception handling
//////////////////////////////////////////////////////////////////////////////////////////////////

#undef TRY
#undef EXCEPT

#if (RELEASE || CCL_ENABLE_EXCEPTION_HANDLING)
	#define TRY try
	#define EXCEPT catch (...)
#else
	#define TRY if(1)
	#define EXCEPT if(0)
#endif

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////
/** Four-character code. */
//////////////////////////////////////////////////////////////////////////////////////////////////

struct FOURCC
{
	union 
	{
		char bytes[4];
		int32 fcc; 
	};

	bool operator == (FOURCC other) const { return fcc == other.fcc; }
	bool operator != (FOURCC other) const { return fcc != other.fcc; }
};

/** Four-character code definition. */
#define DEFINE_FOURCC(name, c1, c2, c3, c4) \
	CCL::FOURCC name = {char(c1), char(c2), char(c3), char(c4)};

//////////////////////////////////////////////////////////////////////////////////////////////////
// Fundamental reference types
//////////////////////////////////////////////////////////////////////////////////////////////////

struct Variant;
typedef const Variant& VariantRef;

class String;
typedef const String& StringRef;

class CString;
typedef const CString& CStringRef;

interface IUrl;
typedef const IUrl& UrlRef;

interface IMessage;
typedef const IMessage& MessageRef;

class ArgumentList;
typedef const ArgumentList& ArgsRef;

/** String identifier. */
typedef CStringRef StringID;

} // namespace CCL

#endif // _ccl_platform_h
