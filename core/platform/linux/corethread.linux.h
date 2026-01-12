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
// Filename    : core/platform/linux/corethread.linux.h
// Description : Linux Multithreading
//
//************************************************************************************************

#ifndef _corethread_linux_h
#define _corethread_linux_h

#include "core/platform/shared/posix/corethread.posix.h"
#include "core/platform/shared/corerecursivereadwritelock.h"

namespace Core {
namespace Platform {

//************************************************************************************************
// LinuxThread
//************************************************************************************************

class LinuxThread: public PosixThread
{
public:
	LinuxThread ();

	// PosixThread
	void start (const ThreadInfo& info) override;
	void setCPUAffinity (int affinity) override;

private:
	cpu_set_t cpuSet;
};

const CStringPtr kThreadName = "Linux Thread";
typedef LinuxThread Thread;

typedef PosixLock Lock;
typedef PosixSignal Signal;

typedef RecursiveReadWriteLock<PosixReadWriteLock, Lock> LinuxRecursiveReadWriteLock;
typedef LinuxRecursiveReadWriteLock ReadWriteLock;

} // namespace Platform
} // namespace Core

#endif // _corethread_linux_h
