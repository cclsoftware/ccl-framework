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
// Filename    : ccl/extras/stores/platform/android/samsung/samsungstoremanager.android.h
// Description : Store Manager using Samsung In-App Purchase SDK
//
//************************************************************************************************

#ifndef _samsungstoremanager_android_h
#define _samsungstoremanager_android_h

#include "ccl/extras/stores/platformstoremanager.h"

#include "ccl/platform/android/cclandroidjni.h"

namespace CCL {
namespace Android {

//************************************************************************************************
// Android::OperationMode
//************************************************************************************************

enum class OperationMode
{
	OPERATION_MODE_TEST_FAILURE = -1,
	OPERATION_MODE_PRODUCTION = 0,
	OPERATION_MODE_TEST = 1
};

//************************************************************************************************
// Android::RequestErrorCode
//************************************************************************************************

enum class RequestErrorCode
{
	IAP_ERROR_NONE = 0,
	IAP_PAYMENT_IS_CANCELED = 1,					///< Payment is cancelled
	IAP_ERROR_INITIALIZATION = -1000,				///< IAP initialization error
	IAP_ERROR_NEED_APP_UPGRADE = -1001,				///< IAP need to be upgraded
	IAP_ERROR_COMMON = -1002,						///< Common error
	IAP_ERROR_ALREADY_PURCHASED = -1003,			///< Repurchase NON-CONSUMABLE item
	IAP_ERROR_WHILE_RUNNING = -1004,				///< When PaymentMethodList Activity is called without Bundle data
	IAP_ERROR_PRODUCT_DOES_NOT_EXIST = -1005,		///< does not exist item or item group id
	IAP_ERROR_CONFIRM_INBOX = -1006,				///< After purchase request not received the results can not be determined whether to buy. So, the confirmation of purchase list is needed.
	IAP_ERROR_ITEM_GROUP_DOES_NOT_EXIST = -1007,	///< Error when item group id does not exist
	IAP_ERROR_NETWORK_NOT_AVAILABLE = -1008,		///< Error when network is not available
	IAP_ERROR_IOEXCEPTION_ERROR = -1009,			///< IOException
	IAP_ERROR_SOCKET_TIMEOUT = -1010,				///< SocketTimeoutException
	IAP_ERROR_CONNECT_TIMEOUT = -1011,				///< ConnectTimeoutException
	IAP_ERROR_NOT_EXIST_LOCAL_PRICE = -1012,		///< The Item is not for sale in the country
	IAP_ERROR_NOT_AVAILABLE_SHOP = -1013,			///< IAP is not serviced in the country
	IAP_ERROR_NEED_SA_LOGIN = -1014					///< SA not logged in
};

//************************************************************************************************
// Android::SamsungStoreManager
//************************************************************************************************

class SamsungStoreManager: public PlatformStoreManager
{
public:
	DECLARE_CLASS (SamsungStoreManager, PlatformStoreManager)

	SamsungStoreManager ();

	void onRequestProductsCompleted (AsyncOperation* operation, RequestErrorCode errorCode, Core::Java::JniObjectArray products);
	void onPurchaseCompleted (AsyncOperation* operation, RequestErrorCode errorCode, jobject purchase);
	void onQueryPurchasesCompleted (AsyncOperation* operation, RequestErrorCode errorCode, Core::Java::JniObjectArray ownedProducts);

	// PlatformStoreManager
	StringID getID () const override { return PlatformStoreID::kSamsungGalaxyStore; }

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

#endif // _samsungstoremanager_android_h
