//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : core/platform/wasm/corethread.wasm.h
// Description : WebAssembly Multithreading
//
//************************************************************************************************

#ifndef _corethread_wasm_h
#define _corethread_wasm_h

#include "core/platform/shared/posix/corethread.posix.h"
#include "core/platform/shared/corerecursivereadwritelock.h"

namespace Core {
namespace Platform {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Use POSIX implementations
//////////////////////////////////////////////////////////////////////////////////////////////////

const CStringPtr kThreadName = "WebAssembly Thread";

typedef PosixLock Lock;
typedef PosixSignal Signal;
typedef PosixThread Thread;

typedef RecursiveReadWriteLock<PosixReadWriteLock, Lock> WasmRecursiveReadWriteLock;
typedef WasmRecursiveReadWriteLock ReadWriteLock;

} // namespace Platform
} // namespace Core

#endif // _corethread_wasm_h
