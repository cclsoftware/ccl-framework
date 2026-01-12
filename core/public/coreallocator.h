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
// Filename    : core/public/coreallocator.h
// Description : Allocator interface and implementation classes
//
//************************************************************************************************

#ifndef _coreallocator_h
#define _coreallocator_h

#include "core/public/coretypes.h"

namespace Core {

//************************************************************************************************
// Allocator
/** Memory allocator interface. */
//************************************************************************************************

struct Allocator
{
	/** Get default allocator instance. */
	static Allocator& getDefault ();

	virtual ~Allocator () {}
	
	/** Allocate a contiguous memory block of a given size. */
	virtual void* allocate (uint32 size) = 0;
	
	/**	Resize a memory block that was previously allocated with allocate or allocate a new block.
		Might move existing data to a new location. */
	virtual void* reallocate (void* address, uint32 size) = 0;
	
	/** Free previously allocated data. */
	virtual void deallocate (void* address) = 0;
};

//************************************************************************************************
// HeapAllocator
/** Heap allocator class. */
//************************************************************************************************

class HeapAllocator: public Allocator
{
public:
	// Allocator
	void* allocate (uint32 size) override;
	void* reallocate (void* address, uint32 size) override;
	void deallocate (void* address) override;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// Allocator implementation
//////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef DSP_TI32
inline Allocator& Allocator::getDefault ()
{
	static HeapAllocator defaultAllocator;
    return defaultAllocator;
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////
// HeapAllocator implementation
//////////////////////////////////////////////////////////////////////////////////////////////////

inline void* HeapAllocator::allocate (uint32 size) 
{ 
	return ::core_malloc (size); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void* HeapAllocator::reallocate (void* address, uint32 size)
{ 
	return ::core_realloc (address, size); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void HeapAllocator::deallocate (void* address)
{ 
	::core_free (address);
}

} // namespace Core

#endif // _coreallocator_h


