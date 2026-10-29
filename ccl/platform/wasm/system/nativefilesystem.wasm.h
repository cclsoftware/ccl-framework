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
// Filename    : ccl/platform/wasm/system/nativefilesystem.wasm.h
// Description : WebAssembly file system
//
//************************************************************************************************

#ifndef _ccl_nativefilesystem_wasm_h
#define _ccl_nativefilesystem_wasm_h

#include "ccl/platform/shared/posix/system/nativefilesystem.posix.h"

namespace CCL {

//************************************************************************************************
// WasmNativeFileSystem
//************************************************************************************************

class WasmNativeFileSystem: public PosixNativeFileSystem
{
protected:
	// PosixNativeFileSystem
	IStream* openPlatformStream (UrlRef url, int mode) override;
};

//************************************************************************************************
// WasmFileStream
//************************************************************************************************

class WasmFileStream: public PosixFileStream
{
public:
	// PosixFileStream
	tbool CCL_API getPath (IUrl& path) override;

protected:
	POSIXPath path;

	friend class WasmNativeFileSystem;

	WasmFileStream (WasmNativeFileSystem* fileSystem, void* file = nullptr, int options = 0);
};

} // namespace CCL

#endif // _ccl_nativefilesystem_wasm_h
