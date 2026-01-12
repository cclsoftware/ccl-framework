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
// Filename    : core/system/corethread.h
// Description : Thread class
//
//************************************************************************************************

#ifndef _corethread_h
#define _corethread_h

#include "core/platform/corefeatures.h"

#if CORE_THREAD_IMPLEMENTATION == CORE_PLATFORM_IMPLEMENTATION
	#include CORE_PLATFORM_IMPLEMENTATION_HEADER (corethread)
#elif CORE_THREAD_IMPLEMENTATION == CORE_EXTERNAL_PLATFORM_IMPLEMENTATION
	#include CORE_EXTERNAL_PLATFORM_IMPLEMENTATION_HEADER (corethread)
#elif CORE_THREAD_IMPLEMENTATION == CORE_FEATURE_IMPLEMENTATION_POSIX
	#include "core/platform/shared/posix/corethread.posix.h"
#elif CORE_THREAD_IMPLEMENTATION == CORE_FEATURE_IMPLEMENTATION_CMSIS
	#include "core/platform/shared/cmsis/corethread.cmsis.h"
#else
	#include "core/platform/shared/coreplatformthread.h"
#endif

#include "core/public/corestringbuffer.h"

namespace Core {
namespace Threads {

//************************************************************************************************
// CurrentThread
/** Thread Functions
	 \ingroup core_thread */
//************************************************************************************************

namespace CurrentThread
{
	/**
	* \brief Get identifier of current thread
	* \ingroup core_thread
	*/
	ThreadID getID ();

	/**
	* \brief Set priority of current thread
	* \ingroup core_thread
	* \param newPrio New priority of the current thread
	* \return Resulting priority (might not match \a newPrio)
	*/
	ThreadPriority setPriority (ThreadPriority newPrio);

	/**
	* \brief Suspend execution of current thread for given time.
	* \details OS usually does a context switch to another thread.
	* \ingroup core_thread
	* \param milliseconds Time to sleep in milliseconds
	*/
	void sleep (uint32 milliseconds);

	/**
	* \brief Suspend execution of current thread for given time.
	* \details OS usually does a context switch to another thread.
	* \ingroup core_thread
	* \param milliseconds Time to sleep in microseconds
	*/
	void ussleep (uint32 microseconds);

	/**
	* \brief Give up current scheduling period.
	* \details I.e. sleep(0) on PC, pthread_yield() on Linux.
	* \ingroup core_thread
	*/
	void yield ();
}

//************************************************************************************************
// TLS
/** Thread local storage
	 \ingroup core_thread */
//************************************************************************************************

namespace TLS
{
	/**
	* \brief Allocates a thread local storage (TLS) key.
	* \ingroup core_thread
	*/
	TLSRef allocate ();

	/**
	* \brief Retrieves the value associated with a thread local storage (TLS) key.
	* \ingroup core_thread
	* \param slot A TLS key, which was previously allocated by calling TLS::allocate.
	*/
	void* getValue (TLSRef slot);

	/**
	* \brief Sets the value associated with a thread local storage (TLS) key.
	* \ingroup core_thread
	* \param slot A TLS key, which was previously allocated by calling TLS::allocate.
	*/
	bool setValue (TLSRef slot, void* value);

	/**
	* \brief Releases a thread local storage (TLS) key.
	* \ingroup core_thread
	* \param slot A TLS key, which was previously allocated by calling TLS::allocate.
	*/
	bool release (TLSRef slot);
}

//************************************************************************************************
// Thread
/** Thread class. 
	\ingroup core_thread */
//************************************************************************************************

class Thread: public Platform::IThreadEntry
{
public:

	/**
	* \brief Thread constructor
	* \details The thread does not start executing
	* \param name Name of the thread
	*/
	Thread (CStringPtr name = Platform::kThreadName);

	/**
	* \brief Thread constructor with thread ID
	* \details Opens an existing thread with a given ID.
	* \param threadId ID of an existing thread
	*/
	Thread (ThreadID threadId);

	/**
	* \brief Start executing
	*/
	void start ();

	/**
	* \brief Terminate
	* \details Might not be implemented for all platforms.
	* \deprecated This function might be dangerous.
	* \attention The thread might not exit cleanly and could leave the process in an inconsistent state.
	*/
	void terminate ();

	/**
	* \brief Wait for the thread to finish
	* \details Depending on the platform implementation, the thread might be terminated after waiting for \a milliseconds.
	* \attention Calling this function might block the execution for an unknown amount of time if the platform implementation ignores the parameter.
	* \attention Calling this function might leave the process in an inconsistent state, see \a Thread::terminate.
	* \param milliseconds Time in milliseconds to wait before terminating the thread.
	* \return true if the thread finished execution, false if the thread was terminated after waiting for \a milliseconds or joining failed.
	*/
	bool join (uint32 milliseconds);

	/**
	* \brief Sets the priority of the thread
	* \details Might not be implemented for all platforms.\n
	* Call this before calling \a Thread::start
	* \param name The priority of the thread
	* \see Core::Threads::ThreadPriority
	*/
	void setPriority (int priority);

	/**
	* \brief Sets the CPU affinity of the thread
	* \details Might not be implemented for all platforms.\n
	* Call this before calling \a Thread::start.
	* \param name The CPU affinity of the thread
	*/
	void setCPUAffinity (int cpu);

	/**
	* \brief Get the current priority of the thread
	* \see Core::Threads::ThreadPriority
	* \return The current priority
	*/
	int getPriority () const;

	/**
	* \brief Get the ID of the thread
	* \return The thread ID
	*/
	ThreadID getID () const;

	/**
	* \brief Get the name of the thread
	* \return The thread name
	*/
	CStringPtr getName () const;

	/**
	* \brief Get the current priority of the thread represented in a platform-specific way
	* \return The current priority
	*/
	int getNativePriority () const;

	/**
	* \brief Get the time this thread has executed in used mode
	* \attention The exact time representation might be platform-specific
	* \return The time this thread has executed in user mode
	*/
	int64 getUserModeTime () const;

	/**
	* \brief Report if anything went wrong
	* \return A combination of ThreadErrors, or 0 if none
	*/
	int getErrors () const;

	/**
	* \brief Get the platform-specific thread implementation object
	* \details This object might reveal additional methods and attributes.
	* \return The platform-specific thread implementation object
	*/
	Platform::Thread& getPlatformThread ();

	// IThreadEntry
	virtual int threadEntry () override = 0;

protected:
	CString64 name;

	/**
	 * Platform::Thread will be substituted via typedef in platform code
	 */
	Platform::Thread platformThread;
};

//************************************************************************************************
// ThreadPriorityHandler
/** Thread priority handler interface.
	\ingroup core_thread */
//************************************************************************************************

struct ThreadPriorityHandler: Platform::ThreadPriorityHandler
{
	virtual bool setSelfToRealtimePriority (ThreadPriority priority) override = 0;

	void setCustomHandler (ThreadPriorityHandler* handler);
};

//************************************************************************************************
// Lock
/** User-mode lock. 
	\ingroup core_thread */
//************************************************************************************************

class Lock
{
public:
	void lock ();
	bool tryLock ();
	void unlock ();

protected:
	Platform::Lock platformLock;
};

//************************************************************************************************
// ScopedLock
/** Helper to lock/unlock within scope.
	\ingroup core_thread */
//************************************************************************************************

template<class TLock>
struct TScopedLock
{
	TScopedLock (TLock& _lock);
	TScopedLock (TLock* lock);
	~TScopedLock ();	
	
	TLock* lock;
};
	
typedef TScopedLock<Lock> ScopedLock;

//************************************************************************************************
// PriorityScope
/** Helper to set/reset priority within scope.
	\ingroup core_thread */
//************************************************************************************************

struct PriorityScope
{
	PriorityScope (ThreadPriority priority, bool apply = true);
	~PriorityScope ();

	ThreadPriority oldPriority;
	bool apply;
};

//************************************************************************************************
// Signal
/** Synchronization object with explicit signal/reset. Rarely used. 
	\ingroup core_thread */
//************************************************************************************************

class Signal
{
public:
	Signal (bool manualReset = false);

	void signal ();
	void reset ();
	bool wait (uint32 milliseconds);

protected:
	Platform::Signal platformSignal;
};

//************************************************************************************************
// ReadWriteLock
/** User-mode read/write lock, can be more efficient than a simple lock, because multiple readers 
	don't block each other. 
	\ingroup core_thread */
//************************************************************************************************

class ReadWriteLock
{
public:
	void lockWrite ();
	void unlockWrite ();
	void lockRead ();
	void unlockRead ();

protected:
	Platform::ReadWriteLock platformLock;
};

//************************************************************************************************
// CurrentThread implementation
//************************************************************************************************

inline ThreadID CurrentThread::getID ()
{ return Platform::CurrentThread::getID (); }

inline ThreadPriority CurrentThread::setPriority (ThreadPriority newPrio)
{ return Platform::CurrentThread::setPriority (newPrio); }

inline void CurrentThread::sleep (uint32 milliseconds)
{ return Platform::CurrentThread::sleep (milliseconds); }

inline void CurrentThread::ussleep (uint32 microseconds)
{ return Platform::CurrentThread::ussleep (microseconds); }

inline void CurrentThread::yield ()
{ return Platform::CurrentThread::yield (); }

//************************************************************************************************
// TLS implementation
//************************************************************************************************

inline TLSRef TLS::allocate ()
{ return Platform::TLS::allocate (); }

inline void* TLS::getValue (TLSRef slot)
{ return Platform::TLS::getValue (slot); }

inline bool TLS::setValue (TLSRef slot, void* value)
{ return Platform::TLS::setValue (slot, value); }

inline bool TLS::release (TLSRef slot)
{ return Platform::TLS::release (slot); }

//************************************************************************************************
// ThreadPriorityHandler implementation
//************************************************************************************************

inline void ThreadPriorityHandler::setCustomHandler (ThreadPriorityHandler* handler)
{ Platform::ThreadPriorityHandler::customHandler = handler; }

//************************************************************************************************
// Thread implementation
//************************************************************************************************

inline Thread::Thread (CStringPtr name)
: name (name)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline Thread::Thread (ThreadID threadId)
: name (Platform::kThreadName)
{
	bool succeeded = platformThread.open (threadId);
	if(!succeeded)
	{
		ASSERT (succeeded)
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void Thread::start ()
{ platformThread.start (Platform::ThreadInfo (name, this)); }

inline void Thread::terminate () 
{ platformThread.terminate (); }

inline bool Thread::join (uint32 milliseconds) 
{ return platformThread.join (milliseconds); }

inline void Thread::setPriority (int priority)
{ platformThread.setPriority (priority); }

inline void Thread::setCPUAffinity (int cpu)
{ platformThread.setCPUAffinity (cpu); }

inline int Thread::getPriority () const
{ return platformThread.getPriority (); }

inline ThreadID Thread::getID () const
{ return platformThread.getID (); }

inline CStringPtr Thread::getName () const
{ return name; }

inline int Thread::getNativePriority () const
{ return platformThread.getPlatformPriority (); }

inline int64 Thread::getUserModeTime () const
{ return platformThread.getUserModeTime (); }

inline int Thread::getErrors () const 
{ return platformThread.getErrors (); }

inline Platform::Thread& Thread::getPlatformThread ()
{ return platformThread; }

//************************************************************************************************
// Lock implementation
//************************************************************************************************

inline void Lock::lock ()
{ platformLock.lock (); }

inline bool Lock::tryLock ()
{ return platformLock.tryLock (); }

inline void Lock::unlock ()
{ return platformLock.unlock (); }

//************************************************************************************************
// ScopedLock implementation
//************************************************************************************************

template<class TLock>
inline TScopedLock<TLock>::TScopedLock (TLock& _lock)
: lock (&_lock)
{
	lock->lock ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TLock>
inline TScopedLock<TLock>::TScopedLock (TLock* lock)
: lock (lock)
{
	if(lock) 
		lock->lock ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TLock>
inline TScopedLock<TLock>::~TScopedLock ()
{
	if(lock)
		lock->unlock ();
}

//************************************************************************************************
// PriorityScope implementation
//************************************************************************************************

inline PriorityScope::PriorityScope (ThreadPriority priority, bool apply)
: oldPriority (priority), apply (apply)
{
	if(apply)
		oldPriority = CurrentThread::setPriority (priority);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline PriorityScope::~PriorityScope ()
{
	if(apply)
		CurrentThread::setPriority (oldPriority);
}

//************************************************************************************************
// Signal implementation
//************************************************************************************************

inline Signal::Signal (bool manualReset)
: platformSignal (manualReset)
{};

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void Signal::signal ()
{ platformSignal.signal (); }

inline void Signal::reset ()
{ platformSignal.reset (); }

inline bool Signal::wait (uint32 milliseconds)
{ return platformSignal.wait (milliseconds); }

//************************************************************************************************
// ReadWriteLock implementation
//************************************************************************************************

inline void ReadWriteLock::lockWrite ()
{ platformLock.lockWrite (); }

inline void ReadWriteLock::unlockWrite ()
{ platformLock.unlockWrite (); }

inline void ReadWriteLock::lockRead ()
{ platformLock.lockRead (); }

inline void ReadWriteLock::unlockRead ()
{ platformLock.unlockRead (); }

} // namespace Threads
} // namespace Core

#endif // _corethread_h
