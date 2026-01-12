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
// Filename    : ccl/extras/packages/usercontentpackages.cpp
// Description : User Content Packages
//
//************************************************************************************************

#include "ccl/extras/packages/usercontentpackages.h"
#include "ccl/extras/packages/unifiedpackage.h"
#include "ccl/extras/packages/packageicons.h"
#include "ccl/extras/extensions/installdata.h"

using namespace CCL;
using namespace Packages;
using namespace Install;

//************************************************************************************************
// UserContentPackageSource
//************************************************************************************************

UserContentPackageSource::UserContentPackageSource (IContentServer& server)
: ManifestPackageSource ("usercontent"),
  server (server)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserContentPackageSource::initializeData (bool silent)
{
	if(silent)
		return;

	ManifestPackageSource::initializeData (silent);

	ASSERT (installData.isEmpty ())

	Manifest* purchasedContentManifest = NEW Manifest;
	if(server.requestUserContentList (*purchasedContentManifest, IContentServer::kSuppressErrors | IContentServer::kSuppressLogin | IContentServer::kSkipSubscriptionContent) == false)
		safe_release (purchasedContentManifest);
	else
		installData.add ({purchasedContentManifest, UnifiedPackage::kPurchasedContentOrigin});

	Manifest* subscriptionContentManifest = NEW Manifest;
	if(server.requestUserContentList (*subscriptionContentManifest, IContentServer::kSuppressErrors | IContentServer::kSuppressLogin | IContentServer::kSkipPurchasedContent) == false)
		safe_release (subscriptionContentManifest);
	else
		installData.add ({subscriptionContentManifest, UnifiedPackage::kSubscriptionContentOrigin});
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UnifiedPackage* UserContentPackageSource::createCategoryPackage (const InstallData& data, const Package* category, StringRef id)
{
	UnifiedPackage* package = ManifestPackageSource::createCategoryPackage (data, category, id);
	if(package)
	{
		package->isProduct (true);
		if(package->getIcon () == nullptr)
			PackageIconCache::instance ().requestPackageIcon (package, category->getID ());
	}
	return package;
}
