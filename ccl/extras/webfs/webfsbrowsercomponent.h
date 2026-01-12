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
// Filename    : ccl/extras/webfs/webfsbrowsercomponent.h
// Description : WebFS Browser Component
//
//************************************************************************************************

#ifndef _ccl_webfsbrowsercomponent_h
#define _ccl_webfsbrowsercomponent_h

#include "ccl/app/component.h"

#include "ccl/public/extras/iwebfilebrowser.h"

namespace CCL {
namespace Web {

//************************************************************************************************
// BrowserComponent
//************************************************************************************************

class BrowserComponent: public Component,
						public IWebFileBrowserModel
{
public:
	DECLARE_CLASS_ABSTRACT (BrowserComponent, Component)
	
	BrowserComponent (StringRef name = nullptr, StringRef title = nullptr);
	
	virtual void onImageDownloadCompleted (UrlRef webfsUrl, IImage* image) {}

	// IWebFileBrowserModel
	void CCL_API attachToBrowser (IWebFileBrowserHost* host, tbool state) override;
	tbool CCL_API triggerThumbnailDownload (IFileDescriptor* webfsItem, UrlRef webfsUrl) override;
	
	CLASS_INTERFACE (IWebFileBrowserModel, Component)
	
protected:	
	LinkedList<IWebFileBrowserHost*> browserHostList;
	bool itemUpdateInProgress;
	
	void syncItemUpdateProgress ();
	void updateItem (UrlRef webfsUrl);
	void downloadImage (UrlRef pictureUrl, UrlRef webfsUrl);
	void cancelImageDownloads ();
};

} // namespace Web
} // namespace CCL

#endif // _ccl_webfsbrowsercomponent_h

