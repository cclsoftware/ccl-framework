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
// Filename    : core/platform/win/corethread.win.cpp
// Description : Windows Multithreading
//
//************************************************************************************************

#include "corethread.win.h"

#include "core/system/coredebug.h"

#include <intrin.h>
#include <Avrt.h>
#include <mmsystem.h>

#pragma comment (lib, "Avrt.lib")
#pragma comment (lib, "winmm.lib")

using namespace Core;
using namespace Threads;
using namespace Platform;

//************************************************************************************************
// Thread Functions
//************************************************************************************************

static int ToNativeThreadPriority (ThreadPriority priority)
{
	static const int nativePriorities[] =
	{
		THREAD_PRIORITY_LOWEST,			 // kPriorityLow
		THREAD_PRIORITY_BELOW_NORMAL,	 // kPriorityBelowNormal
		THREAD_PRIORITY_NORMAL,			 // kPriorityNormal
		THREAD_PRIORITY_ABOVE_NORMAL,	 // kPriorityAboveNormal
		THREAD_PRIORITY_HIGHEST,		 // kPriorityHigh
		THREAD_PRIORITY_TIME_CRITICAL,	 // kPriorityTimeCritial,
		THREAD_PRIORITY_TIME_CRITICAL,	 // kPriorityRealtimeBase
		THREAD_PRIORITY_TIME_CRITICAL,	 // kPriorityRealtimeMiddle
		THREAD_PRIORITY_TIME_CRITICAL	 // kPriorityRealtimeTop
	};

	return nativePriorities[priority];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static ThreadPriority FromNativePriority (int winPrio)
{
	// start with kPriorityRealtime, so THREAD_PRIORITY_TIME_CRITICAL is not reproted as kPriorityRealtimeTop
	for(int corePrio = kPriorityTimeCritical; corePrio >= kPriorityLow; corePrio--) 
		if(ToNativeThreadPriority (corePrio) <= winPrio)
			return corePrio;
	return kPriorityNormal;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ThreadID CurrentThread::getID ()
{
	return ::GetCurrentThreadId ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ThreadPriority CurrentThread::setPriority (ThreadPriority newPrio)
{
	HANDLE currentThread = ::GetCurrentThread ();
	ThreadPriority oldPrio = FromNativePriority (::GetThreadPriority (currentThread));
	if(oldPrio != newPrio)
		::SetThreadPriority (currentThread, ToNativeThreadPriority (newPrio));
	return oldPrio;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CurrentThread::sleep (uint32 milliseconds)
{
	::Sleep (milliseconds);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CurrentThread::ussleep (uint32 microseconds)
{
	::Sleep (microseconds / 1000);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CurrentThread::yield ()
{
	::Sleep (0);
}

//************************************************************************************************
// Thread local storage
//************************************************************************************************

TLSRef TLS::allocate ()
{
	return ::TlsAlloc ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void* TLS::getValue (TLSRef slot)
{
	return ::TlsGetValue (slot);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TLS::setValue (TLSRef slot, void* value)
{
	return ::TlsSetValue (slot, value) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TLS::release (TLSRef slot)
{
	return ::TlsFree (slot) != 0;
}

//************************************************************************************************
// Win32Thread
//************************************************************************************************

static bool WaitForHandle (void* handle, uint32 milliseconds)
{
	DWORD result = ::WaitForSingleObject ((HANDLE)handle, milliseconds);
	return (result == WAIT_ABANDONED || result == WAIT_OBJECT_0) ? true : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static DWORD WINAPI ThreadEntry (LPVOID param)
{
	Win32Thread* thread = (Win32Thread*)param;
	thread->setSelfThreadName ();
	
	if(thread->getPriority () >= kPriorityRealtimeBase)
		thread->setSelfToRealTimePriority ();

	if(IThreadEntry* entry = thread->getThreadEntry ())
		return entry->threadEntry ();

	return 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Win32Thread::enableHighResolutionScheduling (bool state)
{
	MMRESULT result = TIMERR_NOERROR;
	if(state)
		result = ::timeBeginPeriod (1);
	else
		result = ::timeEndPeriod (1);
	ASSERT (result == TIMERR_NOERROR)
	return result == TIMERR_NOERROR;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Win32Thread::Win32Thread ()
: priority (kPriorityNormal),
  errors (0),
  handle (NULL),
  threadId (0),
  cpu (-1),
  entry (nullptr),
  name (kThreadName)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Win32Thread::open (ThreadID _threadId)
{
	ASSERT (entry == nullptr && handle == NULL)
	threadId = static_cast<DWORD> (_threadId);

	// THREAD_ALL_ACCESS needed to change settings (e.g cpu affinity)
	handle = ::OpenThread (THREAD_ALL_ACCESS, FALSE, threadId);

	// fallback: try with less flags if access was not granted
	const DWORD fallbackAccessRights =
		THREAD_QUERY_LIMITED_INFORMATION | THREAD_RESUME | SYNCHRONIZE;

	if(!handle)
		handle = ::OpenThread (fallbackAccessRights, FALSE, threadId);

	return (handle != NULL);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Win32Thread::~Win32Thread ()
{
	::CloseHandle (handle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Win32Thread::start (const ThreadInfo& info)
{
	handle = ::CreateThread (nullptr, 0, ThreadEntry, this, CREATE_SUSPENDED, &threadId);
	ASSERT (handle != NULL)

	entry = info.entry;
	name = info.name;

	ASSERT (entry != nullptr)
		
	::SetThreadPriority (handle, ToNativeThreadPriority (priority)); // always set priority (so if setSelfToRealTimePriority fails, a realtime thread has at least THREAD_PRIORITY_TIME_CRITICAL)
	::ResumeThread (handle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Win32Thread::terminate ()
{
	::TerminateThread (handle, -1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Win32Thread::join (uint32 milliseconds)
{
	return WaitForHandle (handle, milliseconds);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ThreadPriorityHandler* ThreadPriorityHandler::customHandler = nullptr;
bool Win32Thread::setSelfToRealTimePriority ()
{
	BOOL success = FALSE;

	if(ThreadPriorityHandler* handler = ThreadPriorityHandler::customHandler)
	{
		success = handler->setSelfToRealtimePriority (priority);
	}
	else
	{
		DWORD taskIndex = 0;
		HANDLE taskHandle = ::AvSetMmThreadCharacteristics (L"Pro Audio", &taskIndex);
		if(taskHandle != NULL)
		{
			if(priority == kPriorityRealtimeTop)
				success = ::AvSetMmThreadPriority (taskHandle, AVRT_PRIORITY_CRITICAL);
			else if(priority == kPriorityRealtimeMiddle)
				success = ::AvSetMmThreadPriority (taskHandle, AVRT_PRIORITY_HIGH);
			else if(priority == kPriorityRealtimeBase)
				success = ::AvSetMmThreadPriority (taskHandle, AVRT_PRIORITY_NORMAL);		
		}

		if(success == false)
		{
			int error = ::GetLastError ();
			if(error == ERROR_THREAD_ALREADY_IN_TASK)
				success = true;

			#if DEBUG
			if(success == false)
			{
				LPVOID lpMsgBuf = nullptr;
				::FormatMessageA (FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_MAX_WIDTH_MASK,
								  nullptr, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&lpMsgBuf, 0, nullptr);												
				DebugPrintf ("Thread::setSelfToRealTimePriority FAILED with error '%s'\n", (char*)lpMsgBuf);
				::LocalFree (lpMsgBuf);
			}
			#endif
		}
	}
	
	if(success == false)
		errors |= kErrorThreadPriority;

	// use ideal cpu hint instead of cpu affinity when working with MMCSS
	applyIdealProcessor (cpu);
	return success != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Win32Thread::getPlatformPriority () const
{
	return ::GetThreadPriority (handle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Win32Thread::getPriority () const
{
	return priority;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Win32Thread::setPriority (int _priority)
{	
	priority = _priority;
	
	// apply now if called from current thread
	if(threadId == ::GetCurrentThreadId ()) 
	{
		if(priority <= kPriorityTimeCritical)
		{			
			::SetThreadPriority (handle, ToNativeThreadPriority (priority));
		}
		else
			setSelfToRealTimePriority ();
	}	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 Win32Thread::getUserModeTime () const
{
	int64 creationTime = 0, exitTime = 0, kernelTime = 0, userTime = 0;
	::GetThreadTimes (handle, (FILETIME*)&creationTime, (FILETIME*)&exitTime, (FILETIME*)&kernelTime, (FILETIME*)&userTime);
	return userTime;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Threads::ThreadID Win32Thread::getID () const
{
	return threadId;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Win32Thread::getErrors () const
{
	return errors;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Win32Thread::setCPUAffinity (int _cpu)
{
	cpu = _cpu;

	if(priority >= kPriorityRealtimeBase && threadId == ::GetCurrentThreadId ())
		setSelfToRealTimePriority ();
	else
		applyIdealProcessor (cpu);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Win32Thread::applyIdealProcessor (int cpuId)
{
	if(cpuId >= 0)
	{
		ASSERT (handle != NULL)
		DWORD result = ::SetThreadIdealProcessor (handle, cpuId);
		if(result == DWORD (-1))
		{		
			#if DEBUG
			int error = ::GetLastError ();
			LPVOID lpMsgBuf = nullptr;
			::FormatMessageA (FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_MAX_WIDTH_MASK,
							nullptr, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&lpMsgBuf, 0, nullptr);								
			DebugPrintf ("Thread::applyIdealProcessor FAILED with error '%s'\n", (char*)lpMsgBuf);
			::LocalFree (lpMsgBuf);
			#endif

			errors |= kErrorThreadCPUAffinity;
		}		
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Win32Thread::setSelfThreadName ()
{
	struct THREAD_INFO
	{
		DWORD type;			// this must be 0x1000
		LPCSTR name;		// this is the pointer to thread name string
		DWORD threadID;		// -1 for the current thread
		DWORD flags;		// not used, current set to 0x0000
	};

	// set up the thread name struct
	THREAD_INFO info;
	info.type = 0x1000;
	info.name = name;
	info.threadID = -1;
	info.flags = 0;
	__try
	{
		::RaiseException (0x406D1388, 0, sizeof(info)/sizeof(DWORD), (ULONG_PTR*)&info);
	}
    __except(EXCEPTION_EXECUTE_HANDLER)
	{}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IThreadEntry* Win32Thread::getThreadEntry ()
{
	return entry;
}

//************************************************************************************************
// Win32Lock
//************************************************************************************************

Win32Lock::Win32Lock ()
{
	::InitializeCriticalSection (&data);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Win32Lock::~Win32Lock ()
{
	::DeleteCriticalSection (&data);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Win32Lock::lock ()
{
	::EnterCriticalSection (&data);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Win32Lock::tryLock ()
{
	return ::TryEnterCriticalSection (&data) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Win32Lock::unlock ()
{
	::LeaveCriticalSection (&data);
}

//************************************************************************************************
// Win32Signal
//************************************************************************************************

Win32Signal::Win32Signal (bool manualReset)
{
	handle = ::CreateEvent (nullptr, manualReset, FALSE, nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Win32Signal::~Win32Signal ()
{
	::CloseHandle (handle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Win32Signal::signal ()
{
	::SetEvent (handle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Win32Signal::reset ()
{
	::ResetEvent (handle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Win32Signal::wait (uint32 milliseconds)
{
	return WaitForHandle (handle, milliseconds);
}

//************************************************************************************************
// Win32ReadWriteLock
//************************************************************************************************

DEFINE_RECURSIVE_READ_WRITE_LOCK (Win32RecursiveReadWriteLock)

//////////////////////////////////////////////////////////////////////////////////////////////////

Win32ReadWriteLock::Win32ReadWriteLock ()
{
	::InitializeSRWLock (&rwLock);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Win32ReadWriteLock::lockWrite ()
{	
	::AcquireSRWLockExclusive (&rwLock);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Win32ReadWriteLock::unlockWrite ()
{
	::ReleaseSRWLockExclusive (&rwLock);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Win32ReadWriteLock::lockRead ()
{
	::AcquireSRWLockShared (&rwLock);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Win32ReadWriteLock::unlockRead ()
{
	::ReleaseSRWLockShared (&rwLock);
}
