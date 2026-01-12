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
// Filename    : ccl/system/nativefilesystem.h
// Description : Native File System
//
//************************************************************************************************

#ifndef _ccl_nativefilesystem_h
#define _ccl_nativefilesystem_h

#include "ccl/public/base/unknown.h"
#include "ccl/public/storage/iurl.h"
#include "ccl/public/base/istream.h"
#include "ccl/public/system/inativefilesystem.h"

namespace CCL {

class Container;
class Iterator;
class Object;

//************************************************************************************************
// NativeFileSystem
/** The native file system. */
//************************************************************************************************

class NativeFileSystem:	public Unknown,
						public INativeFileSystem
{
public:
	static NativeFileSystem& instance ();

	// IFileSystem
	IStream* CCL_API openStream (UrlRef url, int mode = IStream::kOpenMode, IUnknown* context = nullptr) override;
	tbool CCL_API fileExists (UrlRef url) override;
	tbool CCL_API getFileInfo (FileInfo& info, UrlRef url) override;
	tbool CCL_API removeFile (UrlRef url, int mode = 0) override;
	tbool CCL_API renameFile (UrlRef url, StringRef newName, int mode = 0) override;
	IFileIterator* CCL_API newIterator (UrlRef url, int mode = IFileIterator::kAll) override;
	tbool CCL_API createFolder (UrlRef url) override;
	tbool CCL_API removeFolder (UrlRef url, int mode = 0) override;
	tbool CCL_API isCaseSensitive () override;

	// IVolumeFileSystem
	tbool CCL_API getVolumeInfo (VolumeInfo& info, UrlRef rootUrl) override;
	tbool CCL_API isLocalFile (UrlRef url) override;
	tbool CCL_API isHiddenFile (UrlRef url) override;
	tbool CCL_API isWriteProtected (UrlRef url) override;

	// INativeFileSystem
	tbool CCL_API getPathType (int& type, UrlRef baseFolder, StringRef fileName) override;
	tbool CCL_API moveFile (UrlRef dstPath, UrlRef srcPath, int mode = 0, IProgressNotify* progress = nullptr) override;
	tbool CCL_API copyFile (UrlRef dstPath, UrlRef srcPath, int mode = 0, IProgressNotify* progress = nullptr) override;
	tbool CCL_API setFileTime (UrlRef url, const FileTime& modifiedTime) override;
	ISearcher* CCL_API createSearcher (ISearchDescription& description) override;
	tbool CCL_API getWorkingDirectory (IUrl& url) override;
	tbool CCL_API setWorkingDirectory (UrlRef url) override;
	tbool CCL_API getFirstError (int& errorCode) override;
	String CCL_API getErrorString (int errorCode) override;
	tbool CCL_API beginTransaction () override;
	tbool CCL_API endTransaction (int mode, IProgressNotify* progress = nullptr) override;

	CLASS_INTERFACE3 (IFileSystem, IVolumeFileSystem, INativeFileSystem, Unknown)

protected:
	friend class FileStream;

	NativeFileSystem ();

	tbool createParentFolder (UrlRef url);
	void onError (int errorCode, const IUrl* url = nullptr);			///< sets first error if not done yet, calls ccl_raise 
	void onNativeError (int nativeError, const IUrl* url = nullptr);	///< translates error code first
	void checkFirstError (int errorCode);
	String getErrorString (int errorCode, const IUrl* url, int* nativeError = nullptr);
	void setTransaction (Object* transaction);
	Object* getTransaction () const;

	// to be implemented by platform subclass:
	virtual IStream* openPlatformStream (UrlRef url, int mode);
	virtual bool createPlatformFolder (UrlRef url);
	virtual bool removePlatformFolder (UrlRef url, int mode);
	virtual int translateNativeError (int nativeError);	///< translate platform error code to CCL::NativeFileSystem code
};

//************************************************************************************************
// FileStream
/** A native file stream. */
//************************************************************************************************

class FileStream: public Unknown,
				  public IStream,
				  public INativeFileStream
{
public:
	// IStream
	int CCL_API read (void* buffer, int size) override;
	int CCL_API write (const void* buffer, int size) override;
	int64 CCL_API tell () override;
	tbool CCL_API isSeekable () const override;
	int64 CCL_API seek (int64 pos, int mode) override;

	// INativeFileStream
	void* CCL_API getNativeFileStream () override;
	void CCL_API setOptions (int options) override;
	tbool CCL_API getPath (IUrl& path) override;
	tbool CCL_API setEndOfFile (int64 eof) override;

	CLASS_INTERFACE2 (IStream, INativeFileStream, Unknown)

protected:
	friend class NativeFileSystem;
	NativeFileSystem* fileSystem;
	void* file;
	int options;

	FileStream (NativeFileSystem* fileSystem, void* file = nullptr, int options = 0);

	void onNativeError (int nativeError, const IUrl* url = nullptr);
};

//************************************************************************************************
// NativeFileIterator
/** Native file system iterator. */
//************************************************************************************************

class NativeFileIterator: public Unknown,
						  public IFileIterator
{
public:
	NativeFileIterator (UrlRef url, int mode);
	~NativeFileIterator ();

	// IFileIterator
	const IUrl* CCL_API next () override;

	CLASS_INTERFACE (IFileIterator, Unknown)

protected:
	int mode;
	IUrl* baseUrl;
	IUrl* current;
	void* iter;
};

//************************************************************************************************
// NativeVolumesIterator
/** Iterator for volumes (disk drives). */
//************************************************************************************************

class NativeVolumesIterator: public Unknown,
							 public IFileIterator
{
public:
	NativeVolumesIterator ();
	~NativeVolumesIterator ();

	// IFileIterator
	const IUrl* CCL_API next () override;

	CLASS_INTERFACE (IFileIterator, Unknown)

protected:
	Container* volumes;
	Iterator* iter;

	void construct ();
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline void FileStream::onNativeError (int nativeError, const IUrl* url)
{ return fileSystem->onNativeError (nativeError, url); }

} // namespace CCL

#endif // _ccl_nativefilesystem_h
