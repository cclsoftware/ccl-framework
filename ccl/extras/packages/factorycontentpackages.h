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
// Filename    : ccl/extras/packages/factorycontentpackages.h
// Description : Factory Content Packages
//
//************************************************************************************************

#ifndef _ccl_factorycontentpackages_h
#define _ccl_factorycontentpackages_h

#include "ccl/extras/packages/installdatapackages.h"

namespace CCL {
namespace Packages {

//************************************************************************************************
// FactoryContentPackageSource
/** Package source used to retrieve UnifiedPackage representations of factory content packages. */
//************************************************************************************************

class FactoryContentPackageSource: public ManifestPackageSource
{
public:
	FactoryContentPackageSource (bool keepCategoryPackages = true);

	DECLARE_STRINGID_MEMBER (kSourceName)

	bool containsFile (StringRef fileId) const;

	Install::Manifest* createManifest () const;

	// ManifestPackageSource
	void initializeData (bool silent = false) override;
	UnifiedPackage* createCategoryPackage (const InstallData& data, const Install::Package* category, StringRef id = nullptr) override;
	UnifiedPackage* createFilePackage (const InstallData& data, const Install::File* file) override;
	
private:
	bool keepCategoryPackages;
};

} // namespace Packages
} // namespace CCL

#endif // _ccl_factorycontentpackages_h
