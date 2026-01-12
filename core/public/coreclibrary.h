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
// Filename    : core/public/coreclibrary.h
// Description : C Runtime Library includes and substitutes
//
//************************************************************************************************

#ifndef _coreclibrary_h
#define _coreclibrary_h

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <wctype.h>
#include <assert.h>
#include <stddef.h>

#if __linux__ && !defined(__ANDROID__)
using std::nullptr_t;
#endif

#if _MSC_VER
	#define strcasecmp _stricmp
#endif

#if defined(CTL_RTOS) || defined(__ZEPHYR__) || defined(LITTLEKERNEL)
	#define isascii isASCII // works for gcc and clang
#endif

#if defined (DSP_TI32)
	#define CORE_STRCASECMP_NOT_PROVIDED

#elif defined (__ZEPHYR__)
	#define CORE_SSCANF_NOT_PROVIDED
	#define CORE_STRCASECMP_NOT_PROVIDED

#elif defined (LITTLEKERNEL)
	#define CORE_SSCANF_NOT_PROVIDED
	#define CORE_STRCASECMP_NOT_PROVIDED
#endif

#ifdef CORE_STRCASECMP_NOT_PROVIDED
	extern "C" int strcasecmp (char const* s1, char const* s2);
#endif

#ifdef CORE_SSCANF_NOT_PROVIDED
	extern "C" int sscanf (const char* s, const char* format, ...);
#endif

#endif // _coreclibrary_h
