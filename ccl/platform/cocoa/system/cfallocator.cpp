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
// Filename    : ccl/platform/cocoa/system/cfallocator.cpp
// Description : Mac allocator class
//
//************************************************************************************************

#include "ccl/platform/cocoa/system/cfallocator.h"

#include "ccl/public/base/platform.h"

#include <CoreFoundation/CFString.h>

namespace CCL {
namespace MacOS {

//************************************************************************************************
// CFAllocator
//************************************************************************************************

const void* CFAllocatorRetain (const void *info);
void CFAllocatorRelease (const void *info);
CFStringRef CFAllocatorCopyDescription (const void *info);
void* CFAllocatorAllocate (CFIndex allocSize, CFOptionFlags hint, void *info);
void* CFAllocatorReallocate (void *ptr, CFIndex newsize, CFOptionFlags hint, void *info);
void CFAllocatorDeallocate (void *ptr, void *info);
CFIndex CFAllocatorPreferredSize (CFIndex size, CFOptionFlags hint, void *info);

//////////////////////////////////////////////////////////////////////////////////////////////////

CFAllocatorRef GetAllocator ()
{
	static CFAllocatorRef allocator = nullptr;
	static CFAllocatorContext context = {0};
	
	if(allocator == 0)
	{
		context.version = 0;
		context.info = 0;
		context.retain = CFAllocatorRetain;
		context.release = CFAllocatorRelease;
		context.copyDescription = CFAllocatorCopyDescription;
		context.allocate = CFAllocatorAllocate;
		context.reallocate = CFAllocatorReallocate;
		context.deallocate = CFAllocatorDeallocate;
		context.preferredSize = CFAllocatorPreferredSize;
		
		allocator = CFAllocatorCreate (kCFAllocatorUseContext, &context);
	}
	return allocator;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const void* CFAllocatorRetain (const void *info)
{
	return info;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
	
void CFAllocatorRelease (const void *info)
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CFStringRef CFAllocatorCopyDescription (const void *info)
{
	return CFSTR ("CCL Allocator");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void* CFAllocatorAllocate (CFIndex allocSize, CFOptionFlags hint, void *info)
{
	return core_malloc ((int)allocSize); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void* CFAllocatorReallocate (void *ptr, CFIndex newsize, CFOptionFlags hint, void *info)
{
	return core_realloc (ptr, (int)newsize);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CFAllocatorDeallocate (void *ptr, void *info)
{
	core_free (ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CFIndex CFAllocatorPreferredSize (CFIndex size, CFOptionFlags hint, void *info)
{
	return size;
}	

//////////////////////////////////////////////////////////////////////////////////////////////////
	
} // namespace MacOS
} // namespace CCL
