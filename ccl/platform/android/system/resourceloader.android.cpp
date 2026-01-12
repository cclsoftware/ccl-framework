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
// Filename    : ccl/platform/android/system/resourceloader.android.cpp
// Description : Android Resource Loader
//
//************************************************************************************************

#include "ccl/system/virtualfilesystem.h"

#include "ccl/platform/android/system/assetfilesystem.h"
#include "ccl/platform/android/system/system.android.h"

#include "ccl/public/systemservices.h"

namespace CCL {

//************************************************************************************************
// AndroidResourceFileSystem
//************************************************************************************************

class AndroidResourceFileSystem: public ResourceFileSystem
{
public:
	// ResourceFileSystem
	IStream* CCL_API openStream (UrlRef url, int mode, IUnknown* context = nullptr) override;
	tbool CCL_API getFileInfo (FileInfo& info, UrlRef url) override;
	IFileIterator* CCL_API newIterator (UrlRef url, int mode = IFileIterator::kAll) override;
	tbool CCL_API fileExists (UrlRef url) override;

private:
	Url toAssetUrl (UrlRef url) const;

	Android::AssetFileSystem assetFileSystem;
};

} // namespace CCL

using namespace CCL;
using namespace Android;

//************************************************************************************************
// ResourceFileSystem
//************************************************************************************************

ResourceFileSystem& ResourceFileSystem::instance ()
{
	static AndroidResourceFileSystem theInstance;
	return theInstance;
}

//************************************************************************************************
// AndroidResourceFileSystem
//************************************************************************************************

IStream* CCL_API AndroidResourceFileSystem::openStream (UrlRef url, int mode, IUnknown* context)
{
	return assetFileSystem.openStream (toAssetUrl (url), mode, context);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API AndroidResourceFileSystem::getFileInfo (FileInfo& info, UrlRef url)
{
	return assetFileSystem.getFileInfo (info, toAssetUrl (url));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFileIterator* CCL_API AndroidResourceFileSystem::newIterator (UrlRef url, int mode)
{
	return assetFileSystem.newIterator (toAssetUrl (url), mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API AndroidResourceFileSystem::fileExists (UrlRef url)
{
	return assetFileSystem.fileExists (toAssetUrl (url));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Url AndroidResourceFileSystem::toAssetUrl (UrlRef url) const
{
	// get image for the referenced module
	if(ModuleRef module = System::GetModuleWithIdentifier (url.getHostName ()))
	{
		if(AutoPtr<IExecutableImage> image = System::GetExecutableLoader ().createImage (module))
		{
			String moduleId;
			image->getIdentifier (moduleId);

			// create asset URL for path in module resources folder
			return AssetUrl (String ("resources/").append (moduleId).append ("/").append (url.getPath ()), url.getType ());
		}
	}

	return Url::kEmpty;
}
