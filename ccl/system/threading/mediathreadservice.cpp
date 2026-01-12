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
// Filename    : ccl/system/threading/mediathreadservice.cpp
// Description : Multimedia Threading Services
//
//************************************************************************************************

#include "ccl/system/threading/mediathreadservice.h"
#include "ccl/system/threading/thread.h" // for NativeThreadRegistrar

#include "ccl/public/system/floatcontrol.h"
#include "ccl/public/systemservices.h"

namespace CCL {

//************************************************************************************************
// CustomThreadPriorityHandler
//************************************************************************************************

class CustomThreadPriorityHandler: public Core::Threads::ThreadPriorityHandler
{
public:
	PROPERTY_SHARED_AUTO (IMediaThreadPriorityHandler, handler, Handler)

	// ThreadPriorityHandler
	bool setSelfToRealtimePriority (Threading::ThreadPriority priority) override
	{
		ASSERT (handler != nullptr)
		return handler ? handler->setSelfToRealtimePriority (priority) == kResultOk : false;
	}
};

static CustomThreadPriorityHandler customPriorityHandler;

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// MediaThreadService
//************************************************************************************************

MediaThreadService::MediaThreadService ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MediaThreadService::startup ()
{
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MediaThreadService::shutdown ()
{
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

double CCL_API MediaThreadService::getMediaTime ()
{
	return System::GetProfileTime ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMediaTimer* CCL_API MediaThreadService::createTimer (StringID name, IMediaTimerTask& task, uint32 period, int timerID, Threading::ThreadPriority priority)
{
	return NEW MediaTimer (name, task, period, timerID, priority);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MediaThreadService::getThreadsSnapshot (Threading::ThreadInfo infos[], int& count)
{
	count = Threading::NativeThreadRegistrar::getSnapshot (infos, count);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MediaThreadService::setPriorityHandler (IMediaThreadPriorityHandler* priorityHandler)
{
	customPriorityHandler.setHandler (priorityHandler);
	Core::Threads::ThreadPriorityHandler::customHandler = priorityHandler ? &customPriorityHandler : nullptr;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMediaThreadWorkgroupHandler* CCL_API MediaThreadService::getWorkgroupHandler ()
{
	return nullptr;
}

//************************************************************************************************
// MediaTimer
//************************************************************************************************

int CCL_API MediaTimer::threadEntry (void* arg)
{
	return ((MediaTimer*)arg)->run ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MediaTimer::MediaTimer (StringID name, IMediaTimerTask& task, uint32 period, int timerID, Threading::ThreadPriority priority)
: name (name),
  task (task),
  period (period),
  timerID (timerID),
  priority (priority),
  thread (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

MediaTimer::~MediaTimer ()
{
	stop ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API MediaTimer::getTimerID () const
{
	return timerID;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API MediaTimer::isRunning () const
{
	return thread != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API MediaTimer::start ()
{
	if(!thread)
	{
		shouldExit = 0;
		thread = System::CreateNativeThread ({threadEntry, name, this});
		thread->setPriority (priority);
		thread->start ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API MediaTimer::stop ()
{
	if(thread)
	{
		shouldExit = 1;
		if(!thread->join (5000))
			thread->terminate ();
		thread->release ();
		thread = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int MediaTimer::run ()
{
	setFloatEnv ();

	while(!shouldExit)
	{
		double profileStartTime = System::GetProfileTime ();
		
		double time = System::GetMediaThreadService ().getMediaTime ();
		task.task (timerID, time);

		int durationMs = int ((System::GetProfileTime () - profileStartTime) * 1000.);

		uint32 sleepTime = ccl_max (1, int (period) - durationMs);

		System::ThreadSleep (sleepTime);
	}
	return 0;
}
