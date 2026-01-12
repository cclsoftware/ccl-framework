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
// Filename    : ccl/platform/win/system/nativefilesystem.win.h
// Description : Windows native file system
//
//************************************************************************************************

#ifndef _ccl_nativefilesystem_win_h
#define _ccl_nativefilesystem_win_h

#include "ccl/system/nativefilesystem.h"

namespace CCL {

//************************************************************************************************
// WindowsNativeFileSystem
//************************************************************************************************

class WindowsNativeFileSystem: public NativeFileSystem
{
public:
	// IFileSystem
	tbool CCL_API fileExists (UrlRef url) override;
	tbool CCL_API getFileInfo (FileInfo& info, UrlRef url) override;
	tbool CCL_API removeFile (UrlRef url, int mode = 0) override;
	IFileIterator* CCL_API newIterator (UrlRef url, int mode = IFileIterator::kAll) override;
	tbool CCL_API isCaseSensitive () override;

	// IVolumeFileSystem
	tbool CCL_API getVolumeInfo (VolumeInfo& info, UrlRef rootUrl) override;
	tbool CCL_API isHiddenFile (UrlRef url) override;
	tbool CCL_API isWriteProtected (UrlRef url) override;
	tbool CCL_API moveFile (UrlRef dstPath, UrlRef srcPath, int mode = 0, IProgressNotify* progress = nullptr) override;
	tbool CCL_API copyFile (UrlRef dstPath, UrlRef srcPath, int mode = 0, IProgressNotify* progress = nullptr) override;

	// INativeFileSystem
	tbool CCL_API getPathType (int& type, UrlRef baseFolder, StringRef fileName) override;
	tbool CCL_API setFileTime (UrlRef url, const FileTime& modifiedTime) override;
	ISearcher* CCL_API createSearcher (ISearchDescription& description) override;
	tbool CCL_API getWorkingDirectory (IUrl& url) override;
	tbool CCL_API setWorkingDirectory (UrlRef url) override;
	tbool CCL_API beginTransaction () override;
	tbool CCL_API endTransaction (int mode, IProgressNotify* progress = nullptr) override;

protected:
	// NativeFileSystem
	IStream* openPlatformStream (UrlRef url, int mode) override;
	bool createPlatformFolder (UrlRef url) override;
	bool removePlatformFolder (UrlRef url, int mode) override;
	int translateNativeError (int nativeError) override;
};

//************************************************************************************************
// WindowsFileStream
//************************************************************************************************

class WindowsFileStream: public FileStream
{
public:
	// IStream
	int CCL_API read (void* buffer, int size) override;
	int CCL_API write (const void* buffer, int size) override;
	int64 CCL_API tell () override;
	int64 CCL_API seek (int64 pos, int mode) override;

	// INativeFileStream
	void CCL_API setOptions (int options) override;
	tbool CCL_API getPath (IUrl& path) override;
	tbool CCL_API setEndOfFile (int64 eof) override;

protected:
	NativePath path;

	friend class WindowsNativeFileSystem;

	WindowsFileStream (WindowsNativeFileSystem* fileSystem, void* file = nullptr, int options = 0);
	~WindowsFileStream ();
};

//************************************************************************************************
// WindowsFileIterator
//************************************************************************************************

class WindowsFileIterator: public NativeFileIterator
{
public:
	WindowsFileIterator (UrlRef url, int mode);
	~WindowsFileIterator ();

	// IFileIterator
	const IUrl* CCL_API next () override;
};

//************************************************************************************************
// WindowsVolumesIterator
//************************************************************************************************

class WindowsVolumesIterator: public NativeVolumesIterator
{
public:
	WindowsVolumesIterator ();
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline WindowsFileStream::WindowsFileStream (WindowsNativeFileSystem* fileSystem, void* file, int options)
: FileStream (fileSystem, file, options) {}

} // namespace CCL

#endif // _ccl_nativefilesystem_win_h
