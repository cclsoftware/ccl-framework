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
// Filename    : ccl/extras/stores/platform/android/android/amazonstoremanager.android.cpp
// Description : Store Manager using Amazon AppStore API
//
//************************************************************************************************

#include "ccl/extras/stores/platform/android/amazon/amazonstoremanager.android.h"

#include "ccl/base/message.h"

namespace CCL {
namespace Android {

//************************************************************************************************
// com.amazon.device.iap.model.Product
//************************************************************************************************

DECLARE_JNI_CLASS (Product, "com/amazon/device/iap/model/Product")
	DECLARE_JNI_METHOD (jstring, getSku)
	DECLARE_JNI_METHOD (jstring, getTitle)
	DECLARE_JNI_METHOD (jstring, getPrice)
END_DECLARE_JNI_CLASS (Product)

DEFINE_JNI_CLASS (Product)
	DEFINE_JNI_METHOD (getSku, "()Ljava/lang/String;")
	DEFINE_JNI_METHOD (getTitle, "()Ljava/lang/String;")
	DEFINE_JNI_METHOD (getPrice, "()Ljava/lang/String;")
END_DEFINE_JNI_CLASS

//************************************************************************************************
// com.amazon.device.iap.model.Receipt
//************************************************************************************************

DECLARE_JNI_CLASS (Receipt, "com/amazon/device/iap/model/Receipt")
	DECLARE_JNI_METHOD (jstring, getReceiptId)
	DECLARE_JNI_METHOD (jstring, getSku)
	DECLARE_JNI_METHOD (bool, isCanceled)
END_DECLARE_JNI_CLASS (Receipt)

DEFINE_JNI_CLASS (Receipt)
	DEFINE_JNI_METHOD (getReceiptId, "()Ljava/lang/String;")
	DEFINE_JNI_METHOD (getSku, "()Ljava/lang/String;")
	DEFINE_JNI_METHOD (isCanceled, "()Z")
END_DEFINE_JNI_CLASS

//************************************************************************************************
// dev.ccl.AmazonStoreContext
//************************************************************************************************

DECLARE_JNI_CLASS (AmazonStoreContext, "dev/ccl/cclextras/stores/AmazonStoreContext")
	DECLARE_JNI_CONSTRUCTOR (construct)
	DECLARE_JNI_METHOD (jstring, requestProducts, JniIntPtr, jobjectArray)
	DECLARE_JNI_METHOD (jstring, purchaseProduct, JniIntPtr, jstring)
	DECLARE_JNI_METHOD (jstring, queryPurchases, JniIntPtr)
END_DECLARE_JNI_CLASS (AmazonStoreContext)

DEFINE_JNI_CLASS (AmazonStoreContext)
	DEFINE_JNI_CONSTRUCTOR (construct, "()V")
	DEFINE_JNI_METHOD (requestProducts, "(J[Ljava/lang/String;)Ljava/lang/String;")
	DEFINE_JNI_METHOD (purchaseProduct, "(JLjava/lang/String;)Ljava/lang/String;")
	DEFINE_JNI_METHOD (queryPurchases, "(J)Ljava/lang/String;")
END_DEFINE_JNI_CLASS

} // namespace Android
} // namespace CCL

using namespace CCL;
using namespace Android;
using namespace Core::Java;

//************************************************************************************************
// AmazonStoreManager
//************************************************************************************************

AmazonStoreManager::AmazonStoreManager ()
{
	transactions->objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* AmazonStoreManager::startup ()
{
	// create Java AmazonStoreContext object
	JniAccessor jni;
	context.assign (jni, jni.newObject (AmazonStoreContext, AmazonStoreContext.construct));

	return AsyncOperation::createCompleted ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AmazonStoreManager::shutdown ()
{
	if(context)
	{
		JniAccessor jni;
		context.assign (jni, nullptr);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL::IAsyncOperation* AmazonStoreManager::requestProducts (const ConstVector<String>& productIds)
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
	AmazonStoreContext.requestProducts (context, JniCast<AsyncOperation>::toIntPtr (op), jProductIds);
	return op;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL::IAsyncOperation* AmazonStoreManager::purchaseProduct (StringRef productId)
{
	JniAccessor jni;
	JniString jProductId (jni, StringChars (productId), productId.length ());

	AsyncOperation* op = NEW AsyncOperation;
	LocalStringRef jRequestId (jni, AmazonStoreContext.purchaseProduct (context, JniCast<AsyncOperation>::toIntPtr (op), jProductId));

	StoreTransaction* transaction = NEW StoreTransaction;
	transaction->setTransactionID (fromJavaString (jRequestId));
	transaction->setProductID (productId);
	transaction->setState (CCL::PurchaseState::kInProgress);

	transactions->add (transaction);
	deferSignal (NEW Message (kTransactionsChanged));

	op->setResult (Variant (transaction->asUnknown (), true));
	return op;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL::IAsyncOperation* AmazonStoreManager::getTransactions ()
{
	return AsyncOperation::createCompleted (Variant (transactions->asUnknown (), true), true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL::IAsyncOperation* AmazonStoreManager::getLocalLicenses ()
{
	AsyncOperation* op = NEW AsyncOperation;
	AmazonStoreContext.queryPurchases (context, JniCast<AsyncOperation>::toIntPtr (op));
	return op;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AmazonStoreManager::onRequestProductsCompleted (AsyncOperation* operation, ProductDataRequestStatus productDataRequestStatus, JniObjectArray jProducts)
{
	if(productDataRequestStatus != ProductDataRequestStatus::SUCCESSFUL)
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
		LocalStringRef jId (jni, Product.getSku (jProduct));
		LocalStringRef jName (jni, Product.getTitle (jProduct));
		LocalStringRef jPrice (jni, Product.getPrice (jProduct));

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

void AmazonStoreManager::onPurchaseCompleted (AsyncOperation* operation, PurchaseRequestStatus purchaseRequestStatus, jobject receipt)
{
	StoreTransaction* transaction = unknown_cast<StoreTransaction> (operation->getResult ().asUnknown ());
	switch(purchaseRequestStatus)
	{
	case PurchaseRequestStatus::SUCCESSFUL:
	case PurchaseRequestStatus::ALREADY_PURCHASED:
		if(Receipt.isCanceled (receipt))
			transaction->setState (PurchaseState::kCanceled);
		else
			transaction->setState (PurchaseState::kCompleted);
		break;
	case PurchaseRequestStatus::PENDING:
		transaction->setState (PurchaseState::kDeferred);
		break;
	default:
	case PurchaseRequestStatus::FAILED:
	case PurchaseRequestStatus::NOT_SUPPORTED:
	case PurchaseRequestStatus::INVALID_SKU:
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

void AmazonStoreManager::onQueryPurchasesCompleted (AsyncOperation* operation, PurchaseUpdatesRequestStatus purchaseUpdatesRequestStatus, JniObjectArray receipts)
{
	if(purchaseUpdatesRequestStatus != PurchaseUpdatesRequestStatus::SUCCESSFUL)
	{
		operation->setStateDeferred (IAsyncInfo::kFailed);
		return;
	}

	AutoPtr<ObjectArray> licenses = NEW ObjectArray;
	licenses->objectCleanup (true);

	JniAccessor jni;
	int length = receipts.getLength ();
	for(int i = 0; i < length; i++)
	{
		LocalRef receipt (jni, receipts[i]);
		LocalStringRef jProductId (jni, Receipt.getSku (receipt));
		bool canceled = Receipt.isCanceled (receipt);

		StoreLicense* license = NEW StoreLicense;

		license->setProductID (fromJavaString (jProductId));
		license->setVerificationResult (canceled ? LicenseVerificationResult::kInvalid : LicenseVerificationResult::kValid);

		licenses->add (license);
	}

	operation->setResult (Variant (licenses->asUnknown (), true));
	operation->setStateDeferred (IAsyncInfo::kCompleted);
}

//************************************************************************************************
// AmazonStoreContext Java native methods
//************************************************************************************************

DECLARE_JNI_CLASS_METHOD (dev_ccl_cclextras_stores, void, AmazonStoreContext, onRequestProductsCompletedNative, JniIntPtr nativeOperation, jint productDataRequestStatus, jobjectArray products)
{
	if(AmazonStoreManager* manager = ccl_cast<AmazonStoreManager> (&PlatformStoreManager::instance ()))
		manager->onRequestProductsCompleted (JniCast<AsyncOperation>::fromIntPtr (nativeOperation), ProductDataRequestStatus (productDataRequestStatus), JniObjectArray (env, products));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD (dev_ccl_cclextras_stores, void, AmazonStoreContext, onPurchaseCompletedNative, JniIntPtr nativeOperation, jint purchaseRequestStatus, jobject receipt)
{
	if(AmazonStoreManager* manager = ccl_cast<AmazonStoreManager> (&PlatformStoreManager::instance ()))
		manager->onPurchaseCompleted (JniCast<AsyncOperation>::fromIntPtr (nativeOperation), PurchaseRequestStatus (purchaseRequestStatus), receipt);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD (dev_ccl_cclextras_stores, void, AmazonStoreContext, onQueryPurchasesCompletedNative, JniIntPtr nativeOperation, jint purchaseUpdatesRequestStatus, jobjectArray receipts)
{
	if(AmazonStoreManager* manager = ccl_cast<AmazonStoreManager> (&PlatformStoreManager::instance ()))
		manager->onQueryPurchasesCompleted (JniCast<AsyncOperation>::fromIntPtr (nativeOperation), PurchaseUpdatesRequestStatus (purchaseUpdatesRequestStatus), JniObjectArray (env, receipts));
}

DEFINE_CLASS_HIDDEN (AmazonStoreManager, PlatformStoreManager)
