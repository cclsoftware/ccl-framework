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
// Filename    : core/platform/rtos/corethread.zephyr.cpp
// Description : Zephyr Multithreading
//
//************************************************************************************************

#include "core/public/corebasicmacros.h"

#include "core/platform/zephyr/corethread.zephyr.h"
#include "core/platform/shared/coreplatformsupervisor.h"

LOG_MODULE_REGISTER (CoreThread, LOG_LEVEL_DBG);

using namespace Core;
using namespace Platform;
using namespace Threads;

//************************************************************************************************
// Utility Thread Functions
//************************************************************************************************

/*
 * Zephyr priority is inverted:
 * - highest priority is numerically smaller
 * - lowest priority is numerically larger
 * - priority < 0: cooperative priorities (not used for app)
 * - priority >= 0: preemptive priorities
 */
static int ToNativePriority (ThreadPriority priority)
{
	#ifndef CONFIG_TOP_PRIORITY
	#define CONFIG_TOP_PRIORITY 0
	#endif

	int zephyrPriority = CONFIG_TOP_PRIORITY + 6;
	switch(priority)
	{
	case kPriorityLow:
		zephyrPriority = CONFIG_TOP_PRIORITY + 8;
		break;
	case kPriorityBelowNormal:
		zephyrPriority = CONFIG_TOP_PRIORITY + 7;	
		break;
	case kPriorityNormal:
		zephyrPriority = CONFIG_TOP_PRIORITY + 6;	
		break;
	case kPriorityAboveNormal:
		zephyrPriority = CONFIG_TOP_PRIORITY + 5;	
		break;
	case kPriorityHigh:
		zephyrPriority = CONFIG_TOP_PRIORITY + 4;
		break;
	case kPriorityTimeCritical: // TODO: verify
		zephyrPriority = CONFIG_TOP_PRIORITY + 3;
		break;

	// these should not be used by normal app tasks
	case kPriorityRealtime:
		zephyrPriority = CONFIG_TOP_PRIORITY + 2;
		break;
	case kPriorityRealtimeMiddle:
		zephyrPriority = CONFIG_TOP_PRIORITY + 1;
		break;
	case kPriorityRealtimeTop:
		zephyrPriority = CONFIG_TOP_PRIORITY;
		break;
	}
	return zephyrPriority;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static ThreadPriority FromNativePriority (int zephyrPrio)
{
	for(int corePrio = kPriorityRealtimeTop; corePrio >= kPriorityLow; corePrio--) 
		if(ToNativePriority (corePrio) >= zephyrPrio)
			return corePrio;
	return kPriorityNormal;
}

//************************************************************************************************
// Platform::CurrentThread functions
//************************************************************************************************

ThreadID Platform::CurrentThread::getID()
{
	return reinterpret_cast<ThreadID> (k_current_get ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ThreadPriority Platform::CurrentThread::setPriority (Threads::ThreadPriority newPrio)
{
	k_thread_priority_set (k_current_get (), ToNativePriority (newPrio));
	return newPrio;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Platform::CurrentThread::sleep (uint32 milliseconds)
{
	k_sleep (K_MSEC(milliseconds));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Platform::CurrentThread::ussleep (uint32 microseconds)
{
	/* This is not going to work with any sane scheduler tick */
	k_usleep (microseconds);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Platform::CurrentThread::yield ()
{
	k_yield ();
}

//************************************************************************************************
// ZephyrThread
//************************************************************************************************

ZephyrThread::ZephyrThread ()
: stack (0),
  entry (0),
  priority (CONFIG_NUM_PREEMPT_PRIORITIES-2),  // can't match idle prio
  running (false)
{
	/*
	 * Avoid any system activities here as we have likely reached this location
	 * at an early boot stage using static initializers.  
	 */
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ZephyrThread::~ZephyrThread ()
{
	k_thread_abort (&nativeThread);
	GetSystemSupervisor ().freeThreadStack (k_thread_name_get (&nativeThread));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ZephyrThread::entryWrapper (void* corePlatformThread, void* p2, void* p3)
{
	ARG_UNUSED (p2);
	ARG_UNUSED (p3);
	ZephyrThread* thread = static_cast<ZephyrThread*> (corePlatformThread);
	__ASSERT (thread, "ThreadEntry called without a ZephyrThread instance.");
	if(thread)
	{
		IThreadEntry* entry = thread->getEntry ();
		if(entry != 0)
			entry->threadEntry ();
		
		LOG_DBG ("Thread terminated");
		thread->stopped ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IThreadEntry* ZephyrThread::getEntry () const
{
	return entry;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ZephyrThread::stopped ()
{
	GetSystemSupervisor ().freeThreadStack (k_thread_name_get (&nativeThread));
	running = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ZephyrThread::open (Threads::ThreadID id)
{
	/* Not applicable */
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ZephyrThread::start (const ThreadInfo& info)
{
	LOG_DBG ("Starting thread \"%s\"", info.name);

	entry = info.entry;  // thread entry function

	void* threadStack = 0;
	if(GetSystemSupervisor ().getThreadStack (threadStack, stackSize, info.name))
	{
		stack = reinterpret_cast<k_thread_stack_t*> (threadStack);
	}
	else
	{
		LOG_DBG ("No thread stack available");
		return;
	}
	
	if(k_thread_create (&nativeThread, stack, stackSize, &entryWrapper, this,
					 0, 0, ToNativePriority (kPriorityLow), K_INHERIT_PERMS, K_FOREVER) == nullptr)
	{
		LOG_DBG ("thread initialization failed");
	}

	running = true;

	k_thread_name_set (&nativeThread, info.name);
	k_thread_start (&nativeThread);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ZephyrThread::terminate ()
{
	__ASSERT (false, "Deprecated method Thread::terminate called.");
	/* 
	 * It is possible to abort a thread, but there is no use case for this.
	 * Thread termination should be executed by returning from the entry
	 * function in an organized manner.  
	 */
	k_thread_abort (&nativeThread);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ZephyrThread::join (uint32 milliseconds)
{
	int64 time_stamp = k_uptime_get ();
	while(running)
	{
		k_sleep (K_MSEC(1));
		if(k_uptime_delta (&time_stamp) > milliseconds)
			return false;
	}

	GetSystemSupervisor ().freeThreadStack (k_thread_name_get (&nativeThread));

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ZephyrThread::getPriority () const
{
	return priority;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ZephyrThread::setPriority (int _priority)
{
	if(running)
	{
		LOG_DBG ("Changing native thread priority of %s from %d to %d", 
				k_thread_name_get (&nativeThread), priority, ToNativePriority (_priority));
		k_thread_priority_set (&nativeThread, ToNativePriority (_priority));
	}
	priority = _priority;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ZephyrThread::setCPUAffinity (int cpu)
{
    // Not implemented
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ZephyrThread::getPlatformPriority () const
{
	return FromNativePriority (priority);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 ZephyrThread::getUserModeTime () const
{
    // Not implemented
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ThreadID ZephyrThread::getID () const
{
	return reinterpret_cast<ThreadID> (&nativeThread);  // return unique thread address
}

//************************************************************************************************
// ZephyrLock
//************************************************************************************************

ZephyrLock::ZephyrLock ()
{
	k_mutex_init (&mutex);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ZephyrLock::~ZephyrLock ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ZephyrLock::lock ()
{
	k_mutex_lock (&mutex, K_FOREVER);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ZephyrLock::tryLock ()
{
	return k_mutex_lock (&mutex, K_NO_WAIT) == 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ZephyrLock::unlock ()
{
	k_mutex_unlock (&mutex);
}

//************************************************************************************************
// ZephyrSignal
//************************************************************************************************

ZephyrSignal::ZephyrSignal (bool manualReset)
: manualReset (manualReset)
{
	k_event_init (&event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
		
ZephyrSignal::~ZephyrSignal ()
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ZephyrSignal::signal ()
{
	k_event_set (&event, kTrackedEvent);
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

void ZephyrSignal::reset ()
{
	k_event_set (&event, 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ZephyrSignal::wait (uint32 milliseconds)
{
	k_timeout_t timeout = K_NO_WAIT;
	switch(milliseconds)
	{
	case kWaitForever :
	 	timeout = K_FOREVER;
		break;
	case 0 :
		break;
	default :
		timeout = K_MSEC (milliseconds);
		break;
	}
	return k_event_wait (&event, kTrackedEvent, !manualReset, timeout) != 0;
}

//************************************************************************************************
// ZephyrReadWriteLock
// Implementation based on pthread_rwlock.c
//************************************************************************************************

ZephyrReadWriteLock::ZephyrReadWriteLock ()
: owner (0)
{
	k_sem_init (&activeReaderSemaphore, 1, 1);
	k_sem_init (&writeSemaphore, 1, 1);
	k_sem_init (&readSemaphore,  + 1, GetSystemSupervisor ().getMaxThreads () + 1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ZephyrReadWriteLock::~ZephyrReadWriteLock ()
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ZephyrReadWriteLock::lockWrite ()
{
	if(k_sem_take (&writeSemaphore, K_FOREVER) == 0)
	{
		if(k_sem_take (&activeReaderSemaphore, K_FOREVER) == 0)
			owner = k_current_get ();
		else
			k_sem_give (&writeSemaphore);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ZephyrReadWriteLock::unlockWrite ()
{
	if(k_current_get () == owner)
	{
		owner = 0;
		k_sem_give (&activeReaderSemaphore);
		k_sem_give (&writeSemaphore);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ZephyrReadWriteLock::lockRead ()
{
	if(k_sem_take (&writeSemaphore, K_FOREVER) == 0)
	{
		k_sem_take (&activeReaderSemaphore, K_NO_WAIT);
		k_sem_take (&readSemaphore, K_NO_WAIT);
		k_sem_give (&writeSemaphore);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ZephyrReadWriteLock::unlockRead ()
{
	if(k_sem_count_get (&readSemaphore) == GetSystemSupervisor ().getMaxThreads ())
		k_sem_give (&activeReaderSemaphore);
	k_sem_give (&readSemaphore);
}
