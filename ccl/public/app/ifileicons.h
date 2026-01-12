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
// Filename    : ccl/public/app/ifileicons.h
// Description : File Icons Interface
//
//************************************************************************************************

#ifndef _ccl_ifileicons_h
#define _ccl_ifileicons_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

class FileType;
interface IImage;
interface IContainer;

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID
{
	/** File icons instance. */
	DEFINE_CID (FileIcons, 0x23d1a134, 0xc4bf, 0x472b, 0xb4, 0xe8, 0xa, 0xec, 0x16, 0x73, 0x25, 0x69);
}

//************************************************************************************************
// IFileIcons
/**	\ingroup app_inter */
//************************************************************************************************

interface IFileIcons: IUnknown
{
	/** File icon flags. */
	enum FileIconFlags
	{
		kOpen = 1<<0,					///< retrieve icon for "open" state
		kNoDefaultFolderIcon = 1<<1		///< do not return the default folder icon
	};

	/** Extra types for volume icons. */
	enum VolumeExtraTypes
	{		
		kVolumeList = -1,	///< type for volume list icon (aka "Computer")
		kPackageList = -2	///< type for package root icon
	};

	/** Create icon for given URL, must be released by caller. */
	virtual IImage* CCL_API createIcon (UrlRef url, int flags = 0) = 0;
	
	/** Create icon for given file type, must be released by caller. */
	virtual IImage* CCL_API createIcon (const FileType& fileType, int flags = 0) = 0;
	
	/** Create icon for given file name, must be released by caller. */
	virtual IImage* CCL_API createIcon (StringRef fileName, int flags = 0) = 0;

	/** Create icon for volume (see VolumeInfo::Type and VolumeExtraTypes), must be released by caller. */
	virtual IImage* CCL_API createVolumeIcon (int type, int flags = 0) = 0;

	/** Assign special icon for given folder location (null to reset). */
	virtual void CCL_API setFolderIcon (UrlRef path, IImage* icon) = 0;

	/** Get default folder icon. */
	virtual IImage* CCL_API getDefaultFolderIcon (tbool open = false) const = 0;

	/** Create an icon copy with content preview. */
	virtual IImage* CCL_API createFolderPreview (IImage* folderIcon, const IContainer& content, int size = 0) const = 0;

	DECLARE_IID (IFileIcons)
};

DEFINE_IID (IFileIcons, 0xec0a58c0, 0x12df, 0x433f, 0xae, 0x26, 0x43, 0x80, 0x4c, 0x64, 0xfe, 0x33)

} // namespace CCL

#endif // _ccl_ifileicons_h
