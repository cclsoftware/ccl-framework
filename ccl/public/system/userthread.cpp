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
// Filename    : ccl/public/system/userthread.cpp
// Description : User thread base class
//
//************************************************************************************************

#include "ccl/public/system/userthread.h"

#include "ccl/public/base/debug.h"
#include "ccl/public/systemservices.h"

using namespace CCL;
using namespace Threading;

//************************************************************************************************
// UserThread
//************************************************************************************************

int CCL_API UserThread::threadFunc (void* arg)
{
	UserThread* userThread = reinterpret_cast<UserThread*> (arg);
	int result = userThread->threadEntry ();
	userThread->threadAlive = false;
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UserThread::UserThread (const char* threadName)
: threadName (threadName),
  thread (nullptr),
  threadAlive (false),
  terminateRequested (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

UserThread::~UserThread ()
{
	ASSERT (!isThreadStarted ())
	stopThread (1000);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UserThread::isThreadStarted () const
{
	return thread != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UserThread::isThreadAlive () const
{
	return threadAlive;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UserThread::shouldTerminate () const
{ 
	return terminateRequested; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserThread::requestTerminate ()
{ 
	terminateRequested = true; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserThread::startThread (ThreadPriority priority, int cpuAffinity)
{
	if(thread == nullptr)
	{
		CCL_PRINTF ("UserThread \"%s\": startThread ()\n", threadName)
		terminateRequested = false;

		thread = System::CreateNativeThread ({threadFunc, threadName, this});
		thread->setPriority (priority);
		if(cpuAffinity >= 0)
			thread->setCPUAffinity (cpuAffinity);
		thread->start ();
		threadAlive = true;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UserThread::stopThread (unsigned int milliseconds)
{
	bool result = true;
	if(thread)
	{
		CCL_PRINTF ("UserThread \"%s\": stopThread ()\n", threadName)
		terminateRequested = true;

		if(!thread->join (milliseconds))
		{
			thread->terminate ();
			result = false;
		}
		
		thread->release ();
		thread = nullptr;
		threadAlive = false;
	}
	return result;
}
