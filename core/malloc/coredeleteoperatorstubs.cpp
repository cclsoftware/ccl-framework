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
// Filename    : core/malloc/coredeleteoperatorstubs.cpp
// Description : Global new/delete operator stubs for products without a heap
//
//************************************************************************************************

#include "core/public/coreplatform.h"

#include <new>

//************************************************************************************************
// Global delete operators
//************************************************************************************************

#if __cpp_decltype
void operator delete (void* memory) noexcept
#else
void operator delete (void* memory) throw ()
#endif
{}

#if __cpp_decltype_auto
void operator delete (void* memory, size_t size) noexcept
{
	operator delete (memory);
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////

#if __cpp_decltype
void operator delete[] (void* memory) noexcept
#else
void operator delete[] (void* memory) throw ()
#endif
{}

#if __cpp_decltype_auto
void operator delete[] (void* memory, size_t size) noexcept
{
	operator delete[] (memory);
}
#endif
