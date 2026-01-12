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
// Filename    : ccl/platform/android/system/nativefilesystem.android.cpp
// Description : Android native file system
//
//************************************************************************************************

#include "ccl/platform/android/system/nativefilesystem.android.h"

#include "ccl/public/text/cstring.h"

#include <errno.h>
#include <unistd.h>

using namespace CCL;

//************************************************************************************************
// AndroidNativeFileSystem::AndroidFileStream
//************************************************************************************************

class AndroidNativeFileSystem::AndroidFileStream: public PosixFileStream
{
public:
	AndroidFileStream (PosixNativeFileSystem* fileSystem, void* file = 0, int options = 0)
	: PosixFileStream (fileSystem, file, options)
	{}

	// PosixFileStream
	tbool CCL_API getPath (IUrl& path) override;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API AndroidNativeFileSystem::AndroidFileStream::getPath (IUrl& path)
{ 
	char filePath[STRING_STACK_SPACE_MAX] = {0};
	MutableCString fdPath ("/proc/self/fd/");
	fdPath.appendInteger (intFromPointer (file));
	ssize_t length = ::readlink (fdPath, filePath, ARRAY_COUNT (filePath));
	if(length > 0 && length + 1 < STRING_STACK_SPACE_MAX)
	{
		filePath[length] = 0;
		return path.fromPOSIXPath (filePath, IUrl::kFile);
	}
	return false;
}

//************************************************************************************************
// AndroidNativeFileSystem
//************************************************************************************************

String AndroidNativeFileSystem::translateMode (int mode)
{
	bool wantRead = mode & CCL::IStream::kReadMode;
	bool wantWrite = mode & CCL::IStream::kWriteMode;
	bool wantCreate = mode & CCL::IStream::kCreate;

	String modeString;
	if(wantRead && wantWrite)
		modeString = "rw";
	else if(wantWrite)
		modeString = "w";
	else
		modeString = "r";

	if(wantCreate)
		modeString.append ("t");

	return modeString;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStream* AndroidNativeFileSystem::openPlatformStream (UrlRef url, int mode)
{
	int handle = openFileDescriptor (url, mode);
	if(handle == -1)
	{
		onNativeError (errno, &url);
		return nullptr;
	}
	
	return NEW AndroidFileStream (this, reinterpret_cast<void*> (handle), mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStream* AndroidNativeFileSystem::createStreamFromHandle (int _handle)
{
	int handle = ::dup (_handle);
	if(handle == -1)
	{
		onNativeError (errno, 0);
		return nullptr;
	}

	return NEW AndroidFileStream (this, reinterpret_cast<void*> (handle));
}

//************************************************************************************************
// NativeFileSystem
//************************************************************************************************

NativeFileSystem& NativeFileSystem::instance ()
{
	static AndroidNativeFileSystem theInstance;
	return theInstance;
}
