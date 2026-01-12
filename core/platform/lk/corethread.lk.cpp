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
// Filename    : core/platform/lk/corethread.lk.cpp
// Description : Little Kernel Thread Primitives
//
//************************************************************************************************

#include "corethread.lk.h"

// Little Kernel
#include "err.h"
#include "kernel/semaphore.h"

using namespace Core;
using namespace Platform;

//************************************************************************************************
// CurrentThread
//************************************************************************************************

Threads::ThreadID CurrentThread::getID ()
{
	return reinterpret_cast<Threads::ThreadID> (get_current_thread ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Threads::ThreadPriority CurrentThread::setPriority (Threads::ThreadPriority newPrio)
{
	thread_set_priority (newPrio + DEFAULT_PRIORITY - Threads::ThreadPriorityEnum::kPriorityNormal);
	return get_current_thread ()->priority + Threads::ThreadPriorityEnum::kPriorityNormal - DEFAULT_PRIORITY;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CurrentThread::sleep (uint32 milliseconds)
{
	thread_sleep (milliseconds);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CurrentThread::ussleep (uint32 microseconds)
{
	thread_sleep (microseconds/1000);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CurrentThread::yield ()
{
	thread_yield ();
}

//************************************************************************************************
// LKThread
//************************************************************************************************

LKThread::LKThread ()
: lkThread (0),
  priority (DEFAULT_PRIORITY),
  entry (0),
  cpu (-1)
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LKThread::~LKThread ()
{
	// next user accessible best thing to abort and delete
	if(lkThread != 0 && lkThread->state != THREAD_DEATH)
		thread_detach_and_resume (lkThread);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LKThread::open (Threads::ThreadID _id)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LKThread::start (const ThreadInfo& info)
{
	entry = info.entry;
	if(entry != 0)
	{
		lkThread = thread_create (info.name, entryWrapper, entry, getPlatformPriority (), LK_THREAD_STACKSIZE);
		thread_set_curr_cpu (lkThread, cpu);
		thread_set_pinned_cpu (lkThread, cpu);
		thread_resume (lkThread);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LKThread::join (uint32 milliseconds)
{
	if(lkThread == 0)
		return true;

	int retcode = 0;
	return thread_join (lkThread, &retcode, milliseconds) == NO_ERROR;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LKThread::terminate ()
{
	/*not supported without using LittleKernel internal only intended functions*/
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int LKThread::getPriority () const
{
	return priority;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LKThread::setPriority (int _priority)
{
	priority = _priority;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LKThread::setCPUAffinity (int affinity)
{
	cpu = affinity;
} 

//////////////////////////////////////////////////////////////////////////////////////////////////

int LKThread::getPlatformPriority () const
{
	return priority + DEFAULT_PRIORITY - Threads::ThreadPriorityEnum::kPriorityNormal;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 LKThread::getUserModeTime () const
{
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Threads::ThreadID LKThread::getID () const
{
	return reinterpret_cast<Threads::ThreadID> (lkThread);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int LKThread::entryWrapper (void *entryPtr)
{
	reinterpret_cast<IThreadEntry*>(entryPtr)->threadEntry ();
	return 0;
}

//************************************************************************************************
// LKLock
//************************************************************************************************

LKLock::LKLock ()
: count (0)
{
	mutex_init (&lkMutex);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LKLock::~LKLock ()
{
	unlock ();
	mutex_destroy (&lkMutex);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LKLock::lock ()
{
	if(!is_mutex_held (&lkMutex))
		mutex_acquire (&lkMutex);
	atomic_add (&count, 1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LKLock::tryLock ()
{
	if(is_mutex_held (&lkMutex))
	{
		atomic_add (&count, 1);
		return true;
	}
	else if(mutex_acquire_timeout (&lkMutex, 0) == NO_ERROR)
	{
		atomic_add (&count, 1);
		return true;
	}
	else
		return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LKLock::unlock ()
{
	if(is_mutex_held (&lkMutex))
	{
		if(atomic_add (&count, -1) == 1)
			mutex_release (&lkMutex);
	}
}

//************************************************************************************************
// LKSignal
//************************************************************************************************

LKSignal::LKSignal (bool manualReset)
{
	uint flags = manualReset ? EVENT_FLAG_AUTOUNSIGNAL : 0;
	event_init (&lkEvent, false, flags);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LKSignal::~LKSignal ()
{
	event_destroy (&lkEvent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LKSignal::signal ()
{
	/* not sure if we want to reschedule or not*/
	event_signal (&lkEvent, false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LKSignal::reset ()
{
	event_unsignal (&lkEvent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LKSignal::wait (uint32 milliseconds)
{
	return event_wait_timeout (&lkEvent, milliseconds) == NO_ERROR;
}

//************************************************************************************************
// LKReadWriteLock
//************************************************************************************************

LKReadWriteLock::LKReadWriteLock ()
: owner (0)
{
	sem_init (&activeReaderSemaphore, 1);
	sem_init (&writeSemaphore, 1);
	sem_init (&readSemaphore, CONFIG_FWAPP_MAX_THREADS + 1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LKReadWriteLock::~LKReadWriteLock ()
{
	sem_destroy (&activeReaderSemaphore);
	sem_destroy (&writeSemaphore);
	sem_destroy (&readSemaphore);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LKReadWriteLock::lockWrite ()
{
	if(sem_wait (&writeSemaphore) == NO_ERROR)
	{
		if(sem_wait (&activeReaderSemaphore) == NO_ERROR)
			owner = get_current_thread ();
		else
			sem_post (&writeSemaphore, false);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LKReadWriteLock::unlockWrite ()
{
	if(get_current_thread () == owner)
	{
		owner = 0;
		sem_post (&activeReaderSemaphore, false);
		sem_post (&writeSemaphore, false);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LKReadWriteLock::lockRead ()
{
	if(sem_wait (&writeSemaphore) == 0)
	{
		sem_trywait (&activeReaderSemaphore);
		sem_trywait (&readSemaphore);
		sem_post (&writeSemaphore, false);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LKReadWriteLock::unlockRead ()
{
	if(readSemaphore.count == CONFIG_FWAPP_MAX_THREADS)
		sem_post (&activeReaderSemaphore, false);
	sem_post (&readSemaphore, false);
}
