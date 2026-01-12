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
// Filename    : core/public/coremempool.h
// Description : Memory Pool
//
//************************************************************************************************

#ifndef _coremempool_h
#define _coremempool_h

#include "core/public/coretypes.h"
#include "core/public/corelinkedlist.h"

#define CORE_MEMORYPOOL_DEADLAND_FILL DEBUG // Enable memory checking in debug builds
#if CTL_RTOS
#define CORE_MEMORYPOOL_LAZY_INITIALIZATION 1 // Lazy memory pool initialization
#endif

namespace Core {

//************************************************************************************************
// Memory Pool Macros
//************************************************************************************************

#define DEFINE_OBJECTPOOL(Class, Pool) \
DEFINE_OBJECTPOOL_SIZE (Class, Pool, 0)

#define DEFINE_OBJECTPOOL_SIZE(Class, Pool, count) \
namespace Core { template<> Pool Core::PooledObject<Class, Pool>::__pool (sizeof(Class), count, #Class); }

//************************************************************************************************
// MemoryPoolInitializer
/** Helper for lazy memory pool initialization. */
//************************************************************************************************

#if CORE_MEMORYPOOL_LAZY_INITIALIZATION
class MemoryPoolInitializer
{
	typedef void (*InitFunction) (void* instance, uint32 arg);

	struct Entry
	{
		void* pool;
		InitFunction init;
		uint32 arg;
		Entry (): pool (0), init (0), arg (0) {}
	};

	static const int kMaxLazyPoolCount = 16;
	Entry entries[kMaxLazyPoolCount];
	int entryCount;

public:
	MemoryPoolInitializer ()
	: entryCount (0) 
	{}

	static MemoryPoolInitializer& instance ()
	{
		static MemoryPoolInitializer theInstance;
		return theInstance;
	}

	void add (void* pool, InitFunction init, uint32 arg)
	{
		ASSERT (entryCount < kMaxLazyPoolCount)
		Entry& e = entries[entryCount++];
		e.pool = pool;
		e.init = init;
		e.arg = arg;
	}

	void initAll ()
	{
		for(int i = 0; i < entryCount; i++)
			(*entries[i].init) (entries[i].pool, entries[i].arg);
		entryCount = 0;
	}
};
#endif // CORE_MEMORYPOOL_LAZY_INITIALIZATION

//************************************************************************************************
// MemoryPool
/**	Memory pool template class, needs an outside atomic stack implementation.
	\ingroup core */
//************************************************************************************************

template <class AtomicPolicy>
class MemoryPool
{
public:
	MemoryPool (uint32 blockSize, uint32 count = 0, CStringPtr name = nullptr);
	~MemoryPool ();
	
	/** Get memory pool name. */
	CStringPtr getName () const;

	/** Allocate pool memory blocks. */
	bool allocate (uint32 count);
	
	/** Grow pool by given number of blocks. */
	bool grow (uint32 count);
	
	/** Free all memory blocks. */
	void deallocate ();
	
	/** Use a memory block. */
	void* newBlock ();
	
	/** Free a memory block. */
	void deleteBlock (void* ptr);
	
	/** Get pool block size. */
	uint32 getBlockSize () const;
	
	/** Get number of bytes allocated. */
	uint32 getBytesAllocated () const;
	
	/** Get relation between used/free memory blocks (0..1). */
	float getBlockUtilization () const;

	/** Check if memory has been overwritten. */
	bool check () const;
	
protected:
	typedef typename AtomicPolicy::Element Block;
	
	static const int kAlignment = 16;
	#if CORE_MEMORYPOOL_DEADLAND_FILL
	static const uint8 kDeadByte = 0xFF;
	static const int kBlockHeader = 'MEMB';	
	#endif

	static void construct (void* instance, uint32 count);

	typename AtomicPolicy::Stack* blockStack;
	uint32 blockSize;
	uint32 numBlocksAllocated;
	uint32 numBlocksUsed;
	CStringPtr name;
	
	struct Bucket
	{
		char* data;
		uint32 blockCount;

		Bucket (char* data = nullptr, uint32 blockCount = 0)
		: data (data),
		  blockCount (blockCount)
		{}
	};

	LinkedList<Bucket> allocatedData;

	uint32 getBlockOffset () const;
};

//************************************************************************************************
// PooledObject
/**	Template class for memory-pooled objects.
	\ingroup core */
//************************************************************************************************

template <class T, class Pool>
class PooledObject
{
public:
	static Pool& getPool ()
	{
		return __pool;
	}
	
	static T* pool_new ()
	{
		void* block = __pool.newBlock ();
		return block ? new (block) T : nullptr;		
	}
	
	template <class T2>
	static T* pool_new (const T2& a)
	{
		void* block = __pool.newBlock ();
		return block ? new (block) T (a) : nullptr;
	}

	template <class T2, class T3>
	static T* pool_new (const T2& a, const T3& b)
	{
		void* block = __pool.newBlock ();
		return block ? new (block) T (a, b) : nullptr;
	}
	
	void* operator new (size_t size)
	{
		ASSERT (size == sizeof(T))
		return __pool.newBlock ();
	}

	void operator delete (void* ptr)
	{
		__pool.deleteBlock (ptr);
	}

	void* operator new (size_t size, void* block) // placement variant 
	{
		return block;
	}

	void operator delete (void* ptr, void* block) // not called, but compiler complains
	{
		__pool.deleteBlock (block);
	}

private:
	static Pool __pool;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// MemoryPool implementation
//////////////////////////////////////////////////////////////////////////////////////////////////

template <class AtomicPolicy>
MemoryPool<AtomicPolicy>::MemoryPool (uint32 blockSize, uint32 count, CStringPtr name)
: blockStack (nullptr),
  blockSize (blockSize),
  numBlocksAllocated (0),
  numBlocksUsed (0),
  name (name)
{
	#if CORE_MEMORYPOOL_LAZY_INITIALIZATION
	MemoryPoolInitializer::instance ().add (this, construct, count);
	#else
	construct (this, count);
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class AtomicPolicy>
void MemoryPool<AtomicPolicy>::construct (void* instance, uint32 count)
{
	MemoryPool<AtomicPolicy>* This = reinterpret_cast<MemoryPool<AtomicPolicy>*> (instance);
	This->blockStack = AtomicPolicy::createStack ();
	ASSERT (This->blockStack != nullptr)
	if(count > 0)
		This->allocate (count);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class AtomicPolicy>
MemoryPool<AtomicPolicy>::~MemoryPool ()
{
	#if CORE_MEMORYPOOL_LAZY_INITIALIZATION
	if(blockStack == 0)
		return;
	#endif
	deallocate ();	
	AtomicPolicy::releaseStack (blockStack);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class AtomicPolicy>
CStringPtr MemoryPool<AtomicPolicy>::getName () const
{
	return name;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class AtomicPolicy>
bool MemoryPool<AtomicPolicy>::allocate (uint32 count)
{
	ASSERT (allocatedData.isEmpty ())
	if(!allocatedData.isEmpty ())
		return false;
			
	return grow (count);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class AtomicPolicy>
uint32 MemoryPool<AtomicPolicy>::getBlockOffset () const
{
	uint32 blockOffset = blockSize;
	#if CORE_MEMORYPOOL_DEADLAND_FILL
	blockOffset += kAlignment;
	#endif
	blockOffset = ((blockOffset + kAlignment - 1) / kAlignment) * kAlignment;
	return blockOffset;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class AtomicPolicy>
bool MemoryPool<AtomicPolicy>::grow (uint32 count)
{
	uint32 blockOffset = getBlockOffset ();		
	uint32 totalBytes = blockOffset * count;
	totalBytes += kAlignment;

	char* newData = NEW char[totalBytes];
	ASSERT (newData != nullptr)
	if(newData == nullptr)
		return false;

	#if CORE_MEMORYPOOL_DEADLAND_FILL
	::memset (newData, kDeadByte, totalBytes);
	#endif

	char* ptr = newData;
	for(uint32 i = 0; i < count; i++, ptr += blockOffset)
	{
		#if CORE_MEMORYPOOL_DEADLAND_FILL
		*((int*)ptr) = kBlockHeader;
		ptr += kAlignment;
		#endif
			
		Block* block = reinterpret_cast<Block*> (ptr);
		block->next = nullptr;
		blockStack->push (block);

		#if CORE_MEMORYPOOL_DEADLAND_FILL
		ptr -= kAlignment;
		#endif
	}
		
	allocatedData.append (Bucket (newData, count));
	numBlocksAllocated += count;
	return true;
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

template <class AtomicPolicy>
void MemoryPool<AtomicPolicy>::deallocate ()
{
	blockStack->flush ();
		
	while(!allocatedData.isEmpty ())
	{
		Bucket b = allocatedData.removeFirst ();
		ASSERT (b.data != nullptr)
		delete [] b.data;
	}

	numBlocksAllocated = 0;
	numBlocksUsed = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class AtomicPolicy>
void* MemoryPool<AtomicPolicy>::newBlock ()
{
	Block* block = blockStack->pop ();
	if(block)
		AtomicPolicy::add ((int32&)numBlocksUsed, 1);
	return block;
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

template <class AtomicPolicy>
void MemoryPool<AtomicPolicy>::deleteBlock (void* ptr)
{
	Block* block = reinterpret_cast<Block*> (ptr);
	if(block)
	{
		#if CORE_MEMORYPOOL_DEADLAND_FILL
		::memset (block, kDeadByte, blockSize);
		#endif
			
		block->next = nullptr;
		blockStack->push (block);
		AtomicPolicy::add ((int32&)numBlocksUsed, -1);
	}
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

template <class AtomicPolicy>
uint32 MemoryPool<AtomicPolicy>::getBlockSize () const
{
	return blockSize;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class AtomicPolicy>
uint32 MemoryPool<AtomicPolicy>::getBytesAllocated () const
{
	return numBlocksAllocated * getBlockOffset () + kAlignment;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class AtomicPolicy>
float MemoryPool<AtomicPolicy>::getBlockUtilization () const
{
	if(numBlocksAllocated == 0)
		return 0.f;
	return (float)numBlocksUsed / (float)numBlocksAllocated; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class AtomicPolicy>
bool MemoryPool<AtomicPolicy>::check () const
{
	#if CORE_MEMORYPOOL_DEADLAND_FILL
	uint32 blockOffset = getBlockOffset ();			
	ListForEach (allocatedData, Bucket, bucket)
		char* ptr = bucket.data;
		for(uint32 i = 0; i < bucket.blockCount; i++, ptr += blockOffset)
			if(*((int*)ptr) != kBlockHeader)
				return false;
	EndFor
	#endif
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace Core

#endif // _coremempool_h

