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
// Filename    : core/platform/linux/corethread.linux.cpp
// Description : Linux Multithreading
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "corethread.linux.h"

#include "core/system/coredebug.h"

#include "core/public/corestringbuffer.h"
#include "core/public/coremacros.h"

using namespace Core;
using namespace Platform;
using namespace Threads;

//************************************************************************************************
// LinuxThread
//************************************************************************************************

LinuxThread::LinuxThread ()
{
	CPU_ZERO (&cpuSet);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxThread::start (const ThreadInfo& info)
{
	PosixThread::start (info);
	
	if(threadId)
	{
		CString16 shortName = name;
		pthread_setname_np (threadId, shortName);

		if(CPU_COUNT (&cpuSet) > 0)
			pthread_setaffinity_np (threadId, sizeof(cpuSet), &cpuSet);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxThread::setCPUAffinity (int affinity)
{
	CPU_ZERO (&cpuSet);
	CPU_SET (affinity, &cpuSet);

	if(threadId && CPU_COUNT (&cpuSet) > 0)
		pthread_setaffinity_np (threadId, sizeof(cpuSet), &cpuSet);
}

//************************************************************************************************
// LinuxRecursiveReadWriteLock
//************************************************************************************************

DEFINE_RECURSIVE_READ_WRITE_LOCK (LinuxRecursiveReadWriteLock)
