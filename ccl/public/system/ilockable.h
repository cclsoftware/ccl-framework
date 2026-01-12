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
// Filename    : ccl/public/system/ilockable.h
// Description : Lock Interface
//
//************************************************************************************************

#ifndef _ccl_ilockable_h
#define _ccl_ilockable_h

#include "ccl/public/base/iunknown.h"
#include "ccl/public/base/debug.h"

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Built-in lock classes
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID
{
	DEFINE_CID (ExclusiveLock, 0xb0a26115, 0x732a, 0x4351, 0x8c, 0x2c, 0xea, 0xac, 0x3a, 0xcc, 0x8, 0x6e);
	DEFINE_CID (ReadWriteLock, 0x900461d4, 0x63c1, 0x41df, 0xa7, 0xa2, 0x87, 0x6, 0x60, 0xa4, 0x8d, 0x89);
}

namespace Threading {

//************************************************************************************************
// ILockable
/**	\ingroup ccl_system */
//************************************************************************************************

interface ILockable: IUnknown
{
	enum AccessMethods
	{
		kExclusive,
		kRead,
		kWrite
	};

	virtual void CCL_API lock (int access = kExclusive) = 0;

	virtual tbool CCL_API tryLock (int access = kExclusive) = 0;
	
	virtual void CCL_API unlock (int access = kExclusive) = 0;

	DECLARE_IID (ILockable)
};

DEFINE_IID (ILockable, 0xdc49f203, 0xc07a, 0x4013, 0x86, 0x30, 0xa0, 0xdd, 0x53, 0x6b, 0x3, 0x6f)

//************************************************************************************************
// ILockProvider
/**	\ingroup ccl_system */
//************************************************************************************************

interface ILockProvider: IUnknown
{
	virtual ILockable* CCL_API getLock () const = 0;

	DECLARE_IID (ILockProvider)
};

DEFINE_IID (ILockProvider, 0xd2038d6e, 0x3169, 0x4681, 0xaa, 0x3c, 0xd5, 0xa3, 0xd0, 0x9a, 0x8a, 0x86)

//************************************************************************************************
// AutoLock
/**	\ingroup ccl_system */
//************************************************************************************************

struct AutoLock
{
	AutoLock (ILockable* lockable, int access = ILockable::kExclusive)
	: lockable (lockable),
	  access (access)
	{ if(lockable) lockable->lock (access); }

	AutoLock (const ILockProvider& provider, int access = ILockable::kExclusive)
	: lockable (provider.getLock ()),
	  access (access)
	{ if(lockable) lockable->lock (access); }

	~AutoLock ()
	{ if(lockable) lockable->unlock (access); }

	ILockable* lockable;
	int access;
};

//************************************************************************************************
// AutoTryLock
/**	\ingroup ccl_system */
//************************************************************************************************

struct AutoTryLock
{
	AutoTryLock (ILockable* lockable, int access = ILockable::kExclusive)
	: lockable (lockable),
	  access (access),
	  success (false)
	{ if(lockable) success = lockable->tryLock (access) != 0; }

	AutoTryLock (const ILockProvider& provider, int access = ILockable::kExclusive)
	: lockable (provider.getLock ()),
	  access (access),
	  success (false)
	{ if(lockable) success = lockable->tryLock (access) != 0; }

	~AutoTryLock ()
	{ if(lockable && success) lockable->unlock (access); }

	bool isLocked () const {return success;}
private:
	ILockable* lockable;
	int access;
	bool success;
};

//************************************************************************************************
// DebuggingAutoLock
//************************************************************************************************

#if DEBUG
struct DebuggingAutoLock
{
	DebuggingAutoLock (ILockable* lockable, int access = ILockable::kExclusive)
	: lockable (lockable),
	  access (access)
	{ if(lockable) { lockable->lock (access); Debugger::println ("LOCK"); } else Debugger::println ("LOCK FAILED, BECAUSE LOCK ZERO!\n"); }

	DebuggingAutoLock (const ILockProvider& provider, int access = ILockable::kExclusive)
	: lockable (provider.getLock ()),
	  access (access)
	{ if(lockable) { lockable->lock (access); Debugger::println ("LOCK"); } else Debugger::println ("LOCK FAILED, BECAUSE LOCK ZERO!"); }

	~DebuggingAutoLock ()
	{ if(lockable) { lockable->unlock (access); Debugger::println ("UNLOCK"); } else Debugger::println ("UNLOCK FAILED, BECAUSE LOCK ZERO!"); }

	ILockable* lockable;
	int access;
};
#endif

} // namespace Threading
} // namespace CCL

#endif // _ccl_ilockable_h
