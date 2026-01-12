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
// Filename    : core/platform/win/coreinterprocess.win.h
// Description : Win32 Interprocess Communication
//
//************************************************************************************************

#ifndef _coreinterprocess_win_h
#define _coreinterprocess_win_h

#include "core/platform/shared/coreplatforminterprocess.h"

#include <windows.h>

namespace Core {
namespace Platform {

//************************************************************************************************
// Win32SharedMemory
//************************************************************************************************

class Win32SharedMemory: public ISharedMemory
{
public:
	Win32SharedMemory ();
	~Win32SharedMemory ();

	// ISharedMemory
	bool create (CStringPtr name, uint32 size, bool global = false) override;
	bool open (CStringPtr name, uint32 size, bool global = false) override;
	void close () override;
	void* getMemoryPointer () override { return memoryPointer; }

protected:
	HANDLE handle;
	void* memoryPointer;
};

typedef Win32SharedMemory SharedMemory;

//************************************************************************************************
// Win32Semaphore
//************************************************************************************************

class Win32Semaphore: public ISemaphore
{
public:
	Win32Semaphore ();
	~Win32Semaphore ();

	// ISemaphore
	bool create (CStringPtr name) override;
	bool open (CStringPtr name) override;
	void close () override;
	void lock () override;
	void unlock () override;

protected:
	HANDLE handle;
};

typedef Win32Semaphore Semaphore;

//************************************************************************************************
// Win32Pipe
//************************************************************************************************

class Win32Pipe: public IPipe
{
public:
	Win32Pipe ();
	~Win32Pipe ();

	// IPipe
	bool create (CStringPtr name) override;
	bool open (CStringPtr name) override;
	void close () override;
	int read (void* buffer, int size) override;
	int write (const void* buffer, int size) override;

protected:
	HANDLE handle;
};

typedef Win32Pipe Pipe;

} // namespace Platform
} // namespace Core

#endif // _coreinterprocess_win_h
