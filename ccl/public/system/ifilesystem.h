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
// Filename    : ccl/public/system/ifilesystem.h
// Description : File System Interface
//
//************************************************************************************************

#ifndef _ccl_ifilesystem_h
#define _ccl_ifilesystem_h

#include "ccl/public/base/istream.h"
#include "ccl/public/base/datetime.h"

namespace CCL {

/** File time type. */
typedef DateTime FileTime;

//************************************************************************************************
// File System Macros
//************************************************************************************************

/** Helper macro for file iteration.
    \ingroup base_io */
#define ForEachFile(createIter, path) \
{ CCL::AutoPtr<CCL::IFileIterator> __iter = createIter; \
  const CCL::IUrl* path; \
  if(__iter) while((path = __iter->next ()) != nullptr) {

//************************************************************************************************
// FileInfo
/** Basic file information.
	\ingroup base_io  */
//************************************************************************************************

struct FileInfo
{
	int flags;				///< file flags (defined by file system)
	int64 fileSize;			///< file size
	FileTime createTime;	///< time of creation of file (local time)
	FileTime modifiedTime;	///< time of last modification of file (local time)
	FileTime accessTime;	///< time of last access of file (local time)

	FileInfo ()
	: flags (0),
	  fileSize (0)
	{}
};

//************************************************************************************************
// IFileIterator
/** Interface for file iteration.
	\ingroup base_io  */
//************************************************************************************************

interface IFileIterator: IUnknown
{
	enum Modes
	{
		kFiles = 1<<0,				///< iterate files
		kFolders = 1<<1,			///< iterate folders
		kAll = kFiles|kFolders,		///< iterate files + folders
		kBundlesAsFiles = 1<<2,		///< do not descend into bundles
		kIgnoreHidden = 1<<3		///< ignore hidden files/folders
	};

	/** Returns next URL or null to stop iteration. */
	virtual const IUrl* CCL_API next () = 0;

	DECLARE_IID (IFileIterator)
};

DEFINE_IID (IFileIterator, 0x1fa13b4d, 0x3736, 0x4cbf, 0xab, 0xf2, 0xc, 0x8d, 0x80, 0x86, 0xce, 0xdc)

//************************************************************************************************
// IFileSystem
/** File system interface
	\ingroup base_io  */
//************************************************************************************************

interface IFileSystem: IUnknown
{
	/** Mode flags for file operations. */
	enum ModeFlags
	{
		kDeleteRecursively = 1<<0,	///< flag for removeFolder()
		kDeleteToTrashBin  = 1<<1	///< flag for removeFile() and removeFolder()
	};

	/** Open stream with given location and mode. */
	virtual IStream* CCL_API openStream (UrlRef url, int mode = IStream::kOpenMode, IUnknown* context = nullptr) = 0;
	
	/** Check if file or folder exists. */
	virtual tbool CCL_API fileExists (UrlRef url) = 0;

	/** Retrieve file information. */
	virtual tbool CCL_API getFileInfo (FileInfo& info, UrlRef url) = 0;

	/** Delete file physically. */
	virtual tbool CCL_API removeFile (UrlRef url, int mode = 0) = 0;

	/** Rename file or directory. */
	virtual tbool CCL_API renameFile (UrlRef url, StringRef newName, int mode = 0) = 0;

	/** Create new file iterator for given folder location. */
	virtual IFileIterator* CCL_API newIterator (UrlRef url, int mode = IFileIterator::kAll) = 0;

	/** Create new folder. */
	virtual tbool CCL_API createFolder (UrlRef url) = 0;

	/** Delete folder physically. */
	virtual tbool CCL_API removeFolder (UrlRef url, int mode = 0) = 0;

	/** Check if the filesystem is case sensitive. */
	virtual tbool CCL_API isCaseSensitive () = 0;

	DECLARE_IID (IFileSystem)
};

DEFINE_IID (IFileSystem, 0x3e510860, 0x30ad, 0x4f70, 0xa9, 0x33, 0x6b, 0xc, 0x1d, 0xc4, 0x13, 0x28)

//************************************************************************************************
// AbstractFileSystem
/** Mix-in base class for file systems not implementing all features.
	\ingroup base_io  */
//************************************************************************************************

class AbstractFileSystem: public IFileSystem
{
public:
	// IFileSystem
	IStream* CCL_API openStream (UrlRef url, int mode = IStream::kOpenMode, IUnknown* context = nullptr) override { return nullptr; }
	tbool CCL_API fileExists (UrlRef url) override { return false; }
	tbool CCL_API getFileInfo (FileInfo& info, UrlRef url) override { return false; }
	tbool CCL_API removeFile (UrlRef url, int mode = 0) override { return false; }
	tbool CCL_API renameFile (UrlRef url, StringRef newName, int mode = 0) override { return false; }
	IFileIterator* CCL_API newIterator (UrlRef url, int mode = IFileIterator::kAll) override { return nullptr; }
	tbool CCL_API createFolder (UrlRef url) override { return false; }
	tbool CCL_API removeFolder (UrlRef url, int mode = 0) override { return false; }
	tbool CCL_API isCaseSensitive () override { return true; }
};

} // namespace CCL

#endif // _ccl_ifilesystem_h
