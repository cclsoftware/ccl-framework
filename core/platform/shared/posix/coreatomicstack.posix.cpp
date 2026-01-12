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
// Filename    : core/platform/shared/posix/coreatomicstack.posix.cpp
// Description : Atomic Stack POSIX implementation
//
//************************************************************************************************

#include "coreatomicstack.posix.h"

#include "core/system/corespinlock.h"

namespace Core {
namespace Platform {

//************************************************************************************************
// ListEntry
//************************************************************************************************

struct ListEntry
{
	// Want a volatile pointer to non-volatile data so place after all type info
	ListEntry* volatile next;
};
	
//************************************************************************************************
// ListHeader
//************************************************************************************************

struct ListHeader
{
	// Want a volatile pointer to non-volatile data so place after all type info
	ListEntry* volatile head;
	volatile long depth;
	volatile int32 mutex;
};

//************************************************************************************************
// PriorityScope
/** Raises the current thread's priority to match the highest priority of any thread using
 * a specific AtomicStack instance and resets it when leaving scope. */
//************************************************************************************************

struct PosixAtomicStack::PriorityScope
{
	PriorityScope (int32 volatile& maxPriority)
	: originalPolicy (0),
	  originalSchedParam {0}
	{
		pthread_getschedparam (pthread_self (), &originalPolicy, &originalSchedParam);
		
		int policy = originalPolicy;
		if(originalPolicy == SCHED_OTHER)
			policy = SCHED_RR;

		sched_param schedParam = {0};

		while(true)
		{
			schedParam.sched_priority = AtomicGet (maxPriority);
			if(schedParam.sched_priority >= originalSchedParam.sched_priority)
				break;
			if(AtomicTestAndSet (maxPriority, originalSchedParam.sched_priority, schedParam.sched_priority))
			{
				schedParam.sched_priority = originalSchedParam.sched_priority;
				break;
			}
		}
		pthread_setschedparam (pthread_self (), policy, &schedParam);
	}
	
	~PriorityScope ()
	{
		pthread_setschedparam (pthread_self (), originalPolicy, &originalSchedParam);
	}
	
private:
	int originalPolicy;
	sched_param originalSchedParam;
};

} // namespace Platform
} // namespace Core

using namespace Core;
using namespace Platform;

//************************************************************************************************
// PosixAtomicStack
//************************************************************************************************

PosixAtomicStack::PosixAtomicStack ()
: head (nullptr),
  maxThreadPriority (0)
{
	::posix_memalign ((void**)&head, 16, sizeof(ListHeader));
	ASSERT (head != nullptr)
	head->head = nullptr;
	head->depth = 0;
	head->mutex = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PosixAtomicStack::~PosixAtomicStack ()
{
	(::free) (head);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PosixAtomicStack::Element* PosixAtomicStack::pop ()
{
	ASSERT (head != nullptr)
	ListEntry* oldHead = head->head;
	if(oldHead == nullptr)
		return nullptr;
	
	PriorityScope scope (maxThreadPriority);
	CoreSpinLock::lock (head->mutex);
	
	oldHead = head->head;
	if(oldHead != nullptr)
	{
		head->head = oldHead->next;
		--head->depth;
		ASSERT (head->depth >= 0)
	}

	CoreSpinLock::unlock (head->mutex);
	
	Element* e = reinterpret_cast<Element*> (oldHead);
	return e;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PosixAtomicStack::push (Element* e)
{
	ASSERT(((int64)e & 0x7) == 0)
	ListEntry* newEntry = reinterpret_cast<ListEntry*> (e);
	ASSERT (head != nullptr)
	
	PriorityScope scope (maxThreadPriority);
	CoreSpinLock::lock (head->mutex);
	
	ListEntry* oldHead = head->head;
	newEntry->next = oldHead;
	head->head = newEntry;
	++head->depth;

	CoreSpinLock::unlock (head->mutex);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PosixAtomicStack::flush ()
{
	ASSERT (head != nullptr)

	PriorityScope scope (maxThreadPriority);
	CoreSpinLock::lock (head->mutex);
	
	head->head = nullptr;
	head->depth = 0;

	CoreSpinLock::unlock (head->mutex);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int PosixAtomicStack::depth ()
{
	ASSERT (head != nullptr)
	return static_cast<int> (head->depth);
}
