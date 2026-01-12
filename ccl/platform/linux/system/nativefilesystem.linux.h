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
// Filename    : ccl/platform/linux/system/nativefilesystem.linux.h
// Description : Linux file system
//
//************************************************************************************************

#ifndef _ccl_nativefilesystem_linux_h
#define _ccl_nativefilesystem_linux_h

#include "ccl/platform/shared/posix/system/nativefilesystem.posix.h"

namespace CCL {

//************************************************************************************************
// LinuxNativeFileSystem
//************************************************************************************************

class LinuxNativeFileSystem: public PosixNativeFileSystem
{
public:
	// PosixNativeFileSystem
	IFileIterator* CCL_API newIterator (UrlRef url, int mode = IFileIterator::kAll) override;
	tbool CCL_API getVolumeInfo (VolumeInfo& info, UrlRef rootUrl) override;

protected:
	// PosixNativeFileSystem
	IStream* openPlatformStream (UrlRef url, int mode) override;
};

//************************************************************************************************
// LinuxFileStream
//************************************************************************************************

class LinuxFileStream: public PosixFileStream
{
public:
	// PosixFileStream
	tbool CCL_API getPath (IUrl& path) override;

protected:
	friend class LinuxNativeFileSystem;

	LinuxFileStream (LinuxNativeFileSystem* fileSystem, void* file = nullptr, int options = 0);
};

//************************************************************************************************
// LinuxVolumesIterator
//************************************************************************************************

class LinuxVolumesIterator: public NativeVolumesIterator
{
public:
	LinuxVolumesIterator ();
};

} // namespace CCL

#endif // _ccl_nativefilesystem_linux_h
