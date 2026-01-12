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
// Filename    : core/platform/android/coreinterprocess.android.h
// Description : Android Interprocess Communication
//
//************************************************************************************************

#ifndef _coreinterprocess_android_h
#define _coreinterprocess_android_h

#include "core/platform/shared/posix/coreinterprocess.posix.h"

#include <asm-generic/fcntl.h>

namespace Core {
namespace Platform {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Missing POSIX functions
//////////////////////////////////////////////////////////////////////////////////////////////////

extern int shm_open (const char *name, int oflag, mode_t mode);

//////////////////////////////////////////////////////////////////////////////////////////////////
// Use POSIX implementations
//////////////////////////////////////////////////////////////////////////////////////////////////

typedef PosixSharedMemory SharedMemory;
typedef PosixSemaphore Semaphore;
typedef PosixPipe Pipe;

} // namespace Platform
} // namespace Core

#endif // _coreinterprocess_android_h
