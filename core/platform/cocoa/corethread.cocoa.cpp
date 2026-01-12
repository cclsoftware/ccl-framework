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
// Filename    : core/platform/cocoa/corethread.cocoa.cpp
// Description : Cocoa Multithreading
//
//************************************************************************************************

#include "corethread.cocoa.h"

#include <mach/mach_init.h>
#include <mach/mach_time.h>
#include <mach/thread_policy.h>
#include <mach/thread_act.h>
#include <libkern/OSAtomic.h>

namespace Core {
namespace Platform {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Missing POSIX functions
//////////////////////////////////////////////////////////////////////////////////////////////////

int pthread_yield (void)
{
	// Note: The pthread_yield subroutine forces the calling thread to relinquish use of its processor, 
	//		 and to wait in the run queue before it is scheduled again. 
	//
	// 		 pthread_yield_np notifies the scheduler that the current thread is willing to release 
	//		 its processor to other threads of the same or higher priority. 
	pthread_yield_np ();
	return 0;
}

} // namespace Platform
} // namespace Core

using namespace Core;
using namespace Platform;
using namespace Threads;

//////////////////////////////////////////////////////////////////////////////////////////////////

static void* CocoaThreadEntry (void* param)
{
	PosixThread* thread = static_cast<PosixThread*> (param);
	pthread_setname_np (thread->getName ());
	
	if(IThreadEntry* entry = thread->getThreadEntry ())
		return reinterpret_cast<void*> (entry->threadEntry ());
	
	return reinterpret_cast<void*> (1);
}

//************************************************************************************************
// CocoaThread
//************************************************************************************************

void CocoaThread::start (const ThreadInfo& info)
{
	entry = info.entry;
	name = info.name;
	
	pthread_attr_t threadAttributes;
	pthread_attr_init (&threadAttributes);
	pthread_attr_setdetachstate (&threadAttributes, PTHREAD_CREATE_JOINABLE);
	pthread_create (&threadId, &threadAttributes, CocoaThreadEntry, this);
	pthread_attr_destroy (&threadAttributes);
	
	if(priority != kPriorityNormal)
		setPriority (priority);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaThread::setPriority (int _priority)
{
	priority = _priority;
	if(threadId == 0)
		return;

	if(priority <= kPriorityTimeCritical)
	{
		sched_param param;
		memset (&param, 0, sizeof(sched_param));
		switch(priority)
		{
		case kPriorityLow:
			param.sched_priority = 15;
			break;
		case kPriorityBelowNormal:
			param.sched_priority = 27;
			break;
		case kPriorityNormal:
			param.sched_priority = 31;
			break;
		case kPriorityAboveNormal:
			param.sched_priority = 47;
			break;
		case kPriorityHigh:
			param.sched_priority = 55;
			break;
		case kPriorityTimeCritical:
			param.sched_priority = 63;
			break;
		}
		int result = pthread_setschedparam (threadId, SCHED_RR, &param);
		ASSERT (result == 0)
	}
	else
	{
		mach_timebase_info_data_t machClockInfo;
		mach_timebase_info (&machClockInfo);
		double ticksToNanoseconds = (double)machClockInfo.numer / (double)machClockInfo.denom;
        double msToAbsoluteFactor = 1.e6 / ticksToNanoseconds;
		thread_time_constraint_policy tPolicy;
		memset (&tPolicy, 0, sizeof(thread_time_constraint_policy));
        tPolicy.period = (uint32_t)(50. * msToAbsoluteFactor);
		tPolicy.constraint = (uint32_t)(45. * msToAbsoluteFactor);
		tPolicy.preemptible = true;
		switch(priority)
		{
		case kPriorityRealtimeBase:
			tPolicy.computation =  (uint32_t)(20. * msToAbsoluteFactor);
			break;
		case kPriorityRealtimeMiddle:
			tPolicy.computation =  (uint32_t)(35. * msToAbsoluteFactor);
			break;
		case kPriorityRealtimeTop:
			tPolicy.computation =  (uint32_t)(45. * msToAbsoluteFactor);
			break;
		}
		kern_return_t result = thread_policy_set (pthread_mach_thread_np (threadId), THREAD_TIME_CONSTRAINT_POLICY, (thread_policy_t)&tPolicy, THREAD_TIME_CONSTRAINT_POLICY_COUNT);
		ASSERT (result == KERN_SUCCESS)
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaThread::setCPUAffinity (int affinity)
{
	// Not supported for Cocoa
	// CPU affinity does not pin a thread to a specific core on the Mac, see e.g. https://developer.apple.com/forums/thread/44002
	// Moreover, trying to set an affinity policy on Apple Silicon returns KERN_NOT_SUPPORTED, even with Rosetta 2
}
