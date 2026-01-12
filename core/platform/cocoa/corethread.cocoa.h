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
// Filename    : core/platform/cocoa/corethread.cocoa.h
// Description : Cocoa Multithreading
//
//************************************************************************************************

#ifndef _corethread_cocoa_h
#define _corethread_cocoa_h

#include "core/platform/shared/posix/corethread.posix.h"

namespace Core {
namespace Platform {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Missing POSIX functions
//////////////////////////////////////////////////////////////////////////////////////////////////

extern int pthread_yield (void);

//************************************************************************************************
// CocoaThread
//************************************************************************************************

class CocoaThread: public PosixThread
{
public:

	// PosixThread
	void start (const ThreadInfo& info) override;
	void setPriority (int priority) override;
	void setCPUAffinity (int affinity) override;	
};

const CStringPtr kThreadName = "Mac Thread";
typedef CocoaThread Thread;

typedef PosixLock Lock;
typedef PosixSignal Signal;
typedef PosixReadWriteLock ReadWriteLock;

} // namespace Platform
} // namespace Core

#endif // _corethread_cocoa_h
