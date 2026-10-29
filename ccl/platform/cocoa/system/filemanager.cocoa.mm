//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
//
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/platform/cocoa/system/filemanager.cocoa.mm
// Description : Shared Cocoa (Mac/iOS) file system manager base
//
//************************************************************************************************

#include "filemanager.cocoa.h"

#include "ccl/platform/cocoa/macutils.h"

namespace CCL {

//************************************************************************************************
// CocoaFileManager
//************************************************************************************************

tbool CCL_API CocoaFileManager::getFileSystemRepresentation (char pathBuffer[], int bufferSize, UrlRef url) const
{
	StringRef urlPath = url.getPath ();
	String path;
	if(!url.isRelative () && urlPath.at (0) != '/')
		path = Url::strPathChar;
	path << urlPath;
	return MacUtils::toFileSystemRepresentation (pathBuffer, bufferSize, path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CocoaFileManager::startUsing (UrlRef url)
{
	NSURL* nsurl = [NSURL alloc];
	MacUtils::urlToNSUrl (url, nsurl);
	BOOL success = [nsurl startAccessingSecurityScopedResource];
	if(success == NO)
		return kResultFailed;

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CocoaFileManager::stopUsing (UrlRef url)
{
	NSURL* nsurl = [NSURL alloc];
	MacUtils::urlToNSUrl (url, nsurl);
	[nsurl stopAccessingSecurityScopedResource];

	return kResultOk;
}

} // namespace CCL
