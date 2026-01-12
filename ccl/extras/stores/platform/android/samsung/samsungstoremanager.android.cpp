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
// Filename    : ccl/extras/stores/platform/android/samsung/samsungstoremanager.android.cpp
// Description : Store Manager using Samsung In-App Purchase SDK
//
//************************************************************************************************

#include "ccl/extras/stores/platform/android/samsung/samsungstoremanager.android.h"

#include "ccl/base/message.h"
#include "ccl/base/storage/configuration.h"

namespace CCL {
namespace Android {

//************************************************************************************************
// com.samsung.android.sdk.iap.lib.vo.ProductVo
//************************************************************************************************

DECLARE_JNI_CLASS (ProductVo, "com/samsung/android/sdk/iap/lib/vo/ProductVo")
	DECLARE_JNI_METHOD (jstring, getItemId)
	DECLARE_JNI_METHOD (jstring, getItemName)
	DECLARE_JNI_METHOD (jstring, getItemPriceString)
END_DECLARE_JNI_CLASS (ProductVo)

DEFINE_JNI_CLASS (ProductVo)
	DEFINE_JNI_METHOD (getItemId, "()Ljava/lang/String;")
	DEFINE_JNI_METHOD (getItemName, "()Ljava/lang/String;")
	DEFINE_JNI_METHOD (getItemPriceString, "()Ljava/lang/String;")
END_DEFINE_JNI_CLASS

//************************************************************************************************
// com.samsung.android.sdk.iap.lib.vo.OwnedProductVo
//************************************************************************************************

DECLARE_JNI_CLASS (OwnedProductVo, "com/samsung/android/sdk/iap/lib/vo/OwnedProductVo")
	DECLARE_JNI_METHOD (jstring, getItemId)
END_DECLARE_JNI_CLASS (OwnedProductVo)

DEFINE_JNI_CLASS (OwnedProductVo)
	DEFINE_JNI_METHOD (getItemId, "()Ljava/lang/String;")
END_DEFINE_JNI_CLASS

//************************************************************************************************
// dev.ccl.SamsungStoreContext
//************************************************************************************************

DECLARE_JNI_CLASS (SamsungStoreContext, "dev/ccl/cclextras/stores/SamsungStoreContext")
	DECLARE_JNI_CONSTRUCTOR (construct, int)
	DECLARE_JNI_METHOD (void, requestProducts, JniIntPtr, jobjectArray)
	DECLARE_JNI_METHOD (bool, purchaseProduct, JniIntPtr, jstring)
	DECLARE_JNI_METHOD (bool, queryPurchases, JniIntPtr)
END_DECLARE_JNI_CLASS (SamsungStoreContext)

DEFINE_JNI_CLASS (SamsungStoreContext)
	DEFINE_JNI_CONSTRUCTOR (construct, "(I)V")
	DEFINE_JNI_METHOD (requestProducts, "(J[Ljava/lang/String;)V")
	DEFINE_JNI_METHOD (purchaseProduct, "(JLjava/lang/String;)Z")
	DEFINE_JNI_METHOD (queryPurchases, "(J)Z")
END_DEFINE_JNI_CLASS

} // namespace Android
} // namespace CCL

using namespace CCL;
using namespace Android;
using namespace Core::Java;

//************************************************************************************************
// SamsungStoreManager
//************************************************************************************************

SamsungStoreManager::SamsungStoreManager ()
{
	transactions->objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* SamsungStoreManager::startup ()
{
	// check whether to use production or test mode
	String appStoreMode;
	Configuration::Registry& registry = Configuration::Registry::instance ();
	registry.getValue (appStoreMode, "CCL.Android", "AppStoreMode");

	OperationMode operationMode = OperationMode::OPERATION_MODE_PRODUCTION;
	if(appStoreMode == "test")
		operationMode = OperationMode::OPERATION_MODE_TEST;
	else if(appStoreMode == "failure")
		operationMode = OperationMode::OPERATION_MODE_TEST_FAILURE;

	// create Java SamsungStoreContext object
	JniAccessor jni;
	context.assign (jni, jni.newObject (SamsungStoreContext, SamsungStoreContext.construct, operationMode));

	return AsyncOperation::createCompleted ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SamsungStoreManager::shutdown ()
{
	if(context)
	{
		JniAccessor jni;
		context.assign (jni, nullptr);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL::IAsyncOperation* SamsungStoreManager::requestProducts (const ConstVector<String>& productIds)
{
	if(productIds.isEmpty ())
	{
		AutoPtr<ObjectArray> products = NEW ObjectArray;
		return AsyncOperation::createCompleted (Variant (products->asUnknown (), true), true);
	}

	JniAccessor jni;
	JniStringArray jProductIds (jni, productIds.count ());
	for(int i = 0; i < productIds.count (); i++)
	{
		MutableCString productId (productIds[i], Text::kUTF8);
		jProductIds.setElement (i, productId);
	}

	AsyncOperation* op = NEW AsyncOperation;
	SamsungStoreContext.requestProducts (context, JniCast<AsyncOperation>::toIntPtr (op), jProductIds);
	return op;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL::IAsyncOperation* SamsungStoreManager::purchaseProduct (StringRef productId)
{
	static int nextTransactionId = 0;

	StoreTransaction* transaction = NEW StoreTransaction;
	transaction->setTransactionID (String (productId).append ("-").appendIntValue (nextTransactionId++));
	transaction->setProductID (productId);
	transaction->setState (CCL::PurchaseState::kInProgress);

	transactions->add (transaction);
	deferSignal (NEW Message (kTransactionsChanged));

	JniAccessor jni;
	JniString jProductId (jni, StringChars (productId), productId.length ());

	AutoPtr<AsyncOperation> op = NEW AsyncOperation;
	if(!SamsungStoreContext.purchaseProduct (context, JniCast<AsyncOperation>::toIntPtr (op), jProductId))
		return AsyncOperation::createFailed (true);

	op->setResult (Variant (transaction->asUnknown (), true));

	return op.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL::IAsyncOperation* SamsungStoreManager::getTransactions ()
{
	return AsyncOperation::createCompleted (Variant (transactions->asUnknown (), true), true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL::IAsyncOperation* SamsungStoreManager::getLocalLicenses ()
{
	AutoPtr<AsyncOperation> op = NEW AsyncOperation;
	if(!SamsungStoreContext.queryPurchases (context, JniCast<AsyncOperation>::toIntPtr (op)))
		return AsyncOperation::createFailed (true);

	return op.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SamsungStoreManager::onRequestProductsCompleted (AsyncOperation* operation, RequestErrorCode errorCode, JniObjectArray jProducts)
{
	if(errorCode != RequestErrorCode::IAP_ERROR_NONE)
	{
		operation->setStateDeferred (IAsyncInfo::kFailed);
		return;
	}

	AutoPtr<ObjectArray> products = NEW ObjectArray;
	products->objectCleanup (true);

	JniAccessor jni;
	int length = jProducts.getLength ();
	for(int i = 0; i < length; i++)
	{
		LocalRef jProduct (jni, jProducts[i]);
		LocalStringRef jId (jni, ProductVo.getItemId (jProduct));
		LocalStringRef jName (jni, ProductVo.getItemName (jProduct));
		LocalStringRef jPrice (jni, ProductVo.getItemPriceString (jProduct));

		StoreProduct* product = NEW StoreProduct;

		product->setID (fromJavaString (jId));
		product->setName (fromJavaString (jName));
		product->setPrice (fromJavaString (jPrice));

		products->add (product);
	}

	operation->setResult (Variant (products->asUnknown (), true));
	operation->setStateDeferred (IAsyncInfo::kCompleted);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SamsungStoreManager::onPurchaseCompleted (AsyncOperation* operation, RequestErrorCode errorCode, jobject purchase)
{
	JniAccessor jni;
	StoreTransaction* transaction = unknown_cast<StoreTransaction> (operation->getResult ().asUnknown ());
	switch(errorCode)
	{
	case RequestErrorCode::IAP_ERROR_NONE:
	case RequestErrorCode::IAP_ERROR_ALREADY_PURCHASED:
		transaction->setState (PurchaseState::kCompleted);
		break;
	case RequestErrorCode::IAP_ERROR_CONFIRM_INBOX:
		transaction->setState (PurchaseState::kDeferred);
		break;
	case RequestErrorCode::IAP_PAYMENT_IS_CANCELED:
		transaction->setState (PurchaseState::kCanceled);
		break;
	default:
		transaction->setState (PurchaseState::kFailed);
		break;
	}

	deferSignal (NEW Message (kTransactionsChanged));

	if(transaction->getState () == PurchaseState::kCompleted)
		deferSignal (NEW Message (kLocalLicensesChanged));

	if(transaction->getState () == PurchaseState::kFailed)
		operation->setStateDeferred (IAsyncInfo::kFailed);
	else
		operation->setStateDeferred (IAsyncInfo::kCompleted);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SamsungStoreManager::onQueryPurchasesCompleted (AsyncOperation* operation, RequestErrorCode errorCode, JniObjectArray ownedProducts)
{
	if(errorCode != RequestErrorCode::IAP_ERROR_NONE)
	{
		operation->setStateDeferred (IAsyncInfo::kFailed);
		return;
	}

	AutoPtr<ObjectArray> licenses = NEW ObjectArray;
	licenses->objectCleanup (true);

	JniAccessor jni;
	int length = ownedProducts.getLength ();
	for(int i = 0; i < length; i++)
	{
		LocalRef ownedProduct (jni, ownedProducts[i]);
		LocalStringRef jProductId (jni, OwnedProductVo.getItemId (ownedProduct));

		StoreLicense* license = NEW StoreLicense;

		license->setProductID (fromJavaString (jProductId));
		license->setVerificationResult (LicenseVerificationResult::kValid);

		licenses->add (license);
	}

	operation->setResult (Variant (licenses->asUnknown (), true));
	operation->setStateDeferred (IAsyncInfo::kCompleted);
}

//************************************************************************************************
// SamsungStoreContext Java native methods
//************************************************************************************************

DECLARE_JNI_CLASS_METHOD (dev_ccl_cclextras_stores, void, SamsungStoreContext, onRequestProductsCompletedNative, JniIntPtr nativeOperation, jint errorCode, jobjectArray products)
{
	if(SamsungStoreManager* manager = ccl_cast<SamsungStoreManager> (&PlatformStoreManager::instance ()))
		manager->onRequestProductsCompleted (JniCast<AsyncOperation>::fromIntPtr (nativeOperation), RequestErrorCode (errorCode), JniObjectArray (env, products));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD (dev_ccl_cclextras_stores, void, SamsungStoreContext, onPurchaseCompletedNative, JniIntPtr nativeOperation, jint errorCode, jobject purchase)
{
	if(SamsungStoreManager* manager = ccl_cast<SamsungStoreManager> (&PlatformStoreManager::instance ()))
		manager->onPurchaseCompleted (JniCast<AsyncOperation>::fromIntPtr (nativeOperation), RequestErrorCode (errorCode), purchase);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD (dev_ccl_cclextras_stores, void, SamsungStoreContext, onQueryPurchasesCompletedNative, JniIntPtr nativeOperation, jint errorCode, jobjectArray ownedProducts)
{
	if(SamsungStoreManager* manager = ccl_cast<SamsungStoreManager> (&PlatformStoreManager::instance ()))
		manager->onQueryPurchasesCompleted (JniCast<AsyncOperation>::fromIntPtr (nativeOperation), RequestErrorCode (errorCode), JniObjectArray (env, ownedProducts));
}

DEFINE_CLASS_HIDDEN (SamsungStoreManager, PlatformStoreManager)
