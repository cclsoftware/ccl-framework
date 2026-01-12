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
// Filename    : ccl/public/system/ithreading.h
// Description : Multithreading Interfaces
//
//************************************************************************************************

#ifndef _ccl_ithreading_h
#define _ccl_ithreading_h

#include "ccl/public/base/iunknown.h"

#include "core/public/corethreading.h"

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Built-in synchronization classes
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID
{
	DEFINE_CID (CriticalSection, 0xd50316c0, 0x6c0a, 0x455b, 0x9b, 0xc, 0x5a, 0x22, 0x49, 0xf, 0xb1, 0x8e)
	DEFINE_CID (ManualSignal, 0x22667c1f, 0x6f57, 0x4f73, 0xb9, 0xb, 0x31, 0x8a, 0x67, 0x4, 0x92, 0xf8)
	DEFINE_CID (Signal, 0x52eea740, 0x69b0, 0x4682, 0xb6, 0x8, 0x9, 0x2a, 0xef, 0xfe, 0x29, 0x45)
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Thread definitions
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Threading {

typedef void* WorkgroupID;
typedef void* WorkgroupToken;

// import Core Framework definitions
using namespace Core::Threads::Threading;

/** TLS destructor function. */
typedef void (CCL_API *ThreadLocalDestructor) (void* data);

/** Thread function. */
typedef int (CCL_API *ThreadFunction) (void* arg);

//************************************************************************************************
// IThread
/** Thread interface, created via System::CreateNativeThread(). 
	\ingroup ccl_system */
//************************************************************************************************

interface IThread: IUnknown
{
	/** Get thread identifier. */
	virtual Core::Threads::ThreadID CCL_API getThreadID () const = 0;

	/** Get current priority. */
	virtual ThreadPriority CCL_API getPriority () const = 0;

	/** Set current priority. */
	virtual void CCL_API setPriority (ThreadPriority priority) = 0;
	
	/** Set CPU affinity. */
	virtual void CCL_API setCPUAffinity (int cpu) = 0;

	/** Start thread. */
	virtual void CCL_API start () = 0;

	/** Terminate thread. */
	virtual void CCL_API terminate () = 0;
	
	/** Wait for thread to finish. */
	virtual tbool CCL_API join (uint32 milliseconds) = 0;

	/** Get thread errors. */
	virtual ThreadErrors CCL_API getErrors () const = 0;

	DECLARE_IID (IThread)
};

DEFINE_IID (IThread, 0x5fe0a233, 0x31bf, 0x4e65, 0xb7, 0x36, 0x83, 0x8d, 0xed, 0x1e, 0x12, 0xf6)

//************************************************************************************************
// ISyncPrimitive
/**	Interface for synchronization primitives (not all method are applicable to all types). 
	\ingroup ccl_system */
//************************************************************************************************

interface ISyncPrimitive: IUnknown
{
	/** Wait for ownership of object. */
	virtual tresult CCL_API lock () = 0;

	/** Attempt to get ownership without blocking. */
	virtual tresult CCL_API tryLock () = 0;
	
	/** Release ownership of object. */
	virtual tresult CCL_API unlock () = 0;

	/** Set object into signaled state. */
	virtual tresult CCL_API signal () = 0;

	/** Set object into non-signaled state. */
	virtual tresult CCL_API reset () = 0;

	/** Perform blocking wait on object with optional timeout. */
	virtual tresult CCL_API wait (uint32 milliseconds) = 0;

	DECLARE_IID (ISyncPrimitive)
};

DEFINE_IID (ISyncPrimitive, 0xfc2b8587, 0xb07, 0x4392, 0xaf, 0xbf, 0x67, 0x62, 0xf9, 0xfa, 0x6, 0xdc)

//************************************************************************************************
// IAtomicStack
/** Stack using lock-free synchronization. 
	\ingroup ccl_system */
//************************************************************************************************

interface IAtomicStack: IUnknown
{
	/** Base class for stack elements. */
	CCL_ALIGN(struct) Element
	{
		Element* next;
		Element (): next (nullptr) {}
	};

	/** Pop first stack element. */
	virtual Element* CCL_API pop () = 0;
	
	/** Push element to stack. */
	virtual void CCL_API push (Element* e) = 0;
	
	/** Flush (empty) the stack. */
	virtual void CCL_API flush () = 0; 
	
	/** Returns current stack depth. */
	virtual int CCL_API depth () = 0;

	DECLARE_IID (IAtomicStack)
};

DEFINE_IID (IAtomicStack, 0x2bac92cd, 0xc1c0, 0x4336, 0x9e, 0x1c, 0x16, 0x88, 0x71, 0x85, 0xe4, 0xa4)

} // namespace Threading
} // namespace CCL

#endif // _ccl_ithreading_h
