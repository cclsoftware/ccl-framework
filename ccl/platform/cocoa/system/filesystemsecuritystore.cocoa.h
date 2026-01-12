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

#ifndef _ccl_filesystemsecuritystore_cocoa_h
#define _ccl_filesystemsecuritystore_cocoa_h

#include "ccl/system/filesystemsecuritystore.h"

#include "ccl/base/storage/settings.h"
#include "ccl/base/storage/url.h"

#include "ccl/public/system/threadsync.h"

namespace CCL {

//************************************************************************************************
// BookmarkItem
//************************************************************************************************

class BookmarkItem: public Object
{
public:
	DECLARE_CLASS (BookmarkItem, Object)
	
	BookmarkItem () = default;
	BookmarkItem (UrlRef url, StringRef bookmark);
	
	PROPERTY_STRING (bookmark, Bookmark)
	PROPERTY_OBJECT (Url, url, Url)
	
	// Object
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;
};

//************************************************************************************************
// CocoaFileSystemSecurityStore
//************************************************************************************************

class CocoaFileSystemSecurityStore: public FileSystemSecurityStore
{
public:
	CocoaFileSystemSecurityStore ();
	
	static IFileSystemSecurityStore& instance ();
	
	// FileSystemSecurityStore
	tbool CCL_API setSecurityData (UrlRef url, VariantRef data) override;
	tbool CCL_API getSecurityData (Variant& data, UrlRef url) override;
	void CCL_API saveSecurityData () override;
	void CCL_API loadSecurityData () override;
	
private:
	static CocoaFileSystemSecurityStore theInstance;
	Threading::CriticalSection lock;
	
	ObjectArray bookmarks;
	XmlSettings settings;
	bool open = false;
	
	void load ();
	void save ();
	StringRef getBookmark (UrlRef url) const;
};

} // namespace CCL

#endif // _ccl_filesystemsecuritystore_cocoa_h


