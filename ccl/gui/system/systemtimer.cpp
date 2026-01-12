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
// Filename    : ccl/gui/system/systemtimer.h
// Description : System Timer
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/system/systemtimer.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//************************************************************************************************
// SystemTimer
//************************************************************************************************

ObjectList SystemTimer::timers;

//////////////////////////////////////////////////////////////////////////////////////////////////

void SystemTimer::trigger (void* systemTimer)
{
	ListForEachObject (timers, SystemTimer, timer)
		if(timer->systemTimer == systemTimer)
		{
			trigger (timer);
			break;
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SystemTimer::trigger (SystemTimer* timer)
{
	timer->task ();
	if(timer->isKilled ())
		timer->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SystemTimer::SystemTimer (unsigned int periodMilliseconds)
: systemTimer (nullptr),
  tasksIterators (5,5),
  killed (false),
  period ((double)periodMilliseconds / 1000.),
  lastTriggerTime (0)
{
	timers.add (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SystemTimer::~SystemTimer ()
{
	ASSERT (tasks.isEmpty ())
	ASSERT (tasksIterators.isEmpty ())

	timers.remove (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SystemTimer::serviceTimers ()
{
	double now = System::GetProfileTime ();
	
	ListForEachObject (timers, SystemTimer, timer)
		if(timer->lastTriggerTime + timer->period < now)
		{
			trigger (timer);
			timer->lastTriggerTime = now;
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SystemTimer::task ()
{
	struct TaskIteratorGuard
	{
		Vector<TasksIterator*>& tasksIterators;
		TasksIterator& iter;

		TaskIteratorGuard (Vector<TasksIterator*>& tasksIterators, TasksIterator& iter)
		: tasksIterators (tasksIterators), iter (iter)
		{
			tasksIterators.add (&iter);
		}

		~TaskIteratorGuard ()
		{
			tasksIterators.remove (&iter);
		}
	};

	TasksIterator iter (tasks);
	TaskIteratorGuard guard (tasksIterators, iter);

	while(!iter.done ())
	{
		ITimerTask* task = iter.next ();

		#if PROFILE
		UnknownPtr<IObject> obj = task;
		CCL_PROFILE_START (timer)
		#endif
		
		task->onTimer (this);
		
		#if PROFILE
		CCL_PROFILE_STOP (timer)
		if(obj)
			CCL_PRINTLN (obj->getTypeInfo ().getClassName ())
		else
			CCL_PRINTLN ("Unknown")
		#endif
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SystemTimer::kill ()
{
	killed = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SystemTimer::addTask (ITimerTask* task)
{
	tasks.append (task);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SystemTimer::removeTask (ITimerTask* task)
{
	VectorForEachFast (tasksIterators, TasksIterator*, iter)
		// Skip this task if the iterator has prefetched it.
		// This makes it safe to remove another task from a tasks onTimer () call
		if(iter && iter->peekNext () == task)
			iter->next ();
	EndFor

	tasks.remove (task);
}
