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
// Filename    : core/platform/lk/corethread.lk.h
// Description : Little Kernel Thread Primitives
//
//************************************************************************************************

#ifndef _corethread_lk_h
#define _corethread_lk_h

#include "core/platform/shared/coreplatformthread.h"

// Little Kernel
#include "kernel/event.h"
#include "kernel/mutex.h"
#include "kernel/semaphore.h"
#include "kernel/thread.h"

namespace Core {
namespace Platform {

//************************************************************************************************
// LKThread
//************************************************************************************************

class LKThread: public IThread
{
public:
	LKThread ();
	~LKThread ();

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

	static int entryWrapper (void *entry);

protected:
	thread_t* lkThread;
	IThreadEntry* entry;
	int priority;
	int cpu;
};

typedef LKThread Thread;

const CStringPtr kThreadName = "LKThread"; 

//************************************************************************************************
// LKLock
//************************************************************************************************

class LKLock: public ILock
{
public:
	LKLock ();
	~LKLock ();

	// ILock
	void lock () override;
	bool tryLock () override;
	void unlock () override;

protected:
	mutex_t lkMutex;
	int count;
};

typedef LKLock Lock;

//************************************************************************************************
// LKSignal
//************************************************************************************************

class LKSignal: public ISignal
{
public:
	LKSignal (bool manualReset);
	~LKSignal ();

	// ISignal
	void signal () override;
	void reset () override;
	bool wait (uint32 milliseconds) override;

protected:
	event_t lkEvent;
};

typedef LKSignal Signal;

//************************************************************************************************
// LKReadWriteLock
//************************************************************************************************

class LKReadWriteLock: public IReadWriteLock
{
public:
	LKReadWriteLock ();
	~LKReadWriteLock ();

	void lockWrite () override;
	void unlockWrite () override;
	void lockRead () override;
	void unlockRead () override;

protected:
	thread_t* owner;
	semaphore_t activeReaderSemaphore;
	semaphore_t writeSemaphore;
	semaphore_t readSemaphore;
};

typedef LKReadWriteLock ReadWriteLock;

} // namespace Platform
} // namespace Core

#endif // _corethread_lk_h
