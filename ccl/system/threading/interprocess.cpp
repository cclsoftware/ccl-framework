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
// Filename    : ccl/system/threading/interprocess.cpp
// Description : Interprocess Communication
//
//************************************************************************************************

#include "ccl/system/threading/interprocess.h"

#include "ccl/public/systemservices.h"

using namespace CCL;
using namespace Core;
using namespace Threading;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Process and Interprocess Communication APIs
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT ProcessID CCL_API System::CCL_ISOLATED (GetProcessSelfID) ()
{
	return Core::Threads::CurrentProcess::getID ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT ISharedMemory* CCL_API System::CCL_ISOLATED (CreateIPCSharedMemory) ()
{
	return NEW NativeSharedMemory;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT ISemaphore* CCL_API System::CCL_ISOLATED (CreateIPCSemaphore) ()
{
	return NEW NativeSemaphore;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT INamedPipe* CCL_API System::CCL_ISOLATED (CreateIPCPipe) ()
{
	return NEW NativePipe;
}

//************************************************************************************************
// NativeSharedMemory
//************************************************************************************************

tresult CCL_API NativeSharedMemory::create (CStringPtr name, uint32 size)
{
	if(!Core::Threads::SharedMemory::create (name, size))
		return kResultFailed;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NativeSharedMemory::open (CStringPtr name, uint32 size)
{
	if(!Core::Threads::SharedMemory::open (name, size))
		return kResultFailed;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NativeSharedMemory::close ()
{
	Core::Threads::SharedMemory::close ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void* CCL_API NativeSharedMemory::getMemoryPointer ()
{
	return Core::Threads::SharedMemory::getMemoryPointer ();
}

//************************************************************************************************
// NativeSemaphore
//************************************************************************************************

tresult CCL_API NativeSemaphore::create (CStringPtr name)
{
	if(!Core::Threads::Semaphore::create (name))
		return kResultFailed;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NativeSemaphore::open (CStringPtr name)
{
	if(!Core::Threads::Semaphore::open (name))
		return kResultFailed;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NativeSemaphore::close ()
{
	Core::Threads::Semaphore::close ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NativeSemaphore::lock ()
{
	Core::Threads::Semaphore::lock ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NativeSemaphore::unlock ()
{
	Core::Threads::Semaphore::unlock ();
	return kResultOk;
}

//************************************************************************************************
// NativePipe
//************************************************************************************************

tresult CCL_API NativePipe::create (CStringPtr name)
{
	if(!Core::Threads::Pipe::create (name))
		return kResultFailed;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NativePipe::open (CStringPtr name)
{
	if(!Core::Threads::Pipe::open (name))
		return kResultFailed;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NativePipe::close ()
{
	Core::Threads::Pipe::close ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API NativePipe::read (void* buffer, int size)
{
	return Core::Threads::Pipe::read (buffer, size);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API NativePipe::write (const void* buffer, int size)
{
	return Core::Threads::Pipe::write (buffer, size);
}
