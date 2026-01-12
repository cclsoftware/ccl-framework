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
// Filename    : ccl/public/network/web/iwebnewsreader.h
// Description : Internet News Reader
//
//************************************************************************************************

#ifndef _ccl_iwebnewsreader_h
#define _ccl_iwebnewsreader_h

#include "ccl/public/base/datetime.h"

namespace CCL {

interface IStream;

namespace Web {

//////////////////////////////////////////////////////////////////////////////////////////////////
// News Feed Attributes
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Feed 
{
	// Item/Feed Attributes
	DEFINE_STRINGID (kID, "id")
	DEFINE_STRINGID (kTitle, "title")
	DEFINE_STRINGID (kAuthorName, "authorName")
	DEFINE_STRINGID (kAuthorEmail, "authorEmail")
	DEFINE_STRINGID (kCategoryTerm, "categoryTerm")
	DEFINE_STRINGID (kCategoryLabel, "categoryLabel")
	DEFINE_STRINGID (kLanguage, "language")
	DEFINE_STRINGID (kSummary, "summary")
	DEFINE_STRINGID (kContent, "content")

	// Link Relations
	DEFINE_STRINGID (kAlternate, "alternate")
	DEFINE_STRINGID (kEnclosure, "enclosure")

	// Link Attributes
	DEFINE_STRINGID (kRelation, "relation")
	DEFINE_STRINGID (kType, "type")
	DEFINE_STRINGID (kHRef, "href")
	DEFINE_STRINGID (kDevicePixelRatio, "devicePixelRatio")
}

//************************************************************************************************
// IWebNewsLink
//************************************************************************************************

interface IWebNewsLink: IUnknown
{
	/** Get link attribute. */
	virtual StringRef CCL_API getAttribute (StringID id) const = 0;

	DECLARE_IID (IWebNewsLink)
};

DEFINE_IID (IWebNewsLink, 0x75fd9ced, 0xe1b6, 0x49b7, 0x9d, 0xda, 0x62, 0xd7, 0x44, 0xc, 0x21, 0x44)

//************************************************************************************************
// IWebNewsItem
//************************************************************************************************

interface IWebNewsItem: IUnknown
{
	/** Get item attribute. */
	virtual StringRef CCL_API getAttribute (StringID id) const = 0;

	/** Get time last updated. */
	virtual void CCL_API getLastUpdated (DateTime& dateTime) const = 0;

	/** Get n-th link of given relation associated with this item. */
	virtual IWebNewsLink* CCL_API getLink (StringID relation, int index = 0) const = 0;

	DECLARE_IID (IWebNewsItem)
};

DEFINE_IID (IWebNewsItem, 0x41b4ebab, 0xd635, 0x45da, 0xae, 0x7b, 0x3f, 0x40, 0xc9, 0x36, 0x8c, 0x53)

//************************************************************************************************
// IWebNewsFeed
//************************************************************************************************

interface IWebNewsFeed: IWebNewsItem
{	
	/** Get number of new items. */
	virtual int CCL_API countItems () const = 0;

	/** Get news item at given index. */
	virtual IWebNewsItem* CCL_API getItem (int index) const = 0;

	DECLARE_IID (IWebNewsFeed)
};

DEFINE_IID (IWebNewsFeed, 0x55827769, 0x6fd6, 0x46d2, 0xb3, 0x7e, 0xab, 0xa, 0xa6, 0x6b, 0x78, 0x66)

//************************************************************************************************
// IWebNewsReader
//************************************************************************************************

interface IWebNewsReader: IUnknown
{
	/** Load feed from stream. */
	virtual tresult CCL_API loadFeed (IStream& stream) = 0;

	/** Get currently loaded feed. */
	virtual IWebNewsFeed* CCL_API getFeed () const = 0;

	DECLARE_IID (IWebNewsReader)
};

DEFINE_IID (IWebNewsReader, 0x89b5475d, 0xa0bf, 0x419d, 0x9e, 0xd6, 0x4, 0xa7, 0x99, 0x9c, 0x4, 0x93)

} // namespace Web
} // namespace CCL

#endif // _ccl_iwebnewsreader_h
