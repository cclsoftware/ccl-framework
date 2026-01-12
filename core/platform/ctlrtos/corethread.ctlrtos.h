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
// Filename    : core/platform/ctlrtos/corethread.ctl.h
// Description : Crossworks Tasking Library Multithreading
//
//************************************************************************************************

#ifndef _corethread_ctl_h
#define _corethread_ctl_h

#include "core/platform/shared/coreplatformthread.h"

#include <ctl_api.h>

namespace Core {
namespace Platform {

//************************************************************************************************
// CtlThread
//************************************************************************************************

class CtlThread: public IThread
{
public:
	CtlThread ();
	~CtlThread ();

	bool setStackSize (int size);
	void setThreadState (int state);

	IThreadEntry* getThreadEntry ();

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
	CTL_TASK_t threadInfo; 
	int stackSize;
	unsigned* threadStack;
	int ctlPriority;
	int threadState;
	IThreadEntry* entry;
	int priority;
	CStringPtr name;
};

const CStringPtr kThreadName = "CTL Thread";
typedef CtlThread Thread;

//************************************************************************************************
// CtlLock
//************************************************************************************************

class CtlLock: public ILock
{
public:
	CtlLock ();
	~CtlLock ();

	// ILock
	void lock () override;
	bool tryLock () override;
	void unlock () override;

protected:
	CTL_MUTEX_t mutexId;
};

typedef CtlLock Lock;

//************************************************************************************************
// CtlSignal
/** Not implemented */
//************************************************************************************************

class CtlSignal: public ISignal
{
public:
	CtlSignal (bool manualReset = false) {};
	~CtlSignal () {};

	// ISignal
	void signal () override {};
	void reset () override {};
	bool wait (uint32 milliseconds) override { return false; };
};

typedef CtlSignal Signal;

//************************************************************************************************
// CtlReadWriteLock
/** Not implemented */
//************************************************************************************************

class CtlReadWriteLock: public IReadWriteLock
{
public:
	CtlReadWriteLock () {};
	~CtlReadWriteLock () {};

	// IReadWriteLock
	void lockWrite () override {};
	void unlockWrite () override {};
	void lockRead () override {};
	void unlockRead () override {};
};

typedef CtlReadWriteLock ReadWriteLock;

} // namespace Platform
} // namespace Core

#endif // _coreplatformthread_ctl_h
