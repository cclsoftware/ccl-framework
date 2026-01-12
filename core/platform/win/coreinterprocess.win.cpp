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
// Filename    : core/platform/win/coreinterprocess.win.cpp
// Description : Win32 Interprocess Communication
//
//************************************************************************************************

#include "core/platform/win/coreinterprocess.win.h"

#include "core/public/corethreading.h"
#include "core/public/corestringbuffer.h"

using namespace Core;
using namespace Platform;

//************************************************************************************************
// Process Functions
//************************************************************************************************

Threads::ProcessID CurrentProcess::getID ()
{
	return ::GetCurrentProcessId ();
}

//************************************************************************************************
// Win32SharedMemory
//************************************************************************************************

static CString64 addNamespace (CStringPtr name, bool global)
{
	if(global)
	{
		CString64 globalName ("Global\\");
		globalName.append (name);
		return globalName;
	}
	return name;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Win32SharedMemory::Win32SharedMemory ()
: handle (NULL),
  memoryPointer (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Win32SharedMemory::~Win32SharedMemory ()
{
	ASSERT (handle == NULL)
	close ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Win32SharedMemory::create (CStringPtr name, uint32 size, bool global)
{
	ASSERT (handle == NULL)
	if(handle != NULL)
		return false;

	SECURITY_ATTRIBUTES* secAttrPtr = nullptr;
	SECURITY_ATTRIBUTES secAttr = {0};
	SECURITY_DESCRIPTOR secDesc;
	if(global)
	{
		secAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
		secAttr.bInheritHandle = TRUE;

		if(InitializeSecurityDescriptor (&secDesc, SECURITY_DESCRIPTOR_REVISION) &&
			SetSecurityDescriptorDacl (&secDesc, TRUE, (PACL)nullptr, FALSE))
			secAttr.lpSecurityDescriptor = &secDesc;
		secAttrPtr = &secAttr;
	}

	handle = ::CreateFileMappingA (INVALID_HANDLE_VALUE, secAttrPtr, PAGE_READWRITE, 0, size, addNamespace (name, global));
	if(!handle)
		return false;

	memoryPointer = ::MapViewOfFile (handle, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	return memoryPointer != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Win32SharedMemory::open (CStringPtr name, uint32, bool global)
{
	ASSERT (handle == NULL)
	if(handle != NULL)
		return false;

	handle = ::OpenFileMappingA (FILE_MAP_ALL_ACCESS, FALSE, addNamespace (name, global));
	if(!handle)
		return false;

	memoryPointer = ::MapViewOfFile (handle, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	return memoryPointer != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Win32SharedMemory::close ()
{
	if(memoryPointer)
		::UnmapViewOfFile (memoryPointer),
		memoryPointer = nullptr;

	if(handle)
		::CloseHandle (handle),
		handle = NULL;
}

//************************************************************************************************
// Win32Semaphore
//************************************************************************************************

Win32Semaphore::Win32Semaphore ()
: handle (NULL)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Win32Semaphore::~Win32Semaphore ()
{
	ASSERT (handle == NULL)
	close ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Win32Semaphore::create (CStringPtr name)
{
	ASSERT (handle == NULL)
	if(handle != NULL)
		return false;

	handle = ::CreateMutexA (nullptr, FALSE, name);
	if(::GetLastError() == ERROR_ALREADY_EXISTS)
	{
		if(handle)
			::CloseHandle (handle);
		handle = nullptr;
	}

	return handle != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Win32Semaphore::open (CStringPtr name)
{
	ASSERT (handle == NULL)
	if(handle != NULL)
		return false;

	handle = ::OpenMutexA (MUTEX_ALL_ACCESS, FALSE, name);

	return handle != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Win32Semaphore::close ()
{
	if(handle)
		::CloseHandle (handle),
		handle = NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Win32Semaphore::lock ()
{
	ASSERT (handle != NULL)
	DWORD result = ::WaitForSingleObject (handle, INFINITE);
	ASSERT (result == WAIT_ABANDONED || result == WAIT_OBJECT_0)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Win32Semaphore::unlock ()
{
	ASSERT (handle != NULL)
	::ReleaseMutex (handle);
}

//************************************************************************************************
// Win32Pipe
//************************************************************************************************

Win32Pipe::Win32Pipe ()
: handle (NULL)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Win32Pipe::~Win32Pipe ()
{
	ASSERT (handle == NULL)
	close ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Win32Pipe::create (CStringPtr name)
{
	ASSERT (handle == NULL)
	if(handle != NULL)
		return false;

	char pipeName[256];
	::strcpy (pipeName, "\\\\.\\pipe\\");
	::strcat (pipeName, name);

	static const int kPipeBufferSize = 4096;

	handle = ::CreateNamedPipeA (pipeName,
		PIPE_ACCESS_DUPLEX,       // read/write access
		PIPE_TYPE_MESSAGE |       // message type pipe
		PIPE_READMODE_MESSAGE |   // message-read mode
		PIPE_WAIT,                // blocking mode
		PIPE_UNLIMITED_INSTANCES, // max. instances
		kPipeBufferSize,          // output buffer size
		kPipeBufferSize,          // input buffer size
		NMPWAIT_USE_DEFAULT_WAIT, // client time-out
		nullptr);                 // default security attribute

	if(handle == INVALID_HANDLE_VALUE)
	{
		handle = NULL;
		return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Win32Pipe::open (CStringPtr name)
{
	ASSERT (handle == NULL)
	if(handle != NULL)
		return false;

	char pipeName[256] = {};
	::strcpy (pipeName, "\\\\.\\pipe\\");
	::strcat (pipeName, name);

	handle = ::CreateFileA (pipeName, GENERIC_READ|GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, NULL);
	if(handle == INVALID_HANDLE_VALUE)
	{
		handle = NULL;
		return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Win32Pipe::close ()
{
	if(handle)
		::CloseHandle (handle),
	handle = NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Win32Pipe::read (void* buffer, int size)
{
	DWORD bytesRead = 0;
	::ReadFile (handle, buffer, size, &bytesRead, nullptr);
	return (int)bytesRead;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Win32Pipe::write (const void* buffer, int size)
{
	DWORD bytesWriten = 0;
	::WriteFile (handle, buffer, size, &bytesWriten, nullptr);
	return (int)bytesWriten;
}
