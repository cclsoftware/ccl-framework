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
// Filename    : core/platform/shared/coreplatformthread.h
// Description : Multithreading platform implementation base
//
//************************************************************************************************

#ifndef _coreplatformthread_h
#define _coreplatformthread_h

#include "core/public/corethreading.h"
#include "core/platform/corefeatures.h" 

namespace Core {
namespace Platform {

//************************************************************************************************
// IThreadEntry
//************************************************************************************************

struct IThreadEntry
{
	virtual ~IThreadEntry () {}

    virtual int threadEntry () = 0;
};

//************************************************************************************************
// ThreadInfo
//************************************************************************************************

struct ThreadInfo
{
	ThreadInfo (CStringPtr name = nullptr, IThreadEntry* entry = nullptr)
	: name (name),
	  entry (entry)
	{}

    CStringPtr name;
    IThreadEntry* entry;
};

//************************************************************************************************
// Current thread function namespace
//************************************************************************************************

namespace CurrentThread
{
	Threads::ThreadID getID ();
	Threads::ThreadPriority setPriority (Threads::ThreadPriority newPrio);
    void sleep (uint32 milliseconds);
    void ussleep (uint32 microseconds);
    void yield ();  
}

//************************************************************************************************
// Thread Local Storage function namespace
//************************************************************************************************

namespace TLS
{
	Threads::TLSRef allocate ();
    void* getValue (Threads::TLSRef slot);
    bool setValue (Threads::TLSRef slot, void* value);
    bool release (Threads::TLSRef slot);
}

//************************************************************************************************
// ThreadPriorityHandler
//************************************************************************************************

struct ThreadPriorityHandler
{
	virtual bool setSelfToRealtimePriority (Threads::ThreadPriority priority) = 0;

	static ThreadPriorityHandler* customHandler;
};

//************************************************************************************************
// IThread
//************************************************************************************************

struct IThread
{
	virtual ~IThread () {}

	virtual bool open (Threads::ThreadID id) = 0;
	virtual void start (const ThreadInfo& info) = 0;
	virtual bool join (uint32 milliseconds) = 0;
	virtual void terminate () = 0;

	virtual int getPriority () const = 0;
	virtual void setPriority (int priority) = 0;
	virtual void setCPUAffinity (int affinity) = 0;
	virtual int getPlatformPriority () const = 0;
	virtual int64 getUserModeTime () const = 0;
	virtual Threads::ThreadID getID () const = 0;
	virtual int getErrors () const = 0;
};

//************************************************************************************************
// ILock
//************************************************************************************************

struct ILock
{
	virtual ~ILock () {}

	virtual void lock () = 0;
	virtual bool tryLock () = 0;
	virtual void unlock () = 0;
};

//************************************************************************************************
// ISignal
//************************************************************************************************

struct ISignal
{
	virtual ~ISignal () {}

	virtual void signal () = 0;
	virtual void reset () = 0;
	virtual bool wait (uint32 milliseconds) = 0;
};

//************************************************************************************************
// IReadWriteLock
//************************************************************************************************

struct IReadWriteLock
{
	virtual ~IReadWriteLock () {}

	virtual void lockWrite () = 0;
	virtual void unlockWrite () = 0;
	virtual void lockRead () = 0;
	virtual void unlockRead () = 0;
};

#if CORE_THREAD_IMPLEMENTATION == CORE_FEATURE_UNIMPLEMENTED

//************************************************************************************************
// Current thread function stub implementation
//************************************************************************************************

namespace CurrentThread
{
	inline Threads::ThreadID getID () { return 0; }
	inline Threads::ThreadPriority setPriority (Threads::ThreadPriority newPrio) { return 0; }
	inline void sleep (uint32 milliseconds) {}
	inline void ussleep (uint32 microseconds) {}
	inline void yield () {}
}

//************************************************************************************************
// Thread Local Storage function namespace
//************************************************************************************************

namespace TLS
{
	inline Threads::TLSRef allocate () { return 0; }
	inline void* getValue (Threads::TLSRef slot) { return nullptr; }
	inline bool setValue (Threads::TLSRef slot, void* value) { return false; }
	inline bool release (Threads::TLSRef slot) { return false; }
}

//************************************************************************************************
// ThreadStub
//************************************************************************************************

class ThreadStub: public IThread
{
public:
	// IThread
	bool open (Threads::ThreadID id) { return false; }
	void start (const ThreadInfo& info) {}
	bool join (uint32 milliseconds) { return false; }
	void terminate () {}
	int getPriority () const { return 0; }
	void setPriority (int priority) {}
	void setCPUAffinity (int cpu) {}
	int getPlatformPriority () const { return 0; }
	int64 getUserModeTime () const { return 0; }
	Threads::ThreadID getID () const { return 0; }
};

const CStringPtr kThreadName = "Stub";
typedef ThreadStub Thread;

//************************************************************************************************
// LockStub
//************************************************************************************************

class LockStub: public ILock
{
public:	
	// ILock
	void lock () {}
	bool tryLock () { return true; }
	void unlock () {}
};

typedef LockStub Lock;

//************************************************************************************************
// SignalStub
//************************************************************************************************

class SignalStub: public ISignal
{
public:
	SignalStub (bool) {}
	
	// ISignal
	void signal () {}
	void reset () {}
	bool wait (uint32 milliseconds) { return false; }
};

typedef SignalStub Signal;

//************************************************************************************************
// ReadWriteLockStub
//************************************************************************************************

class ReadWriteLockStub: public IReadWriteLock
{
public:
	// IReadWriteLock
	void lockWrite () {}
	void unlockWrite () {}
	void lockRead () {}
	void unlockRead () {}
};

typedef ReadWriteLockStub ReadWriteLock;

#endif // CORE_THREAD_IMPLEMENTATION == CORE_FEATURE_UNIMPLEMENTED

} // namespace Platform
} // namespace Core

#endif // _coreplatformthread_h
