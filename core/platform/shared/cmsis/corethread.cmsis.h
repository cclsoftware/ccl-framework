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
// Filename    : core/platform/shared/cmsis/corethread.cmsis.h
// Description : CMSIS Multithreading
//
//************************************************************************************************

#ifndef _corethread_cmsis_h
#define _corethread_cmsis_h

#include "core/platform/shared/coreplatformthread.h"
#include "core/platform/corefeatures.h"

#include "core/system/coretime.h"

#include <cmsis_os2.h>
#include <rtx_os.h>

namespace Core {
namespace Platform {

//************************************************************************************************
// CmsisThread
//************************************************************************************************

class CmsisThread: public IThread
{
public:
	CmsisThread ();
	~CmsisThread ();

	bool setStackSize (int size);
	IThreadEntry* getThreadEntry ();

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
	static const int kDefaultStackSize = 2048;

	static osPriority_t toNativePriority (int priority);
	static int fromNativePriority (osPriority_t priority);
	
	osThreadId_t threadId;
	osRtxThread_t threadData;
	IThreadEntry* entry;
	CStringPtr name;
	int stackSize;
	void* threadStack;
};

#if CORE_THREAD_IMPLEMENTATION == CORE_FEATURE_IMPLEMENTATION_CMSIS
const CStringPtr kThreadName = "CMSIS Thread";
typedef CmsisThread Thread;
#endif

//************************************************************************************************
// CmsisLock
//************************************************************************************************

class CmsisLock: public ILock
{
public:
	CmsisLock ();
	CmsisLock (const osMutexAttr_t& attributes);
	~CmsisLock ();

	// ILock
	void lock () override;
	bool tryLock () override;
	void unlock () override;

protected:
	osMutexId_t mutexId;
	osRtxMutex_t mutexData;
};

#if CORE_THREAD_IMPLEMENTATION == CORE_FEATURE_IMPLEMENTATION_CMSIS
typedef CmsisLock Lock;
#endif

//************************************************************************************************
// CmsisSignal
//************************************************************************************************

class CmsisSignal: public ISignal
{
public:
	CmsisSignal (bool manualReset = false);
	~CmsisSignal ();

	// ISignal
	void signal () override;
	void reset () override;
	bool wait (uint32 milliseconds) override;

protected:
};

#if CORE_THREAD_IMPLEMENTATION == CORE_FEATURE_IMPLEMENTATION_CMSIS
typedef CmsisSignal Signal;
#endif

//************************************************************************************************
// CmsisReadWriteLock
//************************************************************************************************

class CmsisReadWriteLock: public IReadWriteLock
{
public:
	CmsisReadWriteLock ();
	~CmsisReadWriteLock ();

	// IReadWriteLock
	void lockWrite () override;
	void unlockWrite () override;
	void lockRead () override;
	void unlockRead () override;

protected:
};

#if CORE_THREAD_IMPLEMENTATION == CORE_FEATURE_IMPLEMENTATION_CMSIS
typedef CmsisReadWriteLock ReadWriteLock;
#endif

//************************************************************************************************
// CmsisLock implementation
//************************************************************************************************

INLINE void CmsisLock::lock ()
{
	osMutexAcquire (mutexId, osWaitForever);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

INLINE bool CmsisLock::tryLock ()
{
	return osMutexAcquire (mutexId, 0) == osOK;
}

} // namespace Platform
} // namespace Core

#endif // _corethread_cmsis_h
