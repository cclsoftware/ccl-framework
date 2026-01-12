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
// Filename    : ccl/public/system/iallocator.h
// Description : Memory Allocator
//
//************************************************************************************************

#ifndef _ccl_iallocator_h
#define _ccl_iallocator_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

//************************************************************************************************
// IAllocator
/** Memory allocation interface. 
	\ingroup ccl_system */
//************************************************************************************************

interface IAllocator: IUnknown
{
	/** Allocate memory block of given size. Returns memory address in case of success or null otherwise. */
	virtual void* CCL_API allocate (unsigned int size) = 0;
	
	/** Resize existing memory block. Returns new memory address in case of success or null otherwise. */
	virtual void* CCL_API reallocate (void* ptr, unsigned int size) = 0;

	/** Dispose a previously allocated memory block. */
	virtual void CCL_API dispose (void* ptr) = 0;
	
	DECLARE_IID (IAllocator)
};

DEFINE_IID (IAllocator, 0x84fcbd1c, 0xbac2, 0x4aba, 0x8f, 0x53, 0x9b, 0x5f, 0x2f, 0x8b, 0x9f, 0xeb)

} // namespace CCL

#endif // _ccl_iallocator_h
