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
// Filename    : core/platform/android/corethread.android.h
// Description : Android Multithreading
//
//************************************************************************************************

#ifndef _corethread_android_h
#define _corethread_android_h

#include "core/platform/shared/posix/corethread.posix.h"
#include "core/platform/shared/corerecursivereadwritelock.h"

namespace Core {
namespace Platform {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Missing POSIX functions
//////////////////////////////////////////////////////////////////////////////////////////////////

extern int pthread_yield (void);
extern int pthread_cancel (pthread_t thread);

#if __ANDROID_API__ < 28
extern int pthread_attr_setinheritsched (pthread_attr_t* attr, int flag);
#endif

//************************************************************************************************
// AndroidThread
//************************************************************************************************

class AndroidThread: public PosixThread
{
public:
	// PosixThread
	void start (const ThreadInfo& info) override;
};

const CStringPtr kThreadName = "Android Thread";
typedef AndroidThread Thread;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Use POSIX implementations
//////////////////////////////////////////////////////////////////////////////////////////////////

typedef PosixLock Lock;
typedef PosixSignal Signal;

typedef RecursiveReadWriteLock<PosixReadWriteLock, Lock> AndroidRecursiveReadWriteLock;
typedef AndroidRecursiveReadWriteLock ReadWriteLock;

} // namespace Platform
} // namespace Core

#endif // _corethread_android_h
