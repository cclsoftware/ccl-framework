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
// Filename    : ccl/network/web/webnewsreader.h
// Description : Internet News Reader
//
//************************************************************************************************

#ifndef _ccl_webnewsreader_h
#define _ccl_webnewsreader_h

#include "ccl/public/network/web/iwebnewsreader.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/text/cclstring.h"

#include "ccl/base/collections/objectarray.h"

namespace CCL {
namespace Web {

//************************************************************************************************
// WebNewsLink
//************************************************************************************************

class WebNewsLink: public Object,
				   public IWebNewsLink
{
public:
	DECLARE_CLASS (WebNewsLink, Object)

	PROPERTY_STRING (relation, Relation)
	PROPERTY_STRING (type, Type)
	PROPERTY_STRING (href, HRef)
	PROPERTY_STRING (devicePixelRatio, DevicePixelRatio)

	// IWebNewsLink
	StringRef CCL_API getAttribute (StringID id) const override;

	CLASS_INTERFACE (IWebNewsLink, Object)
};

//************************************************************************************************
// WebNewsItem
//************************************************************************************************

class WebNewsItem: public Object,
				   public IWebNewsItem
{
public:
	DECLARE_CLASS (WebNewsItem, Object)

	WebNewsItem ();

	void addAttribute (StringID id, StringRef value);
	PROPERTY_OBJECT (DateTime, lastUpdated, LastUpdated)

	void addLink (WebNewsLink* link);

	// IWebNewsItem
	StringRef CCL_API getAttribute (StringID id) const override;
	void CCL_API getLastUpdated (DateTime& dateTime) const override;
	IWebNewsLink* CCL_API getLink (StringID relation, int index = 0) const override;

	CLASS_INTERFACE (IWebNewsItem, Object)

protected:
	class Attribute: public Object
	{
	public:
		Attribute (StringID id = nullptr, StringRef value = nullptr)
		: id (id),
		  value (value)
		{}

		PROPERTY_MUTABLE_CSTRING (id, ID)
		PROPERTY_STRING (value, Value)
	};

	ObjectArray attributes;
	ObjectArray links;

	Attribute* findAttribute (StringID id) const;
};

//************************************************************************************************
// WebNewsFeed
//************************************************************************************************

class WebNewsFeed: public WebNewsItem,
				   public IWebNewsFeed
{
public:
	DECLARE_CLASS (WebNewsFeed, WebNewsItem)

	WebNewsFeed ();

	void addItem (WebNewsItem* item);

	// IWebNewsFeed
	StringRef CCL_API getAttribute (StringID id) const override;
	void CCL_API getLastUpdated (DateTime& dateTime) const override;
	IWebNewsLink* CCL_API getLink (StringID relation, int index = 0) const override;
	int CCL_API countItems () const override;
	IWebNewsItem* CCL_API getItem (int index) const override;

	CLASS_INTERFACE (IWebNewsFeed, WebNewsItem)

protected:
	ObjectArray items;
};

//************************************************************************************************
// WebNewsReader
//************************************************************************************************

class WebNewsReader: public Object,
					 public IWebNewsReader
{
public:
	DECLARE_CLASS (WebNewsReader, Object)

	WebNewsReader ();
	~WebNewsReader ();

	// IWebNewsReader
	tresult CCL_API loadFeed (IStream& stream) override;
	IWebNewsFeed* CCL_API getFeed () const override;

	CLASS_INTERFACE (IWebNewsReader, Object)

protected:
	WebNewsFeed* feed;

	virtual WebNewsFeed& getFeed ();
	virtual bool parseFeed (IStream& stream);
};

} // namespace Web
} // namespace CCL

#endif // _ccl_webnewsreader_h
