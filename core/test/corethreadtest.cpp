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
// Filename    : core/test/corethreadtest.cpp
// Description : Core Thread Tests
//
//************************************************************************************************

#include "corethreadtest.h"

#include "core/system/corethread.h"
#include "core/system/coretime.h"

namespace Core {
namespace Test {

//************************************************************************************************
// TestThread
//************************************************************************************************

class TestThread: public Threads::Thread
{
public:
	TestThread (int& value)
	: Thread ("Thread Test Thread"),
	  value (value)
	{}

	// Thread
	int threadEntry ()
	{
		value = 2;
		while(value == 2)
			Threads::CurrentThread::sleep (500);
		value = 3;
		return true;
	}

protected:
	int& value;
};

//************************************************************************************************
// LockingThread
//************************************************************************************************

class LockingThread: public Threads::Thread
{
public:
	LockingThread (int& value, Threads::Lock& lock)
	: Thread ("Locking Test Thread"),
	  value (value),
	  lock (lock)
	{}

	// Thread
	int threadEntry ()
	{
		value = 2;
		lock.lock ();
		value = 3;
		lock.unlock ();
		return true;
	}

protected:
	int& value;
	Threads::Lock& lock;
};

//************************************************************************************************
// SignalThread
//************************************************************************************************

class SignalThread: public Threads::Thread
{
public:
	static const int kTimeout = 50;

	SignalThread (Threads::Signal& signal)
	: Thread ("Signal Test Thread"),
	  signal (signal),
	  waitTime1 (0),
	  waitTime2 (0),
	  returnValue1 (false),
	  returnValue2 (false)
	{}

	// Thread
	int threadEntry ()
	{
		abs_time time1 = SystemClock::getMilliseconds ();
		returnValue1 = signal.wait (kTimeout);
		abs_time time2 = SystemClock::getMilliseconds ();
		waitTime1 = time2 - time1;
		returnValue2 = signal.wait (kTimeout);
		waitTime2 = SystemClock::getMilliseconds () - time2;
		return 1;
	}

	PROPERTY_VARIABLE (abs_time, waitTime1, WaitTime1)
	PROPERTY_VARIABLE (abs_time, waitTime2, WaitTime2)
	PROPERTY_BOOL (returnValue1, ReturnValue1)
	PROPERTY_BOOL (returnValue2, ReturnValue2)

protected:
	Threads::Signal& signal;
};

} // namespace Test
} // namespace Core

using namespace Core;
using namespace Threads;
using namespace Test;

//************************************************************************************************
// ThreadTest
//************************************************************************************************

CORE_REGISTER_TEST (ThreadTest)

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr ThreadTest::getName () const
{
	return "Core Thread";
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ThreadTest::run (ITestContext& testContext)
{
	static bool succeeded;
	succeeded = true;

	static int value;
	value = 1;
	
	TestThread* testThread = NEW TestThread (value);

	CurrentThread::sleep (50);
	if(value != 1)
	{
		CORE_TEST_FAILED ("Thread started before calling Thread::start ().")
		succeeded = false;
	}

	testThread->start ();

	for(int i = 0; i < 10 && value != 2; ++i)
		CurrentThread::sleep (500);
	if(value != 2)
	{
		CORE_TEST_FAILED ("Thread did not start after calling Thread::start ().")
		succeeded = false;
	}

	value = 1;
	for(int i = 0; i < 10 && value != 3; ++i)
		CurrentThread::sleep (500);
	if(value != 3)
	{
		CORE_TEST_FAILED ("CurrentThread::sleep does not seem to work.")
		succeeded = false;
	}

	if(testThread->join (500) == false)
	{
		CORE_TEST_FAILED ("Failed to join a thread.")
		succeeded = false;
	}
	
	delete testThread;

	return succeeded;
}

//************************************************************************************************
// LockTest
//************************************************************************************************

CORE_REGISTER_TEST (LockTest)

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr LockTest::getName () const
{
	return "Core Lock";
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LockTest::run (ITestContext& testContext)
{
	static bool succeeded;
	succeeded = true;

	static Lock lock;
	if(lock.tryLock () == false)
	{
		CORE_TEST_FAILED ("Failed to lock.")
		succeeded = false;
	}

	if(lock.tryLock () == false)
	{
		CORE_TEST_FAILED ("Failed to lock recursively.")
		succeeded = false;
	}

	lock.unlock ();
	lock.unlock ();

	static int value;
	value = 1;
	LockingThread* testThread = NEW LockingThread (value, lock);

	lock.lock ();
	testThread->start ();
	CurrentThread::sleep (100);

	if(value == 1)
	{
		CORE_TEST_FAILED ("Thread did not start after calling Thread::start ().")
		succeeded = false;
	}
	else if(value == 3)
	{
		CORE_TEST_FAILED ("A thread did not wait on a lock to be unlocked.")
		succeeded = false;
	}

	lock.unlock ();

	testThread->join (100);

	if(value != 3)
	{
		CORE_TEST_FAILED ("A thread did not continue after waiting for a lock.")
		succeeded = false;
	}

	value = 1;
	LockingThread* testThread2 = NEW LockingThread (value, lock);
	{
		ScopedLock scopedLock (lock);
		testThread2->start ();
		CurrentThread::sleep (100);

		if(value == 1)
		{
			CORE_TEST_FAILED ("Thread did not start after calling Thread::start ().")
			succeeded = false;
		}
		else if(value == 3)
		{
			CORE_TEST_FAILED ("A thread did not wait on a lock to be unlocked.")
			succeeded = false;
		}
	}

	testThread2->join (100);

	if(value != 3)
	{
		CORE_TEST_FAILED ("ScopedLock does not seem to work.")
		succeeded = false;
	}

	delete testThread;
	delete testThread2;

	return succeeded;
}

//************************************************************************************************
// CoreSignalTest
//************************************************************************************************

CORE_REGISTER_TEST (SignalTest)

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr SignalTest::getName () const
{
	return "Core Signal";
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SignalTest::run (ITestContext& testContext)
{
	bool succeeded = true;
	abs_time waitTime1 = 0;
	abs_time waitTime2 = 0;
	bool returnValue = false;
	Threads::Signal signal;
	SignalThread signalThread (signal);
	static const int kSignalTime = 10;

	signalThread.start ();
	CurrentThread::sleep (kSignalTime);

	signal.signal ();
	signalThread.join (100);

	if(!signalThread.isReturnValue1 ())
	{
		CORE_TEST_FAILED ("signal.wait () did not wake on signal and return true")
		succeeded = false;
	}

	if(signalThread.getWaitTime1 () < kSignalTime)
	{
		CORE_TEST_FAILED ("signal.wait () woke too early")
		succeeded = false;
	}

	if(signalThread.getWaitTime1 () > 2 * kSignalTime)
	{
		CORE_TEST_FAILED ("signal.wait () took too long to wake")
		succeeded = false;
	}

	if(signalThread.getWaitTime2 () < SignalThread::kTimeout)
	{
		CORE_TEST_FAILED ("signal.wait () thread woke too early when no signal called")
		succeeded = false;
	}
	
	if(signalThread.isReturnValue2 ())
	{
		CORE_TEST_FAILED ("signal.wait () did not return false on timeout when no signal called")
		succeeded = false;
	}
	return succeeded;
}
