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
// Filename    : ccl/base/memorypool.h
// Description : Memory Pool
//
//************************************************************************************************

#ifndef _ccl_memorypool_h
#define _ccl_memorypool_h

#include "ccl/public/base/debug.h"
#include "ccl/public/collections/linkedlist.h"

#include "ccl/public/system/ithreading.h"
#include "ccl/public/system/atomic.h"

#include "core/public/coremempool.h"

namespace CCL {

using Core::PooledObject;

//************************************************************************************************
// AtomicPolicy
//************************************************************************************************

class AtomicPolicy
{
public:
	typedef Threading::IAtomicStack Stack;
	typedef Threading::IAtomicStack::Element Element;

	static Stack* createStack ();
	static void releaseStack (Stack* stack);
	static INLINE int32 add (int32 volatile& variable, int32 value) { return AtomicAddInline (variable, value); }
};

//************************************************************************************************
// MemoryPool
/**	Thread-safe memory pool. The pool uses a lock-free stack.  
	\ingroup ccl_base */
//************************************************************************************************

class MemoryPool: public Core::MemoryPool<AtomicPolicy>
{
public:
	MemoryPool (uint32 blockSize, uint32 count = 0, CStringPtr name = nullptr);
	~MemoryPool ();

	void dump () const;
	bool checkMemory () const;

	static void dumpAll ();

protected:
	typedef LinkedList<MemoryPool*> Registrar;

	static Registrar& getRegistrar ();
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// MemoryPool inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline MemoryPool::MemoryPool (uint32 blockSize, uint32 count, CStringPtr name)
: Core::MemoryPool<AtomicPolicy> (blockSize, count, name)
{
	#if DEBUG
	getRegistrar ().append (this);
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline MemoryPool::~MemoryPool ()
{
	SOFT_ASSERT (numBlocksUsed == 0, "Memory blocks still in use!")

	#if DEBUG
	getRegistrar ().remove (this);
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_memorypool_h
