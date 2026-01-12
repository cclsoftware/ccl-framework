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
// Filename    : ccl/public/system/threadsync.h
// Description : Synchronization classes
//
//************************************************************************************************

#ifndef _ccl_threadsync_h
#define _ccl_threadsync_h

#include "ccl/public/systemservices.h"

namespace CCL {
namespace Threading {

//************************************************************************************************
// SyncObject
/** Base class for synchronization objects. 
	\ingroup ccl_system */
//************************************************************************************************

class SyncObject
{
public:
	SyncObject (ISyncPrimitive* primitive = nullptr);
	~SyncObject ();

protected:
	ISyncPrimitive* primitive;

	SyncObject (const SyncObject&);
	SyncObject& operator = (const SyncObject&);
};

//************************************************************************************************
// CriticalSection
/** Userspace lock. 
	\ingroup ccl_system */
//************************************************************************************************

class CriticalSection: public SyncObject
{
public:
	CriticalSection ();

	/** Attempt to enter critical section without blocking. */
	bool tryEnter ();
	
	/** Wait for ownership of critical section. */
	void enter ();

	/** Release ownership of critical section. */
	void leave ();
};

//************************************************************************************************
// ScopedLock
/** Scoped locking helper for CriticalSection. 
	\ingroup ccl_system */
//************************************************************************************************

struct ScopedLock
{
	ScopedLock (CriticalSection& criticalSection)
	: criticalSection (criticalSection) 
	{ criticalSection.enter (); }

	~ScopedLock () 
	{ criticalSection.leave (); }
	
	CriticalSection& criticalSection;
};

//************************************************************************************************
// ScopedTryLock
/** Scoped locking helper for CriticalSection (check 'success' after ctor). 
	\ingroup ccl_system */
//************************************************************************************************

struct ScopedTryLock
{
	ScopedTryLock (CriticalSection& criticalSection)
	: criticalSection (criticalSection) 
	{ success = criticalSection.tryEnter (); }

	~ScopedTryLock () 
	{ if(success) criticalSection.leave (); }
	
	CriticalSection& criticalSection;
	bool success;
};

//************************************************************************************************
// Signal
/** Signal object. 
	\ingroup ccl_system */
//************************************************************************************************

class Signal: public SyncObject
{
public:
	Signal (bool manualReset = false);

	void signal ();
	void reset ();
	bool wait (uint32 milliseconds);
};

//************************************************************************************************
// AtomicInt
/** Thread and MP-safe integer variable.
	\ingroup ccl_system */
//************************************************************************************************

class AtomicInt
{
public:
	AtomicInt ()
	: value (0)
	{}

	AtomicInt (const AtomicInt& a)
	: value (0)
	{ assign (a); }

	/** Get current value. */
	int getValue () const;

	/** Assign new value, returns old value. */
	int assign (int v);	

	/** Perform atomic addition, returns old value. */
	int add (int v);

	/** Increment by one, returns old value. */
	int increment ();

	/** Decrement value by one, returns old value. */
	int decrement ();

	/** Set only if current value equals comperand, returns true if value was set. */
	tbool testAndSet (int value, int comperand);

	/** Cast to plain integer. */
	operator const int () const;

	AtomicInt& operator = (int v);
	AtomicInt& operator ++ ();
	AtomicInt& operator -- ();
	AtomicInt& operator += (int v);
	AtomicInt& operator -= (int v);

protected:
	int volatile value;
};

//************************************************************************************************
// AtomicPtr
/** Thread- and MP-safe pointer variable. 
	\ingroup ccl_system */
//************************************************************************************************

class AtomicPtr
{
public:
	AtomicPtr ()
	: ptr (nullptr)
	{}

	AtomicPtr (const AtomicPtr& ap)
	: ptr (nullptr)
	{ assign (ap); }
	
	/** Get current value. */
	void* getPtr () const;

	/** Assign new value, returns old value. */
	void* assign (void* ptr);

	/** Set only if current value equals comperand, returns true if value was set. */
	tbool testAndSet (void* ptr, void* comperand);

	/** Cast to plain pointer. */
	operator void* () const;

	/** Assign plain pointer. */
	AtomicPtr& operator = (void* ptr);

protected:
	void* volatile ptr;
};

//************************************************************************************************
// SpinLock
/** Userspace spin lock. 
	\ingroup ccl_system */
//************************************************************************************************

class SpinLock
{
public:
	SpinLock ()
	: value (0)
	{}

	bool tryLock ();
	void lock ();
	void unlock ();

protected:
	int volatile value;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// SpinLock inline 
//////////////////////////////////////////////////////////////////////////////////////////////////

INLINE bool SpinLock::tryLock ()
{ return System::SpinLockTryLock (value) != 0; }

INLINE void SpinLock::lock ()
{ System::SpinLockLock (value); }

INLINE void SpinLock::unlock ()
{ System::SpinLockUnlock (value); }

//////////////////////////////////////////////////////////////////////////////////////////////////
// CriticalSection inline 
//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool CriticalSection::tryEnter () 
{ return primitive->tryLock () == kResultOk; }

inline void CriticalSection::enter () 
{ primitive->lock (); }

inline void CriticalSection::leave () 
{ primitive->unlock (); }

//////////////////////////////////////////////////////////////////////////////////////////////////
// Signal inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline void Signal::signal ()
{ primitive->signal (); } 

inline void Signal::reset ()
{ primitive->reset (); }

inline bool Signal::wait (uint32 milliseconds)
{ return primitive->wait (milliseconds) == kResultOk; }

//////////////////////////////////////////////////////////////////////////////////////////////////
// AtomicInt inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline int AtomicInt::add (int v) 
{ return AtomicAddInline (value, v); }

inline int AtomicInt::assign (int v)
{ return AtomicSetInline (value, v); }

inline tbool AtomicInt::testAndSet (int v, int comperand)
{ return AtomicTestAndSetInline (value, v, comperand); }

inline int AtomicInt::increment ()
{ return add (1); }

inline int AtomicInt::decrement ()
{ return add (-1); }

inline int AtomicInt::getValue () const
{ return AtomicGetInline (value); }

inline AtomicInt::operator const int () const 
{ return getValue (); }

inline AtomicInt& AtomicInt::operator = (int v)
{ assign (v); return *this; }

inline AtomicInt& AtomicInt::operator ++ () 
{ increment (); return *this; }

inline AtomicInt& AtomicInt::operator -- ()
{ decrement (); return *this; }

inline AtomicInt& AtomicInt::operator += (int v)
{ add (v);  return *this; }

inline AtomicInt& AtomicInt::operator -= (int v) 
{ add (-v); return *this; }

//////////////////////////////////////////////////////////////////////////////////////////////////
// AtomicPtr inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline void* AtomicPtr::getPtr () const
{ return AtomicGetPtrInline (ptr); }

inline AtomicPtr::operator void* () const 
{ return getPtr (); }

inline AtomicPtr& AtomicPtr::operator = (void* ptr)	
{ assign (ptr); return *this; }

inline void* AtomicPtr::assign (void* newPtr)
{ return AtomicSetPtrInline (ptr, newPtr); }

inline tbool AtomicPtr::testAndSet (void* newPtr, void* comperand)
{ return AtomicTestAndSetPtrInline (ptr, newPtr, comperand); }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace Threading
} // namespace CCL

#endif // _ccl_threadsync_h
