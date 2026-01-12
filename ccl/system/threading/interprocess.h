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
// Filename    : ccl/system/threading/interprocess.h
// Description : Interprocess Communication
//
//************************************************************************************************

#ifndef _ccl_interprocess_h
#define _ccl_interprocess_h

#include "ccl/public/base/unknown.h"
#include "ccl/public/system/iinterprocess.h"

#if CCL_PLATFORM_WINDOWS
#include "ccl/platform/win/cclwindows.h"
#endif

#include "core/system/coreinterprocess.h"

namespace CCL {
namespace Threading {

//************************************************************************************************
// NativeSharedMemory
//************************************************************************************************

class NativeSharedMemory: public Unknown,
						  public ISharedMemory,
						  private Core::Threads::SharedMemory
{
public:
	// ISharedMemory
	tresult CCL_API create (CStringPtr name, uint32 size) override;
	tresult CCL_API open (CStringPtr name, uint32 size) override;
	tresult CCL_API close () override;
	void* CCL_API getMemoryPointer () override;

	CLASS_INTERFACE (ISharedMemory, Unknown)
};

//************************************************************************************************
// NativeSemaphore
//************************************************************************************************

class NativeSemaphore: public Unknown,
					   public ISemaphore,
					   private Core::Threads::Semaphore
{
public:
	// ISemaphore
	tresult CCL_API create (CStringPtr name) override;
	tresult CCL_API open (CStringPtr name) override;
	tresult CCL_API close () override;
	tresult CCL_API lock () override;
	tresult CCL_API unlock () override;

	CLASS_INTERFACE (ISemaphore, Unknown)
};

//************************************************************************************************
// NativePipe
//************************************************************************************************

class NativePipe: public Unknown,
				  public INamedPipe,
				  private Core::Threads::Pipe
{
public:
	// INamedPipe
	tresult CCL_API create (CStringPtr name) override;
	tresult CCL_API open (CStringPtr name) override;
	tresult CCL_API close () override;
	int CCL_API read (void* buffer, int size) override;
	int CCL_API write (const void* buffer, int size) override;

	CLASS_INTERFACE (INamedPipe, Unknown)
};

} // namespace Threading
} // namespace CCL

#endif // _ccl_interprocess_h
