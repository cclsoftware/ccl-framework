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
// Filename    : core/platform/shared/coreplatforminterprocess.h
// Description : Interprocess Communication platform implementation base
//
//************************************************************************************************

#ifndef _coreplatforminterprocess_h
#define _coreplatforminterprocess_h

#include "core/public/corethreading.h"
#include "core/platform/corefeatures.h"

namespace Core {
namespace Platform {

//************************************************************************************************
// Process Functions
//************************************************************************************************

namespace CurrentProcess
{
	Threads::ProcessID getID ();
}

//************************************************************************************************
// ISharedMemory
//************************************************************************************************

struct ISharedMemory
{
	virtual ~ISharedMemory () {}

	virtual bool create (CStringPtr name, uint32 size, bool global = false) = 0;
    virtual bool open (CStringPtr name, uint32 size, bool global = false) = 0;
    virtual void close () = 0;
	virtual void* getMemoryPointer () = 0;
};

//************************************************************************************************
// ISemaphore
//************************************************************************************************

struct ISemaphore
{
	virtual ~ISemaphore () {}

	virtual bool create (CStringPtr name) = 0;
    virtual bool open (CStringPtr name) = 0;
    virtual void close () = 0;
    virtual void lock () = 0;
    virtual void unlock () = 0;
};

//************************************************************************************************
// IPipe
//************************************************************************************************

struct IPipe
{
	virtual ~IPipe () {}

	virtual bool create (CStringPtr name) = 0;
    virtual bool open (CStringPtr name) = 0;
    virtual void close () = 0;
	virtual int read (void* buffer, int size) = 0;
	virtual int write (const void* buffer, int size) = 0;
};

#if CORE_INTERPROCESS_IMPLEMENTATION == CORE_FEATURE_UNIMPLEMENTED

//************************************************************************************************
// SharedMemory stub
//************************************************************************************************

class SharedMemoryStub: public ISharedMemory
{
public:
	// ISharedMemory
	bool create (CStringPtr name, uint32 size, bool global = false) { return false; }
	bool open (CStringPtr name, uint32 size, bool global = false) { return false; }
	void close () {}
	void* getMemoryPointer () { return nullptr; }
};

typedef SharedMemoryStub SharedMemory;

//************************************************************************************************
// Semaphore stub
//************************************************************************************************

class SemaphoreStub: public ISemaphore
{
public:
	// ISemaphore
	bool create (CStringPtr name) { return false; }
	bool open (CStringPtr name) { return false; }
	void close () {}
	void lock () {}
	void unlock () {}
};

typedef SemaphoreStub Semaphore;

//************************************************************************************************
// Pipe stub
//************************************************************************************************

class PipeStub: public IPipe
{
public:
	// IPipe
	bool create (CStringPtr name) { return false; }	
	bool open (CStringPtr name) { return false; }
	void close () {}
	int read (void* buffer, int size) { return 0; }
	int write (const void* buffer, int size) { return 0; }
};

typedef PipeStub Pipe;

#endif

} // namespace Platform
} // namespace Core

#endif // _coreplatforminterprocess_h
