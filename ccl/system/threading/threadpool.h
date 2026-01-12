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
// Filename    : ccl/system/threading/threadpool.h
// Description : Thread Pool
//
//************************************************************************************************

#ifndef _ccl_threadpool_h
#define _ccl_threadpool_h

#include "ccl/public/base/unknown.h"
#include "ccl/public/text/cstring.h"
#include "ccl/public/system/threadsync.h"
#include "ccl/public/system/ithreadpool.h"
#include "ccl/public/collections/linkedlist.h"

namespace CCL {
namespace Threading {

class WorkerThread;
class TimerThread;

//************************************************************************************************
// ThreadPool
//************************************************************************************************

class ThreadPool: public Unknown,
				  public IThreadPool
{
public:
	enum Defaults { kDefaultTimeout = 10 * 1000 };

	ThreadPool (int maxThreadCount = 5, 
				ThreadPriority priority = kPriorityBelowNormal,
				StringID name = "ThreadPool",
				int idleTimeout = kDefaultTimeout);
	~ThreadPool ();

	PROPERTY_VARIABLE (ThreadPriority, threadPriority, ThreadPriority)

	// IThreadPool
	int CCL_API getMaxThreadCount () const override;
	int CCL_API getActiveThreadCount () const override;
	void CCL_API allocateThreads (int minCount) override;
	void CCL_API scheduleWork (IWorkItem* item) override;
	void CCL_API cancelWork (WorkID id, tbool force = false) override;
	void CCL_API cancelAll () override;
	void CCL_API addPeriodic (IPeriodicItem* item) override;
	void CCL_API removePeriodic (IPeriodicItem* item) override;
	void CCL_API reduceThreads (tbool force) override;
	void CCL_API terminate () override;

	// internal methods used by WorkerThread
	bool beginWork (WorkerThread& thread);
	void endWork (WorkerThread& thread);

	// called by TimerThread
	void executePeriodic (int64 now);

	CLASS_INTERFACE (IThreadPool, Unknown)

protected:
	int cpuCount;
	int maxThreadCount;
	int minThreadCount;
	int threadIdleTimeout;
	int64 lastReduceTime;
	AtomicInt threadCount;
	AtomicInt poolTerminated;
	CriticalSection theLock;
	MutableCString name;
	LinkedList<IWorkItem*> workItems;
	LinkedList<WorkerThread*> workerThreads;

	CriticalSection periodicLock;
	LinkedList<IPeriodicItem*> periodicItems;
	TimerThread* timerThread;
};

//************************************************************************************************
// WorkerThread
//************************************************************************************************

class WorkerThread
{
public:
	WorkerThread (ThreadPool& pool, StringID name, int cpuIndex = -1);
	~WorkerThread ();

	PROPERTY_BOOL (started, Started)
	PROPERTY_POINTER (IWorkItem, currentWork, CurrentWork)
	PROPERTY_VARIABLE (int64, idleTime, IdleTime)

	void start ();
	void signal ();
	void waitWorkFinished ();
	void exit ();

protected:
	ThreadPool& pool;
	int cpuIndex;
	IThread* thread;
	Signal workSignal;
	AtomicInt shouldExit;
	CriticalSection workLock;

	int run ();
	static int CCL_API run (void* arg);
};

//************************************************************************************************
// TimerThread
//************************************************************************************************

class TimerThread
{
public:
	TimerThread (ThreadPool& pool);
	~TimerThread ();

	void start ();
	void exit ();

protected:
	struct DeferredExit;

	ThreadPool& pool;
	IThread* thread;
	AtomicInt shouldExit;

	int run ();
	static int CCL_API run (void* arg);
};

} // namespace Threading
} // namespace CCL

#endif // _ccl_threadpool_h
