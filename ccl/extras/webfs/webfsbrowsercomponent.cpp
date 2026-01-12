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
// Filename    : ccl/extras/webfs/webfsbrowsercomponent.cpp
// Description : WebFS Browser Component
//
//************************************************************************************************

#define DEBUG_LOG 1

#include "ccl/extras/webfs/webfsbrowsercomponent.h"

#include "ccl/extras/web/webelements.h"

#include "ccl/public/system/ifileitem.h"

#include "ccl/public/network/web/iwebfileservice.h"
#include "ccl/public/netservices.h"

using namespace CCL;
using namespace Web;

//************************************************************************************************
// BrowserComponent
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (BrowserComponent, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

BrowserComponent::BrowserComponent (StringRef name, StringRef title)
: Component (name, title),
  itemUpdateInProgress (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API BrowserComponent::attachToBrowser (IWebFileBrowserHost* host, tbool state)
{
	if(state)
		browserHostList.append (host);
	else
		browserHostList.remove (host);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BrowserComponent::syncItemUpdateProgress ()
{
	bool newState = ImageDownloader::instance ().hasQueuedRequests ();
	if(itemUpdateInProgress != newState)
	{
		itemUpdateInProgress = newState;

		if(!browserHostList.isEmpty ())
			ListForEach (browserHostList, IWebFileBrowserHost*, host)
				host->setItemUpdateInProgress (itemUpdateInProgress);
			EndFor

		CCL_PRINTF ("Item update in progress: %s\n", newState ? "true" : "false")
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BrowserComponent::updateItem (UrlRef webfsUrl)
{
	if(!browserHostList.isEmpty ())
		ListForEach (browserHostList, IWebFileBrowserHost*, host)
			host->updateItemInBrowser (webfsUrl);
		EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BrowserComponent::downloadImage (UrlRef pictureUrl, UrlRef webfsUrl)
{
	struct ImageSetter: Object,
						IImageElementCallback
	{
		BrowserComponent& handler;
		Url webfsUrl;

		ImageSetter (BrowserComponent& handler, UrlRef url)
		: handler (handler),
		  webfsUrl (url)
		{}

		// IImageElementCallback
		void onImageDownloadCompleted (IImage* image) override
		{
			CCL_PRINTF ("onImageDownloadCompleted (%s): %s\n", image ? "success" : "no image", 
						MutableCString (UrlDisplayString (webfsUrl)).str ())

			if(image)
				handler.onImageDownloadCompleted (webfsUrl, image);

			handler.syncItemUpdateProgress ();
		}

		CLASS_INTERFACE (IImageElementCallback, Object)
	};

	CCL_PRINTF ("downloadImage: %s, %s\n", 
				MutableCString (UrlDisplayString (pictureUrl)).str (), 
				MutableCString (UrlDisplayString (webfsUrl)).str ())

	AutoPtr<ImageSetter> setter = NEW ImageSetter (*this, webfsUrl);
	ImageDownloader& downloader = Web::ImageDownloader::instance ();
	downloader.requestImage (setter, pictureUrl);
	syncItemUpdateProgress ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API BrowserComponent::triggerThumbnailDownload (IFileDescriptor* webfsItem, UrlRef webfsUrl)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BrowserComponent::cancelImageDownloads ()
{
	ImageDownloader::instance ().cancelAll ();
	syncItemUpdateProgress ();
}
