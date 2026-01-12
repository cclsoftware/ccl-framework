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
// Filename    : core/malloc/corenewoperatorstubs.cpp
// Description : Global new/delete operator stubs for products without a heap
//
//************************************************************************************************

#include "core/public/coreplatform.h"

#include <new>

//************************************************************************************************
// Global new operators
//************************************************************************************************

void* operator new (std::size_t size)
{
	return (void*)0xDEADC0DE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void* operator new[] (std::size_t size)
{
	return (void*)0xDEADC0DE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

#if __cpp_aligned_new

void operator delete (void* pointer, std::size_t size, std::align_val_t alignment) noexcept
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void operator delete[] (void* pointer, std::size_t size, std::align_val_t alignment) noexcept
{}

#endif
