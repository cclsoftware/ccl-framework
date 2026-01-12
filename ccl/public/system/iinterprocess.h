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
// Filename    : ccl/public/system/iinterprocess.h
// Description : Interprocess Communication Interfaces
//
//************************************************************************************************

#ifndef _ccl_iinterprocess_h
#define _ccl_iinterprocess_h

#include "ccl/public/base/iunknown.h"

namespace CCL {
namespace Threading {

//************************************************************************************************
// ISharedMemory
/** Shared memory block. 
	\ingroup ccl_system */
//************************************************************************************************

interface ISharedMemory: IUnknown
{
	/** Create shared memory block of given size. */
	virtual tresult CCL_API create (CStringPtr name, uint32 size) = 0;
	
	/** Open shared memory block of given size. */
	virtual tresult CCL_API open (CStringPtr name, uint32 size) = 0;
	
	/** Close shared memory block. */
	virtual tresult CCL_API close () = 0;

	/** Get mapped address of shared memory block. */
	virtual void* CCL_API getMemoryPointer () = 0;

	DECLARE_IID (ISharedMemory)
};

DEFINE_IID (ISharedMemory, 0xbcf02db8, 0xc345, 0x4f0e, 0xa3, 0x82, 0x20, 0xca, 0x57, 0x37, 0x29, 0xa7)

//************************************************************************************************
// ISemaphore
/** Interprocess semaphore.
	\ingroup ccl_system */
//************************************************************************************************

interface ISemaphore: IUnknown
{
	/** Create named sempahore. */
	virtual tresult CCL_API create (CStringPtr name) = 0;
	
	/** Open named semaphore. */
	virtual tresult CCL_API open (CStringPtr name) = 0;

	/** Close named semaphore. */
	virtual tresult CCL_API close () = 0;

	/** Lock named semaphore. */
	virtual tresult CCL_API lock () = 0;

	/** Unlock named semaphore. */
	virtual tresult CCL_API unlock () = 0;

	DECLARE_IID (ISemaphore)
};

DEFINE_IID (ISemaphore, 0x504efd47, 0x38c3, 0x4639, 0xaa, 0x3f, 0xc6, 0x50, 0x7c, 0x3, 0xb0, 0x34)

//************************************************************************************************
// INamedPipe
/** Interprocess pipe. 
	\ingroup ccl_system */
//************************************************************************************************

interface INamedPipe: IUnknown
{
	/** Create named pipe. */
	virtual tresult CCL_API create (CStringPtr name) = 0;
	
	/** Open named pipe. */
	virtual tresult CCL_API open (CStringPtr name) = 0;

	/** Close named pipe. */
	virtual tresult CCL_API close () = 0;

	/** Read data from pipe. */
	virtual int CCL_API read (void* buffer, int size) = 0;

	/** Write data to pipe. */
	virtual int CCL_API write (const void* buffer, int size) = 0;

	DECLARE_IID (INamedPipe)
};

DEFINE_IID (INamedPipe, 0xc0125a57, 0x3dfb, 0x474b, 0x8c, 0x2f, 0x70, 0x3a, 0xb3, 0xe5, 0xa1, 0xa1)

} // namespace Threading
} // namespace CCL

#endif // _ccl_iinterprocess_h
