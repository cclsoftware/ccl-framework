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
// Filename    : ccl/extras/packages/factorycontentpackages.cpp
// Description : Factory Content Packages
//
//************************************************************************************************

#include "ccl/extras/packages/factorycontentpackages.h"

#include "ccl/extras/extensions/installdata.h"

#include "ccl/public/text/translation.h"

using namespace CCL;
using namespace Packages;
using namespace Install;

//************************************************************************************************
// FactoryContentPackageSource
//************************************************************************************************

DEFINE_STRINGID_MEMBER_ (FactoryContentPackageSource, kSourceName, "factorycontent")

//////////////////////////////////////////////////////////////////////////////////////////////////

FactoryContentPackageSource::FactoryContentPackageSource (bool keepCategoryPackages)
: ManifestPackageSource (kSourceName, kLocalSource),
  keepCategoryPackages (keepCategoryPackages)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FactoryContentPackageSource::containsFile (StringRef fileId) const
{
	for(const InstallData& data : installData)
	{
		if(data.manifest != nullptr && data.manifest->findFile (fileId) != nullptr)
				return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Manifest* FactoryContentPackageSource::createManifest () const
{
	Manifest* manifest = NEW Manifest;
	ResourceUrl manifestPath (Manifest::kFileName);
	if(!ManifestLoader (*manifest).loadAll (manifestPath))
		safe_release (manifest);
	return manifest;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FactoryContentPackageSource::initializeData (bool silent)
{
	ManifestPackageSource::initializeData (silent);

	ASSERT (installData.isEmpty ())

	if(Manifest* manifest = createManifest ())
		installData.add ({manifest, UnifiedPackage::kFactoryContentOrigin});
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UnifiedPackage* FactoryContentPackageSource::createCategoryPackage (const InstallData& data, const Package* category, StringRef id)
{
	UnifiedPackage* package = ManifestPackageSource::createCategoryPackage (data, category, id);
	if(package)
	{
		if(keepCategoryPackages)
			package->isCritical (true);
		String translatedTitle = TRANSLATE2 ("Installer", package->getTitle ());
		package->setTitle (translatedTitle);
	}
	return package;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UnifiedPackage* FactoryContentPackageSource::createFilePackage (const InstallData& data, const Install::File* file)
{
	UnifiedPackage* package = ManifestPackageSource::createFilePackage (data, file);
	if(package)
	{
		package->setTitle (TRANSLATE2 ("Installer", package->getTitle ()));
		package->setDescription (TRANSLATE2 ("Installer", package->getDescription ()));
	}
	return package;
}
