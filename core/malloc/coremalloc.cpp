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
// Filename    : core/malloc/coremalloc.cpp
// Description : Memory Allocator
//
//************************************************************************************************

// Debug build with Microsoft compiler (not aligned)
#define HAVE_CRTDBG_H			(0 && _MSC_VER && DEBUG)

// Debug build with Microsoft compiler (aligned)
#define HAVE_ALIGNED_CRTDBG_H	(1 && _MSC_VER && DEBUG)	

// Debug build with debug_malloc
#define HAVE_DEBUG_MALLOC_H		(1 && (CORE_PLATFORM_MAC || CORE_PLATFORM_IOS || CORE_PLATFORM_LINUX) && DEBUG)

// disable debug_malloc when running with address sanitizer
#if defined (__has_feature)
#if __has_feature (address_sanitizer)
#undef HAVE_DEBUG_MALLOC_H
#define HAVE_DEBUG_MALLOC_H 0
#endif
#endif

// Release build with aligned malloc
#define ALIGNED_MALLOC			(CORE_PLATFORM_WINDOWS && RELEASE)

#if _MSC_VER
	#define aligned_malloc	_aligned_malloc
	#define aligned_realloc	_aligned_realloc
	#define aligned_free	_aligned_free
#endif

// disable macros in coremalloc.h (required, keep it 0 here!)
#define CORE_MALLOC_ENABLED 0
#include "core/public/coreplatform.h"

#if HAVE_CRTDBG_H || HAVE_ALIGNED_CRTDBG_H
#include <crtdbg.h>
#endif

#if HAVE_DEBUG_MALLOC_H
// include debug_malloc.cpp here to avoid having to add it to project files
#include "submodules/debug_malloc/debug_malloc.cpp"
#endif

//************************************************************************************************
// CRT Debug Heap
//************************************************************************************************

#if HAVE_CRTDBG_H || HAVE_ALIGNED_CRTDBG_H

#ifdef _DLL
#include "core/platform/win/coremalloc.win.h"
#define FILENAME(filename) core_get_debug_filename (filename)
#else
#define FILENAME(filename) filename
#endif

struct CrtDebugInitializer
{
#ifdef _DLL
	CrtDebugInitializer ()
	{
		::_CrtSetDbgFlag (_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
		::_CrtSetReportMode (_CRT_ERROR, _CRTDBG_MODE_DEBUG);
	}
#else
	CrtDebugInitializer ()
	{
		::_CrtSetDbgFlag (_CRTDBG_ALLOC_MEM_DF);
		::_CrtSetReportMode (_CRT_ERROR, _CRTDBG_MODE_DEBUG);
		::atexit (dump);
	}

	static void dump ()
	{
		::_CrtDumpMemoryLeaks ();
	}
#endif
};

static CrtDebugInitializer theInitializer;
#endif

//************************************************************************************************
// Memory Allocation APIs
//************************************************************************************************

#undef core_malloc
#undef core_realloc
#undef core_free

void* core_malloc (unsigned int size)
{
#if HAVE_ALIGNED_CRTDBG_H
	return ::_aligned_malloc (size, 16);
#elif HAVE_DEBUG_MALLOC_H
	return nvwa::debug_malloc (size, nullptr, 0);
#elif ALIGNED_MALLOC
	return ::aligned_malloc (size, 16);
#else
	return ::malloc (size);
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void* core_malloc_debug (unsigned int size, const char* filename, int line)
{
#if HAVE_ALIGNED_CRTDBG_H
	return ::_aligned_malloc_dbg (size, 16, FILENAME (filename), line);
#elif HAVE_CRTDBG_H
	return ::_malloc_dbg (size, _NORMAL_BLOCK, FILENAME (filename), line);
#elif HAVE_DEBUG_MALLOC_H
	return nvwa::debug_malloc (size, filename, line);
#elif ALIGNED_MALLOC
	return ::aligned_malloc (size, 16);
#else
	return ::malloc (size);
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void* core_realloc (void* memory, unsigned int size)
{
#if HAVE_ALIGNED_CRTDBG_H
	return ::_aligned_realloc (memory, size, 16);
#elif HAVE_DEBUG_MALLOC_H
	return nvwa::debug_realloc (memory, size, __FILE__ , __LINE__);
#elif ALIGNED_MALLOC
	return ::aligned_realloc (memory, size, 16);
#else
	return ::realloc (memory, size);
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void* core_realloc_debug (void* memory, unsigned int size, const char* filename, int line)
{
#if HAVE_ALIGNED_CRTDBG_H
	return ::_aligned_realloc_dbg (memory, size, _NORMAL_BLOCK, FILENAME (filename), line);
#elif HAVE_CRTDBG_H
	return ::_realloc_dbg (memory, size, _NORMAL_BLOCK, FILENAME (filename), line);
#elif HAVE_DEBUG_MALLOC_H
	return nvwa::debug_realloc (memory, size, filename, line);
#elif ALIGNED_MALLOC
	return ::aligned_realloc (memory, size, 16);
#else
	return ::realloc (memory, size);
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void core_free (void* memory)
{
#if HAVE_ALIGNED_CRTDBG_H
	::_aligned_free_dbg (memory);
#elif HAVE_CRTDBG_H
	::_free_dbg (memory, _NORMAL_BLOCK);
#elif HAVE_DEBUG_MALLOC_H
	nvwa::debug_free (memory);
#elif ALIGNED_MALLOC
	::aligned_free (memory);
#else
	::free (memory);
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void core_alloc_use ()
{
#if HAVE_DEBUG_MALLOC_H
	nvwa::debug_malloc_use ();
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void core_alloc_unuse ()
{
#if HAVE_DEBUG_MALLOC_H
	nvwa::debug_malloc_unuse ();
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int core_check_heap ()
{
#if HAVE_CRTDBG_H || HAVE_ALIGNED_CRTDBG_H
	return ::_CrtCheckMemory ();
#elif HAVE_DEBUG_MALLOC_H
	return nvwa::check_mem_corruption () == 0;
#else
	return 1;
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int core_check_ptr (void* ptr, int size)
{
#if HAVE_CRTDBG_H
	return _CrtIsValidPointer (ptr, size, 1) && _CrtIsValidHeapPointer (ptr);
#else
	return 1;
#endif	
}

//************************************************************************************************
/** Replacement new/delete operators 
* \ingroup core_malloc
* These operator definitions replace all occurences of new/delete in the program with calls to core_malloc/core_free.
* Note that there must never be more than one definition of each of these operators in a program:
*   "The program is ill-formed, no diagnostic required if more than one replacement is provided in the program 
*     for any of the replaceable allocation function, or if a replacement is declared with the inline specifier." 
* On Windows, Mac and iOS this means that there must never be more than one definition in a shared library or executable.
* On Linux and Android this means that there must never be more than one definition in the process.
*/
//************************************************************************************************

// CORE_DISABLE_NEW_OPERATOR might be set by CMake when linking ccltext, see ccltext.<platform>.cmake
#ifndef CORE_DISABLE_NEW_OPERATOR
#include "core/malloc/corenewoperator.cpp"
#endif
