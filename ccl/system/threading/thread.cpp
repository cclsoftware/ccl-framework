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
// Filename    : ccl/system/threading/thread.cpp
// Description : Multithreading
//
//************************************************************************************************

#define THREAD_REGISTRAR_ENABLED 1

#include "ccl/system/threading/thread.h"
#include "ccl/system/threading/threadlocalstorage.h"

#include "ccl/public/systemservices.h"
#include "ccl/public/system/threadlocal.h"
#include "ccl/public/system/ilockable.h"
#include "ccl/public/system/imediathreading.h" // for ThreadInfo
#include "ccl/public/system/floatcontrol.h"

using namespace CCL;
using namespace Threading;

#if (CCL_PLATFORM_WINDOWS && RELEASE)
#define TRY_THREAD		__try
#define EXCEPT_THREAD	__except(EXCEPTION_EXECUTE_HANDLER)
#else
#define TRY_THREAD
#define EXCEPT_THREAD
#endif

static NativeThread* GetMainNativeThread ();

//////////////////////////////////////////////////////////////////////////////////////////////////
// System Threading APIs
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT void CCL_API System::CCL_ISOLATED (ThreadSleep) (uint32 milliseconds)
{
	Core::Threads::CurrentThread::sleep (milliseconds);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT ThreadID CCL_API System::CCL_ISOLATED (GetThreadSelfID) ()
{
	return Core::Threads::CurrentThread::getID ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT IThread* CCL_API System::CCL_ISOLATED (CreateNativeThread) (const ThreadDescription& description)
{
	ASSERT (description.function)
	if(!description.function)
		return nullptr;
	return NEW NativeThread (description.function, description.arg, description.name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT IThread* CCL_API System::CCL_ISOLATED (CreateThreadSelf) ()
{
	NativeThread* thread = (NativeThread*)NativeTLS::getValue (NativeThread::selfSlot);
	if(thread != nullptr)
	{
		thread->retain ();
		return thread;
	}

	return NEW NativeThread (System::GetThreadSelfID ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT IThread& CCL_API System::CCL_ISOLATED (GetMainThread) ()
{
	return *GetMainNativeThread ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT void CCL_API System::CCL_ISOLATED (SwitchMainThread) ()
{
	NativeThread::switchMainThread ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT IThread* CCL_API System::CCL_ISOLATED (CreateThreadWithIdentifier) (ThreadID id)
{
	NativeThread* thread = NativeThreadRegistrar::openThread (id);
	if(thread == nullptr)
		thread = NEW NativeThread (id);
	return thread;
}

//************************************************************************************************
// NativeThread
//************************************************************************************************

TLSRef NativeThread::selfSlot;
static NativeThread* mainThread = nullptr;

//////////////////////////////////////////////////////////////////////////////////////////////////

static void ReleaseNativeMainThread ()
{
	safe_release (mainThread);
};

//////////////////////////////////////////////////////////////////////////////////////////////////

static NativeThread* GetMainNativeThread ()
{
	if(mainThread == nullptr)
	{
		mainThread = NEW NativeThread (System::GetThreadSelfID ());
		::atexit (ReleaseNativeMainThread);
	}
	return mainThread;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NativeThread::initMainThread ()
{
	selfSlot = NativeTLS::allocate ();
	NativeTLS::setValue (selfSlot, GetMainNativeThread ());
	NativeThreadRegistrar::addThread (GetMainNativeThread ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NativeThread::exitMainThread ()
{
	NativeThreadRegistrar::removeThread (GetMainNativeThread ());
	NativeThreadRegistrar::cleanup ();
	TLS::cleanupOnThreadExit ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NativeThread::switchMainThread ()
{
	if(mainThread)
	{
		if(mainThread->getThreadID () != System::GetThreadSelfID ())
		{
			exitMainThread ();
			safe_release (mainThread);
			initMainThread ();
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeThread::NativeThread (ThreadFunction function, void* arg, CStringPtr name)
: Thread (name),
  function (function),
  arg (arg),
  flags (0)
{
	NativeThreadRegistrar::addThread (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeThread::NativeThread (ThreadID id, int flags)
: Thread (id),
  function (nullptr),
  arg (nullptr),
  flags (flags)
{
	// do not register thread here!!!
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeThread::~NativeThread ()
{
	NativeThreadRegistrar::removeThread (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr NativeThread::getName () const
{
	return Thread::name; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int NativeThread::threadEntry ()
{
	NativeTLS::setValue (selfSlot, this);

	setFloatEnv ();

	int result = -1;
	TRY_THREAD
	{
		result = function (arg);
	}
	EXCEPT_THREAD
	{
		// TODO: pass to framework exception handler?
	}

	// cleanup TLS
	TLS::cleanupOnThreadExit ();

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ThreadID CCL_API NativeThread::getThreadID () const
{
	return (ThreadID)Thread::getID ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ThreadPriority CCL_API NativeThread::getPriority () const
{
	return Thread::getPriority ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API NativeThread::setCPUAffinity (int cpu)
{
	setFloatEnv ();

	Thread::setCPUAffinity (cpu);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API NativeThread::setPriority (ThreadPriority priority)
{
	Thread::setPriority (priority);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool NativeThread::getThreadTimes (ThreadTimes& times)
{
	int64 userTime = Thread::getUserModeTime ();
	times.userTime = (double)userTime / 10000000.;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int NativeThread::getNativePriority () const
{
	return Thread::getNativePriority ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API NativeThread::start ()
{
	Thread::start ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API NativeThread::terminate ()
{
	Thread::terminate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API NativeThread::join (uint32 milliseconds)
{
	return Thread::join (milliseconds);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ThreadErrors CCL_API NativeThread::getErrors () const
{
	return Thread::getErrors ();
}

//************************************************************************************************
// NativeThreadRegistrar
//************************************************************************************************

Core::Threads::Lock NativeThreadRegistrar::lock;
LinkedList<NativeThread*> NativeThreadRegistrar::threads;
double NativeThreadRegistrar::savedSnapshotTime = 0;

//////////////////////////////////////////////////////////////////////////////////////////////////

void NativeThreadRegistrar::cleanup ()
{
#if THREAD_REGISTRAR_ENABLED
	// add/remove of foreign threads might not be balanced, we have to clean them up manually
	lock.lock ();
	ListForEach (threads, NativeThread*, thread)
		SOFT_ASSERT (thread->isForeignThread (), "Native thread not properly removed")
		if(thread->isForeignThread ())
		{
			threads.remove (thread);
			thread->isRegistered (false);
			unsigned int refCount = thread->release ();
			ASSERT (refCount == 0)
		}
	EndFor
	lock.unlock ();
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NativeThreadRegistrar::addThread (NativeThread* thread)
{
#if THREAD_REGISTRAR_ENABLED
	ASSERT (!thread->isRegistered ())
	if(thread->isRegistered ())
		return;
	lock.lock ();
	threads.append (thread);
	lock.unlock ();
	thread->isRegistered (true);
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NativeThreadRegistrar::removeThread (NativeThread* thread)
{
#if THREAD_REGISTRAR_ENABLED
	if(!thread->isRegistered ())
		return;
	lock.lock ();
	threads.remove (thread);
	lock.unlock ();
	thread->isRegistered (false); // avoid double calls (dtor order of static mainThread object!) 
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NativeThreadRegistrar::addThread (ThreadID id)
{
#if THREAD_REGISTRAR_ENABLED
	lock.lock ();

	bool found = false;
	ListForEach (threads, NativeThread*, thread)
		if(thread->getThreadID () == id)
		{
			found = true;
			break;
		}
	EndFor
	
	if(found == false)
	{
		NativeThread* thread = NEW NativeThread (id, NativeThread::kForeignThread);
		thread->isRegistered (true);
		threads.append (thread);
	}
	lock.unlock ();
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NativeThreadRegistrar::removeThread (ThreadID id)
{
#if THREAD_REGISTRAR_ENABLED
	lock.lock ();

	ListForEach (threads, NativeThread*, thread)
		if(thread->getThreadID () == id)
		{
			if(thread->isForeignThread ())
			{
				// cleanup TLS for foreign threads here
				if(System::GetThreadSelfID () == id)
					TLS::cleanupOnThreadExit ();

				threads.remove (thread);
				thread->isRegistered (false);
				unsigned int refCount = thread->release ();
				ASSERT (refCount == 0)
			}
			break;
		}
	EndFor

	lock.unlock ();
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeThread* NativeThreadRegistrar::openThread (ThreadID id)
{
#if THREAD_REGISTRAR_ENABLED
	NativeThread* result = nullptr;
	lock.lock ();
	ListForEach (threads, NativeThread*, thread)
		if(thread->getThreadID () == id)
		{
			result = return_shared (thread);
			break;
		}
	EndFor
	lock.unlock ();
	return result;
#else
	return 0;
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int NativeThreadRegistrar::getSnapshot (ThreadInfo infos[], int max)
{
#if THREAD_REGISTRAR_ENABLED
	lock.lock ();

	double now = System::GetProfileTime ();
	double timeDelta = now - savedSnapshotTime;
	savedSnapshotTime = now;

	int count = 0;
	ListForEach (threads, NativeThread*, thread)
		if(count >= max)
			break;

		ThreadInfo& dst = infos[count];
		dst.id = thread->getThreadID ();
		Core::ConstString (thread->getName ()).copyTo (dst.name, sizeof(dst.name));
		dst.priority = thread->getPriority ();
		dst.nativePriority = thread->getNativePriority ();

		ThreadTimes tt;
		thread->getThreadTimes (tt);

		const ThreadTimes& savedTimes = thread->getSavedTimes ();
		double userDelta = tt.userTime - savedTimes.userTime;
		thread->setSavedTimes (tt);
		
		dst.activity = ccl_bound<float> ((float)userDelta / (float)timeDelta, 0, 1);

		count++;
	EndFor

	lock.unlock ();
	return count;
#else
	return 0;
#endif
}
