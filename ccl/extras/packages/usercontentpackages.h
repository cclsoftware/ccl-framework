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
// Filename    : ccl/extras/packages/usercontentpackages.h
// Description : User Content Packages
//
//************************************************************************************************

#ifndef _ccl_usercontentpackages_h
#define _ccl_usercontentpackages_h

#include "ccl/extras/packages/installdatapackages.h"
#include "ccl/extras/extensions/icontentserver.h"

namespace CCL {
namespace Packages {

//************************************************************************************************
// UserContentPackageSource
/** Package source used to retrieve UnifiedPackage representations of user content. @sa IContentServer */
//************************************************************************************************

class UserContentPackageSource: public ManifestPackageSource
{
public:
	UserContentPackageSource (Install::IContentServer& server);

private:
	Install::IContentServer& server;

	// ManifestPackageSource
	void initializeData (bool silent = false) override;
	UnifiedPackage* createCategoryPackage (const InstallData& data, const Install::Package* category, StringRef id = nullptr) override;
};

} // namespace Packages
} // namespace CCL

#endif // _ccl_usercontentpackages_h
