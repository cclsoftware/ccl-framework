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
// Filename    : ccl/system/threading/threadlocks.h
// Description : Lock classes
//
//************************************************************************************************

#ifndef _ccl_threadlocks_h
#define _ccl_threadlocks_h

#include "ccl/system/threading/thread.h"

#include "ccl/public/system/ilockable.h"

namespace CCL {
namespace Threading {

//************************************************************************************************
// SyncPrimitive
//************************************************************************************************

class SyncPrimitive: public Unknown,
					 public ISyncPrimitive
{
public:
	// ISyncPrimitive
	tresult CCL_API lock () override;
	tresult CCL_API tryLock () override;
	tresult CCL_API unlock () override;
	tresult CCL_API signal () override;
	tresult CCL_API reset () override;
	tresult CCL_API wait (uint32 milliseconds) override;

	CLASS_INTERFACE (ISyncPrimitive, Unknown)
};

//************************************************************************************************
// NativeCriticalSection
//************************************************************************************************

class NativeCriticalSection: public SyncPrimitive,
							 private Core::Threads::Lock
{
public:
	// ISyncPrimitive
	tresult CCL_API lock () override;
	tresult CCL_API tryLock () override;
	tresult CCL_API unlock () override;
};

//************************************************************************************************
// NativeSignal
//************************************************************************************************

class NativeSignal: public SyncPrimitive,
					private Core::Threads::Signal
{
public:
	NativeSignal (bool manualReset);

	// ISyncPrimitive
	tresult CCL_API signal () override;
	tresult CCL_API reset () override;
	tresult CCL_API wait (uint32 milliseconds) override;
};

//************************************************************************************************
// ExclusiveLock
//************************************************************************************************

class ExclusiveLock: public Unknown,
					 public ILockable,
					 private Core::Threads::Lock
{
public:
	// ILockable
	void CCL_API lock (int access = kExclusive) override;
	tbool CCL_API tryLock (int access = kExclusive) override;
	void CCL_API unlock (int access = kExclusive) override;

	CLASS_INTERFACE (ILockable, Unknown)
};

//************************************************************************************************
// ReadWriteLock
//************************************************************************************************

class ReadWriteLock: public Unknown,
					 public ILockable,
					 private Core::Threads::ReadWriteLock
{
public:
	// ILockable
	void CCL_API lock (int access = kRead) override;
	tbool CCL_API tryLock (int access = kRead) override;
	void CCL_API unlock (int access = kRead) override;

	CLASS_INTERFACE (ILockable, Unknown)
};

} // namespace Threading
} // namespace CCL

#endif // _ccl_threadlocks_h
