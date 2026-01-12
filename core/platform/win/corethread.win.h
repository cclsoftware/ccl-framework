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
// Filename    : core/platform/win/corethread.win.h
// Description : Windows Multithreading
//
//************************************************************************************************

#ifndef _corethread_win_h
#define _corethread_win_h

#include "core/platform/shared/coreplatformthread.h"
#include "core/platform/shared/corerecursivereadwritelock.h"

#include <windows.h>

namespace Core {
namespace Platform {

//************************************************************************************************
// Win32Thread
//************************************************************************************************

class Win32Thread: public IThread
{
public:
	Win32Thread ();
	~Win32Thread ();

	/** Enable/disable higher resolution thread scheduling. */
	static bool enableHighResolutionScheduling (bool state);

	bool setSelfToRealTimePriority ();
	void applyIdealProcessor (int cpuId);
	void setSelfThreadName ();

	IThreadEntry* getThreadEntry ();

	// IThread
	bool open (Threads::ThreadID id) override;
	void start (const ThreadInfo& info) override;
	bool join (uint32 milliseconds) override;
	void terminate () override;
	int getPriority () const override;
	void setPriority (int priority) override;
	void setCPUAffinity (int cpu) override;
	int getPlatformPriority () const override;
	int64 getUserModeTime () const override;
	Threads::ThreadID getID () const override;
	int getErrors () const override;

protected:
	HANDLE handle;
	DWORD threadId;
	IThreadEntry* entry;
	int priority;
	int errors;
	
	CStringPtr name;
	int cpu;
};

const CStringPtr kThreadName = "Win32 Thread";
typedef Win32Thread Thread;

//************************************************************************************************
// Win32Lock
//************************************************************************************************

class Win32Lock: public ILock
{
public:
	Win32Lock ();
	~Win32Lock ();

	// ILock
	void lock () override;
	bool tryLock () override;
	void unlock () override;

protected:
	CRITICAL_SECTION data;
};

typedef Win32Lock Lock;

//************************************************************************************************
// Win32Signal
//************************************************************************************************

class Win32Signal: public ISignal
{
public:
	Win32Signal (bool manualReset = false);
	~Win32Signal ();

	// ISignal
	void signal () override;
	void reset () override;
	bool wait (uint32 milliseconds) override;

protected:
	HANDLE handle;
};

typedef Win32Signal Signal;

//************************************************************************************************
// Win32ReadWriteLock
//************************************************************************************************

class Win32ReadWriteLock: public IReadWriteLock
{
public:
	Win32ReadWriteLock ();

	// IReadWriteLock
	void lockWrite () override;
	void unlockWrite () override;
	void lockRead () override;
	void unlockRead () override;

protected:
	SRWLOCK rwLock;
};

typedef RecursiveReadWriteLock<Win32ReadWriteLock, Lock> Win32RecursiveReadWriteLock;
typedef Win32RecursiveReadWriteLock ReadWriteLock;

} // namespace Platform
} // namespace Core

#endif // _corethread_win_h
