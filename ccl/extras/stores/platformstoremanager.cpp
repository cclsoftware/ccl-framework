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
// Filename    : ccl/extras/stores/platformstoremanager.cpp
// Description : Platform Store Manager
//
//************************************************************************************************

#include "ccl/extras/stores/platformstoremanager.h"

#include "ccl/base/storage/url.h"
#include "ccl/base/storage/configuration.h"

#include "ccl/public/systemservices.h"

using namespace CCL;

//************************************************************************************************
// PlatformStoreID
//************************************************************************************************

DEFINE_STRINGID_ (PlatformStoreID::kDemo, "demo")
DEFINE_STRINGID_ (PlatformStoreID::kAppleAppStoreMacOS, "apple.mac")
DEFINE_STRINGID_ (PlatformStoreID::kAppleAppStoreIOS, "apple.ios")
DEFINE_STRINGID_ (PlatformStoreID::kMicrosoftStore, "microsoft")
DEFINE_STRINGID_ (PlatformStoreID::kGooglePlayStore, "playstore")
DEFINE_STRINGID_ (PlatformStoreID::kAmazonAppStore, "amazon")
DEFINE_STRINGID_ (PlatformStoreID::kSamsungGalaxyStore, "samsung")

//************************************************************************************************
// PlatformStoreManager
//************************************************************************************************

bool PlatformStoreManager::loadAppStoreConfiguration ()
{
	const String kConfigFileName ("appstore.config");
	ResourceUrl configResourcePath (System::GetMainModuleRef (), kConfigFileName);

	Configuration::Registry& registry = Configuration::Registry::instance ();
	return registry.loadFromFile (configResourcePath);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_ABSTRACT_HIDDEN (PlatformStoreManager, Object)
DEFINE_STRINGID_MEMBER_ (PlatformStoreManager, kTransactionsChanged, "transactionsChanged")
DEFINE_STRINGID_MEMBER_ (PlatformStoreManager, kLocalLicensesChanged, "localLicensesChanged")

