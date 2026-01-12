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
// Filename    : ccl/extras/web/webelements.h
// Description : Web Elements
//
//************************************************************************************************

#ifndef _ccl_webelements_h
#define _ccl_webelements_h

#include "ccl/app/component.h"

#include "ccl/base/storage/url.h"
#include "ccl/base/collections/objectlist.h"

#include "ccl/public/gui/framework/iview.h"

#include "ccl/public/network/web/ixmlhttprequest.h"
#include "ccl/public/network/web/iwebnewsreader.h"

namespace CCL {
interface IImage;

namespace Web {

interface IWebNewsItem;

//************************************************************************************************
// Element
//************************************************************************************************

class Element: public Component
{
public:
	DECLARE_CLASS (Element, Component)

	Element (StringRef name = nullptr);
	~Element ();

	void setSource (UrlRef path);
	UrlRef getSource () const;

	void startDownload (bool deferred = false);
	void restartDownload (bool deferred = false);
	IAsyncInfo::State getDownloadState () const;

	// Component
	tbool CCL_API getProperty (Variant &var, MemberID propertyId) const override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

protected:
	AutoPtr<IXMLHttpRequest> request;
	Url remotePath;
	double downloadStartTime = -1.;

	virtual void onDownloadCompleted ();
	virtual void onContentNotify (StringID contentType);
	void onRequestEvent (MessageRef msg);
};

//************************************************************************************************
// IImageElementCallback
//************************************************************************************************

interface IImageElementCallback: IUnknown
{
	virtual void onImageDownloadCompleted (IImage* image) = 0;

	DECLARE_IID (IImageElementCallback)
};

//************************************************************************************************
// ImageElementCache
//************************************************************************************************

class ImageElementCache: public Object
{
public:
	DECLARE_CLASS (ImageElementCache, Object)

	static ImageElementCache* create (IImage* image);

	IImage* loadImage () const;

protected:
	Url tempPath;

	~ImageElementCache ();

	bool saveImage (IImage* image);
	void removeImage ();
};

//************************************************************************************************
// PersistentImageCache
//************************************************************************************************

class PersistentImageCache: public Object
{
public:
	DECLARE_CLASS (PersistentImageCache, Object)

	PersistentImageCache (UrlRef basePath = Url (), int timeout = 0, int maxDelay = 0);

	bool saveImage (IImage* image, StringRef name, bool saveThumbnail = false);
	void deleteImage (StringRef name);
	IImage* loadImage (StringRef name) const;

protected:
	Url basePath;
	int timeout;
	int maxDelay;
	
	void getImageLocation (Url& imageLocation, StringRef name, bool highResolution) const;
};

//************************************************************************************************
// ImageElement
//************************************************************************************************

class ImageElement: public Element
{
public:
	DECLARE_CLASS (ImageElement, Element)

	ImageElement (StringRef name = nullptr);

	PROPERTY_POINTER (IImageElementCallback, callback, Callback)
	PROPERTY_MUTABLE_CSTRING (contentType, ContentType)
	PROPERTY_VARIABLE (float, sourceScaleFactor, SourceScaleFactor)

	void setImage (IImage* image);
	void resetImage ();
	bool restoreFromCache (const ImageElementCache* cache);

protected:
	IImageProvider* provider;

	// Element
	tbool CCL_API getProperty (Variant &var, MemberID propertyId) const override;
	void onDownloadCompleted () override;
	void onContentNotify (StringID contentType) override;
};

//************************************************************************************************
// ImageDownloader
//************************************************************************************************

class ImageDownloader: public Component,
					   public ComponentSingleton<ImageDownloader>,
					   public IImageElementCallback
{
public:
	ImageDownloader ();

	bool hasQueuedRequests () const;
	void requestImage (IImageElementCallback* callback, UrlRef url);
	void cancelAll ();

	CLASS_INTERFACE (IImageElementCallback, Component)

protected:
	struct ImageRequest: Object
	{
		Url url;
		SharedPtr<IImageElementCallback> callback;

		ImageRequest (UrlRef url, IImageElementCallback* callback = nullptr)
		: url (url),
		  callback (callback)
		{}
	};

	ObjectList requestQueue;
	ImageElement* helperElement;
	SharedPtr<ImageRequest> currentRequest;

	void triggerNext ();

	// IImageElementCallback
	void onImageDownloadCompleted (IImage* image) override;
};

//************************************************************************************************
// FeedElement
//************************************************************************************************

class FeedElement: public Element
{
public:
	DECLARE_CLASS (FeedElement, Element)

	FeedElement (StringRef name = nullptr, StringID formName = "NewsFeed");

	PROPERTY_MUTABLE_CSTRING (formName, FormName)
	PROPERTY_VARIABLE (int, maxItemCount, MaxItemCount)

protected:
	SharedPtr<IWebNewsFeed> feed;
	ViewPtr feedView;

	bool getItemLink (Url& url, IWebNewsItem& item, StringID relation, int index = 0);

	// Element
	tbool CCL_API getProperty (Variant &var, MemberID propertyId) const override;
	IView* CCL_API createView (StringID name, VariantRef data, const Rect& bounds) override;
	void onDownloadCompleted () override;
};

//************************************************************************************************
// FeedListComponent
//************************************************************************************************

class FeedListComponent: public Component,
						 public ComponentSingleton<FeedListComponent>
{
public:
	DECLARE_CLASS (FeedListComponent, Component)

	FeedListComponent ();

	int addFeed (UrlRef feedUrl, int maxItemCount = -1, StringID formName = nullptr);
	bool reloadFeed (int index, UrlRef feedUrl);

	// Component
	tresult CCL_API appendContextMenu (IContextMenu& contextMenu) override;
	tbool CCL_API interpretCommand (const CommandMsg& msg) override;

protected:
	IParameter* feedList;

	void reloadAll ();
};

} // namespace Web
} // namespace CCL

#endif // _ccl_webelements_h
