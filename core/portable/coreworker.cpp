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
// Filename    : core/portable/coreworker.cpp
// Description : Background Worker
//
//************************************************************************************************

#include "core/portable/coreworker.h"

using namespace Core;
using namespace Portable;

//************************************************************************************************
// BackgroundWorker::WorkerThread
//************************************************************************************************

class BackgroundWorker::WorkerThread: public Threads::Thread
{
public:	
	BackgroundWorker& worker;

	WorkerThread (BackgroundWorker& worker, CStringPtr name = Platform::kThreadName)
	: Thread (name),
	  worker (worker)
	{}
	
	// Thread
	int threadEntry () override
	{
		while(!worker.shouldTerminate)
		{
			if(BackgroundTask* task = worker.retrieveTask ())
			{
				worker.setCurrentTask (task);
				task->work ();
				worker.setCurrentTask (nullptr);
				delete task;
			}
			else
				Threads::CurrentThread::sleep (100);
		}
		return 0;
	}
};

//************************************************************************************************
// BackgroundWorker
//************************************************************************************************

BackgroundWorker::BackgroundWorker ()
: priority (Threads::kPriorityLow),
  currentTask (nullptr),
  thread (nullptr),
  shouldTerminate (false)
{}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

BackgroundWorker::~BackgroundWorker ()
{
	ASSERT (thread == nullptr) // terminate() must be called!
	if(thread)
		delete thread;

	ASSERT (currentTask == nullptr)
	ASSERT (tasks.isEmpty ())
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BackgroundWorker::setPriority (Threads::ThreadPriority _priority)
{
	priority = _priority;
	if(thread)
		thread->setPriority (priority);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BackgroundWorker::addTask (BackgroundTask* task)
{
	Threads::ScopedLock scopedLock (lock);
	tasks.append (task);
	if(thread == nullptr)
	{
		shouldTerminate = false;
		thread = NEW WorkerThread (*this, "BackgroundWorker");
		thread->setPriority (priority);
		thread->start ();
	}
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

BackgroundWorker::CancelResult BackgroundWorker::cancelTask (BackgroundTaskID id)
{
	ASSERT (id != nullptr) // null is not a valid task identifier!
	if(id == nullptr)
		return kCancelNotFound;

	Threads::ScopedLock scopedLock (lock);

	if(currentTask && currentTask->id == id)
	{
		currentTask->cancel ();
		return kCancelPending;
	}

	IntrusiveListForEach (tasks, BackgroundTask, t)
		if(t->id == id)
		{
			tasks.remove (t);
			delete t;
			return kCancelDone;
		}
	EndFor
	return kCancelNotFound;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BackgroundTask* BackgroundWorker::retrieveTask ()
{
	BackgroundTask* t = nullptr;
	if(lock.tryLock ())
	{
		t = tasks.removeFirst ();
		lock.unlock ();
	}
	return t;
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

void BackgroundWorker::setCurrentTask (BackgroundTask* task)
{
	Threads::ScopedLock scopedLock (lock);
	currentTask = task;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BackgroundWorker::terminate ()
{
	Threads::ScopedLock scopedLock (lock);
	shouldTerminate = true;
	if(thread)
	{
		if(!thread->join (5000))
			thread->terminate ();
		delete thread;
		thread = nullptr;			
	}
		
	while(BackgroundTask* t = tasks.removeFirst ())
		delete t;
}
