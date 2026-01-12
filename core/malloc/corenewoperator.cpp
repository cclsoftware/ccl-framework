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
// Filename    : core/malloc/corenewoperator.cpp
// Description : Global new/delete operators
//
//************************************************************************************************

#include "core/public/coreplatform.h"

#if CORE_MALLOC_AVAILABLE

#include <new>

//************************************************************************************************
// Global new/delete operators
//************************************************************************************************

void* operator new (std::size_t size)
{
	#if DEBUG
	// setting the filename parameter to 0 prevents the debug heap from catching foreign unbalanced allocations
	return core_malloc_debug ((unsigned int)size, nullptr, 0);
	#else
	return core_malloc ((unsigned int)size);
	#endif
}

#if __cpp_decltype
void operator delete (void* memory) noexcept
#else
void operator delete (void* memory) throw ()
#endif
{
	if(memory)
		core_free (memory);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void* operator new[] (std::size_t size)
{
	#if DEBUG
	return core_malloc_debug ((unsigned int)size, nullptr, 0);
	#else
	return core_malloc ((unsigned int)size);
	#endif
}

#if __cpp_decltype
void operator delete[] (void* memory) noexcept
#else
void operator delete[] (void* memory) throw ()
#endif
{
	if(memory)
		core_free (memory);
}

//************************************************************************************************
// Global non-throwing new/delete operators
//************************************************************************************************

#if __cpp_decltype
void* operator new (std::size_t size, const std::nothrow_t& tag) noexcept
#else
void* operator new (std::size_t size, const std::nothrow_t& tag) throw ()
#endif
{
	return operator new (size);
}

#if __cpp_decltype
void operator delete (void* memory, const std::nothrow_t& tag) noexcept
#else
void operator delete (void* memory, const std::nothrow_t& tag) throw ()
#endif
{
	operator delete (memory);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

#if __cpp_decltype
void* operator new[] (std::size_t size, const std::nothrow_t& tag) noexcept
#else
void* operator new[] (std::size_t size, const std::nothrow_t& tag) throw ()
#endif
{
	return operator new[] (size);
}

#if __cpp_decltype
void operator delete[] (void* memory, const std::nothrow_t& tag) noexcept
#else
void operator delete[] (void* memory, const std::nothrow_t& tag) throw ()
#endif
{
	operator delete[] (memory);
}

//************************************************************************************************
// Global delete operators with size hint (C++14)
//************************************************************************************************

#if __cpp_decltype_auto
void operator delete (void* memory, size_t size) noexcept
{
	operator delete (memory);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void operator delete[] (void* memory, size_t size) noexcept
{
	operator delete[] (memory);
}
#endif

#endif
