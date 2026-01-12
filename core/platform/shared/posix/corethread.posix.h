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
// Filename    : core/platform/shared/posix/corethread.posix.h
// Description : POSIX Multithreading
//
//************************************************************************************************

#ifndef _corethread_posix_h
#define _corethread_posix_h

#include "core/platform/shared/coreplatformthread.h"
#include "core/platform/corefeatures.h"

#include <pthread.h>

namespace Core {
namespace Platform {

//************************************************************************************************
// PosixThread
//************************************************************************************************

class PosixThread: public IThread
{
public:
	PosixThread ();
	~PosixThread ();

	IThreadEntry* getThreadEntry ();
	CStringPtr getName () const { return name; }
	
	// IThread
	bool open (Threads::ThreadID id) override;
	void start (const ThreadInfo& info) override;
	bool join (uint32 milliseconds) override;
	void terminate () override;
	void setPriority (int priority) override;
	int getPriority () const override;
	void setCPUAffinity (int affinity) override;
	int getPlatformPriority () const override;
	int64 getUserModeTime () const override;
	Threads::ThreadID getID () const override;
	int getErrors () const override {return 0;}

protected:
	pthread_t threadId;
	IThreadEntry* entry;
	int priority;
	
	CStringPtr name;
};

#if CORE_THREAD_IMPLEMENTATION == CORE_FEATURE_IMPLEMENTATION_POSIX
const CStringPtr kThreadName = "POSIX Thread";
typedef PosixThread Thread;
#endif

//************************************************************************************************
// PosixLock
//************************************************************************************************

class PosixLock: public ILock
{
public:
	PosixLock ();
	~PosixLock ();

	// ILock
	void lock () override;
	bool tryLock () override;
	void unlock () override;

protected:
	pthread_mutex_t mutexId;
};

#if CORE_THREAD_IMPLEMENTATION == CORE_FEATURE_IMPLEMENTATION_POSIX
typedef PosixLock Lock;
#endif

//************************************************************************************************
// PosixSignal
//************************************************************************************************

class PosixSignal: public ISignal
{
public:
	PosixSignal (bool manualReset = false);
	~PosixSignal ();

	// ISignal
	void signal () override;
	void reset () override;
	bool wait (uint32 milliseconds) override;

protected:
	pthread_mutex_t mutexId;
	pthread_cond_t conditionId;
	bool manualReset;
	int signaled;
};

#if CORE_THREAD_IMPLEMENTATION == CORE_FEATURE_IMPLEMENTATION_POSIX
typedef PosixSignal Signal;
#endif

//************************************************************************************************
// PosixReadWriteLock
//************************************************************************************************

class PosixReadWriteLock: public IReadWriteLock
{
public:
	PosixReadWriteLock ();
	~PosixReadWriteLock ();

	// IReadWriteLock
	void lockWrite () override;
	void unlockWrite () override;
	void lockRead () override;
	void unlockRead () override;

protected:
	pthread_rwlock_t rwlockId;
};

#if CORE_THREAD_IMPLEMENTATION == CORE_FEATURE_IMPLEMENTATION_POSIX
typedef PosixReadWriteLock ReadWriteLock;
#endif

} // namespace Platform
} // namespace Core

#endif // _corethread_posix_h
