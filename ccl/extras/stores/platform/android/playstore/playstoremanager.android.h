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
// Filename    : ccl/extras/stores/platform/android/playstore/playstoremanager.android.h
// Description : Google Play Store Manager using Google Play Billing API
//
//************************************************************************************************

#ifndef _playstoremanager_android_h
#define _playstoremanager_android_h

#include "ccl/extras/stores/platformstoremanager.h"

#include "ccl/platform/android/cclandroidjni.h"

namespace CCL {
namespace Android {

//************************************************************************************************
// Android::BillingResult
//************************************************************************************************

enum class BillingResult
{
	OK = 0,
	USER_CANCELED = 1,

	SERVICE_UNAVAILABLE = 2,
	BILLING_UNAVAILABLE = 3,
	ITEM_UNAVAILABLE = 4,

	DEVELOPER_ERROR = 5,
	ERROR = 6,

	ITEM_ALREADY_OWNED = 7,
	ITEM_NOT_OWNED = 8,

	NETWORK_ERROR = 12,

	SERVICE_DISCONNECTED = -1,
	SERVICE_TIMEOUT = -3,

	FEATURE_NOT_SUPPORTED = -2
};

//************************************************************************************************
// Android::PurchaseState
//************************************************************************************************

enum class PurchaseState
{
	UNSPECIFIED_STATE = 0,

	PURCHASED = 1,
	PENDING = 2
};

//************************************************************************************************
// Android::PlayStoreManager
//************************************************************************************************

class PlayStoreManager: public PlatformStoreManager
{
public:
	DECLARE_CLASS (PlayStoreManager, PlatformStoreManager)

	PlayStoreManager ();

	void onSetupFinished (AsyncOperation* operation, BillingResult billingResult);
	void onRequestProductsCompleted (AsyncOperation* operation, BillingResult billingResult, Core::Java::JniObjectArray skuDetailsList);
	void onQueryPurchasesCompleted (AsyncOperation* operation, BillingResult billingResult, Core::Java::JniObjectArray purchases);
	void onPurchasesUpdated (BillingResult billingResult, Core::Java::JniObjectArray purchases);

	// PlatformStoreManager
	StringID getID () const override { return PlatformStoreID::kGooglePlayStore; }

	IAsyncOperation* startup () override;
	void shutdown () override;

	IAsyncOperation* requestProducts (const ConstVector<String>& productIds) override;
	IAsyncOperation* purchaseProduct (StringRef productId) override;
	IAsyncOperation* getTransactions () override;
	IAsyncOperation* getLocalLicenses () override;

private:
	JniObject context;

	AutoPtr<ObjectArray> transactions = NEW ObjectArray;
	AsyncOperation* pendingPurchaseOperation = nullptr;

	bool getPurchaseProductId (String& productId, jobject purchase) const;
	CCL::PurchaseState determinePurchaseState (BillingResult result, PurchaseState purchaseState) const;

	CCL::LicenseVerificationResult verifyLicense (jobject purchase) const;
};

} // namespace Android
} // namespace CCL

#endif // _playstoremanager_android_h
