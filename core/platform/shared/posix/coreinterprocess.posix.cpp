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
// Filename    : core/platform/shared/posix/coreinterprocess.posix.cpp
// Description : POSIX Interprocess Communication
//
//************************************************************************************************

#include "core/platform/corefeatures.h"

#if CORE_INTERPROCESS_IMPLEMENTATION == CORE_PLATFORM_IMPLEMENTATION
	#include CORE_PLATFORM_IMPLEMENTATION_HEADER (coreinterprocess)
#else
	#include "core/platform/shared/posix/coreinterprocess.posix.h"
#endif

#if CORE_INTERPROCESS_IMPLEMENTATION == CORE_FEATURE_IMPLEMENTATION_POSIX
#include <sys/fcntl.h>
#endif

using namespace Core;
using namespace Platform;

//************************************************************************************************
// Process Functions
//************************************************************************************************

Threads::ProcessID CurrentProcess::getID ()
{
	return getpid ();
}

//************************************************************************************************
// PosixSharedMemory
//************************************************************************************************

PosixSharedMemory::PosixSharedMemory ()
: file (-1),
  created (false),
  mappedSize (0),
  memoryPointer (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

PosixSharedMemory::~PosixSharedMemory ()
{
	ASSERT (file == -1)	
	close ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixSharedMemory::create (CStringPtr name, uint32 size, bool global)
{
	ASSERT (memoryPointer == nullptr)
	if(memoryPointer != nullptr)
		return false;

	int flags = O_RDWR|O_CREAT;
	mode_t mode = S_IRUSR|S_IWUSR;
	if(global)
	 	mode |= S_IROTH|S_IWOTH;

	file = shm_open (name, flags, mode);
	if(file == -1)
		return false;

	mappedName = name; // need name for shm_unlink
	mappedSize = size;
	created = true;

	int result = ::ftruncate (file, mappedSize);
	if(result != 0)
	{
		ASSERT (result == 0)
	}

	int prot = PROT_READ|PROT_WRITE;
	flags = MAP_SHARED;
	memoryPointer = ::mmap (nullptr, size, prot, flags, file, 0);
	if((int64)memoryPointer == -1L)
		memoryPointer = nullptr;

	return memoryPointer != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixSharedMemory::open (CStringPtr name, uint32 size, bool global)
{
	ASSERT (memoryPointer == nullptr)
	if(memoryPointer != nullptr)
		return false;

	int flags = O_RDWR;
	file = shm_open (name, flags, 0);
	if(file == -1)
		return false;

	mappedName = name;
	mappedSize = size;
	created = false;

	int prot = PROT_READ|PROT_WRITE;
	flags = MAP_SHARED;
	memoryPointer = ::mmap (nullptr, size, prot, flags, file, 0);

    if((int64)memoryPointer == -1L)
		memoryPointer = nullptr;

	return memoryPointer != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PosixSharedMemory::close ()
{
	if(memoryPointer)
	{
		int result = ::munmap (memoryPointer, mappedSize);
		if(result != -1)
		{
			ASSERT (result != -1)
		}
		memoryPointer = nullptr;
		mappedSize = 0;
	}
    
    if(file != -1)
    {
		#ifndef CORE_PLATFORM_ANDROID
        if(created)
        {
            int result = shm_unlink (mappedName);
            ASSERT (result == 0)
        }
		#endif
		
        file = -1;
        mappedName = "";
        created = false;
    }
}

//************************************************************************************************
// PosixSemaphore
//************************************************************************************************

PosixSemaphore::PosixSemaphore ()
: semaphore (nullptr),
  created (false)
{
	fd[0] = -1;
	fd[1] = -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PosixSemaphore::~PosixSemaphore ()
{
	ASSERT (semaphore == nullptr)	
	close ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixSemaphore::create (CStringPtr name)
{
	ASSERT (semaphore == nullptr)
	if(semaphore != nullptr)
		return false;

	int flags = O_CREAT|O_EXCL;
	mode_t mode = S_IRUSR|S_IWUSR; // read|write for user

	semaphore = ::sem_open (name, flags, mode, 0);
	if(semaphore == SEM_FAILED)
		semaphore = nullptr;
	else
	{
		savedName = name; // needed for unlink
		created = true;

		// if our process crashes, we won't be able to unlink the seamphore and the operating system doesn't clean up either.
		// as a workaround, spawn a child process which acts as a watch guard.
		// when our end of the pipe is closed (by us or by the OS kernel), the child process cleans up the semaphore
		if(::pipe (fd) == 0)
		{
			pid_t pid = 0;
			pid = ::fork ();
			if(pid == 0)
			{
				// this is executed in the child process

				::close (fd[1]); // close write file descriptor
				
				// blocking read until the parent process closes the semaphore or the operating system closes the pipe
				char buffer;
				::read (fd[0], &buffer, 1);
				
				// unlink the semaphore
				::sem_unlink (savedName);

				::_exit (0);
			}
			::close (fd[0]); // close read file descriptor
		}
	}
	
	return semaphore != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixSemaphore::open (CStringPtr name)
{
	ASSERT (semaphore == nullptr)
	if(semaphore != nullptr)
		return false;

	created = false;
	semaphore = ::sem_open (name, 0);
	if(semaphore == SEM_FAILED)
		semaphore = nullptr;
	else
		savedName = name; // needed for unlink

	return semaphore != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PosixSemaphore::close ()
{
	if(semaphore)
	{
		::sem_close (semaphore);

		if(created)
		{
			::sem_unlink (savedName);
			if(fd[1] >= 0)
				::close (fd[1]); // close write file descriptor
		}

		savedName = "";
		created = false;
		
		semaphore = nullptr;

		fd[0] = -1;
		fd[1] = -1;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PosixSemaphore::lock ()
{
	ASSERT (semaphore != nullptr)
	int result = ::sem_wait (semaphore);
	if(result != 0)
	{
		ASSERT (result == 0)
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PosixSemaphore::unlock ()
{
	ASSERT (semaphore != nullptr)
	int result = ::sem_post (semaphore);
	if(result != 0)
	{
		ASSERT (result == 0)
	}
}

//************************************************************************************************
// PosixPipe
//************************************************************************************************

PosixPipe::PosixPipe ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

PosixPipe::~PosixPipe ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixPipe::create (CStringPtr name)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixPipe::open (CStringPtr name)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PosixPipe::close ()
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int PosixPipe::read (void* buffer, int size)
{
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int PosixPipe::write (const void* buffer, int size)
{
	return -1;
}
