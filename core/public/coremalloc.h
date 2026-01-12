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
// Filename    : core/public/coremalloc.h
// Description : Memory Allocator
//
//************************************************************************************************

#ifndef _coremalloc_h
#define _coremalloc_h

#ifndef _coreplatform_h
#error Do not include this file directly, it will be included by coreplatform.h! 
#endif

#if defined(CORE_PLATFORM_RTOS) || defined(__SANITIZE_ADDRESS__)
	#define CORE_MALLOC_AVAILABLE 0
#else
	#define CORE_MALLOC_AVAILABLE 1
#endif

#ifndef CORE_MALLOC_ENABLED
	#define CORE_MALLOC_ENABLED CORE_MALLOC_AVAILABLE
#endif

#undef NEW

//////////////////////////////////////////////////////////////////////////////////////////////////
// Memory Allocation APIs
//////////////////////////////////////////////////////////////////////////////////////////////////

/*
	Notes on memory alignment:
	
	core_malloc, core_realloc and the corresponding _debug functions attempt to align memory on 16
	byte boundaries, but this is guaranteed only on Windows and macOS desktop platforms.
	
	When memory alignment is a hard requirement (e.g. for using SIMD instructions), users of these
	functions must ensure the correct alignment by other means, e.g. using a Core::IO::Buffer.
*/

#ifdef __cplusplus
extern "C"
{
#endif
	void* core_malloc (unsigned int size);
	void* core_malloc_debug (unsigned int size, const char* filename, int line);
	void* core_realloc (void* memory, unsigned int size);
	void* core_realloc_debug (void* memory, unsigned int size, const char* filename, int line);
	void core_free (void* memory);

	void core_alloc_use (); // increment use count in debug mode
    void core_alloc_unuse (); // decrement use count, print leak report when reaching 0

	int core_check_heap ();
	int core_check_ptr (void* ptr, int size);
#ifdef __cplusplus
}
#endif

#if CORE_MALLOC_ENABLED

//////////////////////////////////////////////////////////////////////////////////////////////////
// Static usage counter object
//////////////////////////////////////////////////////////////////////////////////////////////////

#if DEBUG
#ifdef __cplusplus
class core_alloc_use_counter
{
public:
	core_alloc_use_counter () { core_alloc_use (); }
	~core_alloc_use_counter () { core_alloc_unuse (); }
};

// Counting object for each file including coremalloc.h.
static core_alloc_use_counter __core_alloc_use_count;
#endif
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////
// Redirected memory functions
//////////////////////////////////////////////////////////////////////////////////////////////////

#if DEBUG
	#define core_malloc(s)		core_malloc_debug (s, __FILE__, __LINE__)
	#define core_realloc(p,s)	core_realloc_debug (p, s, __FILE__, __LINE__)
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////
// Global new/delete operators
//////////////////////////////////////////////////////////////////////////////////////////////////

#if DEBUG
	#define NEW ::new (__FILE__, __LINE__)
#else
	#define NEW	::new
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////

INLINE void* operator new (size_t size, const char* filename, int line)
{
	return core_malloc_debug ((unsigned int)size, filename, line);
}

INLINE void operator delete (void* memory, const char* filename, int line)
{
	if(memory)
		core_free (memory);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

INLINE void* operator new[] (size_t size, const char* filename, int line)
{
	return core_malloc_debug ((unsigned int)size, filename, line);
}

INLINE void operator delete[] (void* memory, const char* filename, int line)
{
	if(memory)
		core_free (memory);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

#else
#define NEW ::new
#endif // !CORE_MALLOC_ENABLED

#if !CORE_MALLOC_AVAILABLE
#define core_malloc(s) malloc (s)
#define core_realloc(m, s) realloc (m, s)
#define core_free(m) free (m)
#endif // !CORE_MALLOC_AVAILABLE

#endif // _coremalloc_h
