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
// Filename    : ccl/public/system/ifileitem.h
// Description : File Item Interfaces
//
//************************************************************************************************

#ifndef _ccl_ifileitem_h
#define _ccl_ifileitem_h

#include "ccl/public/base/datetime.h"

namespace CCL {

class FileType;
interface IAttributeList;
interface IProgressNotify;

//************************************************************************************************
// IFileDescriptor
/** Representation of a file which might not exist locally yet.
	\ingroup base_io  */
//************************************************************************************************

interface IFileDescriptor: IUnknown
{
	/** Get beautified title for display. */
	virtual tbool CCL_API getTitle (String& title) const = 0;

	/** Get file name. */
	virtual tbool CCL_API getFileName (String& fileName) const = 0;
	
	/** Get fully qualified file type. */
	virtual tbool CCL_API getFileType (FileType& fileType) const = 0;	

	/** Get file size. */
	virtual tbool CCL_API getFileSize (int64& fileSize) const = 0;

	/** Get file time (last modified). */
	virtual tbool CCL_API getFileTime (DateTime& fileTime) const = 0;

	/** Get associated meta information (optional). */
	virtual tbool CCL_API getMetaInfo (IAttributeList& a) const = 0;

	// Known meta info
	DECLARE_STRINGID_MEMBER (kFileCreatorName)
	DECLARE_STRINGID_MEMBER (kFilePermissions)
	DECLARE_STRINGID_MEMBER (kAlternativeUrl)

	DECLARE_IID (IFileDescriptor)
};

DEFINE_IID (IFileDescriptor, 0x3834d15, 0x8525, 0x4fb8, 0x84, 0xab, 0x1a, 0x4d, 0x28, 0x94, 0xf8, 0xc7)
DEFINE_STRINGID_MEMBER (IFileDescriptor, kFileCreatorName, "File::CreatorName")
DEFINE_STRINGID_MEMBER (IFileDescriptor, kFilePermissions, "File::Permissions")
DEFINE_STRINGID_MEMBER (IFileDescriptor, kAlternativeUrl, "File::AlternativeUrl")

//************************************************************************************************
// IFilePromise
/** A file promise is able to create a file at the given destination path.
	\ingroup base_io  */
//************************************************************************************************

interface IFilePromise: IFileDescriptor
{
	/**	Check if file is created asynchronously. 
		In this case the file is not guaranteed to exist after createFile(). */
	virtual tbool CCL_API isAsync () const = 0;
	
	/** Create the file. */
	virtual tresult CCL_API createFile (UrlRef destPath, IProgressNotify* progress = nullptr) = 0;

	DECLARE_IID (IFilePromise)
};

DEFINE_IID (IFilePromise, 0x6020167, 0xc3da, 0x4456, 0xa8, 0x4c, 0x7c, 0x29, 0xd0, 0x18, 0x5c, 0x8b)

//************************************************************************************************
// IDownloadable
/** Represents a file that can be retrieved from a remote system. 	
	\ingroup base_io  */
//************************************************************************************************

interface IDownloadable: IUnknown
{
	/** Location where the file can be retrieved from. */
	virtual UrlRef CCL_API getSourceUrl () const = 0;

	DECLARE_IID (IDownloadable)
};

DEFINE_IID (IDownloadable, 0x6636537b, 0xb371, 0x4129, 0x95, 0xdd, 0x58, 0xeb, 0x8b, 0xf1, 0x91, 0x59)

//************************************************************************************************
// IFileItemProvider
/** \ingroup base_io  */
//************************************************************************************************

interface IFileItemProvider: IUnknown
{
	/** Open file descriptor for given location (can be null, must be released otherwise). */
	virtual IFileDescriptor* CCL_API openFileItem (UrlRef url) = 0;

	DECLARE_IID (IFileItemProvider)
};

DEFINE_IID (IFileItemProvider, 0xec71934d, 0xac6d, 0x40e1, 0xaa, 0x49, 0xcc, 0x2, 0xbf, 0x9f, 0xb9, 0x43)

} // namespace CCL

#endif // _ccl_ifileitem_h
