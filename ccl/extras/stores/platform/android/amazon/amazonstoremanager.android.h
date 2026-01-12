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
// Filename    : ccl/extras/stores/platform/android/amazon/amazonstoremanager.android.h
// Description : Store Manager using Amazon AppStore API
//
//************************************************************************************************

#ifndef _amazonstoremanager_android_h
#define _amazonstoremanager_android_h

#include "ccl/extras/stores/platformstoremanager.h"

#include "ccl/platform/android/cclandroidjni.h"

namespace CCL {
namespace Android {

//************************************************************************************************
// Android::ProductDataRequestStatus
//************************************************************************************************

enum class ProductDataRequestStatus
{
	SUCCESSFUL = 0,
	FAILED = 1,
	NOT_SUPPORTED = 2
};

//************************************************************************************************
// Android::PurchaseRequestStatus
//************************************************************************************************

enum class PurchaseRequestStatus
{
	SUCCESSFUL = 0,
	FAILED = 1,
	INVALID_SKU = 2,
	ALREADY_PURCHASED = 3,
	PENDING = 4,
	NOT_SUPPORTED = 5
};

//************************************************************************************************
// Android::PurchaseUpdatesRequestStatus
//************************************************************************************************

enum class PurchaseUpdatesRequestStatus
{
	SUCCESSFUL = 0,
	FAILED = 1,
	NOT_SUPPORTED = 2
};

//************************************************************************************************
// Android::AmazonStoreManager
//************************************************************************************************

class AmazonStoreManager: public PlatformStoreManager
{
public:
	DECLARE_CLASS (AmazonStoreManager, PlatformStoreManager)

	AmazonStoreManager ();

	void onRequestProductsCompleted (AsyncOperation* operation, ProductDataRequestStatus productDataRequestStatus, Core::Java::JniObjectArray products);
	void onPurchaseCompleted (AsyncOperation* operation, PurchaseRequestStatus purchaseRequestStatus, jobject receipt);
	void onQueryPurchasesCompleted (AsyncOperation* operation, PurchaseUpdatesRequestStatus purchaseUpdatesRequestStatus, Core::Java::JniObjectArray receipts);

	// PlatformStoreManager
	StringID getID () const override { return PlatformStoreID::kAmazonAppStore; }

	IAsyncOperation* startup () override;
	void shutdown () override;

	IAsyncOperation* requestProducts (const ConstVector<String>& productIds) override;
	IAsyncOperation* purchaseProduct (StringRef productId) override;
	IAsyncOperation* getTransactions () override;
	IAsyncOperation* getLocalLicenses () override;

private:
	JniObject context;

	AutoPtr<ObjectArray> transactions = NEW ObjectArray;
};

} // namespace Android
} // namespace CCL

#endif // _amazonstoremanager_android_h
