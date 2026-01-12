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
// Filename    : ccl/public/extras/iwebfilebrowser.h
// Description : Web File Browser Interfaces
//
//************************************************************************************************

#ifndef _ccl_iwebfilebrowser_h
#define _ccl_iwebfilebrowser_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

interface IImage;
interface IUrlFilter;
interface IFileDescriptor;

namespace Web {

interface ITransfer;

//************************************************************************************************
// IWebFileBrowserHost
/** Callback interface to hosting browser view. */
//************************************************************************************************

interface IWebFileBrowserHost: IUnknown
{
	/** Update WebFS item in browser view. */
	virtual void CCL_API updateItemInBrowser (UrlRef webfsUrl) = 0;

	/** Model is still updating, i.e. host can show a progress indicator. */
	virtual void CCL_API setItemUpdateInProgress (tbool state) = 0;

	DECLARE_IID (IWebFileBrowserHost)
};

DEFINE_IID (IWebFileBrowserHost, 0xfbcbf2ab, 0x427c, 0x42d3, 0xbd, 0x84, 0x79, 0x5d, 0x88, 0x4e, 0x68, 0x14)

//************************************************************************************************
// IWebFileBrowserModel
/** Interface for customization of web volumes in a browser view. */
//************************************************************************************************

interface IWebFileBrowserModel: IUnknown
{
	/** Attach or detach from browser, can be multiple! */
	virtual void CCL_API attachToBrowser (IWebFileBrowserHost* host, tbool state) = 0;

	/** Get icon for WebFS volume. */
	virtual IImage* CCL_API getVolumeIcon (UrlRef webfsUrl) = 0;

	/** Get icon for specified WebFS item. */
	virtual IImage* CCL_API getItemIcon (IFileDescriptor* webfsItem) = 0;

	/** Get thumbnail for specified WebFS item. */
	virtual IImage* CCL_API getItemThumbnail (IFileDescriptor* webfsItem) = 0;

	/** Get URL filter. */
	virtual IUrlFilter* CCL_API getUrlFilter () = 0;
	
	/** Get custom background identifier. */
	virtual StringID CCL_API getCustomBackground (UrlRef webfsUrl) = 0;

	/** Called on single or double click/enter/return key on WebFS volume. */
	virtual tbool CCL_API onOpenVolume (UrlRef webfsUrl, tbool isEdit) = 0;

	/** Directory has been expanded in browser view. */
	virtual void CCL_API onDirectoryExpanded (UrlRef webfsUrl) = 0;

	/** Called by host to trigger download of thumbnail image for specified WebFS item. 
		Model needs to call updateItemInBrowser() asynchronously when done. */
	virtual tbool CCL_API triggerThumbnailDownload (IFileDescriptor* webfsItem, UrlRef webfsUrl) = 0;

	DECLARE_IID (IWebFileBrowserModel)
};

DEFINE_IID (IWebFileBrowserModel, 0xe53852ad, 0x9ff1, 0x42e4, 0x9b, 0xd3, 0xb4, 0xb3, 0xb6, 0x86, 0xb7, 0x9a)

//************************************************************************************************
// IUploader
/** Interface to customize file uploads. */
//************************************************************************************************

interface IUploader: IUnknown
{
	/** Create transfer to upload local file to given server location. */
	virtual ITransfer* CCL_API createTransferForUpload (UrlRef webfsUrl, UrlRef localFile) = 0;

	DECLARE_IID (IUploader)
};

DEFINE_IID (IUploader, 0x9d081919, 0x3d6, 0x4a84, 0x87, 0xd1, 0xb5, 0x1d, 0x62, 0x77, 0xf6, 0x28)

} // namespace Web
} // namespace CCL

#endif // _ccl_iwebfilebrowser_h
