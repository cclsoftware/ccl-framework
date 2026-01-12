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
// Filename    : ccl/network/web/webnewsreader.cpp
// Description : Internet News Reader
//
//************************************************************************************************

#include "ccl/network/web/webnewsreader.h"

using namespace CCL;
using namespace Web;

//************************************************************************************************
// WebNewsLink
//************************************************************************************************

DEFINE_CLASS_HIDDEN (WebNewsLink, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API WebNewsLink::getAttribute (StringID id) const
{
	if(id == Feed::kRelation)
		return relation;
	if(id == Feed::kType)
		return type;
	if(id == Feed::kHRef)
		return href;
	if(id == Feed::kDevicePixelRatio)
		return devicePixelRatio;
	return String::kEmpty;
}

//************************************************************************************************
// WebNewsItem
//************************************************************************************************

DEFINE_CLASS_HIDDEN (WebNewsItem, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

WebNewsItem::WebNewsItem ()
{
	attributes.objectCleanup (true);
	links.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebNewsItem::addAttribute (StringID id, StringRef value)
{
	attributes.add (NEW Attribute (id, value));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WebNewsItem::Attribute* WebNewsItem::findAttribute (StringID id) const
{
	ForEach (attributes, Attribute, a)
		if(a->getID () == id)
			return a;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API WebNewsItem::getAttribute (StringID id) const
{
	Attribute* a = findAttribute (id);
	if(a)
		return a->getValue ();
	return String::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WebNewsItem::getLastUpdated (DateTime& dateTime) const
{
	dateTime = getLastUpdated ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebNewsItem::addLink (WebNewsLink* link)
{
	links.add (link);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IWebNewsLink* CCL_API WebNewsItem::getLink (StringID _relation, int index) const
{
	String relation (_relation);
	int i = 0;
	ForEach (links, WebNewsLink, link)
		if(link->getRelation () == relation)
		{
			if(i == index)
				return link;
			i++;
		}
	EndFor
	return nullptr;
}

//************************************************************************************************
// WebNewsFeed
//************************************************************************************************

DEFINE_CLASS_HIDDEN (WebNewsFeed, WebNewsItem)

//////////////////////////////////////////////////////////////////////////////////////////////////

WebNewsFeed::WebNewsFeed ()
{
	items.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API WebNewsFeed::getAttribute (StringID id) const
{
	return SuperClass::getAttribute (id);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WebNewsFeed::getLastUpdated (DateTime& dateTime) const
{
	SuperClass::getLastUpdated (dateTime);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IWebNewsLink* CCL_API WebNewsFeed::getLink (StringID relation, int index) const
{
	return SuperClass::getLink (relation, index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebNewsFeed::addItem (WebNewsItem* item)
{
	items.add (item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API WebNewsFeed::countItems () const
{
	return items.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IWebNewsItem* CCL_API WebNewsFeed::getItem (int index) const
{
	return (WebNewsItem*)items.at (index);
}

//************************************************************************************************
// WebNewsReader
//************************************************************************************************

DEFINE_CLASS_HIDDEN (WebNewsReader, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

WebNewsReader::WebNewsReader ()
: feed (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

WebNewsReader::~WebNewsReader ()
{
	if(feed)
		feed->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WebNewsFeed& WebNewsReader::getFeed ()
{
	if(feed == nullptr)
		feed = NEW WebNewsFeed;
	return *feed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WebNewsReader::parseFeed (IStream& stream)
{
	CCL_NOT_IMPL ("WebNewsReader::parseFeed not implemented!")
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WebNewsReader::loadFeed (IStream& stream)
{
	// remove old content
	safe_release (feed);

	bool result = parseFeed (stream);

	// cleanup on failure
	if(!result)
		safe_release (feed);

	return result ? kResultOk : kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IWebNewsFeed* CCL_API WebNewsReader::getFeed () const
{
	return feed;
}

