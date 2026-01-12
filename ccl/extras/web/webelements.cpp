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
// Filename    : ccl/extras/web/webelements.cpp
// Description : Web Elements
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/extras/web/webelements.h"

#include "ccl/app/params.h"
#include "ccl/app/utilities/imagefile.h"
#include "ccl/app/utilities/imagebuilder.h"

#include "ccl/base/message.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/text/istringdict.h"
#include "ccl/public/base/istream.h"
#include "ccl/public/system/formatter.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/system/iprotocolhandler.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/iform.h"
#include "ccl/public/gui/framework/iwindow.h"
#include "ccl/public/gui/graphics/dpiscale.h"
#include "ccl/public/network/web/httpstatus.h"
#include "ccl/public/network/web/iwebservice.h"
#include "ccl/public/network/web/iwebrequest.h"
#include "ccl/public/netservices.h"
#include "ccl/public/plugservices.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/guiservices.h"

using namespace CCL;
using namespace Web;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("Feed")
	XSTRING (ReloadFeed, "Refresh")
END_XSTRINGS

//************************************************************************************************
// Element
//************************************************************************************************

static const CString kStartDownload = CSTR ("startDownload");
static const CString kRestartDownload = CSTR ("restartDownload");
DEFINE_CLASS_HIDDEN (Element, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

Element::Element (StringRef name)
: Component (name),
  request (ccl_new<IXMLHttpRequest> (ClassID::XMLHttpRequest))
{
	signalSlots.advise (UnknownPtr<ISubject> (request), nullptr, this, &Element::onRequestEvent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Element::~Element ()
{
	signalSlots.unadvise (UnknownPtr<ISubject> (request));
	cancelSignals ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Element::setSource (UrlRef path)
{
	remotePath = path;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

UrlRef Element::getSource () const
{
	return remotePath;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Element::startDownload (bool deferred)
{	
	if(deferred)
		(NEW Message (kStartDownload))->post (this, -1);
	else
	{
		if(!remotePath.isEmpty () && request->getState () == IAsyncInfo::kNone)
		{
			request->open (HTTP::kGET, remotePath);
			request->send ();
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Element::restartDownload (bool deferred)
{
	if(deferred)
		(NEW Message (kRestartDownload))->post (this, -1);
	else
	{
		request->abort ();
		downloadStartTime = -1.;
		startDownload ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncInfo::State Element::getDownloadState () const
{
	return request->getState ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Element::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "downloadState")
	{
		var = getDownloadState ();
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Element::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kStartDownload)
	{
		startDownload (false);
	}
	else if(msg == kRestartDownload)
	{
		restartDownload (false);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Element::onDownloadCompleted ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Element::onContentNotify (StringID contentType)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Element::onRequestEvent (MessageRef msg)
{
	if(msg == IXMLHttpRequest::kOnProgress)
	{
		// start time measurement
		if(downloadStartTime == -1.)
		{
			downloadStartTime = System::GetProfileTime ();
			CCL_PRINTF ("Download of '%s' in progress...\n", MutableCString (UrlFullString (remotePath, true)).str ())
		}
	}
	else if(msg == IXMLHttpRequest::kOnLoadEnd)
	{
		if(IStream* responseStream = request->getResponseStream ())
			responseStream->rewind ();

		CCL_PRINTF ("Download of '%s' took %.3lf sec\n", 
					MutableCString (UrlFullString (remotePath, true)).str (), 
					System::GetProfileTime () - downloadStartTime)

		onDownloadCompleted ();
	}
	else if(msg == IXMLHttpRequest::kOnReadyStateChange)
	{
		IXMLHttpRequest::ReadyState state = request->getReadyState ();
		if(state == IXMLHttpRequest::kHeadersReceived)
		{
			MutableCString contentType;
			if(request->getResponseHeader (contentType, Meta::kContentType) == kResultOk)
				onContentNotify (contentType);
		}
	}

	signal (Message (kPropertyChanged));
}

//************************************************************************************************
// IImageElementCallback
//************************************************************************************************

DEFINE_IID_ (IImageElementCallback, 0xc1e01403, 0x41ed, 0x4862, 0x88, 0x3c, 0x80, 0x49, 0x6b, 0x21, 0xdb, 0xbe)

//************************************************************************************************
// ImageElementCache
//************************************************************************************************

ImageElementCache* ImageElementCache::create (IImage* image)
{
	AutoPtr<ImageElementCache> cache = NEW ImageElementCache;
	if(cache->saveImage (image))
		return cache.detach ();
	else
		return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (ImageElementCache, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

ImageElementCache::~ImageElementCache ()
{
	removeImage ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ImageElementCache::removeImage ()
{
	if(!tempPath.isEmpty ())
	{
		System::GetFileSystem ().removeFile (tempPath);
		tempPath.assign (Url::kEmpty);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ImageElementCache::saveImage (IImage* image)
{
	ASSERT (image != nullptr)
	removeImage ();

	static const String kFileName ("ImageElement");
	System::GetFileUtilities ().makeUniqueTempFile (tempPath, kFileName);
	return ImageFile (ImageFile::kPNG, image).saveToFile (tempPath);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* ImageElementCache::loadImage () const
{
	if(!tempPath.isEmpty ())
	{
		ImageFile imageFile;
		if(imageFile.loadFromFile (tempPath))
			return return_shared (imageFile.getImage ());
	}
	return nullptr;
}

//************************************************************************************************
// PersistentImageCache
//************************************************************************************************

DEFINE_CLASS_HIDDEN (PersistentImageCache, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

PersistentImageCache::PersistentImageCache (UrlRef basePath, int timeout, int maxDelay)
: basePath (basePath),
  timeout (timeout),
  maxDelay (maxDelay)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PersistentImageCache::saveImage (IImage* image, StringRef name, bool saveThumbnail)
{
	if(image == nullptr)
		return false;

	bool highResolution = ImageBuilder::isHighResolutionImageNeeded ();

	ImageFile imageFile (ImageFile::kPNG, image);
	if(saveThumbnail)
	{
		AutoPtr<IImage> thumbnail = ImageBuilder::createThumbnail (image, highResolution ? 2.f : 1.f, ImageBuilder::kKeepAspectRatio);
		imageFile.setImage (thumbnail);
	}

	Url url;
	getImageLocation (url, name, highResolution);
	
	return imageFile.saveToFile (url);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PersistentImageCache::getImageLocation (Url& imageLocation, StringRef name, bool highResolution) const
{
	imageLocation = basePath;
	imageLocation.descend (highResolution ? String (name).append ("@2x") : name, IUrl::kFile);
	FileType fileType;
	ImageFile (ImageFile::kPNG).getFormat (fileType);
	imageLocation.setFileType (fileType, false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* PersistentImageCache::loadImage (StringRef name) const
{
	SharedPtr<IImage> result = nullptr;

	bool highResolution = ImageBuilder::isHighResolutionImageNeeded ();
	ImageFile imageFile (ImageFile::kPNG);
	
	Url url;
	getImageLocation (url, name, highResolution);

	if(System::GetFileSystem ().fileExists (url))
	{
		AutoPtr<IStream> stream = System::GetFileSystem ().openStream (url, IStream::kOpenMode);
		if(stream == nullptr)
			return nullptr;

		if(imageFile.load (*stream))
			result = imageFile.getImage ();

		FileInfo info;
		System::GetFileSystem ().getFileInfo (info, url);

		if(timeout > 0)
		{
			int64 now = UnixTime::getTime ();
			int additionalTimeout = name.getHashCode () % maxDelay;
			if(UnixTime::fromLocal (info.modifiedTime) < now - (timeout + additionalTimeout) * DateTime::kSecondsInDay)
			{
				System::GetFileSystem ().removeFile (url);
				result.release ();
			}
		}
	}

	return result.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PersistentImageCache::deleteImage (StringRef name)
{
	bool highResolution = ImageBuilder::isHighResolutionImageNeeded ();

	Url url;
	getImageLocation (url, name, highResolution);

	if(System::GetFileSystem ().fileExists (url))
		System::GetFileSystem ().removeFile (url);
}

//************************************************************************************************
// ImageElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ImageElement, Element)

//////////////////////////////////////////////////////////////////////////////////////////////////

ImageElement::ImageElement (StringRef name)
: Element (name),
  provider (nullptr),
  callback (nullptr),
  sourceScaleFactor (1.f)
{
	provider = paramList.addImage (CSTR ("image"));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ImageElement::setImage (IImage* image)
{
	request->abort ();
	provider->setImage (image);
	signal (Message (kPropertyChanged));
	if(image != nullptr)
		contentType = ImageFile::kPNG;
	else
		contentType.empty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ImageElement::resetImage ()
{
	setImage (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ImageElement::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "hasImage")
	{
		var = provider->getImage () != nullptr;
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ImageElement::onDownloadCompleted ()
{
	// load image
	AutoPtr<IImage> image;
	IStream* stream = request->getResponseStream ();
	ASSERT (stream != nullptr)
	if(stream)
	{
		// try to load hi-res image for local files
		if(System::GetFileSystem ().isLocalFile (getSource ()))
		{
			image = ImageFile::loadImage (getSource ());
		}
		else
		{
			// try MIME type first
			FileType type;
			if(!contentType.isEmpty ())
				if(const FileType* knownType = ImageFile::getFormatByMimeType (contentType))
					type = *knownType;
		
			if(!type.isValid ())
				type = remotePath.getFileType ();

			// check if we received data from the server
			ASSERT (stream->isSeekable ())
			int64 byteSize = stream->seek (0, IStream::kSeekEnd);
			stream->seek (0, IStream::kSeekSet);

			image = byteSize > 0 ? ImageFile::loadImage (*stream, type) : nullptr;

			// make sure DPI scaling is applied correctly
			if(image && sourceScaleFactor > 1.f)
			{
				int width = DpiScale::pixelToCoord (image->getWidth (), sourceScaleFactor);
				int height = DpiScale::pixelToCoord (image->getHeight (), sourceScaleFactor);
				image = ImageBuilder::createSizedImage (image, width, height, sourceScaleFactor);
			}
		}

		#if DEBUG_LOG
		if(image == 0)
			CCL_PRINTF ("Failed to load image '%s'\n", MutableCString (UrlFullString (remotePath, true)).str ())
		#endif
	}

	provider->setImage (image);

	// issue callback
	if(callback)
		callback->onImageDownloadCompleted (image);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ImageElement::restoreFromCache (const ImageElementCache* cache)
{
	if(cache != nullptr)
		if(AutoPtr<IImage> image = cache->loadImage ())
		{
			setImage (image);
			return true;
		}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ImageElement::onContentNotify (StringID type)
{
	setContentType (type);
}

//************************************************************************************************
// ImageDownloader
//************************************************************************************************

DEFINE_COMPONENT_SINGLETON (ImageDownloader)

//////////////////////////////////////////////////////////////////////////////////////////////////

ImageDownloader::ImageDownloader ()
: Component ("ImageDownloader"),
  helperElement (nullptr)
{
	requestQueue.objectCleanup (true);

	addComponent (helperElement = NEW ImageElement ("Helper"));
	helperElement->setCallback (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ImageDownloader::hasQueuedRequests () const
{
	return !requestQueue.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ImageDownloader::requestImage (IImageElementCallback* callback, UrlRef url)
{
	requestQueue.add (NEW ImageRequest (url, callback));

	if(helperElement->getDownloadState () != IAsyncInfo::kStarted)
		triggerNext ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ImageDownloader::cancelAll ()
{
	requestQueue.removeAll ();
	currentRequest.release ();
	helperElement->resetImage ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ImageDownloader::triggerNext ()
{
	ASSERT (helperElement->getDownloadState () != IAsyncInfo::kStarted)
	if(AutoPtr<ImageRequest> request = (ImageRequest*)requestQueue.removeFirst ())
	{
		currentRequest = request;

		helperElement->setSource (request->url);
		helperElement->restartDownload ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ImageDownloader::onImageDownloadCompleted (IImage* image)
{
	if(currentRequest)
		currentRequest->callback->onImageDownloadCompleted (image);
	currentRequest.release ();

	triggerNext ();
}

//************************************************************************************************
// FeedElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (FeedElement, Element)

//////////////////////////////////////////////////////////////////////////////////////////////////

FeedElement::FeedElement (StringRef name, StringID formName)
: Element (name),
  formName (formName),
  maxItemCount (-1)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FeedElement::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "itemCount")
	{
		int itemCount = feed ? feed->countItems () : 0;
		if(maxItemCount > 0) // limit max. number of displayed items
			ccl_upper_limit (itemCount, maxItemCount);
		var = itemCount;
		return true;
	}
	else if(propertyId.startsWith (Feed::kCategoryTerm))
	{
		if(feed)
		{
			int64 index = -1;
			propertyId.subString (Feed::kCategoryTerm.length ()).getIntValue (index);
			if(IWebNewsItem* item = feed->getItem ((int)index))
			{
				String category (item->getAttribute (Feed::kCategoryTerm));
				var = category;
				var.share ();
			}
		}
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API FeedElement::createView (StringID name, VariantRef data, const Rect& bounds)
{
	if(name == "FeedView")
	{
		ASSERT (!formName.isEmpty ())

		ITheme* theme = getTheme ();
		ASSERT (theme != nullptr)
		feedView = theme ? theme->createView (formName, asUnknown ()) : nullptr;

		startDownload (true);
		return feedView;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FeedElement::getItemLink (Url& url, IWebNewsItem& item, StringID relation, int index)
{
	url = Url ();

	String string;
	if(IWebNewsLink* link = item.getLink (relation, index))
		string = link->getAttribute (Feed::kHRef);

	if(!string.isEmpty ())
	{
		Url parent (getSource ());
		parent.ascend ();
		url.fromRelativePath (string, parent);
		return true;
	}
	else
		return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FeedElement::onDownloadCompleted ()
{
	// load feed
	if(request->getState () != IAsyncInfo::kCompleted)
		return;
		
	IStream* stream = request->getResponseStream ();
	ASSERT (stream != nullptr)
	if(stream)
	{
		AutoPtr<IWebNewsReader> reader = System::GetWebService ().createReader ();
		ASSERT (reader != nullptr)
		tresult result = reader->loadFeed (*stream);
		SOFT_ASSERT (result == kResultOk, "FeedElement: loadFeed failed!")
		this->feed = reader->getFeed ();
	}

	if(feed == nullptr)
		return;

	// remove elements
	paramList.removeAll ();
	removeAll ();

	int itemCount = feed->countItems ();
	if(maxItemCount > 0) // limit max. number of displayed items
		ccl_upper_limit (itemCount, maxItemCount);

	// build elements
	for(int i = 0; i < itemCount; i++)
	{
		IWebNewsItem* item = feed->getItem (i);

		String title (item->getAttribute (Feed::kTitle));
		String summary (item->getAttribute (Feed::kSummary));
		String content (item->getAttribute (Feed::kContent));
		
		DateTime dateTime;
		item->getLastUpdated (dateTime);

		Url url;
		getItemLink (url, *item, Feed::kAlternate);

		paramList.addIndexedParam (CSTR ("title"), NEW StringParam)->fromString (title);
		paramList.addIndexedParam (CSTR ("summary"), NEW StringParam)->fromString (summary);
		paramList.addIndexedParam (CSTR ("content"), NEW StringParam)->fromString (content);
		paramList.addIndexedParam (CSTR ("link"), NEW StringParam)->fromString (UrlFullString (url, true));
		paramList.addIndexedParam (CSTR ("date"), NEW StringParam)->fromString (Format::DateTime::print (dateTime, Format::DateTime::kFriendly|Format::DateTime::kDate));

		String name ("image");
		name << i;
		ImageElement* imageElement = NEW ImageElement (name);
		addComponent (imageElement);
		
		int imageLinkIndex = -1;
		float imageScaleFactor = 1.f;
		static const String kImagePrefix ("image/");

		// new approach: check for high-resolution image
		if(ImageBuilder::isHighResolutionImageNeeded ())
		{
			for(int i = 0; ; i++)
			{
				IWebNewsLink* link = item->getLink (Feed::kEnclosure, i);
				if(link == nullptr)
					break;

				if(link->getAttribute (Feed::kType).startsWith (kImagePrefix))
					if(link->getAttribute (Feed::kDevicePixelRatio) == CCLSTR ("2x"))
					{
						imageLinkIndex = i;
						imageScaleFactor = 2.f;
						break;
					}
			}
		}
		
		// old behavior: use first link if it's an image
		if(imageLinkIndex == -1)
		{
			if(IWebNewsLink* link = item->getLink (Feed::kEnclosure))
				if(link->getAttribute (Feed::kType).startsWith (kImagePrefix))
					imageLinkIndex = 0;
		}

		if(imageLinkIndex != -1)
		{
			// LATER TODO: Support multiple URLs per image element like HTML "srcset" attribute
			// https://webkit.org/demos/srcset/

			Url src;
			getItemLink (src, *item, Feed::kEnclosure, imageLinkIndex);		
			imageElement->setSource (src);
			imageElement->setSourceScaleFactor (imageScaleFactor);
			imageElement->startDownload ();
		}
	}

	// update view
	UnknownPtr<IForm> form (feedView);
	if(form)
		form->reload ();
}

//************************************************************************************************
// FeedListComponent
//************************************************************************************************

DEFINE_CLASS_HIDDEN (FeedListComponent, Component)
DEFINE_COMPONENT_SINGLETON (FeedListComponent)

//////////////////////////////////////////////////////////////////////////////////////////////////

FeedListComponent::FeedListComponent ()
: Component (CCLSTR ("FeedList")),
  feedList (nullptr)
{
	feedList = paramList.addInteger (0, 0, CSTR ("feedList"));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int FeedListComponent::addFeed (UrlRef feedUrl, int maxItemCount, StringID formName)
{
	int feedCount = countChildren ();
	feedList->setMax (feedCount);
	feedCount++;

	String name ("NewsFeed");
	name << feedCount;
	
	Web::FeedElement* feedElement = NEW Web::FeedElement (name);
	if(!formName.isEmpty ())
		feedElement->setFormName (formName);
	feedElement->setMaxItemCount (maxItemCount);
	feedElement->setSource (feedUrl);
	addComponent (feedElement);
	return feedCount - 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FeedListComponent::reloadFeed (int index, UrlRef feedUrl)
{
	Web::FeedElement* feedElement = unknown_cast<Web::FeedElement> (getChild (index));
	if(feedElement == nullptr)
		return false;

	feedElement->setSource (feedUrl);
	feedElement->restartDownload (true);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FeedListComponent::reloadAll ()
{
	ForEach (*this, Component, c)
		if(Web::Element* feedElement = ccl_cast<Web::Element> (c))
			feedElement->restartDownload (true);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API FeedListComponent::appendContextMenu (IContextMenu& contextMenu)
{
	contextMenu.addCommandItem (XSTR (ReloadFeed), "Feed", "Reload All", this);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FeedListComponent::interpretCommand (const CommandMsg& msg)
{
	if(msg.category == "Feed")
	{
		if(msg.name == "Reload All")
		{
			if(!msg.checkOnly ())
				reloadAll ();
			return true;
		}
	}
	return SuperClass::interpretCommand (msg);
}
