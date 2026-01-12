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
// Filename    : ccl/public/system/ifilemanager.h
// Description : File Manager Interface
//
//************************************************************************************************

#ifndef _ccl_ifilemanager_h
#define _ccl_ifilemanager_h

#include "ccl/public/base/iunknown.h"
#include "ccl/public/text/cstring.h"

namespace CCL {

interface IAsyncOperation;

//////////////////////////////////////////////////////////////////////////////////////////////////
// File Location Types
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace FileLocationType
{
	// File Location Types
	DEFINE_STRINGID (kDocuments, "documents")				
	DEFINE_STRINGID (kICloud, "icloud") 
	DEFINE_STRINGID (kDropBox, "dropbox")
	DEFINE_STRINGID (kGoogleDrive, "googledrive")
	DEFINE_STRINGID (kOneDrive, "onedrive")
	DEFINE_STRINGID (kCloud, "cloud") // other cloud type
	DEFINE_STRINGID (kOther, "other")	
}

//************************************************************************************************
// IFileManager
/** File manager interface.
	\ingroup ccl_system */
//************************************************************************************************

interface IFileManager: IUnknown
{
	/** Flags for addWatchedLocation(). */
	enum Flags
	{
		kDeep = 1<<0, ///< for folders: also monitor contained sub-folders and files
	};

	/** Add location to be watched (file or folder). Generate Signals::kFileSystem messages for given url */
	virtual tresult CCL_API addWatchedLocation (UrlRef url, int flags = kDeep) = 0;

	/** Remove location to be watched. */
	virtual tresult CCL_API removeWatchedLocation (UrlRef url) = 0;

	/** Report that the application is using the given file or folder. 
	    A used file should not be modified. 
		If the application is using a folder, it should be granted write access. */
	virtual tresult CCL_API setFileUsed (UrlRef url, tbool state) = 0;

	/** Report that the application is writing the file. */
	virtual tresult CCL_API setFileWriting (UrlRef url, tbool state) = 0;

	/** Trigger file updates (download from server etc).  */
	virtual IAsyncOperation* CCL_API triggerFileUpdate (UrlRef url) = 0;

	/** Convert file URL to user-friendly display representation, type is IUrl::StringType. */
	virtual tbool CCL_API getFileDisplayString (String& string, UrlRef url, int type) const = 0; 

	/** Classify file or folder location. /see FileLocationType */
	virtual StringID CCL_API getFileLocationType (UrlRef url) const = 0; 

	/** Exit all threads and cleanup. */
	virtual void CCL_API terminate () = 0;
	
	DECLARE_IID (IFileManager)

	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Check if location type is a cloud location */
	bool isCloudLocationType (StringID type) const;
};

DEFINE_IID (IFileManager, 0xc9e5ddd9, 0x7517, 0x42c7, 0xb2, 0xad, 0xa6, 0xf3, 0x10, 0x6, 0x65, 0xe7)

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool IFileManager::isCloudLocationType (StringID id) const
{
	return id == FileLocationType::kICloud 
	    || id == FileLocationType::kDropBox 
		|| id == FileLocationType::kGoogleDrive 
		|| id == FileLocationType::kOneDrive 
		|| id == FileLocationType::kCloud;
}

} // namespace CCL

#endif // _ccl_ifilemanager_h
