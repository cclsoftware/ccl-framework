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
// Filename    : core/platform/ctlrtos/corethread.ctlrtos.cpp
// Description : Crossworks Tasking Library Multithreading
//
//************************************************************************************************

#include "core/platform/ctlrtos/corethread.ctlrtos.h"

#define CTL_DEFAULT_STACK		0x40000  // default stack size in words (embarrassingly large)
#define CTL_DEFAULT_PRIORITY	50		 // default task priority

#define CTL_THREAD_STATE_INIT		0	// has not yet run
#define CTL_THREAD_STATE_RUNNING	1	// threadEntry() active
#define CTL_THREAD_STATE_FINISHED	2	// threadEntry() exited

using namespace Core;
using namespace Platform;
using namespace Threads;

//************************************************************************************************
// Thread Functions
//************************************************************************************************

static int ToNativePriority (ThreadPriority priority)
{
	// CTL priority is 0 (lowest) to 255 (highest)

	int ctlPriority = 50;
	switch(priority)
	{
	case kPriorityLow:
		ctlPriority = 40;
		break;
	case kPriorityBelowNormal:
		ctlPriority = 45;	
		break;
	case kPriorityNormal:
		ctlPriority = 50;	
		break;
	case kPriorityAboveNormal:
		ctlPriority = 55;	
		break;
	case kPriorityHigh:
		ctlPriority = 60;	
		break;
	case kPriorityTimeCritical:
		ctlPriority = 65;	
		break;

	// these should not be used by normal app tasks
	case kPriorityRealtime:
		ctlPriority = 80;	
		break;
	case kPriorityRealtimeMiddle:
		ctlPriority = 90;	
		break;
	case kPriorityRealtimeTop:
		ctlPriority = 100;	
		break;
	}
	return ctlPriority;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static ThreadPriority FromNativePriority (int ctlPrio)
{
	for(int corePrio = kPriorityRealtimeTop; corePrio >= kPriorityLow; corePrio--) 
		if(ToNativePriority (corePrio) <= ctlPrio)
			return corePrio;
	return kPriorityNormal;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ThreadID CurrentThread::getID ()
{
	return (ThreadID)(ctl_task_executing); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ThreadPriority CurrentThread::setPriority (ThreadPriority newPrio)
{
	if(ctl_task_executing == 0)  // should not happen
		return kPriorityNormal;
	if(ctl_task_executing->stack_pointer == 0) // should not happen
		return kPriorityNormal;
	
	ThreadPriority oldPrio = FromNativePriority (ctl_task_executing->priority);
	if(oldPrio != newPrio)
		ctl_task_set_priority (ctl_task_executing, ToNativePriority (newPrio));
	return oldPrio;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CurrentThread::sleep (uint32 milliseconds)
{
	// Minimum timer resolution is 10 milliseconds.  Finer resolution activities
	// should be moved to the kernel
	ctl_timeout_wait (ctl_get_current_time () + milliseconds);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CurrentThread::ussleep (uint32 microseconds)
{
	// Minimum timer resolution is 10 milliseconds.  Finer resolution activities
	// should be moved to the kernel
	ctl_timeout_wait (ctl_get_current_time () + microseconds / 1000);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CurrentThread::yield ()
{
	ctl_task_reschedule ();
}

//************************************************************************************************
// Thread local storage
//************************************************************************************************

TLSRef TLS::allocate ()
{
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void* TLS::getValue (TLSRef slot)
{
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TLS::setValue (TLSRef slot, void* value)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TLS::release (TLSRef slot)
{
	return false;
}

//************************************************************************************************
// CtlThread
//************************************************************************************************

static void ThreadEntry (void* param)
{
	CtlThread* thread = (CtlThread*)param;
	thread->setThreadState (CTL_THREAD_STATE_RUNNING);
	if(IThreadEntry* entry = thread->getThreadEntry ())
		entry->threadEntry ();
	thread->setThreadState (CTL_THREAD_STATE_FINISHED);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CtlThread::CtlThread ()
: stackSize	(CTL_DEFAULT_STACK),
  threadStack (NULL),
  ctlPriority (CTL_DEFAULT_PRIORITY),
  threadState (CTL_THREAD_STATE_INIT),
  priority (kPriorityNormal),
  name ("CtlThread")
{
	memset (&threadInfo, 0, sizeof(CTL_TASK_t));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CtlThread::~CtlThread ()
{
	terminate ();

	if(threadStack != NULL)
	{
		free (threadStack);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CtlThread::start (const ThreadInfo& info)
{
	entry = info.entry;
	name = info.name;
	
	// Create thread stack space
	if (threadStack == NULL)
		setStackSize (stackSize);

#if DEBUG
    memset(threadStack, 0xcd, stackSize);					  // known values
    threadStack[0] = threadStack[(stackSize/4) + 1] = 0xfacefeed;   // marker values
#endif
	
    ctl_task_run (&threadInfo, ctlPriority, ThreadEntry, this, name, stackSize / 4, 
                  threadStack + 1, 0);	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CtlThread::open (ThreadID id)
{
	//TODO Implement me
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ThreadID CtlThread::getID () const
{
	return reinterpret_cast<ThreadID> (&threadInfo);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 CtlThread::getUserModeTime () const
{
	//TODO Implement me
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CtlThread::setThreadState (int state)
{
	threadState = state;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CtlThread::terminate ()
{
	// is this accidental suicide?	
	assert (ctl_task_executing != &threadInfo);

	// Unschedule task if it still exists
	if(threadState == CTL_THREAD_STATE_RUNNING)
	{
		ctl_task_remove (&threadInfo);   // unschedule task
		threadState = CTL_THREAD_STATE_FINISHED;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CtlThread::join (uint32 milliseconds)
{
	// wait for task to finish executing
	CTL_TIME_t start = ctl_get_current_time ();
	while(threadState != CTL_THREAD_STATE_FINISHED)
	{
		ctl_timeout_wait (ctl_get_current_time () + 1); // coarse wait
		if(ctl_get_current_time () > (start + milliseconds))
		{	
			return false;
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CtlThread::setPriority (int _priority)
{
	priority = _priority;

	ctlPriority = ToNativePriority (priority);

	if(threadInfo.stack_pointer == 0)
	{
		// task is not initialized yet
		return;
	}
	ctl_task_set_priority (&threadInfo, ctlPriority);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CtlThread::getPriority () const
{
	return priority;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CtlThread::setCPUAffinity (int affinity)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CtlThread::getPlatformPriority () const
{
	return ctlPriority;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CtlThread::setStackSize (int size)
{
	// stack size may only be set prior to running task
    assert (threadStack == NULL);
    assert (size % 4 == 0);  // must be word multiple

	if(threadStack == NULL)  // stack not allocated
	{
		threadStack = (unsigned*)malloc (size + 8);  // ctl has word markers at either end
		if (threadStack == 0)
		{
			return false;
		}
	}

    stackSize = size;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IThreadEntry* CtlThread::getThreadEntry ()
{
	return entry;
}

//************************************************************************************************
// CtlLock
//************************************************************************************************

CtlLock::CtlLock ()
{
	ctl_mutex_init (&mutexId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CtlLock::~CtlLock ()
{
	// nothing to be done
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CtlLock::lock ()
{
	ctl_mutex_lock (&mutexId, CTL_TIMEOUT_NONE, 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CtlLock::tryLock ()
{
	int result = ctl_mutex_lock_nb (&mutexId);
	if(result == 0)
	{
		// locked by another task
		return false;
	}

	return true;

}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CtlLock::unlock ()
{
	ctl_mutex_unlock (&mutexId);
}
