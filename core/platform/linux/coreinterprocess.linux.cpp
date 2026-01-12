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
// Filename    : core/platform/linux/coreinterprocess.linux.cpp
// Description : Linux Interprocess Communication
//
//************************************************************************************************

#include "core/platform/linux/coreinterprocess.linux.h"

#include <spawn.h>
#include <sys/wait.h>

using namespace Core;
using namespace Platform;

//************************************************************************************************
// LinuxSemaphore
//************************************************************************************************

bool LinuxSemaphore::create (CStringPtr name)
{
	// if a semaphore with the same name exists, we need to check if there is still a process running which uses this semaphore
	// otherwise we can unlink it: the semaphore has been created by a process that crashed or the system crashed before the process could unlink the semaphore

	char arg0[] = "fuser";
	char arg1[] = "-s";
	CString128 path = "/dev/shm/sem.";
	path.append (name);
	char* argv[] = {arg0, arg1, path.getBuffer (), nullptr};
	
	pid_t pid = -1;
	if(::access (path, F_OK) == 0 && ::posix_spawnp (&pid, "fuser", nullptr, nullptr, argv, environ) == 0)
	{
		int status = -1;
		if(::waitpid (pid, &status, 0) == pid && WEXITSTATUS (status) != 0)
			::sem_unlink (name);
	}

	return PosixSemaphore::create (name);
}
