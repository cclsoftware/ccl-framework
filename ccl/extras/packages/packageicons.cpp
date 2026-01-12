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
// Filename    : ccl/extras/packages/packageicons.cpp
// Description : Package Icons
//
//************************************************************************************************

#include "ccl/extras/packages/packageicons.h"
#include "ccl/extras/packages/unifiedpackageinstaller.h"
#include "ccl/extras/extensions/icontentserver.h"

#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/systemservices.h"

namespace CCL {
namespace Packages {

//************************************************************************************************
// PackageIconSetter
//************************************************************************************************

class PackageIconSetter: public Object,
						 public Web::IImageElementCallback
{
public:
	PackageIconSetter (UnifiedPackage* package)
	: package (package)
	{}

	// IImageElementCallback
	void onImageDownloadCompleted (IImage* image) override
	{
		if(image && package)
		{
			package->setIcon (image);
			package->deferChanged ();
			PackageIconCache::instance ().saveImage (image, package->getId (), true);
		}
	}

	CLASS_INTERFACE (IImageElementCallback, Object)

protected:
	SharedPtr<UnifiedPackage> package;
};

} // namespace Packages
} // namespace CCL

using namespace CCL;
using namespace Packages;
using namespace Install;

//************************************************************************************************
// PackageIconCache
//************************************************************************************************

DEFINE_CLASS_HIDDEN (PackageIconCache, PersistentImageCache)
DEFINE_SINGLETON (PackageIconCache)

const String PackageIconCache::kIconCacheFolder = "ImageCache/Packages";
const int PackageIconCache::kCacheTimeout = 30; // days
const int PackageIconCache::kCacheMaxDelay = 10; // days

//////////////////////////////////////////////////////////////////////////////////////////////////

PackageIconCache::PackageIconCache ()
: PersistentImageCache (Url (), kCacheTimeout, kCacheMaxDelay)
{
	System::GetSystem ().getLocation (basePath, CCL::System::kAppSettingsFolder);
	basePath.descend (kIconCacheFolder, IUrl::kFolder);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageIconCache::requestPackageIcon (UnifiedPackage* package, StringRef productId)
{
	if(package == nullptr)
		return false;

	AutoPtr<IImage> icon = loadImage (package->getId ());
	if(icon.isValid ())
	{
		package->setIcon (icon);
		return true;
	}

	IContentServer* contentServer = UnifiedPackageInstaller::instance ().getInstallEngine ().getContentServer ();
	if(contentServer)
	{
		AutoPtr<IUnknown> credentials = contentServer->requestCredentials (IContentServer::kContentDownload, IContentServer::kSuppressErrors | IContentServer::kSuppressLogin);
		if(credentials)
		{
			Url iconUrl;
			contentServer->getIconURL (iconUrl, productId, credentials);
	
			AutoPtr<PackageIconSetter> setter = NEW PackageIconSetter (package);
			Web::ImageDownloader& downloader = Web::ImageDownloader::instance ();
			downloader.requestImage (setter, iconUrl);
			return true;
		}
	}
	return false;
}
