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
// Filename    : threadtest.cpp
// Description : Unit tests for Multithreading
//
//************************************************************************************************

#include "ccl/base/unittest.h"

#include "ccl/public/system/logging.h"
#include "ccl/public/system/threadsync.h"
#include "ccl/public/system/threadlocal.h"
#include "ccl/public/system/ithreadpool.h"

#include "ccl/public/text/cstring.h"

using namespace CCL;
using namespace Threading;

//************************************************************************************************
// PerThreadObject
//************************************************************************************************

class PerThreadObject: public ThreadSingleton<PerThreadObject>
{
public:
	PerThreadObject ()
	: value (0)
	{
		// prepare string in advance, otherwise debug output
		// gets messed up by multiple threads!
		MutableCString s ("*** PerThreadObject Ctor");
		IThread* thread = System::CreateThreadSelf ();
		s.appendFormat (" [Thread %p] ", thread->getThreadID ());
		thread->release ();
		s += "***\n";
		Logging::debug (String (s));
	}
	
	~PerThreadObject ()
	{
		MutableCString s ("~~~ PerThreadObject Dtor");

		IThread* thread = System::CreateThreadSelf ();
		s.appendFormat (" [Thread %p] ", thread->getThreadID ());
		thread->release ();

		s += "~~~\n";
		Logging::debug (String (s));
	}

	PROPERTY_VARIABLE (int, value, Value)
};

DEFINE_THREAD_SINGLETON (PerThreadObject)

//************************************************************************************************
// TestTLSWork
//************************************************************************************************

class TestTLSWork: public Unknown,
				   public AbstractWorkItem
{
public:
	static AtomicInt workCount;

	void CCL_API work () override
	{
		IThread* thread = System::CreateThreadSelf ();
		int value = (int)thread->getThreadID ();
		thread->release ();

		PerThreadObject::instance ().setValue (value);
		++workCount;
	}

	CLASS_INTERFACE (IWorkItem, Unknown)
};

AtomicInt TestTLSWork::workCount;

//************************************************************************************************
// ThreadTest
//************************************************************************************************

CCL_TEST (ThreadTest, TestThreadLocalStorage)
{
	PerThreadObject::instance ().setValue (5);

	static const int kNumThreads = 5;

	TestTLSWork::workCount = 0;
	IThreadPool& threadPool = System::GetThreadPool ();
	for(int i = 0; i < kNumThreads; i++)
	{
		IWorkItem* work = NEW TestTLSWork;
		threadPool.scheduleWork (work);
	}

	while(TestTLSWork::workCount < kNumThreads)
		System::ThreadSleep (100);

	threadPool.reduceThreads (true);
}
