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
// Filename    : ccl/system/allocator.cpp
// Description : Memory Allocator
//
//************************************************************************************************

#include "ccl/system/allocator.h"

#include "ccl/public/systemservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// System Services API
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT IAllocator& CCL_API System::CCL_ISOLATED (GetMemoryAllocator) ()
{
	static StandardAllocator theAllocator;
	return theAllocator;
}

//************************************************************************************************
// StandardAllocator
//************************************************************************************************

void* CCL_API StandardAllocator::allocate (unsigned int size)
{
	return ::core_malloc (size);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void* CCL_API StandardAllocator::reallocate (void* ptr, unsigned int size)
{
	return ::core_realloc (ptr, size);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API StandardAllocator::dispose (void* ptr)
{
	::core_free (ptr);
}
