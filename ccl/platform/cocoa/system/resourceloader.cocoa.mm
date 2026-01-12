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
// Filename    : ccl/platform/cocoa/system/resourceloader.cocoa.mm
// Description : OSX/iOS Resource Loader
//
//************************************************************************************************

#include "ccl/system/virtualfilesystem.h"
#include "ccl/public/systemservices.h"
#include "ccl/base/storage/url.h"
#include "ccl/platform/cocoa/macutils.h"

namespace CCL {

//************************************************************************************************
// BundleResourceFileSystem
//************************************************************************************************

class BundleResourceFileSystem: public ResourceFileSystem
{
public:
	// ResourceFileSystem
	IStream* CCL_API openStream (UrlRef url, int mode, IUnknown* context);
	IFileIterator* CCL_API newIterator (UrlRef url, int mode = IFileIterator::kAll);
	tbool CCL_API fileExists (UrlRef url);
	
private:
	bool toPathInBundle (Url& resourcePath, UrlRef url) const;
};
	
} // namespace CCL

using namespace CCL;

//************************************************************************************************
// ResourceFileSystem
//************************************************************************************************

ResourceFileSystem& ResourceFileSystem::instance ()
{
	static BundleResourceFileSystem theInstance;
	return theInstance;
}

//************************************************************************************************
// BundleResourceFileSystem
//************************************************************************************************

bool BundleResourceFileSystem::toPathInBundle (Url& absPath, UrlRef url) const
{
	CFBundleRef bundleRef = (CFBundleRef)System::GetModuleWithIdentifier (url.getHostName ());
	if(!bundleRef)
		return false;
	
	Url resourceDir;
	CFObj<CFURLRef> bundleResourceDir = CFBundleCopyResourcesDirectoryURL (bundleRef);
	MacUtils::urlFromCFURL (resourceDir, bundleResourceDir, IUrl::kFolder);

	absPath = url;
	absPath.setProtocol (CCLSTR("file"));
	absPath.makeAbsolute (resourceDir);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStream* CCL_API BundleResourceFileSystem::openStream (UrlRef url, int mode, IUnknown* context)
{
	ASSERT ((mode & IStream::kCreate) == 0)
	if(mode & IStream::kCreate)
		return nullptr;

	Url resourcePath;
	if(!toPathInBundle (resourcePath, url))
		return nullptr;
	
	return System::GetFileSystem().openStream (resourcePath, IStream::kReadMode, context);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFileIterator* CCL_API BundleResourceFileSystem::newIterator (UrlRef url, int mode)
{
	Url resourcePath;
	if(!toPathInBundle (resourcePath, url))
		return nullptr;

	return System::GetFileSystem ().newIterator (resourcePath, mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API BundleResourceFileSystem::fileExists (UrlRef url)
{
	Url resourcePath;
	if(!toPathInBundle (resourcePath, url))
		return false;
		
	return System::GetFileSystem ().fileExists (resourcePath);
}
