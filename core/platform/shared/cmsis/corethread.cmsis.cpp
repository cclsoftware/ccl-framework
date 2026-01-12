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
// Filename    : core/platform/shared/cmsis/corethread.cmsis.cpp
// Description : CMSIS Multithreading
//
//************************************************************************************************

#include "core/platform/corefeatures.h"

#if CORE_THREAD_IMPLEMENTATION == CORE_PLATFORM_IMPLEMENTATION
#include CORE_PLATFORM_IMPLEMENTATION_HEADER (corethread)
#else
#include "core/platform/shared/cmsis/corethread.cmsis.h"
#endif

#include "core/system/coredebug.h"

using namespace Core;
using namespace Platform;
using namespace Threads;

//************************************************************************************************
// Thread Functions
//************************************************************************************************

ThreadID CurrentThread::getID ()
{
	return reinterpret_cast<ThreadID> (osThreadGetId ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ThreadPriority CurrentThread::setPriority (ThreadPriority newPrio)
{
	// TODO: implement me!
	ASSERT (0)
	return newPrio;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CurrentThread::sleep (uint32 milliseconds)
{
	osDelay (milliseconds * SystemClock::getFrequency () / 1000);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CurrentThread::ussleep (uint32 microseconds)
{
	osDelay (microseconds * SystemClock::getFrequency () / 1000000);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CurrentThread::yield ()
{
	osThreadYield ();
}

//************************************************************************************************
// Thread local storage
//************************************************************************************************

static void TLSDestructor (void* p)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

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
// CmsisThread
//************************************************************************************************

ThreadPriorityHandler* ThreadPriorityHandler::customHandler = 0;

//////////////////////////////////////////////////////////////////////////////////////////////////

static void ThreadEntry (void* param)
{
	CmsisThread* thread = reinterpret_cast<CmsisThread*> (param);
	IThreadEntry* entry = thread->getThreadEntry ();

	if(entry != 0)
		entry->threadEntry ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CmsisThread::CmsisThread ()
: threadId (0),
  entry (0),
  name ("Thread"),
  stackSize (0),
  threadStack (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

CmsisThread::~CmsisThread ()
{
	if(threadStack != 0)
		free (threadStack);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CmsisThread::setStackSize (int size)
{
	// stack size may only be set prior to running task
    ASSERT (threadStack == 0)
	ASSERT ((size & 3) == 0)   // must be word multiple

	if (threadStack == 0)  // stack not allocated
	{
		threadStack = malloc (size);

		if(threadStack == 0)
			return false;
	}

	stackSize = size;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IThreadEntry* CmsisThread::getThreadEntry ()
{
	return entry;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CmsisThread::open (ThreadID _threadId)
{
	threadId = reinterpret_cast<osThreadId_t> (_threadId);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CmsisThread::start (const ThreadInfo& info)
{
	entry = info.entry;
	name = info.name;
	
	// Create thread stack space
	if(threadStack == 0)
		setStackSize (kDefaultStackSize); // OS_STKSIZE

	osThreadAttr_t attr;
	attr.name = (info.name == 0) ? kThreadName : info.name;
	attr.attr_bits = osThreadJoinable;
	attr.cb_mem = &threadData;
	attr.cb_size = sizeof(threadData);
	attr.stack_mem = threadStack;
	attr.stack_size = stackSize;
	attr.priority = osPriorityNormal;
	attr.tz_module = 0;
	attr.reserved = 0;

	threadId = osThreadNew (ThreadEntry, this, &attr);
	ASSERT (threadId != 0)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CmsisThread::terminate ()
{
	osStatus_t status = osThreadTerminate (threadId);
	if(status != osOK)
	{
		ASSERT (status == osOK)
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CmsisThread::join (uint32 milliseconds)
{
	return osThreadJoin (threadId) == osOK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CmsisThread::setPriority (int priority)
{
	osThreadSetPriority (threadId, toNativePriority (priority));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CmsisThread::getPriority () const
{
	return fromNativePriority (osThreadGetPriority (threadId));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CmsisThread::setCPUAffinity (int affinity)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CmsisThread::getPlatformPriority () const
{
	return osThreadGetPriority(threadId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 CmsisThread::getUserModeTime () const
{
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ThreadID CmsisThread::getID () const
{
	return reinterpret_cast<ThreadID> (threadId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

osPriority_t CmsisThread::toNativePriority (int priority)
{
	switch(priority)
	{
	case kPriorityLow:
		return osPriorityLow;

	case kPriorityBelowNormal:
		return osPriorityBelowNormal;

	case kPriorityNormal:
		return osPriorityNormal;

	case kPriorityAboveNormal:
		return osPriorityAboveNormal;

	case kPriorityHigh:
		return osPriorityHigh;

	case kPriorityRealtimeBase:
		return osPriorityRealtime;

	case kPriorityRealtimeMiddle:
		return osPriorityRealtime4;

	case kPriorityRealtimeTop:
		return osPriorityRealtime7;

	default:
		return osPriorityNormal;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CmsisThread::fromNativePriority (osPriority_t priority)
{
	switch(priority)
	{
	case osPriorityLow:
		return kPriorityLow;

	case osPriorityBelowNormal:
		return kPriorityBelowNormal;

	case osPriorityNormal:
		return kPriorityNormal;

	case osPriorityAboveNormal:
		return kPriorityAboveNormal;

	case osPriorityHigh:
		return kPriorityHigh;

	case osPriorityRealtime:
		return kPriorityRealtimeBase;

	case osPriorityRealtime4:
		return kPriorityRealtimeMiddle;

	case osPriorityRealtime7:
		return kPriorityRealtimeTop;

	default:
		return kPriorityNormal;
	}
}

//************************************************************************************************
// CmsisLock
//************************************************************************************************

CmsisLock::CmsisLock ()
{
	osMutexAttr_t attr;
	attr.attr_bits = osMutexRecursive;
	attr.cb_mem = &mutexData;
	attr.cb_size = sizeof(mutexData);
	attr.name = "CmsisLock Mutex";
		
	mutexId = osMutexNew (&attr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CmsisLock::CmsisLock (const osMutexAttr_t& attributes)
{
	mutexId = osMutexNew (&attributes);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CmsisLock::~CmsisLock ()
{
	osMutexDelete (mutexId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CmsisLock::unlock ()
{
	osMutexRelease (mutexId);
}

//************************************************************************************************
// CmsisSignal
//************************************************************************************************

CmsisSignal::CmsisSignal (bool manualReset)
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////
		
CmsisSignal::~CmsisSignal ()
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CmsisSignal::signal ()
{
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

void CmsisSignal::reset ()
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CmsisSignal::wait (uint32 milliseconds)
{
	return false;
}

//************************************************************************************************
// CmsisReadWriteLock
//************************************************************************************************

CmsisReadWriteLock::CmsisReadWriteLock ()
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CmsisReadWriteLock::~CmsisReadWriteLock ()
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CmsisReadWriteLock::lockWrite ()
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CmsisReadWriteLock::unlockWrite ()
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CmsisReadWriteLock::lockRead ()
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CmsisReadWriteLock::unlockRead ()
{
}
