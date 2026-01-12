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
// Filename    : ccl/extras/stores/platformstoremanager.h
// Description : Platform Store Manager
//
//************************************************************************************************

#ifndef _ccl_platformstoremanager_h
#define _ccl_platformstoremanager_h

#include "ccl/extras/stores/purchasemodel.h"

#include "ccl/base/singleton.h"
#include "ccl/base/asyncoperation.h"

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Platform store macros
//////////////////////////////////////////////////////////////////////////////////////////////////

#define CCL_NO_PLATFORM_STORE CCL_PLATFORM_LINUX // no platform store on Linux

#define CCL_DEMO_STORE_MANAGER_ENABLED (CCL_NO_PLATFORM_STORE || (0 && DEBUG)) // enable demo store

//************************************************************************************************
// PlatformStoreID
//************************************************************************************************

namespace PlatformStoreID
{
	DEFINE_STRINGID (kDemo, "")					///< Demo (development or no platform store)
	DEFINE_STRINGID (kAppleAppStoreMacOS, "")	///< macOS AppStore
	DEFINE_STRINGID (kAppleAppStoreIOS, "")		///< iOS AppStore
	DEFINE_STRINGID (kMicrosoftStore, "")		///< Microsoft Store
	DEFINE_STRINGID (kGooglePlayStore, "")		///< Google Play Store
	DEFINE_STRINGID (kAmazonAppStore, "")		///< Amazon AppStore
	DEFINE_STRINGID (kSamsungGalaxyStore, "")	///< Samsung Galaxy Store
}

//************************************************************************************************
// PlatformStoreManager
/** Abstraction of platform-specific app store used for in-app purchases. */
//************************************************************************************************

class PlatformStoreManager: public Object,
							public ExternalSingleton<PlatformStoreManager>

{
public:
	DECLARE_CLASS_ABSTRACT (PlatformStoreManager, Object)

	/** Load app-specific configuration from "appstore.config" (optional). */
	static bool loadAppStoreConfiguration ();

	/** Get platform store ID. */
	virtual StringID getID () const = 0;
	
	/** Register for notifications on application startup. */
	virtual IAsyncOperation* startup () { return AsyncOperation::createCompleted (); }
	
	/** Unregister from notifications on application shutdown. */
	virtual void shutdown () {}

	/** Request information from store for products with given identifiers. */
	virtual IAsyncOperation* requestProducts (const ConstVector<String>& productIds) { return nullptr; }
	
	/** Initiate purchase of given product. */
	virtual IAsyncOperation* purchaseProduct (StringRef productId) { return nullptr; }

	/** Get currently open transactions. */
	virtual IAsyncOperation* getTransactions ()  { return nullptr; }

	/** Get locally stored licenses (doesn't require store connection). */
	virtual IAsyncOperation* getLocalLicenses () { return nullptr; }

	/** Restore purchases made by user (e.g. when restoring device from backup). */
	virtual IAsyncOperation* restorePurchases () { return nullptr; }
	
	// Notifications
	DECLARE_STRINGID_MEMBER (kTransactionsChanged) ///< state of one or more transactions changed
	DECLARE_STRINGID_MEMBER (kLocalLicensesChanged) ///< state of one or more licenses changed
};

} // namespac CCL

#endif // _ccl_platformstoremanager_h
