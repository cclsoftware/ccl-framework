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
// Filename    : core/platform/zephyr/corethread.zephyr.h
// Description : Zephyr Multithreading
//
//************************************************************************************************

#ifndef _corethread_zephyr_h
#define _corethread_zephyr_h

#include "core/platform/shared/coreplatformthread.h"

#include "corezephyr.h"

namespace Core {
namespace Platform {

//************************************************************************************************
// ZephyrThread
//************************************************************************************************

class ZephyrThread: public IThread
{
public:
	ZephyrThread ();
	~ZephyrThread ();

	static void entryWrapper (void* corePlatformThread, void* p2, void* p3);
	IThreadEntry* getEntry () const;
	void stopped ();

	// IThread
	bool open (Threads::ThreadID id) override;
	void start (const ThreadInfo& info) override;
	bool join (uint32 milliseconds) override;
	void terminate () override;
	int getPriority () const override;
	void setPriority (int priority) override;
	void setCPUAffinity (int affinity) override;
	int getPlatformPriority () const override;
	int64 getUserModeTime () const override;
	Threads::ThreadID getID () const override;
	int getErrors () const override {return 0;}

protected:
	k_thread nativeThread;
	k_thread_stack_t* stack;
	int stackSize;
	IThreadEntry* entry;
	int priority;
	bool running;
};

typedef ZephyrThread Thread;
const CStringPtr kThreadName = "Zephyr Thread"; 

//************************************************************************************************
// ZephyrLock
//************************************************************************************************

class ZephyrLock: public ILock
{
public:
	ZephyrLock ();
	~ZephyrLock ();

	// ILock
	void lock () override;
	bool tryLock () override;
	void unlock () override;

protected:
	k_mutex mutex;
};

typedef ZephyrLock Lock;

//************************************************************************************************
// ZephyrSignal
/** Not implemented */
//************************************************************************************************

class ZephyrSignal: public ISignal
{
public:
	ZephyrSignal (bool manualReset = false);
	~ZephyrSignal ();

	// ISignal
	void signal () override;
	void reset () override;
	bool wait (uint32 milliseconds) override;

protected:
	k_event event;
	bool manualReset;
	static const uint32 kTrackedEvent = 0x0001;
};

typedef ZephyrSignal Signal;

//************************************************************************************************
// ZephyrReadWriteLock
//************************************************************************************************

class ZephyrReadWriteLock: public IReadWriteLock
{
public:
	ZephyrReadWriteLock ();
	~ZephyrReadWriteLock ();

	// IReadWriteLock
	void lockWrite () override;
	void unlockWrite () override;
	void lockRead () override;
	void unlockRead () override;
	
protected:
	k_sem readSemaphore;
	k_sem writeSemaphore;
	k_sem activeReaderSemaphore;
	k_tid_t owner;
};

typedef ZephyrReadWriteLock ReadWriteLock;

} // namespace Platform
} // namespace Core

#endif // _coreplatformthread_zephyr_h
