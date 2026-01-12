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
// Filename    : ccl/text/cclmalloc.cpp
// Description : Memory Allocator
//
//************************************************************************************************

#define DEBUG_LOG 0

// Debug build with Microsoft compiler (not aligned)
#define HAVE_CRTDBG_H			(0 && _MSC_VER && DEBUG)	

// Debug build with Microsoft compiler (aligned)
#define HAVE_ALIGNED_CRTDBG_H	(1 && _MSC_VER && DEBUG)	

// Debug build with debug_malloc
#define HAVE_DEBUG_MALLOC_H		(1 && (CCL_PLATFORM_MAC || CCL_PLATFORM_IOS || CCL_PLATFORM_LINUX) && DEBUG)

// disable debug_malloc when running with address sanitizer
#if defined (__has_feature)
#if __has_feature (address_sanitizer)
#undef HAVE_DEBUG_MALLOC_H
#define HAVE_DEBUG_MALLOC_H 0
#endif
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////

// disable macros in coremalloc.h (required, keep it 0 here!)
#define CORE_MALLOC_ENABLED 0
#include "ccl/public/base/platform.h"

#if HAVE_CRTDBG_H || HAVE_ALIGNED_CRTDBG_H
#include <malloc.h>
// Note: We do not need to define _CRTDBG_MAP_ALLOC, because we redirect ourselves.
#include <crtdbg.h>
#endif

#if HAVE_DEBUG_MALLOC_H
#include "submodules/debug_malloc/debug_malloc.h"
#endif

#if DEBUG_LOG
bool printLog = false;
#define DEBUG_PRINT(s,...) if(printLog) CCL::Debugger::printf (s, __VA_ARGS__);
#else
#define DEBUG_PRINT(s,...)
#endif

#include "ccl/public/base/debug.h"

//************************************************************************************************
// CRT Debug Heap
//************************************************************************************************

#if HAVE_CRTDBG_H || HAVE_ALIGNED_CRTDBG_H

#define FILENAME(filename) ccl_get_debug_filename (filename)
extern const char* ccl_get_debug_filename (const char* filename); // cclmalloc.win.cpp

struct CrtDebugInitializer
{
#if CCL_STATIC_LINKAGE || defined (_DLL)
	CrtDebugInitializer ()
	{
		::_CrtSetDbgFlag (_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
		::_CrtSetReportMode (_CRT_ERROR, _CRTDBG_MODE_DEBUG);
	}		
#else
	CrtDebugInitializer ()
	{
		// Note: automatic leak dump would be done twice for some reason???
		::_CrtSetDbgFlag (_CRTDBG_ALLOC_MEM_DF /*| _CRTDBG_LEAK_CHECK_DF*/);
		::_CrtSetReportMode (_CRT_ERROR, _CRTDBG_MODE_DEBUG);
		::atexit (dump);
	}
	static void dump ()
	{
		_CrtDbgReport (_CRT_WARN, 0, 0, 0, "*** ccltext @exit: Checking for memory leaks ***\n");
		_CrtDumpMemoryLeaks ();
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

CCL_EXPORT void* core_malloc (unsigned int size)
{
	DEBUG_PRINT ("Alloc %d\n", size)

#if HAVE_ALIGNED_CRTDBG_H
	return ::_aligned_malloc_dbg (size, 16, __FILE__, __LINE__);
#elif HAVE_CRTDBG_H
	return ::_malloc_dbg (size, _NORMAL_BLOCK, __FILE__ , __LINE__);
#elif HAVE_DEBUG_MALLOC_H
	return nvwa::debug_malloc (size, nullptr, 0);
#else
	return ::malloc (size);
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT void* core_malloc_debug (unsigned int size, const char* filename, int line)
{
	DEBUG_PRINT ("Alloc %d %s:%d\n", size, filename, line)

#if HAVE_ALIGNED_CRTDBG_H
	return ::_aligned_malloc_dbg (size, 16, FILENAME (filename), line);
#elif HAVE_CRTDBG_H
	return ::_malloc_dbg (size, _NORMAL_BLOCK, FILENAME (filename), line);
#elif HAVE_DEBUG_MALLOC_H
	return nvwa::debug_malloc (size, filename, line);
#else
	return ::malloc (size);
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT void* core_realloc (void* memory, unsigned int size)
{
	DEBUG_PRINT ("Reall %p %d\n", memory, size)

#if HAVE_ALIGNED_CRTDBG_H
	return ::_aligned_realloc (memory, size, 16);
#elif HAVE_CRTDBG_H
	return ::_realloc_dbg (memory, size,_NORMAL_BLOCK, __FILE__ , __LINE__);
#elif HAVE_DEBUG_MALLOC_H
	return nvwa::debug_realloc (memory, size, nullptr, 0);
#else
	return ::realloc (memory, size);
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT void* core_realloc_debug (void* memory, unsigned int size, const char* filename, int line)
{
	DEBUG_PRINT ("Reall %p %d %s:%d\n", memory, size, filename, line)

#if HAVE_ALIGNED_CRTDBG_H
	return ::_aligned_realloc_dbg (memory, size, _NORMAL_BLOCK, FILENAME (filename), line);
#elif HAVE_CRTDBG_H
	return ::_realloc_dbg (memory, size, _NORMAL_BLOCK, FILENAME (filename), line);
#elif HAVE_DEBUG_MALLOC_H
	return nvwa::debug_realloc (memory, size, filename, line);
#else
	return ::realloc (memory, size);
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT void core_free (void* memory)
{
	DEBUG_PRINT ("Free %p\n", memory)

#if HAVE_ALIGNED_CRTDBG_H
	::_aligned_free_dbg (memory);
#elif HAVE_CRTDBG_H
	::_free_dbg (memory, _NORMAL_BLOCK);
#elif HAVE_DEBUG_MALLOC_H
	nvwa::debug_free (memory);
#else
	::free (memory);
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT void core_alloc_use ()
{
#if HAVE_DEBUG_MALLOC_H
	nvwa::debug_malloc_use ();
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT void core_alloc_unuse ()
{
#if HAVE_DEBUG_MALLOC_H
	nvwa::debug_malloc_unuse ();
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT int core_check_heap ()
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

CCL_EXPORT int core_check_ptr (void* ptr, int size)
{
#if HAVE_CRTDBG_H
	return _CrtIsValidPointer (ptr, size, 1) && _CrtIsValidHeapPointer (ptr);
#else
	return 1;
#endif	
}

//************************************************************************************************
// Global new/delete operators
//************************************************************************************************

#if !CCL_STATIC_LINKAGE
#include "core/malloc/corenewoperator.cpp"
#endif
