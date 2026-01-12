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
// Filename    : core/portable/coreworker.h
// Description : Background Worker
//
//************************************************************************************************

#ifndef _coreworker_h
#define _coreworker_h

#include "core/system/corethread.h"
#include "core/public/coreintrusivelist.h"

namespace Core {
namespace Portable {

/** Background task identifer used for cancelation. */
typedef void* BackgroundTaskID;

//************************************************************************************************
// BackgroundTask
/** Abstract base class for background tasks.
	\ingroup core_portable */
//************************************************************************************************

struct BackgroundTask: IntrusiveLink<BackgroundTask>
{
	BackgroundTaskID id;

	BackgroundTask (BackgroundTaskID id = nullptr)
	: id (id)
	{}

	virtual ~BackgroundTask () {}

	virtual void cancel () {}

	virtual void work () = 0;
};

//************************************************************************************************
// BackgroundTaskList
/** List of background tasks. 
	\ingroup core_portable */
//************************************************************************************************

struct BackgroundTaskList: BackgroundTask
{
	IntrusiveLinkedList<BackgroundTask> subTasks;

	BackgroundTaskList (BackgroundTaskID id = nullptr)
	: BackgroundTask (id)
	{}

	~BackgroundTaskList ()
	{
		while(BackgroundTask* subTask = subTasks.removeFirst ())
			delete subTask;
	}

	// BackgroundTask
	void work () override
	{
		IntrusiveListForEach (subTasks, BackgroundTask, subTask)
			subTask->work ();
		EndFor
	}
};

//************************************************************************************************
// BackgroundWorker
/** Manages a background thread with a queue of tasks.
	\ingroup core_portable */
//************************************************************************************************

class BackgroundWorker
{
public:
	BackgroundWorker ();
	virtual ~BackgroundWorker ();

	void setPriority (Threads::ThreadPriority priority);
	
	void addTask (BackgroundTask* task);

	enum CancelResult { kCancelNotFound, kCancelPending, kCancelDone };
	CancelResult cancelTask (BackgroundTaskID id);

	void terminate ();

protected:	
	class WorkerThread;

	Threads::ThreadPriority priority;
	Threads::Lock lock;
	IntrusiveLinkedList<BackgroundTask> tasks;
	BackgroundTask* currentTask;
	WorkerThread* thread;
	bool shouldTerminate;

	BackgroundTask* retrieveTask ();
	void setCurrentTask (BackgroundTask* task);
};

} // namespace Portable
} // namespace Core

#endif // _coreworker_h
