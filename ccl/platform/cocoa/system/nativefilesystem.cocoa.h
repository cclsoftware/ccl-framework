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
// Filename    : ccl/platform/cocoa/system/nativefilesystem.cocoa.h
// Description : Mac OS file system class
//
//************************************************************************************************

#ifndef _ccl_nativefilesystem_cocoa_h
#define _ccl_nativefilesystem_cocoa_h

#include "ccl/platform/shared/posix/system/nativefilesystem.posix.h"

@class NSURL;


namespace CCL {
	
//************************************************************************************************
// CocoaNativeFileSystem
//************************************************************************************************

class CocoaNativeFileSystem: public PosixNativeFileSystem
{
public:
	// IFileSystem
	tbool CCL_API fileExists (UrlRef url);
	tbool CCL_API getFileInfo (FileInfo& info, UrlRef url);
	tbool CCL_API removeFile (UrlRef url, int mode = 0);
	tbool CCL_API renameFile (UrlRef url, StringRef newName, int mode = 0);
	IFileIterator* CCL_API newIterator (UrlRef url, int mode = IFileIterator::kAll);
	tbool CCL_API isCaseSensitive ();
	
	// IVolumeFileSystem
	tbool CCL_API getVolumeInfo (VolumeInfo& info, UrlRef rootUrl);
	tbool CCL_API isHiddenFile (UrlRef url);
	tbool CCL_API isWriteProtected (UrlRef url);
	tbool CCL_API moveFile (UrlRef dstPath, UrlRef srcPath, int mode = 0, IProgressNotify* progress = nullptr);
	tbool CCL_API copyFile (UrlRef dstPath, UrlRef srcPath, int mode = 0, IProgressNotify* progress = nullptr);
	
	// INativeFileSystem
	tbool CCL_API getPathType (int& type, UrlRef baseFolder, StringRef fileName);
	tbool CCL_API setFileTime (UrlRef url, const FileTime& modifiedTime);
	ISearcher* CCL_API createSearcher (ISearchDescription& description);
	
protected:
	static constexpr int kVolumeIsCaseSensitive =  1 << 0;
	static constexpr int kVolumeIsCasePreserving = 1 << 1;
	
	// NativeFileSystem
	IStream* openPlatformStream (UrlRef url, int mode);
	bool createPlatformFolder (UrlRef url);
	bool removePlatformFolder (UrlRef url, int mode);
};



//************************************************************************************************
// CocoaFileStream
//************************************************************************************************

class CocoaFileStream: public PosixFileStream
{
public:
	~CocoaFileStream ();
	
	// INativeFileStream
	tbool CCL_API getPath (IUrl& path);

protected:
	NSURL* url;
	bool secureScopeResource;
	bool secureScopeParent;
	friend class CocoaNativeFileSystem;
	
	CocoaFileStream (CocoaNativeFileSystem* fileSystem, void* file = 0, int options = 0, NSURL* url = 0, bool secureScopeResource = false, bool secureScopeParent = false);
};

//************************************************************************************************
// CocoaFileIterator
//************************************************************************************************

class CocoaFileIterator: public NativeFileIterator
{
public:
	CocoaFileIterator (UrlRef url, int mode);
	~CocoaFileIterator ();
	
	// IFileIterator
	const IUrl* CCL_API next ();
};

//************************************************************************************************
// CocoaVolumesIterator
//************************************************************************************************

class CocoaVolumesIterator: public NativeVolumesIterator
{
public:
	CocoaVolumesIterator ();
};

} // namespace CCL

#endif // _ccl_nativefilesystem_cocoa_h
