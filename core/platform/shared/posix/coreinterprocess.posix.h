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
// Filename    : core/platform/shared/posix/coreinterprocess.win.h
// Description : POSIX Interprocess Communication
//
//************************************************************************************************

#ifndef _coreinterprocess_posix_h
#define _coreinterprocess_posix_h

#include "core/platform/shared/coreplatforminterprocess.h"

#include "core/public/corestringbuffer.h"

#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <semaphore.h>

namespace Core {
namespace Platform {

//************************************************************************************************
// PosixSharedMemory
//************************************************************************************************

class PosixSharedMemory: public ISharedMemory
{
public:
	PosixSharedMemory ();
	~PosixSharedMemory ();

	// ISharedMemory
	bool create (CStringPtr name, uint32 size, bool global = false) override;
	bool open (CStringPtr name, uint32 size, bool global = false) override;
	void close () override;
	void* getMemoryPointer () override { return memoryPointer; }

protected:
	int file;
	bool created;
	uint32 mappedSize;
	CString128 mappedName;
	void* memoryPointer;
};

#if CORE_INTERPROCESS_IMPLEMENTATION == CORE_FEATURE_IMPLEMENTATION_POSIX
typedef PosixSharedMemory SharedMemory;
#endif

//************************************************************************************************
// PosixSemaphore
//************************************************************************************************

class PosixSemaphore: public ISemaphore
{
public:
	PosixSemaphore ();
	~PosixSemaphore ();

	// ISemaphore
	bool create (CStringPtr name) override;
	bool open (CStringPtr name) override;
	void close () override;
	void lock () override;
	void unlock () override;

protected:
	sem_t* semaphore;
	bool created;
	CString128 savedName;
	int fd[2];
};

#if CORE_INTERPROCESS_IMPLEMENTATION == CORE_FEATURE_IMPLEMENTATION_POSIX
typedef PosixSemaphore Semaphore;
#endif

//************************************************************************************************
// PosixPipe
//************************************************************************************************

class PosixPipe: public IPipe
{
public:
	PosixPipe ();
	~PosixPipe ();

	// IPipe
	bool create (CStringPtr name) override;
	bool open (CStringPtr name) override;
	void close () override;
	int read (void* buffer, int size) override;
	int write (const void* buffer, int size) override;
};

#if CORE_INTERPROCESS_IMPLEMENTATION == CORE_FEATURE_IMPLEMENTATION_POSIX
typedef PosixPipe Pipe;
#endif

} // namespace Platform
} // namespace Core

#endif // _coreinterprocess_posix_h
