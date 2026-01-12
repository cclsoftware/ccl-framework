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
// Filename    : core/public/corepoolallocator.h
// Description : Pool Allocator
//
//************************************************************************************************

#ifndef _corepoolallocator_h
#define _corepoolallocator_h

#include "core/public/coreallocator.h"
#include "core/public/corebasicmacros.h"

#define CORE_POOLALLOCATOR_USE_BITFIELD 1 //< slower allocation, but less memory overhead

namespace Core {

//************************************************************************************************
// PoolAllocatorExtern
/** Pool-based allocator base class. Not thread safe! */
//************************************************************************************************

template<typename T, uint32 numBlocks, uint32 blockSize = 1>
class PoolAllocatorExtern: public Allocator
{
public:
	PoolAllocatorExtern (T* pool);

	bool isValidAddress (const T* address) const;

	// Allocator
	void* allocate (uint32 size) override;
	void* reallocate (void* address, uint32 size) override;
	void deallocate (void* address) override;

	#if CORE_DEBUG_INTERNAL
	typedef void (*PrintFunction) (CStringPtr format, ...);
	void printUsage (PrintFunction function);
	#endif

protected:
	static const int kSize = numBlocks * blockSize;
	static const int kBlockSizeBytes = blockSize * sizeof(T);

	T* pool;
	
	#if CORE_POOLALLOCATOR_USE_BITFIELD
	uint32 blockStart[(numBlocks + 31) / 32];
	uint32 blockEnd[(numBlocks + 31) / 32];
	#else
	uint32 allocated[numBlocks];
	#endif

	T* createBlock (uint32 count = 1);
	T* resizeBlock (T* address, uint32 count = 1);
	void freeBlock (T* address);

	#if CORE_DEBUG_INTERNAL
	uint32 maxUsedIndex;
	#endif

	uint32 getAllocatedBlocks (uint32 index) const;
	void setAllocatedBlocks (uint32 index, uint32 count);
	void resetAllocatedBlocks (uint32 index);
};

//************************************************************************************************
// PoolAllocator
/** Pool-based allocator. Not thread safe! */
//************************************************************************************************

template<typename T, uint32 numBlocks, uint32 blockSize = 1, uint32 alignment = 1>
class PoolAllocator: public PoolAllocatorExtern<T, numBlocks, blockSize>
{
public:
	PoolAllocator ()
	: PoolAllocatorExtern<T, numBlocks, blockSize> (staticPool)
	{}

protected:
	CORE_ALIGN (T, alignment) staticPool[PoolAllocatorExtern<T, numBlocks, blockSize>::kSize];
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// PoolAllocatorExtern implementation
//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename T, uint32 numBlocks, uint32 blockSize>
PoolAllocatorExtern<T, numBlocks, blockSize>::PoolAllocatorExtern (T* pool)
: pool (pool),
#if CORE_POOLALLOCATOR_USE_BITFIELD
  blockStart { 0 },
  blockEnd { 0 }
#else
  allocated { 0 }
#endif
#if CORE_DEBUG_INTERNAL
, maxUsedIndex (0)
#endif
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

#if CORE_DEBUG_INTERNAL
template<typename T, uint32 numBlocks, uint32 blockSize>
void PoolAllocatorExtern<T, numBlocks, blockSize>::printUsage (PrintFunction function)
{
	function ("PoolAllocatorExtern usage: %d/%d blocks (%.2f%%).\n", maxUsedIndex, numBlocks, maxUsedIndex * 100. / numBlocks);
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename T, uint32 numBlocks, uint32 blockSize>
bool PoolAllocatorExtern<T, numBlocks, blockSize>::isValidAddress (const T* address) const
{
	uint32 index = static_cast<uint32> (address - pool) / blockSize;
	return index < numBlocks && getAllocatedBlocks (index) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename T, uint32 numBlocks, uint32 blockSize>
inline void* PoolAllocatorExtern<T, numBlocks, blockSize>::allocate (uint32 byteSize)
{
	return createBlock ((byteSize + kBlockSizeBytes - 1) / kBlockSizeBytes);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename T, uint32 numBlocks, uint32 blockSize>
inline void* PoolAllocatorExtern<T, numBlocks, blockSize>::reallocate (void* address, uint32 byteSize)
{
	return resizeBlock (reinterpret_cast<T*> (address), (byteSize + kBlockSizeBytes - 1) / kBlockSizeBytes);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename T, uint32 numBlocks, uint32 blockSize>
inline void PoolAllocatorExtern<T, numBlocks, blockSize>::deallocate (void* address)
{
	if(address != nullptr)
		freeBlock (reinterpret_cast<T*> (address));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename T, uint32 numBlocks, uint32 blockSize>
inline T* PoolAllocatorExtern<T, numBlocks, blockSize>::createBlock (uint32 count)
{
	T* address = nullptr;
	for(uint32 i = 0; i < numBlocks; ++i)
	{
		if(getAllocatedBlocks(i) == 0)
		{
			// this is the start of an unallocated block. Use this as the location of our new block.
			address = pool + i * blockSize;
			// check if the unallocated block is large enough...
			for(uint32 j = 0; j < count; ++j)
			{
				if(i + j >= numBlocks || getAllocatedBlocks(i + j) != 0)
				{
					//...it is not. Reset the address and continue after the end of this block
					if(i + j < numBlocks)
						i = i + j + getAllocatedBlocks(i + j) - 1;
					address = nullptr;
					break;
				}
			}
		}
		else
			// this is the start of an allocated block. Continue after the end this block.
			i += getAllocatedBlocks(i) - 1;
		if(address != nullptr)
		{
			setAllocatedBlocks (i, count);

			#if CORE_DEBUG_INTERNAL
			uint32 index = i + getAllocatedBlocks (i) - 1;
			if(index > maxUsedIndex)
				maxUsedIndex = index;
			#endif

			break;
		}
	}
	return address;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename T, uint32 numBlocks, uint32 blockSize>
inline T* PoolAllocatorExtern<T, numBlocks, blockSize>::resizeBlock (T* address, uint32 count)
{
	T* newaddress = address;

	if(address)
	{
		uint32 index = static_cast<uint32> (address - pool) / blockSize;
		if(index >= numBlocks)
			return nullptr;

		uint32 oldCount = getAllocatedBlocks (index);

		// check if the current block is followed by enough unallocated space to just resize the block
		for(uint32 i = 1; i < count; ++i)
		{
			if(index + i >= numBlocks || getAllocatedBlocks (index + i) != 0)
			{
				newaddress = nullptr;
				break;
			}
		}

		if(newaddress != nullptr)
		{
			// resize
			resetAllocatedBlocks (index);
			setAllocatedBlocks (index, count);
		}
		else
		{
			// otherwise allocate a new block and copy the existing data
			freeBlock (address);
			newaddress = createBlock (count);
			if(newaddress)
				::memmove (newaddress, address, kBlockSizeBytes * (count < oldCount ? count : oldCount));
		}
	}
	else
		newaddress = createBlock (count);

	return newaddress;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename T, uint32 numBlocks, uint32 blockSize>
inline void PoolAllocatorExtern<T, numBlocks, blockSize>::freeBlock (T* address)
{
	uint32 index = static_cast<uint32> (address - pool) / blockSize;
	if(index < numBlocks)
		resetAllocatedBlocks(index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename T, uint32 numBlocks, uint32 blockSize>
uint32 PoolAllocatorExtern<T, numBlocks, blockSize>::getAllocatedBlocks (uint32 index) const
{
	#if CORE_POOLALLOCATOR_USE_BITFIELD
	if((blockStart[index / 32] & (1 << (index % 32))) == 0)
		return 0;
	for(uint32 i = index; i < numBlocks; ++i)
		if(blockEnd[i / 32] & (1 << (i % 32)))
			return i - index + 1;
	return 0;
	#else
	return allocated[index];
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename T, uint32 numBlocks, uint32 blockSize>
void PoolAllocatorExtern<T, numBlocks, blockSize>::setAllocatedBlocks (uint32 index, uint32 count)
{
	#if CORE_POOLALLOCATOR_USE_BITFIELD
	blockStart[index / 32] |= (1 << (index % 32));
	blockEnd[(index + count - 1) / 32] |= (1 << ((index + count - 1) % 32));
	#else
	allocated[index] = count;
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename T, uint32 numBlocks, uint32 blockSize>
void PoolAllocatorExtern<T, numBlocks, blockSize>::resetAllocatedBlocks (uint32 index)
{
	#if CORE_POOLALLOCATOR_USE_BITFIELD
	blockStart[index / 32] &= ~(1 << (index % 32));
	for(uint32 i = index; i < numBlocks; ++i)
	{
		if(blockEnd[i / 32] & (1 << (i % 32)))
		{
			blockEnd[i / 32] &= ~(1 << (i % 32));
			break;
		}
	}
	#else
	allocated[index] = 0;
	#endif
}

} // namespace Core

#endif // _corepoolallocator_h


