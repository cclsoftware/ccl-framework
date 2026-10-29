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
// Filename    : ccl/platform/wasm/system/nativefilesystem.wasm.cpp
// Description : WebAssembly file system
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/wasm/system/nativefilesystem.wasm.h"
#include "ccl/base/storage/url.h"

#include <errno.h>

using namespace CCL;

//************************************************************************************************
// NativeFileSystem
//************************************************************************************************

NativeFileSystem& NativeFileSystem::instance ()
{
	static WasmNativeFileSystem theInstance;
	return theInstance;
}

//************************************************************************************************
// WasmNativeFileSystem
//************************************************************************************************

IStream* WasmNativeFileSystem::openPlatformStream (UrlRef url, int mode)
{
	int handle = openFileDescriptor (url, mode);
	if(handle == -1)
	{
		onNativeError (errno, &url);
		return nullptr;
	}

	WasmFileStream* stream = NEW WasmFileStream (this, reinterpret_cast<void*> (handle), mode);
	stream->path = POSIXPath (url);
	return stream;
}

//************************************************************************************************
// WasmFileStream
//************************************************************************************************

WasmFileStream::WasmFileStream (WasmNativeFileSystem* fileSystem, void* file, int options)
: PosixFileStream (fileSystem, file, options)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WasmFileStream::getPath (IUrl& path)
{
	return path.fromPOSIXPath (this->path, IUrl::kFile);
}
