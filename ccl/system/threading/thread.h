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
// Filename    : ccl/system/threading/thread.h
// Description : Multithreading
//
//************************************************************************************************

#ifndef _ccl_thread_h
#define _ccl_thread_h

#include "ccl/public/base/unknown.h"
#include "ccl/public/system/ithreading.h"
#include "ccl/public/collections/linkedlist.h"

#if CCL_PLATFORM_WINDOWS
#include "ccl/platform/win/cclwindows.h"
#endif

#include "core/system/corethread.h"

namespace CCL {
namespace Threading {

struct ThreadInfo;

//************************************************************************************************
// ThreadTimes
/** Thread timing information. */
//************************************************************************************************

struct ThreadTimes
{
	double userTime;

	ThreadTimes ()
	: userTime (0)
	{}
};

//************************************************************************************************
// NativeThread
/** Thread implementation. */
//************************************************************************************************

class NativeThread: public Unknown,
					public IThread,
					private Core::Threads::Thread
{
public:	
	NativeThread (ThreadFunction function, void* arg, CStringPtr name);
	NativeThread (ThreadID id, int flags = 0);
	~NativeThread ();

	static TLSRef selfSlot;
	static void initMainThread ();
	static void exitMainThread ();
	static void switchMainThread ();

	CStringPtr getName () const;

	enum Flags 
	{
		kRegistered = 1<<0,
		kForeignThread = 1<<1
	};

	PROPERTY_FLAG (flags, kRegistered, isRegistered)	
	PROPERTY_FLAG (flags, kForeignThread, isForeignThread)

	PROPERTY_VARIABLE (ThreadTimes, savedTimes, SavedTimes)
	bool getThreadTimes (ThreadTimes& times);
	int getNativePriority () const;
	
	// IThread
	ThreadID CCL_API getThreadID () const override;
	ThreadPriority CCL_API getPriority () const override;
	void CCL_API setCPUAffinity (int cpu) override;
	void CCL_API setPriority (ThreadPriority priority) override;
	void CCL_API start () override;
	void CCL_API terminate () override;
	tbool CCL_API join (uint32 milliseconds) override;
	ThreadErrors CCL_API getErrors () const override;

	CLASS_INTERFACE (IThread, Unknown)

protected:
	ThreadFunction function;
	void* arg;
	int flags;

	// Core::Threads::Thread
	int threadEntry () override;
};

//************************************************************************************************
// NativeThreadRegistrar
/** Thread registrar. */
//************************************************************************************************

class NativeThreadRegistrar
{
public:
	static void addThread (NativeThread* thread);
	static void removeThread (NativeThread* thread);
	static void addThread (ThreadID id);
	static void removeThread (ThreadID id);
	static void cleanup ();
	static NativeThread* openThread (ThreadID id);
	static int getSnapshot (ThreadInfo infos[], int max);

protected:
	static Core::Threads::Lock lock;
	static LinkedList<NativeThread*> threads;
	static double savedSnapshotTime;
};

} // namespace Threading
} // namespace CCL

#endif // _ccl_thread_h
