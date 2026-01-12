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
// Filename    : ccl/base/development.cpp
// Description : Determine locations in working copy
//
//************************************************************************************************

#include "ccl/base/development.h"
#include "ccl/base/storage/url.h"

#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/systemservices.h"

namespace CCL {

namespace Development 
{
	bool isRootFolder (UrlRef folder);
}

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// Development namespace
//************************************************************************************************

bool Development::isRootFolder (UrlRef folder)
{
	return System::GetFileSystem ().fileExists (Url ("./applications", folder, Url::kFolder))
		&& System::GetFileSystem ().fileExists (Url ("./frameworks", folder, Url::kFolder));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Development::getRootFolder (Url& url, UrlRef startFolder)
{
	if(System::GetSystem ().isProcessSandboxed ())
		return false;
		
	Url folder (startFolder);
	while(!folder.isRootPath ())
	{
		if(isRootFolder (folder))
		{
			url = folder;
			return true;
		}
		folder.ascend ();
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Url& Development::getTopLevelFolder (Url& url, CStringPtr rootProject)
{
	if(System::GetSystem ().isProcessSandboxed ())
		return url;

	// CCL_TOPLEVEL_DIRECTORY is defined by CMake, see cclbase.cmake
	#ifdef CCL_TOPLEVEL_DIRECTORY
	url.fromDisplayString (CCL_TOPLEVEL_DIRECTORY);
	url.descend (rootProject, IUrl::kFolder);
	if(System::GetFileSystem ().fileExists (url))
		return url;
	#endif

	url.fromDisplayString (__FILE__);
	url.ascend (); // remove source.cpp

	while(!url.isRootPath ())
	{
		url.setName (rootProject);
		if(System::GetFileSystem ().fileExists (url))
			return url;
		url.ascend ();
	}

	ASSERT (!url.isRootPath ())

	return url;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Url& Development::makeAbsolutePath (Url& url, CStringPtr rootProject, CStringPtr relativePath, int type)
{
	if(System::GetSystem ().isProcessSandboxed ())
		return url;
	
	Url rootFolder;
	getTopLevelFolder (rootFolder, rootProject);

	#if CCL_PLATFORM_MAC || CCL_PLATFORM_IOS
	url.fromPOSIXPath (relativePath, type);
	#else
	url.fromNativePath (StringChars (String (relativePath)), type);
	#endif
	url.makeAbsolute (rootFolder);
	return url;
}
