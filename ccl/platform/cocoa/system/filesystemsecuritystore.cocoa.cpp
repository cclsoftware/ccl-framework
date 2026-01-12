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
// Filename    : ccl/platform/cocoa/system/filesystemsecuritystore.cpp
// Description : Mac OS file system security store class
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/cocoa/system/filesystemsecuritystore.cocoa.h"

#include "ccl/base/storage/storage.h"

#include "ccl/public/systemservices.h"

using namespace CCL;

//************************************************************************************************
// BookmarkItem
//************************************************************************************************

DEFINE_CLASS (BookmarkItem, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

BookmarkItem::BookmarkItem (UrlRef url, StringRef bookmark)
: bookmark (bookmark),
  url (url)
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BookmarkItem::save (const Storage& storage) const 
{
	Attributes& attributes = storage.getAttributes ();
	
	attributes.set ("url", url);
	attributes.set ("bookmark", bookmark);
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BookmarkItem::load (const Storage& storage)
{
	Attributes& attributes = storage.getAttributes ();
	
	bool foundUrl = attributes.get (url, "url");
	bool foundBookmark = attributes.get (bookmark, "bookmark");
	
	return foundUrl && foundBookmark;
}

//************************************************************************************************
// CocoaFileSystemSecurityStore
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (FileSystemSecurityStore, CocoaFileSystemSecurityStore)

//////////////////////////////////////////////////////////////////////////////////////////////////

IFileSystemSecurityStore& CocoaFileSystemSecurityStore::instance ()
{
	static CocoaFileSystemSecurityStore theInstance;
	return theInstance;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CocoaFileSystemSecurityStore::CocoaFileSystemSecurityStore ()
: settings ("SecurityScopedBookmarks")
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CocoaFileSystemSecurityStore::setSecurityData (UrlRef url, VariantRef data)
{
    Threading::ScopedLock scopedLock (lock);
	String base64 = data.asString ();
	
	if(!base64.isEmpty () && open)
	{
		bookmarks.removeIf ([&] (Object* obj)
		{
			BookmarkItem* item = static_cast<BookmarkItem*> (obj);
			return item->getUrl ().isEqualUrl (url) && item->getBookmark () != base64;
		});
	 
		bookmarks.add (NEW BookmarkItem (url, base64));
	}
	else
	{
		return false;
	}
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CocoaFileSystemSecurityStore::getSecurityData (Variant& data, UrlRef url)
{
	if(open)
	{
		Threading::ScopedLock scopedLock (lock);
		String dataString = getBookmark (url);
		Variant variant (dataString);
		data = variant;
		CCL_PRINTF ("CocoaFileSystemSecurityStore: Read bookmark for url:\n%s\n", MutableCString (url.getPath ()).str ())
		return !dataString.isEmpty ();
	}
	else
	{
		CCL_PRINTF ("CocoaFileSystemSecurityStore: Failed to read bookmark for url:\n%s\n", MutableCString (url.getPath ()).str ())
		return false;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CocoaFileSystemSecurityStore::loadSecurityData ()
{
	Threading::ScopedLock scopedLock (lock);
	// Read settings from disk
	settings.restore ();
	
	Attributes& attributes = settings.getAttributes ("SecurityScopedBookmarks");
	
	attributes.unqueue (bookmarks, "bookmarks");
	
	open = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CocoaFileSystemSecurityStore::saveSecurityData ()
{
	Threading::ScopedLock scopedLock (lock);
	
	if(!open)
		return;
	
	open = false;
	
	// Update settings object
	Attributes& attributes = settings.getAttributes ("SecurityScopedBookmarks");
	attributes.removeAll ();
	attributes.queue ("bookmarks", bookmarks);
	
	settings.flush ();
	bookmarks.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CocoaFileSystemSecurityStore::getBookmark (UrlRef url) const
{
	ArrayForEach (bookmarks, BookmarkItem, obj)
		BookmarkItem* item = static_cast<BookmarkItem*> (obj);
		if(item->getUrl ().isEqualUrl (url, false))
			return item->getBookmark ();
	EndFor
	
	return String::kEmpty;
};
