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
// Filename    : ccl/base/memorypool.cpp
// Description : Memory Pool
//
//************************************************************************************************

#include "ccl/base/memorypool.h"

#include "ccl/public/base/primitives.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//************************************************************************************************
// AtomicPolicy
//************************************************************************************************

AtomicPolicy::Stack* AtomicPolicy::createStack ()
{
	return System::CreateAtomicStack ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AtomicPolicy::releaseStack (Stack* stack)
{
	stack->release ();
}

//************************************************************************************************
// MemoryPool
//************************************************************************************************

MemoryPool::Registrar& MemoryPool::getRegistrar ()
{
	ASSERT (System::IsInMainThread ())
	static Deleter<Registrar> registrar (nullptr);
	if(registrar._ptr == nullptr)
		registrar._ptr = NEW Registrar;
	return *registrar._ptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MemoryPool::dumpAll ()
{
	Debugger::println ("### Memory Pool Statistics ###");
	uint32 totalBytes = 0;
	ListForEach (getRegistrar (), MemoryPool*, pool)
		totalBytes += pool->getBytesAllocated ();
		pool->checkMemory ();
		pool->dump ();
	EndFor
	Debugger::printf ("Total %.2f MB allocated\n", (float)totalBytes / (1024.f * 1024.f));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MemoryPool::checkMemory () const
{
	if(check () == false)
	{
		CCL_DEBUGGER ("Corrupt memory block encountered!")
		return false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MemoryPool::dump () const
{
	Debugger::printf ("# %s : %d x %d = %.2f KBytes [%03.2f %%]\n",
					  name,
					  numBlocksAllocated,
					  blockSize,
					  (float)getBytesAllocated () / 1024.f,
					  getBlockUtilization () * 100.f);
}
