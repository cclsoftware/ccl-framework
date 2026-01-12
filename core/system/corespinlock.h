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
// Filename    : core/system/corespinlock.h
// Description : Spin Lock
//
//************************************************************************************************

#ifndef _corespinlock_h
#define _corespinlock_h

#include "core/system/coreatomic.h"
#include "core/system/corethread.h"

namespace Core {

//************************************************************************************************
// CoreSpinLock
//************************************************************************************************

namespace CoreSpinLock
{
	inline void wait ()
	{
		#if CORE_PLATFORM_WINDOWS
		#if defined (_M_ARM64)
		__yield ();
		#else
		_mm_pause ();
		#endif
		#elif __SSE2__
		__asm__ __volatile__ ("pause");
		#elif CORE_PLATFORM_ARM
		__asm__ __volatile__ ("yield");
		#else
		volatile int a = 0;
		for(int i = 0; i < 10; i++)
			a += 1;
		#endif
	}

	inline bool tryLock (int32 volatile& lock)
	{
		unsigned int maxSpinCount = 10;
		unsigned int testCycles = 100;

		// maxSpinCount is unsigned, we have to decrement before testing for zero,
		// otherwise we end at 0xffffffff which produces a wrong lock result!
		while(!AtomicTestAndSet (lock, 1, 0) && --maxSpinCount)
		{
			int spin = testCycles;
			while(spin-- && lock)
			{
				wait ();
			}
		}

		return maxSpinCount > 0;
	}

	/**
	* Lock the spinlock
	*
	* Exponential backoff is used here to improve the worst case
	* when multiple high priority threads are trying to acquire the same lock and the system is very busy:
	*
	* Try to acquire the lock.
	* If this fails, try to acquire the lock and wait in a loop of 10 cycles.
	* If this fails, try to acquire the lock and wait 10 times in a loop of 1000 cycles.
	* If this fails, yield.
	*/
	inline void lock (int32 volatile& lock)
	{
		unsigned int waitCycles = 10;
		unsigned int yieldCycles = 1000;

		if(AtomicTestAndSet (lock, 1, 0))
			return;

		int spin = waitCycles;
		while(spin--)
		{
			if(AtomicTestAndSet (lock, 1, 0))
				return;

			wait ();
		}

		while(true)
		{
			spin = yieldCycles;
			while(spin--)
			{
				if(AtomicTestAndSet (lock, 1, 0))
					return;

				wait ();
				wait ();
				wait ();
				wait ();
				wait ();
				wait ();
				wait ();
				wait ();
				wait ();
				wait ();
			}

			Threads::CurrentThread::yield ();
		}
	}

	inline void unlock (int32 volatile& lock)
	{
		MemoryFence ();
		AtomicSet (lock, 0);
	}
}

} // namespace Core

#endif // _corespinlock_h
