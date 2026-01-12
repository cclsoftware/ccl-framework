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
// Filename    : core/platform/android/corethread.android.cpp
// Description : Android Multithreading
//
//************************************************************************************************

#include "corethread.android.h"

#include "core/platform/shared/jni/corejnihelper.h"

#include <errno.h>

namespace Core {
namespace Platform {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Missing POSIX functions
//////////////////////////////////////////////////////////////////////////////////////////////////

int pthread_yield (void)
{
	// Note: On Linux, pthread_yield is implemented as a call to sched_yield(2).
	return sched_yield ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int pthread_cancel (pthread_t thread)
{
	// Note: pthread_cancel is not available on Android
	return ENOSYS;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

#if __ANDROID_API__ < 28
int pthread_attr_setinheritsched (pthread_attr_t* attr, int flag)
{
	// Note: pthread_attr_setinheritsched is not available on Android until API level 28
	return ENOTSUP;
}
#endif

} // namespace Platform
} // namespace Core

using namespace Core;
using namespace Platform;
using namespace Threads;

//************************************************************************************************
// AndroidThread
//************************************************************************************************

static void* ThreadEntry (void* param)
{
	Java::JniThreadScope scope;
	AndroidThread* thread = (AndroidThread*)param;
	return reinterpret_cast<void*> (thread->getThreadEntry ()->threadEntry ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidThread::start (const ThreadInfo& info)
{
	entry = info.entry;
	name = info.name;

	pthread_attr_t threadAttributes;
	pthread_attr_init (&threadAttributes);
	pthread_attr_setdetachstate (&threadAttributes, PTHREAD_CREATE_JOINABLE);
	pthread_create (&threadId, &threadAttributes, ThreadEntry, this);
	pthread_attr_destroy (&threadAttributes);
}

//************************************************************************************************
// AndroidRecursiveReadWriteLock
//************************************************************************************************

DEFINE_RECURSIVE_READ_WRITE_LOCK (AndroidRecursiveReadWriteLock)
