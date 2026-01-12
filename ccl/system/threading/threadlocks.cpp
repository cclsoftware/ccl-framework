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
// Filename    : ccl/system/threading/threadlocks.cpp
// Description : Lock classes
//
//************************************************************************************************

#include "ccl/system/threading/threadlocks.h"

#include "ccl/public/systemservices.h"

using namespace CCL;
using namespace Threading;

//////////////////////////////////////////////////////////////////////////////////////////////////
// System Threading APIs
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT ISyncPrimitive* CCL_API System::CCL_ISOLATED (CreateSyncPrimitive) (UIDRef cid)
{
	if(cid == ClassID::CriticalSection)
	{
		return NEW NativeCriticalSection;
	}
	else if(cid == ClassID::ManualSignal)
	{
		return NEW NativeSignal (true);
	}
	else if(cid == ClassID::Signal)
	{
		return NEW NativeSignal (false);
	}

	ASSERT (0)
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT ILockable* CCL_API System::CCL_ISOLATED (CreateAdvancedLock) (UIDRef cid)
{
	if(cid == ClassID::ExclusiveLock)
	{
		return NEW ExclusiveLock;
	}
	else if(cid == ClassID::ReadWriteLock)
	{
		return NEW ReadWriteLock;
	}

	ASSERT (0)
	return nullptr;
}

//************************************************************************************************
// SyncPrimitive
//************************************************************************************************

tresult CCL_API SyncPrimitive::lock ()
{
	ASSERT (0)
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SyncPrimitive::tryLock ()
{
	ASSERT (0)
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SyncPrimitive::unlock ()
{
	ASSERT (0)
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SyncPrimitive::signal ()
{
	ASSERT (0)
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SyncPrimitive::reset ()
{
	ASSERT (0)
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SyncPrimitive::wait (uint32 milliseconds)
{
	ASSERT (0)
	return kResultNotImplemented;
}

//************************************************************************************************
// NativeCriticalSection
//************************************************************************************************

tresult CCL_API NativeCriticalSection::lock ()
{
	Lock::lock ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NativeCriticalSection::tryLock ()
{
	if(Lock::tryLock ())
		return kResultOk;
	return kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NativeCriticalSection::unlock ()
{
	Lock::unlock ();
	return kResultOk;
}

//************************************************************************************************
// NativeSignal
//************************************************************************************************

NativeSignal::NativeSignal (bool manualReset)
: Signal (manualReset)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NativeSignal::signal ()
{
	Signal::signal ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NativeSignal::reset ()
{
	Signal::reset ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NativeSignal::wait (uint32 milliseconds)
{
	if(Signal::wait (milliseconds))
		return kResultOk;
	return kResultFalse;
}

//************************************************************************************************
// ExclusiveLock
//************************************************************************************************

void CCL_API ExclusiveLock::lock (int)
{
	Lock::lock ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ExclusiveLock::tryLock (int)
{
	return Lock::tryLock ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ExclusiveLock::unlock (int)
{
	Lock::unlock ();
}

//************************************************************************************************
// ReadWriteLock
//************************************************************************************************

void CCL_API ReadWriteLock::lock (int access)
{
	if(access == ILockable::kExclusive || access == ILockable::kWrite)
		lockWrite ();
	else
		lockRead ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ReadWriteLock::tryLock (int access)
{
	ASSERT (0)
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ReadWriteLock::unlock (int access)
{
	if(access == ILockable::kExclusive || access == ILockable::kWrite)
		unlockWrite ();
	else
		unlockRead ();
}
