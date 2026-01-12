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
// Filename    : core/platform/linux/coreinterprocess.linux.h
// Description : Linux Interprocess Communication
//
//************************************************************************************************

#ifndef _coreinterprocess_linux_h
#define _coreinterprocess_linux_h

#include "core/platform/shared/posix/coreinterprocess.posix.h"

#include <sys/fcntl.h>

namespace Core {
namespace Platform {

typedef PosixSharedMemory SharedMemory;
typedef PosixPipe Pipe;

//************************************************************************************************
// LinuxSemaphore
//************************************************************************************************

class LinuxSemaphore: public PosixSemaphore
{
public:
	// ISemaphore
	bool create (CStringPtr name) override;
};

typedef LinuxSemaphore Semaphore;

} // namespace Platform
} // namespace Core

#endif // _coreinterprocess_win_h
