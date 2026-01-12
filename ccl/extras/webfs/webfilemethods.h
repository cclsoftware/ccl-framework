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
// Filename    : ccl/extras/webfs/webfilemethods.h
// Description : Web File Methods
//
//************************************************************************************************

#ifndef _ccl_webfilemethods_h
#define _ccl_webfilemethods_h

#include "ccl/base/object.h"

#include "ccl/public/system/ifileitem.h"

namespace CCL {

class Container;

namespace Web {

interface ITransfer;

//************************************************************************************************
// FileMethods
//************************************************************************************************

class FileMethods: public Object
{
public:
	DECLARE_CLASS (FileMethods, Object)

	/** Check if object can be downloaded. */
	bool canDownload (UrlRef webfsUrl);

	/** Create IDownloadable for given WebFS file. */
	IDownloadable* createDownloadable (UrlRef webfsUrl);

	/** Collect mounted volumes (UrlWithTitle). */
	void collectVolumes (Container& volumes);

	/** Queue download to default folder. */
	void downloadFile (UrlRef url);
	
	/** Queue download to given destination folder. */
	void downloadFile (UrlRef url, UrlRef dstPath);

	/** Download + install file to local system. */
	void installFile (UrlRef url, IFileDescriptor& descriptor);

	/** Check if files can be uploaded to server volume. */
	bool canUploadToVolume (UrlRef webfsUrl);
	
	/** Check if specific folders can be modified. */
	bool canModifySpecificFolders (UrlRef webfsUrl);

	/** Check if files can be uploaded to given folder. */
	bool canUploadToFolder (UrlRef webfsUrl);

	/** Collect known folders supporting uploads (UrlWithTitle). */
	void collectUploadTargets (Container& targets, UrlRef webfsUrl, int maxDepth = 2);

	/** Check object can be uploaded from given location. */
	bool canUploadFrom (UrlRef path);

	/** Check object can be uploaded from within given folder location. */
	bool canUploadFromFolder (UrlRef path);

	/** Get local upload folder for given WebFS path. */
	bool getUploadFolder (IUrl& path, UrlRef webfsUrl);
	
	/** Queue upload of object. */
	bool uploadObject (UrlRef webfsUrl, UrlRef path);

	/** Check if file can be renamed. */
	bool canRenameFile (UrlRef webfsUrl);

	/** Check if file can be deleted. */
	bool canDeleteFile (UrlRef webfsUrl);

	/** Check if folder can be created. */
	bool canCreateFolder (UrlRef webfsUrl);

	/** Check if two objects are on the same server volume */
	bool isSameVolume (UrlRef source, UrlRef target);

	//** Check if an object only accepts children objects */
	bool acceptsChildrenOnly (UrlRef webfsUrl);

	//** Check if a folder can be moved remotely */
	bool canMoveFolder (UrlRef webfsUrl);
	
	/** Move object on remote volume */
	bool moveObjectToFolder (UrlRef source, UrlRef folder);

protected:
	ITransfer* createDownloadForUrl (UrlRef url, UrlRef dstPath);
};

} // namespace Web
} // namespace CCL

#endif // _ccl_webfilemethods_h
