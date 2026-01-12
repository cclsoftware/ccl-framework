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
// Filename    : core/platform/shared/corerecursivereadwritelock.h
// Description : Recursive Read Write Lock
//
//************************************************************************************************

#ifndef _corerecursivereadwritelock_h
#define _corerecursivereadwritelock_h

#include "core/platform/shared/coreplatformthread.h"

#include "core/public/corevector.h"

namespace Core {
namespace Platform {

//************************************************************************************************
// RecursiveReadWriteLock
/** Wrapper class around non-recursive ReadWriteLock implementations, allowing recursive locking. */
//************************************************************************************************

#define DEFINE_RECURSIVE_READ_WRITE_LOCK(ImplementationClass) \
template<> Threads::TLSRef ImplementationClass::statsRef = 0; \
DEFINE_INITIALIZER (Allocate ## ImplementationClass ## LockStats) { ImplementationClass::allocateStats (); } \
DEFINE_TERMINATOR (Release ## ImplementationClass ## LockStats) { ImplementationClass::releaseStats (); }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class RWLock, class Lock>
class RecursiveReadWriteLock: public RWLock
{
public:
	static void allocateStats ();
	static void releaseStats ();

	// IReadWriteLock
	void lockWrite () override;
	void unlockWrite () override;
	void lockRead () override;
	void unlockRead () override;

protected:
	static Threads::TLSRef statsRef;

	struct LockStats
	{
		const RecursiveReadWriteLock<RWLock, Lock>* lock;
		int readCount;
		int writeCount;

		LockStats (const RecursiveReadWriteLock<RWLock, Lock>* lock = nullptr, int readCount = 0, int writeCount = 0)
		: lock (lock),
		  readCount (readCount),
	      writeCount (writeCount)
		{}

		bool operator == (const LockStats& other) const
		{
			return other.lock == lock;
		}
	};
	typedef Vector<LockStats> StatsVector;

	Lock writeAcquireLock;

	LockStats* getThreadStats (bool create = false) const;
	void releaseThreadStats ();
	bool holdsWriteLock () const;
	bool holdsReadLock () const;
	void incrementWriteCount ();
	void decrementWriteCount ();
	void incrementReadCount ();
	void decrementReadCount ();
};

//************************************************************************************************
// RecursiveReadWriteLock implementation
//************************************************************************************************

template <class RWLock, class Lock>
void RecursiveReadWriteLock<RWLock, Lock>::allocateStats ()
{
	ASSERT (!statsRef)
	statsRef = TLS::allocate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class RWLock, class Lock>
void RecursiveReadWriteLock<RWLock, Lock>::releaseStats ()
{
	ASSERT (statsRef)
	TLS::release (statsRef);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class RWLock, class Lock>
void RecursiveReadWriteLock<RWLock, Lock>::lockWrite ()
{
	if(!holdsWriteLock ())
	{
		writeAcquireLock.lock ();
		if(holdsReadLock ())
		{
			// exchange read lock with write lock
			RWLock::unlockRead ();
		}
		RWLock::lockWrite ();
		writeAcquireLock.unlock ();
	}
	incrementWriteCount ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class RWLock, class Lock>
void RecursiveReadWriteLock<RWLock, Lock>::unlockWrite ()
{
	decrementWriteCount ();

	if(!holdsWriteLock ())
	{
		if(holdsReadLock ())
		{
			// exchange write lock with read lock
			writeAcquireLock.lock ();
			RWLock::unlockWrite ();
			RWLock::lockRead ();
			writeAcquireLock.unlock ();
		}
		else
			RWLock::unlockWrite ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class RWLock, class Lock>
void RecursiveReadWriteLock<RWLock, Lock>::lockRead ()
{
	if(!holdsReadLock () && !holdsWriteLock ())
		RWLock::lockRead ();

	incrementReadCount ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class RWLock, class Lock>
void RecursiveReadWriteLock<RWLock, Lock>::unlockRead ()
{
	decrementReadCount ();

	if(!holdsReadLock () && !holdsWriteLock ())
		RWLock::unlockRead ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class RWLock, class Lock>
typename RecursiveReadWriteLock<RWLock, Lock>::LockStats* RecursiveReadWriteLock<RWLock, Lock>::getThreadStats (bool create) const
{
	ASSERT (statsRef)
	if(!statsRef)
		return nullptr;

	StatsVector* statsVector = static_cast<StatsVector*> (TLS::getValue (statsRef));
	if(!statsVector)
	{
		if(!create)
			return nullptr;

		statsVector = NEW StatsVector;
		TLS::setValue (statsRef, statsVector);
	}

	int index = statsVector->index (this);
	if(index >= 0)
		return &statsVector->at (index);

	if(!create)
		return nullptr;

	statsVector->add (LockStats (this));
	return &statsVector->last ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class RWLock, class Lock>
void RecursiveReadWriteLock<RWLock, Lock>::releaseThreadStats ()
{
	ASSERT (statsRef)
	if(!statsRef)
		return;

	StatsVector* statsVector = static_cast<StatsVector*> (TLS::getValue (statsRef));
	ASSERT (statsVector)
	if(!statsVector)
		return;

	int index = statsVector->index (this);
	ASSERT (index >= 0)
	if(index < 0)
		return;

	statsVector->removeAt (index);
	if(statsVector->isEmpty ())
	{
		TLS::setValue (statsRef, nullptr);
		delete statsVector;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class RWLock, class Lock>
bool RecursiveReadWriteLock<RWLock, Lock>::holdsWriteLock () const
{
	LockStats* stats = getThreadStats ();
	return stats && stats->writeCount > 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class RWLock, class Lock>
bool RecursiveReadWriteLock<RWLock, Lock>::holdsReadLock () const
{
	LockStats* stats = getThreadStats ();
	return stats && stats->readCount > 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class RWLock, class Lock>
void RecursiveReadWriteLock<RWLock, Lock>::incrementWriteCount ()
{
	LockStats* stats = getThreadStats (true);
	ASSERT (stats)
	if(!stats)
		return;

	stats->writeCount++;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class RWLock, class Lock>
void RecursiveReadWriteLock<RWLock, Lock>::decrementWriteCount ()
{
	LockStats* stats = getThreadStats (true);
	ASSERT (stats)
	if(!stats)
		return;

	stats->writeCount--;

	if(stats->writeCount == 0 && stats->readCount == 0)
		releaseThreadStats ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class RWLock, class Lock>
void RecursiveReadWriteLock<RWLock, Lock>::incrementReadCount ()
{
	LockStats* stats = getThreadStats (true);
	ASSERT (stats)
	if(!stats)
		return;

	stats->readCount++;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class RWLock, class Lock>
void RecursiveReadWriteLock<RWLock, Lock>::decrementReadCount ()
{
	LockStats* stats = getThreadStats (true);
	ASSERT (stats)
	if(!stats)
		return;

	stats->readCount--;

	if(stats->writeCount == 0 && stats->readCount == 0)
		releaseThreadStats ();
}

} // namespace Platform
} // namespace Core

#endif // _corerecursivereadwritelock_h
