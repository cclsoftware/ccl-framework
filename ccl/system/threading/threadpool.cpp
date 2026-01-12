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
// Filename    : ccl/system/threading/threadpool.cpp
// Description : Thread Pool
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/system/threading/threadpool.h"

#include "ccl/base/kernel.h"

#include "ccl/public/systemservices.h"
#include "ccl/public/system/isysteminfo.h"

using namespace CCL;
using namespace Threading;

//////////////////////////////////////////////////////////////////////////////////////////////////
// System Threading APIs
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT IThreadPool& System::CCL_ISOLATED (GetThreadPool) ()
{
	static AutoPtr<ThreadPool> theThreadPool;
	if(theThreadPool == nullptr)
		theThreadPool = NEW ThreadPool;
	return *theThreadPool;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT IThreadPool* System::CCL_ISOLATED (CreateThreadPool) (const ThreadPoolDescription& description)
{
	return NEW ThreadPool (description.maxThreadCount,
						   description.priority, 
						   description.name,
						   description.idleTimeout >= 0 ? description.idleTimeout : ThreadPool::kDefaultTimeout);
}

//************************************************************************************************
// ThreadPool
//************************************************************************************************

ThreadPool::ThreadPool (int maxThreadCount, ThreadPriority priority, StringID name, int idleTimeout)
: maxThreadCount (maxThreadCount),
  minThreadCount (0),
  threadPriority (priority),
  name (name),
  threadIdleTimeout (idleTimeout),
  lastReduceTime (0),
  timerThread (nullptr)
{
	cpuCount = System::GetSystem ().getNumberOfCPUs ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ThreadPool::~ThreadPool ()
{
	terminate (); // too late here for global thread pool, but fine otherwise.
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ThreadPool::terminate ()
{
	{
		ScopedLock scopedLock (theLock);
		if(poolTerminated) // already terminated
			return;

		poolTerminated = 1;

		// there shouldn't be items queued any more, but anyway...
		ASSERT (workItems.isEmpty () == true)
		ListForEach (workItems, IWorkItem*, item)
			item->release ();
		EndFor
	}

	// ensure that all threads are ready to exit
	// hmm... this seems to be a bit complicated... maybe we shouldn't be that paranoid???
	bool anyThreadActive;
	do
	{
		anyThreadActive = false;

		{
			ScopedLock scopedLock (theLock);

			ListForEach (workerThreads, WorkerThread*, thread)
				if(thread->getCurrentWork ())
				{
					thread->getCurrentWork ()->cancel ();
					thread->waitWorkFinished ();
					anyThreadActive = true;
					break;
				}
			EndFor
		}

		if(anyThreadActive)
			System::ThreadSleep (100);
	}
	while(anyThreadActive);

	// now we can exit our worker threads safely
	ListForEach (workerThreads, WorkerThread*, thread)
		thread->exit ();
	EndFor
	workerThreads.removeAll ();

	// exit timer thread
	//ASSERT (periodicItems.isEmpty () == true) framework items might still be registered when app is about to quit!
	if(timerThread)
		timerThread->exit (),
		timerThread = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API ThreadPool::getMaxThreadCount () const
{
	return maxThreadCount;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API ThreadPool::getActiveThreadCount () const
{
	return threadCount;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ThreadPool::allocateThreads (int minCount)
{
    minThreadCount = minCount;
    for(int i = threadCount; i < minThreadCount; i++)
    {
		// spawn new thread if limit not reached yet
		int cpuIndex = threadCount % cpuCount;
        WorkerThread* thread = NEW WorkerThread (*this, name, cpuIndex);
        workerThreads.append (thread);
        ++threadCount;
        thread->start (); 
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ThreadPool::scheduleWork (IWorkItem* item)
{
	ScopedLock scopedLock (theLock);

	ASSERT (!poolTerminated) // should not happen, we are already in dtor!
	if(poolTerminated)
		return;
	
	// add item to queue
	workItems.append (item);

	// try to find a free worker
	ListForEach (workerThreads, WorkerThread*, thread)
		if(!thread->isStarted ()) // thread did even start, we could spawn some more
			continue;

		if(thread->getCurrentWork () == nullptr)
		{
			thread->signal ();
			return;
		}
	EndFor

	if(threadCount < maxThreadCount)
	{
		// spawn new thread if limit not reached yet
		int cpuIndex = threadCount % cpuCount;
		WorkerThread* thread = NEW WorkerThread (*this, name, cpuIndex);
		workerThreads.append (thread);
		++threadCount;
		thread->start ();
	}
	else
	{
		// signal all threads, next one available will grab the item
		ListForEach (workerThreads, WorkerThread*, thread)
			thread->signal ();
		EndFor
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ThreadPool::cancelWork (WorkID id, tbool force)
{
	ScopedLock scopedLock (theLock);

	ASSERT (!poolTerminated) // should not happen, we are already in dtor!
	if(poolTerminated)
		return;

	// check if item is still in work queue
	ListForEach (workItems, IWorkItem*, item)
		if(item->getID () == id)
		{
			workItems.remove (item);
			item->release ();
			return;
		}
	EndFor

	if(force)
	{
		// check if item is currently handled by a thread
		ListForEach (workerThreads, WorkerThread*, thread)
			IWorkItem* item = thread->getCurrentWork ();
			if(item && item->getID () == id)
			{
				item->cancel ();
				thread->waitWorkFinished ();
				return;
			}
		EndFor
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ThreadPool::cancelAll ()
{
	ScopedLock scopedLock (theLock);

	ASSERT (!poolTerminated) // should not happen, we are already in dtor!
	if(poolTerminated)
		return;

	// check if item is still in work queue
	ListForEach (workItems, IWorkItem*, item)
		workItems.remove (item);
		item->release ();
	EndFor

	// check if item is currently handled by a thread
	ListForEach (workerThreads, WorkerThread*, thread)
		IWorkItem* item = thread->getCurrentWork ();
		if(item)
		{
			item->cancel ();
			thread->waitWorkFinished ();
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ThreadPool::addPeriodic (IPeriodicItem* item)
{
	ScopedLock scopedLock (theLock);

	ASSERT (!poolTerminated) // should not happen, we are already in dtor!
	if(poolTerminated)
		return;

	{
		ScopedLock scopedLock (periodicLock);
		periodicItems.append (item);
	}

	if(!timerThread)
	{
		timerThread = NEW TimerThread (*this);
		timerThread->start ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ThreadPool::removePeriodic (IPeriodicItem* item)
{
	ScopedLock scopedLock (theLock);

	//ASSERT (!poolTerminated) // let framework cleanup when app is about to quit
	//if(poolTerminated)
	//	return;

	{
		ScopedLock scopedLock (periodicLock);
		periodicItems.remove (item);
	}

	if(periodicItems.isEmpty ())
	{
		if(timerThread)
			timerThread->exit (),
			timerThread = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ThreadPool::reduceThreads (tbool force)
{
	// don't check that often
	int64 now = System::GetSystemTicks ();
	if(!force && lastReduceTime && (now - lastReduceTime < 5000))
		return;

	ScopedLock scopedLock (theLock);
	lastReduceTime = now = System::GetSystemTicks (); // update after entering lock
	
	// don't stop anything as long as there's work to do
	if(!force && !workItems.isEmpty ())
		return;

	// collect threads which have been idle for a while
	LinkedList<WorkerThread*> idleThreads;
	ListForEach (workerThreads, WorkerThread*, thread)
		if(force && thread->getCurrentWork () == nullptr)
		{
			idleThreads.append (thread);
			continue;
		}

		if(thread->getCurrentWork () == nullptr && thread->getIdleTime ())
		{
			if(now - thread->getIdleTime () >= threadIdleTimeout)
				idleThreads.append (thread);
		}
		else
			thread->setIdleTime (now);
	EndFor

	// remove idle threads
	if(!idleThreads.isEmpty ())
	{
		ListForEach (idleThreads, WorkerThread*, thread)
			workerThreads.remove (thread);
			thread->exit ();
			--threadCount;
		EndFor
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ThreadPool::beginWork (WorkerThread& thread)
{
	ScopedLock scopedLock (theLock);

	IWorkItem* item = workItems.removeFirst ();
	thread.setCurrentWork (item);
	return item != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ThreadPool::endWork (WorkerThread& thread)
{
	ScopedLock scopedLock (theLock);

	thread.setCurrentWork (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ThreadPool::executePeriodic (int64 now)
{
	ScopedLock scopedLock (periodicLock);

	ListForEach (periodicItems, IPeriodicItem*, item)
		if(item->getExecutionTime () <= now)
			item->execute (now);
	EndFor
}

//************************************************************************************************
// WorkerThread
//************************************************************************************************

int CCL_API WorkerThread::run (void* arg)
{
	return ((WorkerThread*)arg)->run ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WorkerThread::WorkerThread (ThreadPool& pool, StringID name, int cpuIndex)
: pool (pool),
  cpuIndex (cpuIndex),
  currentWork (nullptr),
  idleTime (0),
  started (false)
{
	thread = System::CreateNativeThread ({run, name.isEmpty () ? "WorkerThread" : name.str (), this});
	thread->setPriority (pool.getThreadPriority ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WorkerThread::~WorkerThread ()
{
	thread->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WorkerThread::start ()
{
	workSignal.signal (); // initial signal
	thread->start ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WorkerThread::signal ()
{
	workSignal.signal ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WorkerThread::waitWorkFinished ()
{
	ScopedLock scopedLock (workLock);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WorkerThread::exit ()
{
	shouldExit = 1;
	workSignal.signal ();

	if(!thread->join (5000))
	{
		CCL_PRINTLN ("!!! WorkerThread terminated !!!")
		thread->terminate ();
	}

	delete this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int WorkerThread::run ()
{
	if(cpuIndex != -1)
		thread->setCPUAffinity (cpuIndex);

	started = true;
	while(!shouldExit)
	{
		workSignal.wait (kWaitForever);
		if(shouldExit)
			break;
			
		//was: if(pool.beginWork (*this))
		while(!shouldExit && pool.beginWork (*this))
		{
			IWorkItem* oldWorkItem = nullptr;
			{
				ScopedLock scopedLock (workLock);
				currentWork->work ();
				oldWorkItem = currentWork;
			}

			pool.endWork (*this);
			oldWorkItem->release ();
		}
	}
	return 1;
}

//************************************************************************************************
// TimerThread::DeferredExit
//************************************************************************************************

struct TimerThread::DeferredExit: public Unknown
{
	TimerThread& thread;
	DeferredExit (TimerThread& thread): thread (thread) {}
	~DeferredExit ()
	{
		CCL_PRINTLN ("Deferred timer thread exit")
		thread.exit (); // this will delete the object
	}
};

//************************************************************************************************
// TimerThread
//************************************************************************************************

int CCL_API TimerThread::run (void* arg)
{
	return ((TimerThread*)arg)->run ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TimerThread::TimerThread (ThreadPool& pool)
: pool (pool)
{
	thread = System::CreateNativeThread ({run, "TimerThread", this});
	thread->setPriority (pool.getThreadPriority ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TimerThread::~TimerThread ()
{
	thread->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TimerThread::start ()
{
	thread->start ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TimerThread::exit ()
{
	shouldExit = 1;

	// defer exit when called from timer thread,
	// can happen only from within ThreadPool::removePeriodic()
	if(System::GetThreadSelfID () == thread->getThreadID ())
	{
		Kernel::instance ().deferDestruction (NEW DeferredExit (*this));
	}
	else
	{
		if(!thread->join (5000))
		{
			CCL_PRINTLN ("!!! TimerThread terminated !!!")
			thread->terminate ();
		}

		delete this;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int TimerThread::run ()
{
	while(!shouldExit)
	{
		int64 startTime = System::GetSystemTicks ();
		int timeToWait = pool.getThreadPriority () < kPriorityNormal ? 250 : 100; // in milliseconds... to be tested!

		pool.executePeriodic (startTime);

		int64 endTime = System::GetSystemTicks ();
		int delta = (int)(endTime - startTime);
		timeToWait -= delta;
		if(timeToWait < 1)
			timeToWait = 1;

		if(!shouldExit)
			System::ThreadSleep (timeToWait);
	}
	return 1;
}
