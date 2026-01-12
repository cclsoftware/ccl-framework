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
// Filename    : core/platform/azure/corethread.azure.cpp
// Description : Azure RTOS Thread Primitives
//
//************************************************************************************************

#include "corethread.azure.h"
#include "coretime.azure.h"

#include "core/platform/shared/coreplatformsupervisor.h"

using namespace Core;
using namespace Platform;

//************************************************************************************************
// CurrentThread
//************************************************************************************************

Threads::ThreadID CurrentThread::getID ()
{
	return reinterpret_cast<Threads::ThreadID> (tx_thread_identify ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Threads::ThreadPriority CurrentThread::setPriority (Threads::ThreadPriority newPrio)
{
	unsigned int oldPriority;
	unsigned int returnCode = tx_thread_priority_change (tx_thread_identify (), newPrio, &oldPriority);
	return returnCode == TX_SUCCESS ? newPrio : oldPriority;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CurrentThread::sleep (uint32 milliseconds)
{
	if(milliseconds > 0)
	{
		unsigned long ticks = milliseconds * SystemClock::getFrequency () / 1000;
		tx_thread_sleep (ticks);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CurrentThread::ussleep (uint32 microseconds)
{
	if(microseconds > 1000)
	{
		sleep (microseconds / 1000);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CurrentThread::yield ()
{
	tx_thread_relinquish ();
}

//************************************************************************************************
// AzureThread
//************************************************************************************************

AzureThread::AzureThread ()
: entry (nullptr)
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AzureThread::~AzureThread ()
{
	unsigned int state;
	tx_thread_info_get (&txThread, nullptr, &state, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
	tx_thread_terminate (&txThread);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AzureThread::open (Threads::ThreadID id)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AzureThread::start (const ThreadInfo& info)
{
	entry = info.entry;
	if(entry != 0)
	{
		void* stack = nullptr;
		int stackSize = 0;
		GetSystemSupervisor ().getThreadStack (stack, stackSize, info.name);
		tx_thread_create (&txThread, const_cast<char*>(info.name), &entryWrapper, 0, stack, stackSize, Threads::kPriorityNormal, Threads::kPriorityNormal, TX_NO_TIME_SLICE, TX_AUTO_START);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AzureThread::join (uint32 milliseconds)
{
	unsigned int state;
	tx_thread_info_get (&txThread, nullptr, &state, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
	if(state != TX_TERMINATED || state != TX_COMPLETED)
	{
		tx_thread_entry_exit_notify (&txThread, &joinCallback);
		tx_thread_sleep (milliseconds * HighPerformanceClock::getFrequency () / 1000);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AzureThread::terminate ()
{
	tx_thread_terminate (&txThread);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int AzureThread::getPriority () const
{
	return getPlatformPriority ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AzureThread::setPriority (int priority)
{
	unsigned int oldPriority;
	tx_thread_priority_change (&txThread, priority, &oldPriority);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AzureThread::setCPUAffinity (int affinity)
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int AzureThread::getPlatformPriority () const
{
	unsigned int priority = Threads::kPriorityNormal;
	tx_thread_info_get (const_cast<TX_THREAD*>(&txThread), nullptr, nullptr, nullptr, &priority, nullptr, nullptr, nullptr, nullptr);
	return priority;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 AzureThread::getUserModeTime () const
{
	// Not Available
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Threads::ThreadID AzureThread::getID () const
{
	return reinterpret_cast<Threads::ThreadID>(&txThread);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AzureThread::entryWrapper (unsigned long arg)
{
	static_assert (sizeof(unsigned long) >= sizeof(IThreadEntry*));
	reinterpret_cast<IThreadEntry*>(arg)->threadEntry ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AzureThread::joinCallback (TX_THREAD* joinedThread, unsigned int type)
{
	TX_THREAD* suspendedThread = nullptr;
	tx_thread_info_get (joinedThread, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &suspendedThread);
	tx_thread_wait_abort (suspendedThread);
}

//************************************************************************************************
// AzureLock
//************************************************************************************************

AzureLock::AzureLock ()
{
	tx_mutex_create (&txMutex, "txMutex", TX_NO_INHERIT);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AzureLock::~AzureLock ()
{
	tx_mutex_delete (&txMutex);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AzureLock::lock ()
{
	tx_mutex_get (&txMutex, TX_WAIT_FOREVER);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AzureLock::tryLock ()
{
	tx_mutex_get (&txMutex, TX_NO_WAIT);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AzureLock::unlock ()
{
	tx_mutex_put (&txMutex);
}

//************************************************************************************************
// AzureSignal
//************************************************************************************************

AzureSignal::AzureSignal (bool _manualReset)
: manualReset (_manualReset)
{
	tx_event_flags_create (&txEvent, "txEvent");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AzureSignal::~AzureSignal ()
{
	tx_event_flags_delete (&txEvent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AzureSignal::signal ()
{
	tx_event_flags_set (&txEvent, 1, TX_OR);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AzureSignal::reset ()
{
	tx_event_flags_set (&txEvent, 0, TX_AND);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AzureSignal::wait (uint32 milliseconds)
{
	unsigned int getOption = manualReset ? TX_AND : TX_AND_CLEAR;
	unsigned long actualFlags = 0;
	unsigned long waitOption = milliseconds > 0 ? milliseconds * HighPerformanceClock::getFrequency () / 1000 : TX_NO_WAIT;
	tx_event_flags_get (&txEvent, 1, getOption, &actualFlags, waitOption);
	return actualFlags == 1;
}

//************************************************************************************************
// AzureReadWriteLock
//************************************************************************************************

AzureReadWriteLock::AzureReadWriteLock ()
: owner (nullptr)
{
	tx_semaphore_create (&writeSemaphore, "writeSemaphore", 1);
	tx_semaphore_create (&readSemaphore, "readSemaphore", 1);
	tx_semaphore_create (&activeReadSemaphore, "activeReadSemaphore", CONFIG_FWAPP_MAX_THREADS + 1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AzureReadWriteLock::~AzureReadWriteLock ()
{
	tx_semaphore_delete (&writeSemaphore);
	tx_semaphore_delete (&readSemaphore);
	tx_semaphore_delete (&activeReadSemaphore);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AzureReadWriteLock::lockWrite ()
{
	if(tx_semaphore_get (&writeSemaphore, TX_WAIT_FOREVER) == TX_SUCCESS)
	{
		if(tx_semaphore_get (&activeReadSemaphore, TX_WAIT_FOREVER) == TX_SUCCESS)
			owner = tx_thread_identify ();
		else
			tx_semaphore_put (&writeSemaphore);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AzureReadWriteLock::unlockWrite ()
{
	if(tx_thread_identify () == owner)
	{
		owner = nullptr;
		tx_semaphore_put (&writeSemaphore);
		tx_semaphore_put (&activeReadSemaphore);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AzureReadWriteLock::lockRead ()
{
	if(tx_semaphore_get (&writeSemaphore, TX_WAIT_FOREVER) == TX_SUCCESS)
	{
		tx_semaphore_get (&readSemaphore, TX_NO_WAIT);
		tx_semaphore_get (&activeReadSemaphore, TX_NO_WAIT);
		tx_semaphore_put (&writeSemaphore);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AzureReadWriteLock::unlockRead ()
{
	unsigned long count = 0;
	tx_semaphore_info_get (&readSemaphore, nullptr, &count, nullptr, nullptr, nullptr);

	if(count == CONFIG_FWAPP_MAX_THREADS)
		tx_semaphore_put (&activeReadSemaphore);
	tx_semaphore_put (&readSemaphore);
}
