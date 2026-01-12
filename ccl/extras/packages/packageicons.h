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
// Filename    : ccl/extras/packages/packageicons.h
// Description : Package Icons
//
//************************************************************************************************

#ifndef _ccl_packageicons_h
#define _ccl_packageicons_h

#include "ccl/base/singleton.h"

#include "ccl/extras/packages/unifiedpackage.h"
#include "ccl/extras/web/webelements.h"

namespace CCL {
namespace Packages {

//************************************************************************************************
// PackageIconCache
//************************************************************************************************

class PackageIconCache: public Web::PersistentImageCache,
						public Singleton<PackageIconCache>
{
public:
	DECLARE_CLASS (PackageIconCache, PersistentImageCache)

	PackageIconCache ();

	bool requestPackageIcon (UnifiedPackage* package, StringRef productId);

protected:
	static const String kIconCacheFolder;
	static const int kCacheTimeout;
	static const int kCacheMaxDelay;
};

} // namespace Packages
} // namespace CCL

#endif // _ccl_packageicons_h
