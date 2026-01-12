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
// Filename    : ccl/public/system/inativefilesystem.h
// Description : Native File System
//
//************************************************************************************************

#ifndef _ccl_inativefilesystem_h
#define _ccl_inativefilesystem_h

#include "ccl/public/system/ifilesystem.h"

#include "ccl/public/text/cclstring.h"

namespace CCL {

interface IProgressNotify;
interface ISearchDescription;
interface ISearcher;

//////////////////////////////////////////////////////////////////////////////////////////////////
// File System Signals
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Signals
{
	/** Signals related to the native file system */
	DEFINE_STRINGID (kFileSystem, "CCL.FileSystem")

		/** (OUT) arg[0]: IUrl. A file is about to be moved or deleted. */
		DEFINE_STRINGID (kReleaseFile, "ReleaseFile")

		/** (OUT) arg[0]: IUrl. A file has been created or found by the application. */
		DEFINE_STRINGID (kFileCreated, "FileCreated")

		/** (OUT) arg[0]: IUrl. A file has been removed from it's original location. */
		DEFINE_STRINGID (kFileRemoved, "FileRemoved")

		/** (OUT) arg[0]: IUrl old path, arg[1] IUrl new path, arg[2] bool success. A file has been moved to a new location. */
		DEFINE_STRINGID (kFileMoved, "FileMoved")

		/** (OUT) arg[0]: IUrl. A file has been modified. */
		DEFINE_STRINGID (kFileChanged, "FileChanged")
}

//************************************************************************************************
// VolumeInfo
/** Basic volume information.
	\ingroup ccl_system */
//************************************************************************************************

struct VolumeInfo
{
	/** Volume type. */
	enum Type
	{
		kUnknown,	///< unknown type
		kLocal,		///< local harddisk
		kRemote,	///< network drive
		kOptical,	///< CD-RW/DVD drive
		kRemovable,	///< removable drive
		kPackage	///< file system within a file
	};

	int type;				///< see VolumeInfo::Type
	int flags;				///< volume flags (defined by file system)
	String subType;			///< volume sub type (defined by file system)
	String label;			///< volume label
	String serialNumber;	///< serial number
	uint64 bytesTotal;		///< total size in bytes
	uint64 bytesFree;		///< number of bytes free

	VolumeInfo ()
	: type (kUnknown),
	  flags (0),
	  bytesTotal (0),
	  bytesFree (0)
	{}
};

//************************************************************************************************
// IVolumeFileSystem
/** File system supporting volumes.
	\ingroup ccl_system */
//************************************************************************************************

interface IVolumeFileSystem: IFileSystem
{
	/** Mode flags for file operations. */
	enum ModeFlags
	{
		kDoNotOverwrite = 1<<0,			///< do not overwrite existing file (move/copy)
		kDoNotMoveAcrossVolumes = 1<<1,	///< do not move across volumes
		kDisableWriteProtection = 1<<2,	///< try to disable write protection (move/copy)

		kSuppressSlowVolumeInfo = 1<<16 ///< passed via VolumeInfo::type member to getVolumeInfo()
	};

	/** Retrieve basic volume information. */
	virtual tbool CCL_API getVolumeInfo (VolumeInfo& info, UrlRef rootUrl) = 0;

	/** Check if file is local. */
	virtual tbool CCL_API isLocalFile (UrlRef url) = 0;

	/** Check if file is hidden. */
	virtual tbool CCL_API isHiddenFile (UrlRef url) = 0;

	/** Check if file is write protected. */
	virtual tbool CCL_API isWriteProtected (UrlRef url) = 0;

	/** Move file or directory. */
	virtual tbool CCL_API moveFile (UrlRef dstPath, UrlRef srcPath, int mode = 0, IProgressNotify* progress = nullptr) = 0;

	/** Copy file. */
	virtual tbool CCL_API copyFile (UrlRef dstPath, UrlRef srcPath, int mode = 0, IProgressNotify* progress = nullptr) = 0;

	DECLARE_IID (IVolumeFileSystem)
};

DEFINE_IID (IVolumeFileSystem, 0xf39998ff, 0x73d, 0x4ea9, 0xb0, 0xff, 0x71, 0x26, 0xd, 0x59, 0x23, 0xf0)

//************************************************************************************************
// INativeFileSystem
/** Native file system interface.
	\ingroup ccl_system */
//************************************************************************************************

interface INativeFileSystem: IVolumeFileSystem
{
	/** File error codes. */
	enum ErrorCodes
	{
		kUnknownError = 1,
		kFileInUse,
		kFileExists,
		kFileNotFound,
		kFileWriteProtected,
		kAccesDenied,
		kNotDirectory,
		kIsDirectory,
		kInvalidArgument,
		kTooManyOpenFiles,
		kOutOfDiscSpace,
		kDirNotEmpty
	};

	/** Mode used in endTransaction(). */
	enum EndTransactionMode
	{
		kCommitTransaction = 1,
		kCommitTransactionWithUndo,
		kCancelTransaction
	};

	/** Determine type of given resource (IUrl::kFile or IUrl::kFolder). */
	virtual tbool CCL_API getPathType (int& type, UrlRef baseFolder, StringRef fileName) = 0;

	/** Set date and time the specified file or directory was last modified. */
	virtual tbool CCL_API setFileTime (UrlRef url, const FileTime& modifiedTime) = 0;

	/** Create file searcher. */
	virtual ISearcher* CCL_API createSearcher (ISearchDescription& description) = 0;

	/** Get current working directory. */
	virtual tbool CCL_API getWorkingDirectory (IUrl& url) = 0;

	/** Set current working directory. */
	virtual tbool CCL_API setWorkingDirectory (UrlRef url) = 0;

	/** Return (and clear) first error occurred in the calling thread. */
	virtual tbool CCL_API getFirstError (int& errorCode) = 0;

	/** Get localized error string for given error code. */
	virtual String CCL_API getErrorString (int errorCode) = 0;

	/** Begin collecting file operations (for supported operations - fails if transaction for calling thread already exists) */
	virtual tbool CCL_API beginTransaction () = 0;

	/** Perform collected file operations */
	virtual tbool CCL_API endTransaction (int mode, IProgressNotify* progress = nullptr) = 0;
	
	DECLARE_IID (INativeFileSystem)
};

DEFINE_IID (INativeFileSystem, 0xc611aa38, 0xa736, 0x41a0, 0x96, 0x56, 0xa4, 0x15, 0x5c, 0x24, 0xc5, 0xc2)

interface IAsyncIOTask;

//************************************************************************************************
// INativeFileStream
/** Interface to native file stream (extends IStream).
	\ingroup ccl_system */
//************************************************************************************************

interface INativeFileStream: IUnknown
{
	/** Options */
	enum Options
	{
		kWriteThru = 1<<8,			///< ignores file system cache, only works with aligned file position and buffer address
		kReadNonBuffered = 1<<9,	///< ignores file system cache, only works with aligned file position and buffer address
		kWriteFlushed = 1<<10
	};

	/** Get native file stream representation (HANDLE on Windows). */
	virtual void* CCL_API getNativeFileStream () = 0;

	/** Set file read/write options */
	virtual void CCL_API setOptions (int options) = 0;

	/** Get file system path (not guaranteed to succeed). */
	virtual tbool CCL_API getPath (IUrl& path) = 0;

	/** Set file size */
	virtual tbool CCL_API setEndOfFile (int64 eof) = 0;

	DECLARE_IID (INativeFileStream)
};

DEFINE_IID (INativeFileStream, 0x25b61bcb, 0x9937, 0x499d, 0x90, 0xaf, 0xb4, 0xa5, 0x8f, 0x8b, 0xea, 0x17)

} // namespace CCL

#endif // _ccl_iextfilesystem_h
