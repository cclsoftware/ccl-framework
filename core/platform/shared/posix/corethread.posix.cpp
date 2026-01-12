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
// Filename    : core/platform/shared/posix/corethread.posix.cpp
// Description : POSIX Multithreading
//
//************************************************************************************************

#include "core/platform/corefeatures.h"

#if CORE_THREAD_IMPLEMENTATION == CORE_PLATFORM_IMPLEMENTATION
#include CORE_PLATFORM_IMPLEMENTATION_HEADER (corethread)
#else
#include "core/platform/shared/posix/corethread.posix.h"
#endif

#include "core/system/coredebug.h"

#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <sys/resource.h>

using namespace Core;
using namespace Platform;
using namespace Threads;

//************************************************************************************************
// Thread Functions
//************************************************************************************************

static void ToNativeThreadPriority (int& policy, sched_param& param, ThreadPriority priority)
{
	const int realTimePolicy = SCHED_RR;
	int minRealTimePriority = sched_get_priority_min (realTimePolicy);
	int maxRealTimePriority = sched_get_priority_max (realTimePolicy);
	
	switch(priority)
	{
	case kPriorityLow:
		param.sched_priority = 0; // SCHED_OTHER supports only priority 0
		policy = SCHED_OTHER;
		break;

	case kPriorityBelowNormal:
		param.sched_priority = 0;
		policy = SCHED_OTHER;
		break;

	case kPriorityNormal:
		param.sched_priority = 0;
		policy = SCHED_OTHER;
		break;

	case kPriorityAboveNormal:
		param.sched_priority = minRealTimePriority;
		policy = realTimePolicy;
		break;

	case kPriorityHigh:
		param.sched_priority = minRealTimePriority + 1;
		policy = realTimePolicy;
		break;

	case kPriorityTimeCritical:
		param.sched_priority = minRealTimePriority + 2;
		policy = realTimePolicy;
		break;

	case kPriorityRealtimeBase:
		param.sched_priority = minRealTimePriority + (maxRealTimePriority - minRealTimePriority) / 3;
		policy = realTimePolicy;
		break;

	case kPriorityRealtimeMiddle:
		param.sched_priority = minRealTimePriority + 2 * (maxRealTimePriority - minRealTimePriority) / 3;
		policy = realTimePolicy;
		break;

	case kPriorityRealtimeTop:
		param.sched_priority = maxRealTimePriority;
		policy = realTimePolicy;
		break;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ThreadID CurrentThread::getID ()
{
	return (ThreadID)pthread_self ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ThreadPriority CurrentThread::setPriority (ThreadPriority newPrio)
{
	if(newPrio >= kPriorityRealtimeBase)
		if(ThreadPriorityHandler* handler = ThreadPriorityHandler::customHandler)
			if(handler->setSelfToRealtimePriority (newPrio))
				return newPrio;

	sched_param param = {0};
	int policy = 0;
	pthread_getschedparam (pthread_self (), &policy, &param);
	ToNativeThreadPriority (policy, param, newPrio);
	int result = pthread_setschedparam (pthread_self (), policy, &param);
	if(result == 0)
		return newPrio;

	DebugPrintf ("Warning: pthread_setschedparam failed with error %d\n", result);
	return kPriorityNormal;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CurrentThread::sleep (uint32 milliseconds)
{
	usleep (milliseconds * 1000);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CurrentThread::ussleep (uint32 microseconds)
{
	usleep (microseconds);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CurrentThread::yield ()
{
	sched_yield ();
}

//************************************************************************************************
// Thread local storage
//************************************************************************************************

static void TLSDestructor (void* p)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

TLSRef TLS::allocate ()
{
	pthread_key_t key = 0;
	if(pthread_key_create (&key, TLSDestructor) != 0)
		return 0;
	return (TLSRef)key + 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void* TLS::getValue (TLSRef slot)
{
	return pthread_getspecific ((pthread_key_t)slot-1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TLS::setValue (TLSRef slot, void* value)
{
	return pthread_setspecific ((pthread_key_t)slot-1, value) == 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TLS::release (TLSRef slot)
{
	return pthread_key_delete ((pthread_key_t)slot-1) == 0;
}

//************************************************************************************************
// PosixThread
//************************************************************************************************

ThreadPriorityHandler* ThreadPriorityHandler::customHandler = nullptr;

//////////////////////////////////////////////////////////////////////////////////////////////////

static void* ThreadEntry (void* param)
{
	PosixThread* thread = static_cast<PosixThread*> (param);
	
	switch(thread->getPriority ())
	{
	case kPriorityLow :
		::nice (10);
		break;

	case kPriorityBelowNormal :
		::nice (5);
		break;
		
	case kPriorityRealtimeBase :
	case kPriorityRealtimeMiddle :
	case kPriorityRealtimeTop :
		if(ThreadPriorityHandler* handler = ThreadPriorityHandler::customHandler)
			handler->setSelfToRealtimePriority (thread->getPriority ());
		break;
	}
	
	if(IThreadEntry* entry = thread->getThreadEntry ())
		return reinterpret_cast<void*> (entry->threadEntry ());
	
	return reinterpret_cast<void*> (1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PosixThread::PosixThread ()
: priority (kPriorityNormal),
  name ("Thread"),
  threadId (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixThread::open (ThreadID _threadId)
{
	threadId = (pthread_t)_threadId;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PosixThread::~PosixThread ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PosixThread::start (const ThreadInfo& info)
{
	entry = info.entry;
	name = info.name;

	pthread_attr_t threadAttributes;
	pthread_attr_init (&threadAttributes);
	
	pthread_attr_setdetachstate (&threadAttributes, PTHREAD_CREATE_JOINABLE);
	
	sched_param param = {0};
	int policy = 0;
	pthread_attr_getschedparam (&threadAttributes, &param);
	ToNativeThreadPriority (policy, param, priority);
	pthread_attr_setschedpolicy (&threadAttributes, policy);
	pthread_attr_setschedparam (&threadAttributes, &param);
	pthread_attr_setinheritsched (&threadAttributes, PTHREAD_EXPLICIT_SCHED);
	
	int result = pthread_create (&threadId, &threadAttributes, ThreadEntry, this);

	if(result == EPERM)
	{
		DebugPrintf ("Warning: pthread_create failed with error %d. Trying again with PTHREAD_INHERIT_SCHED.\n", result);

		ToNativeThreadPriority (policy, param, kPriorityNormal);
		pthread_attr_setschedpolicy (&threadAttributes, policy);
		pthread_attr_setschedparam (&threadAttributes, &param);
		pthread_attr_setinheritsched (&threadAttributes, PTHREAD_INHERIT_SCHED);

		result = pthread_create (&threadId, &threadAttributes, ThreadEntry, this);
	}

	if(result != 0)
	{
		DebugPrintf ("Warning: pthread_create failed with error %d.\n", result);
		threadId = 0;
	}
	
	pthread_attr_destroy (&threadAttributes);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PosixThread::terminate ()
{
	pthread_cancel (threadId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixThread::join (uint32 milliseconds)
{
	void* value_ptr = nullptr;
	pthread_join (threadId, &value_ptr);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PosixThread::setPriority (int _priority)
{
	priority = _priority;
	if(threadId == 0)
		return;

	sched_param param = {0};
	int policy = 0;
	pthread_getschedparam (threadId, &policy, &param);
	ToNativeThreadPriority (policy, param, priority);

	// Changing the thread priority on Linux only works in certain scenarios.
	// See ticket CCL-400 for more information.

	int result = pthread_setschedparam (threadId, policy, &param);
	if(result != 0)
		DebugPrintf ("Warning: pthread_setschedparam failed with error %d\n", result);
	#if DEBUG
	else
		DebugPrintf ("\nScheduled thread: %s, Priority %i\n", name, priority);
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int PosixThread::getPriority () const
{
	return priority;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PosixThread::setCPUAffinity (int affinity)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

int PosixThread::getPlatformPriority () const
{
	sched_param param = {0};
	int policy = 0;
	
	int result = pthread_getschedparam (threadId, &policy, &param);
	ASSERT (result == 0)
	return param.sched_priority;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 PosixThread::getUserModeTime () const
{
	/*
	 See http://www.gnu.org/software/hurd/gnumach-doc/Thread-Information.html	 
	 struct thread_basic_info;
	 kern_return_t thread_info (thread_t target_thread, int flavor, thread_info_t thread_info, mach_msg_type_number_t *thread_infoCnt);
	*/
	
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ThreadID PosixThread::getID () const
{
	return (ThreadID)threadId;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IThreadEntry* PosixThread::getThreadEntry ()
{
	return entry;
}

//************************************************************************************************
// PosixLock
//************************************************************************************************

PosixLock::PosixLock ()
{
	pthread_mutexattr_t attr;
	pthread_mutexattr_init (&attr);
	pthread_mutexattr_settype (&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init (&mutexId, &attr);
	pthread_mutexattr_destroy (&attr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PosixLock::~PosixLock ()
{
	pthread_mutex_destroy (&mutexId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PosixLock::lock ()
{
	pthread_mutex_lock (&mutexId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixLock::tryLock ()
{
	int result = pthread_mutex_trylock (&mutexId);
	ASSERT (result == 0 || result == EBUSY)
	if(result != EBUSY)
		return true;
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PosixLock::unlock ()
{
	pthread_mutex_unlock (&mutexId);
}

//************************************************************************************************
// PosixSignal
//************************************************************************************************

PosixSignal::PosixSignal (bool manualReset)
: manualReset (manualReset),
  signaled (false)  
{
	pthread_mutex_init (&mutexId, nullptr);
	pthread_cond_init (&conditionId, nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
		
PosixSignal::~PosixSignal ()
{
	pthread_mutex_destroy (&mutexId); 
	pthread_cond_destroy (&conditionId); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PosixSignal::signal ()
{
	pthread_mutex_lock (&mutexId); 
	if(signaled == false)
	{
		signaled = true;		
		if(manualReset)
			pthread_cond_broadcast (&conditionId);
		else
			pthread_cond_signal (&conditionId);
	}
	pthread_mutex_unlock (&mutexId); 
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

void PosixSignal::reset ()
{
	signaled = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixSignal::wait (uint32 milliseconds)
{
	bool timedOut = false;
	
	pthread_mutex_lock (&mutexId); 

	int osResult = 0;
	while(signaled == false && osResult == 0)
	{
		if(milliseconds != kWaitForever)
		{
			long seconds = milliseconds / 1000;
			long nanoseconds = (milliseconds % 1000) * 1000000;
			struct timespec timeOutPeriod = {0, 0};
			clock_gettime (CLOCK_REALTIME, &timeOutPeriod);
			timeOutPeriod.tv_sec += seconds;
			timeOutPeriod.tv_nsec += nanoseconds;
			osResult = pthread_cond_timedwait (&conditionId, &mutexId, &timeOutPeriod);
			if(osResult == ETIMEDOUT)
				timedOut = true;
		} 
		else 
		{ 
			osResult = pthread_cond_wait (&conditionId, &mutexId);
		}
	}
	
	if(manualReset == false)
		signaled = false;
	
	pthread_mutex_unlock (&mutexId); 
	
	if(osResult != 0 && osResult != ETIMEDOUT)
		DebugPrintf ("Warning: pthread_cond_wait failed with error %d.\n", osResult);

	return !timedOut;
}

//************************************************************************************************
// PosixReadWriteLock
//************************************************************************************************

PosixReadWriteLock::PosixReadWriteLock ()
{
	pthread_rwlockattr_t attr;
	pthread_rwlockattr_init (&attr);
	pthread_rwlockattr_setpshared (&attr, PTHREAD_PROCESS_PRIVATE);
	pthread_rwlock_init (&rwlockId, &attr);
	pthread_rwlockattr_destroy (&attr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PosixReadWriteLock::~PosixReadWriteLock ()
{
	pthread_rwlock_destroy (&rwlockId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PosixReadWriteLock::lockWrite ()
{
	pthread_rwlock_wrlock (&rwlockId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PosixReadWriteLock::unlockWrite ()
{
	pthread_rwlock_unlock (&rwlockId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PosixReadWriteLock::lockRead ()
{
	pthread_rwlock_rdlock (&rwlockId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PosixReadWriteLock::unlockRead ()
{
	pthread_rwlock_unlock (&rwlockId);
}
