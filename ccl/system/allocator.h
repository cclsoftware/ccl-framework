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
// Filename    : ccl/system/allocator.h
// Description : Memory Allocator
//
//************************************************************************************************

#ifndef _ccl_allocator_h
#define _ccl_allocator_h

#include "ccl/public/base/unknown.h"
#include "ccl/public/system/iallocator.h"

namespace CCL {

//************************************************************************************************
// StandardAllocator
//************************************************************************************************

class StandardAllocator: public Unknown,
						 public IAllocator
{
public:
	// IAllocator
	void* CCL_API allocate (unsigned int size) override;
	void* CCL_API reallocate (void* ptr, unsigned int size) override;
	void CCL_API dispose (void* ptr) override;

	CLASS_INTERFACE (IAllocator, Unknown)
};

} // namespace CCL

#endif // _ccl_allocator_h
