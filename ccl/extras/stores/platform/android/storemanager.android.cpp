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
// Filename    : ccl/extras/stores/platform/android/storemanager.android.cpp
// Description : Store Manager for Android selecting the Google, Amazon or Samsung implementation
//
//************************************************************************************************

#include "ccl/extras/stores/platform/android/playstore/playstoremanager.android.h"
#include "ccl/extras/stores/platform/android/amazon/amazonstoremanager.android.h"

#if CCL_SAMSUNG_STORE_MANAGER_ENABLED
#include "ccl/extras/stores/platform/android/samsung/samsungstoremanager.android.h"
#endif

#include "ccl/base/storage/configuration.h"

using namespace CCL;
using namespace Android;

#if !CCL_DEMO_STORE_MANAGER_ENABLED
DEFINE_SINGLETON (PlatformStoreManager)

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////
// Select store according to config
//////////////////////////////////////////////////////////////////////////////////////////////

template<> PlatformStoreManager* ExternalSingleton<PlatformStoreManager>::createExternalInstance ()
{
	// load app store configuration
	if(PlatformStoreManager::loadAppStoreConfiguration ())
	{
		String appStoreId (PlatformStoreID::kGooglePlayStore);
		Configuration::Registry& registry = Configuration::Registry::instance ();
		registry.getValue (appStoreId, "CCL.Android", "AppStoreId");

		if(appStoreId == String (PlatformStoreID::kGooglePlayStore))
			return NEW PlayStoreManager;
		else if(appStoreId == String (PlatformStoreID::kAmazonAppStore))
			return NEW AmazonStoreManager;

		#if CCL_SAMSUNG_STORE_MANAGER_ENABLED
		else if(appStoreId == String (PlatformStoreID::kSamsungGalaxyStore))
			return NEW SamsungStoreManager;
		#endif
	}

	return NEW PlayStoreManager;
}

} // namespace CCL
#endif
