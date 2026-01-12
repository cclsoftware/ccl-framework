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
// Filename    : ccl/extras/stores/platform/android/playstore/playstoremanager.android.cpp
// Description : Google Play Store Manager using Google Play Billing API
//
//************************************************************************************************

#include "ccl/extras/stores/platform/android/playstore/playstoremanager.android.h"

#include "ccl/base/message.h"
#include "ccl/base/security/cipher.h"
#include "ccl/base/security/cryptobox.h"

#include "ccl/public/securityservices.h"

namespace CCL {
namespace Android {

//************************************************************************************************
// com.android.billingclient.api.ProductDetails
//************************************************************************************************

DECLARE_JNI_CLASS (ProductDetails, "com/android/billingclient/api/ProductDetails")
	DECLARE_JNI_METHOD (jstring, getProductId)
	DECLARE_JNI_METHOD (jstring, getTitle)
	DECLARE_JNI_METHOD (jobject, getOneTimePurchaseOfferDetails)
END_DECLARE_JNI_CLASS (ProductDetails)

DEFINE_JNI_CLASS (ProductDetails)
	DEFINE_JNI_METHOD (getProductId, "()Ljava/lang/String;")
	DEFINE_JNI_METHOD (getTitle, "()Ljava/lang/String;")
	DEFINE_JNI_METHOD (getOneTimePurchaseOfferDetails, "()Lcom/android/billingclient/api/ProductDetails$OneTimePurchaseOfferDetails;")
END_DEFINE_JNI_CLASS

//************************************************************************************************
// com.android.billingclient.api.ProductDetails.OneTimePurchaseOfferDetails
//************************************************************************************************

DECLARE_JNI_CLASS (OneTimePurchaseOfferDetails, "com/android/billingclient/api/ProductDetails$OneTimePurchaseOfferDetails")
	DECLARE_JNI_METHOD (jstring, getFormattedPrice)
END_DECLARE_JNI_CLASS (OneTimePurchaseOfferDetails)

DEFINE_JNI_CLASS (OneTimePurchaseOfferDetails)
	DEFINE_JNI_METHOD (getFormattedPrice, "()Ljava/lang/String;")
END_DEFINE_JNI_CLASS

//************************************************************************************************
// com.android.billingclient.api.Purchase
//************************************************************************************************

DECLARE_JNI_CLASS (Purchase, "com/android/billingclient/api/Purchase")
	DECLARE_JNI_METHOD (jstring, getOrderId)
	DECLARE_JNI_METHOD (jobject, getProducts)
	DECLARE_JNI_METHOD (int, getPurchaseState)
	DECLARE_JNI_METHOD (jstring, getOriginalJson)
	DECLARE_JNI_METHOD (jstring, getSignature)
END_DECLARE_JNI_CLASS (Purchase)

DEFINE_JNI_CLASS (Purchase)
	DEFINE_JNI_METHOD (getOrderId, "()Ljava/lang/String;")
	DEFINE_JNI_METHOD (getProducts, "()Ljava/util/List;")
	DEFINE_JNI_METHOD (getPurchaseState, "()I")
	DEFINE_JNI_METHOD (getOriginalJson, "()Ljava/lang/String;")
	DEFINE_JNI_METHOD (getSignature, "()Ljava/lang/String;")
END_DEFINE_JNI_CLASS

//************************************************************************************************
// dev.ccl.PlayStoreContext
//************************************************************************************************

DECLARE_JNI_CLASS (PlayStoreContext, "dev/ccl/cclextras/stores/PlayStoreContext")
	DECLARE_JNI_CONSTRUCTOR (construct)
	DECLARE_JNI_METHOD (void, connect, JniIntPtr)
	DECLARE_JNI_METHOD (void, terminate)
	DECLARE_JNI_METHOD (bool, requestProducts, JniIntPtr, jarray)
	DECLARE_JNI_METHOD (bool, purchaseProduct, jstring)
	DECLARE_JNI_METHOD (bool, queryPurchases, JniIntPtr)
END_DECLARE_JNI_CLASS (PlayStoreContext)

DEFINE_JNI_CLASS (PlayStoreContext)
	DEFINE_JNI_CONSTRUCTOR (construct, "()V")
	DEFINE_JNI_METHOD (connect, "(J)V")
	DEFINE_JNI_METHOD (terminate, "()V")
	DEFINE_JNI_METHOD (requestProducts, "(J[Ljava/lang/String;)Z")
	DEFINE_JNI_METHOD (purchaseProduct, "(Ljava/lang/String;)Z")
	DEFINE_JNI_METHOD (queryPurchases, "(J)Z")
END_DEFINE_JNI_CLASS

} // namespace Android
} // namespace CCL

using namespace CCL;
using namespace Android;
using namespace Security;
using namespace Core::Java;

//************************************************************************************************
// PlayStoreManager
//************************************************************************************************

PlayStoreManager::PlayStoreManager ()
{
	transactions->objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlayStoreManager::getPurchaseProductId (String& productId, jobject purchase) const
{
	// note that the billing API supports multiple products per purchase, but
	// we currently support only one in PlatformStoreManager::purchaseProduct;
	// we can thus assume only one product is included in the purchase
	JniAccessor jni;
	LocalRef products (jni, Purchase.getProducts (purchase));
	if(List.size (products) == 0)
		return false;

	LocalStringRef jProductId (jni, jobject_cast<jstring> (List.get (products, 0)));
	productId = fromJavaString (jProductId);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL::PurchaseState PlayStoreManager::determinePurchaseState (BillingResult result, PurchaseState purchaseState) const
{
	CCL::PurchaseState state = CCL::PurchaseState::kFailed;
	switch(result)
	{
	case BillingResult::OK:
	case BillingResult::ITEM_ALREADY_OWNED:
		if(purchaseState == PurchaseState::PENDING)
			state = CCL::PurchaseState::kDeferred;
		else
			state = CCL::PurchaseState::kCompleted;
		break;
	case BillingResult::USER_CANCELED:
		state = CCL::PurchaseState::kCanceled;
		break;
	}

	return state;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL::LicenseVerificationResult PlayStoreManager::verifyLicense (jobject purchase) const
{
	Crypto::Material encryptedPublicKey;
	Crypto::ICryptoKeyStore& store = System::GetCryptoKeyStore ();
	store.getMaterial (encryptedPublicKey, PlatformStoreID::kGooglePlayStore, Crypto::kPublicKey);

	Crypto::Material publicKey;
	Crypto::Cipher cipher;
	cipher.setFromKeyStore (PlatformStoreID::kGooglePlayStore);
	cipher.decrypt (publicKey, encryptedPublicKey);

	JniAccessor jni;
	LocalStringRef jReceiptJson (jni, Purchase.getOriginalJson (purchase));
	LocalStringRef jSignatureBase64 (jni, Purchase.getSignature (purchase));

	Crypto::Material receipt = Crypto::Material ().append (fromJavaString (jReceiptJson), Text::kASCII);
	Crypto::Material signature = Crypto::Material ().fromBase64 (fromJavaString (jSignatureBase64));

	if(!Crypto::RSA::verify (receipt, publicKey, signature))
		return LicenseVerificationResult::kInvalid;

	return LicenseVerificationResult::kValid;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* PlayStoreManager::startup ()
{
	// create Java PlayStoreContext object
	JniAccessor jni;
	context.assign (jni, jni.newObject (PlayStoreContext, PlayStoreContext.construct));
	if(!context)
		return AsyncOperation::createFailed (true);

	AutoPtr<AsyncOperation> op = NEW AsyncOperation;
	PlayStoreContext.connect (context, JniCast<AsyncOperation>::toIntPtr (op));
	return op.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlayStoreManager::shutdown ()
{
	if(context)
	{
		JniAccessor jni;
		PlayStoreContext.terminate (context);
		context.assign (jni, nullptr);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL::IAsyncOperation* PlayStoreManager::requestProducts (const ConstVector<String>& productIds)
{
	JniAccessor jni;
	JniStringArray jProductIds (jni, productIds.count ());
	for(int i = 0; i < productIds.count (); i++)
	{
		MutableCString productId (productIds[i], Text::kUTF8);
		jProductIds.setElement (i, productId);
	}

	AutoPtr<AsyncOperation> op = NEW AsyncOperation;
	if(!PlayStoreContext.requestProducts (context, JniCast<AsyncOperation>::toIntPtr (op), jProductIds))
		return AsyncOperation::createFailed (true);

	return op.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL::IAsyncOperation* PlayStoreManager::purchaseProduct (StringRef productId)
{
	// the billing API supports only one purchase operation at a time
	if(pendingPurchaseOperation != nullptr)
		return AsyncOperation::createFailed (true);

	static int nextTransactionId = 0;

	AutoPtr<StoreTransaction> transaction = NEW StoreTransaction;
	transaction->setTransactionID (String (productId).append ("-").appendIntValue (nextTransactionId++));
	transaction->setProductID (productId);
	transaction->setState (CCL::PurchaseState::kInProgress);

	transactions->add (return_shared<StoreTransaction> (transaction));
	deferSignal (NEW Message (kTransactionsChanged));

	JniAccessor jni;
	JniString jProductId (jni, StringChars (productId), productId.length ());

	pendingPurchaseOperation = NEW AsyncOperation;
	pendingPurchaseOperation->setResult (Variant (transaction->asUnknown (), true));
	if(!PlayStoreContext.purchaseProduct (context, jProductId))
	{
		pendingPurchaseOperation->setStateDeferred (IAsyncInfo::kFailed);
		return pendingPurchaseOperation;
	}

	Promise promise = Promise (pendingPurchaseOperation).then ([&](IAsyncOperation& op)
	{
		pendingPurchaseOperation = nullptr;
	});

	return return_shared<IAsyncOperation> (promise);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL::IAsyncOperation* PlayStoreManager::getTransactions ()
{
	return AsyncOperation::createCompleted (Variant (transactions->asUnknown (), true), true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL::IAsyncOperation* PlayStoreManager::getLocalLicenses ()
{
	AutoPtr<AsyncOperation> op = NEW AsyncOperation;
	if(!PlayStoreContext.queryPurchases (context, JniCast<AsyncOperation>::toIntPtr (op)))
		return AsyncOperation::createFailed (true);

	return op.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlayStoreManager::onSetupFinished (AsyncOperation* operation, BillingResult billingResult)
{
	if(billingResult == BillingResult::OK)
		operation->setStateDeferred (IAsyncInfo::kCompleted);
	else
		operation->setStateDeferred (IAsyncInfo::kFailed);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlayStoreManager::onRequestProductsCompleted (AsyncOperation* operation, BillingResult billingResult, JniObjectArray productDetailsList)
{
	if(billingResult != BillingResult::OK)
	{
		operation->setStateDeferred (IAsyncInfo::kFailed);
		return;
	}

	AutoPtr<ObjectArray> products = NEW ObjectArray;
	products->objectCleanup (true);

	if(productDetailsList != 0)
	{
		JniAccessor jni;
		int length = productDetailsList.getLength ();
		for(int i = 0; i < length; i++)
		{
			LocalRef productDetails (jni, productDetailsList[i]);
			LocalStringRef jId (jni, ProductDetails.getProductId (productDetails));
			LocalStringRef jName (jni, ProductDetails.getTitle (productDetails));

			LocalRef offerDetails (jni, ProductDetails.getOneTimePurchaseOfferDetails (productDetails));
			LocalStringRef jPrice (jni, OneTimePurchaseOfferDetails.getFormattedPrice (offerDetails));

			StoreProduct* product = NEW StoreProduct;

			product->setID (fromJavaString (jId));
			product->setName (fromJavaString (jName));
			product->setPrice (fromJavaString (jPrice));

			products->add (product);
		}
	}

	operation->setResult (Variant (products->asUnknown (), true));
	operation->setStateDeferred (IAsyncInfo::kCompleted);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlayStoreManager::onQueryPurchasesCompleted (AsyncOperation* operation, BillingResult billingResult, JniObjectArray purchases)
{
	if(billingResult != BillingResult::OK)
	{
		operation->setStateDeferred (IAsyncInfo::kFailed);
		return;
	}

	AutoPtr<ObjectArray> licenses = NEW ObjectArray;
	licenses->objectCleanup (true);

	if(purchases != 0)
	{
		JniAccessor jni;
		int length = purchases.getLength ();
		for(int i = 0; i < length; i++)
		{
			LocalRef purchase (jni, purchases[i]);

			// find product id associated with this purchase
			String productId;
			if(!getPurchaseProductId (productId, purchase))
				continue;

			StoreLicense* license = NEW StoreLicense;
			PurchaseState purchaseState = PurchaseState (Purchase.getPurchaseState (purchase));

			license->setProductID (productId);

			// verify receipt signature
			if(purchaseState == PurchaseState::PURCHASED)
				license->setVerificationResult (verifyLicense (purchase));

			licenses->add (license);
		}
	}

	operation->setResult (Variant (licenses->asUnknown (), true));
	operation->setStateDeferred (IAsyncInfo::kCompleted);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlayStoreManager::onPurchasesUpdated (BillingResult billingResult, JniObjectArray purchases)
{
	JniAccessor jni;

	bool transactionsChanged = false;
	bool licensesChanged = false;

	if(pendingPurchaseOperation) // called at the end of a purchase flow
	{
		PurchaseState purchaseState = PurchaseState::UNSPECIFIED_STATE;
		if(purchases != 0)
		{
			LocalRef purchase (jni, purchases[0]);
			purchaseState = PurchaseState (Purchase.getPurchaseState (purchase));
			if(purchaseState == PurchaseState::PURCHASED)
				licensesChanged = true;
		}

		StoreTransaction* transaction = unknown_cast<StoreTransaction> (pendingPurchaseOperation->getResult ().asUnknown ());
		transaction->setState (determinePurchaseState (billingResult, purchaseState));
		transactionsChanged = true;

		pendingPurchaseOperation->setStateDeferred (IAsyncInfo::kCompleted);
	}
	else if(purchases != 0) // called when a pending purchase is completed
	{
		int length = purchases.getLength ();
		for(int i = 0; i < length; i++)
		{
			LocalRef purchase (jni, purchases[i]);

			// find product id associated with this purchase
			String productId;
			if(!getPurchaseProductId (productId, purchase))
				continue;

			// issue a license change for completed purchases 
			PurchaseState purchaseState = PurchaseState (Purchase.getPurchaseState (purchase));
			if(purchaseState == PurchaseState::PURCHASED)
				licensesChanged = true;

			// look for pending transaction with this product id
			for(auto transaction : iterate_as<StoreTransaction> (*transactions))
			{
				if(transaction->getState () != CCL::PurchaseState::kDeferred)
					continue;

				if(transaction->getProductID () == productId)
				{
					transaction->setState (determinePurchaseState (billingResult, purchaseState));
					transactionsChanged = true;
				}
			}
		}
	}

	if(transactionsChanged)
		deferSignal (NEW Message (kTransactionsChanged));

	if(licensesChanged)
		deferSignal (NEW Message (kLocalLicensesChanged));
}

//************************************************************************************************
// PlayStoreContext Java native methods
//************************************************************************************************

DECLARE_JNI_CLASS_METHOD (dev_ccl_cclextras_stores, void, PlayStoreContext, onSetupFinishedNative, JniIntPtr nativeOperation, jint billingResponseCode)
{
	if(PlayStoreManager* manager = ccl_cast<PlayStoreManager> (&PlatformStoreManager::instance ()))
		manager->onSetupFinished (JniCast<AsyncOperation>::fromIntPtr (nativeOperation), BillingResult (billingResponseCode));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD (dev_ccl_cclextras_stores, void, PlayStoreContext, onRequestProductsCompletedNative, JniIntPtr nativeOperation, jint billingResponseCode, jobjectArray productDetailsList)
{
	if(PlayStoreManager* manager = ccl_cast<PlayStoreManager> (&PlatformStoreManager::instance ()))
		manager->onRequestProductsCompleted (JniCast<AsyncOperation>::fromIntPtr (nativeOperation), BillingResult (billingResponseCode), JniObjectArray (env, productDetailsList));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD (dev_ccl_cclextras_stores, void, PlayStoreContext, onQueryPurchasesCompletedNative, JniIntPtr nativeOperation, jint billingResponseCode, jobjectArray purchases)
{
	if(PlayStoreManager* manager = ccl_cast<PlayStoreManager> (&PlatformStoreManager::instance ()))
		manager->onQueryPurchasesCompleted (JniCast<AsyncOperation>::fromIntPtr (nativeOperation), BillingResult (billingResponseCode), JniObjectArray (env, purchases));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD (dev_ccl_cclextras_stores, void, PlayStoreContext, onPurchasesUpdatedNative, jint billingResponseCode, jobjectArray purchases)
{
	if(PlayStoreManager* manager = ccl_cast<PlayStoreManager> (&PlatformStoreManager::instance ()))
		manager->onPurchasesUpdated (BillingResult (billingResponseCode), JniObjectArray (env, purchases));
}

DEFINE_CLASS_HIDDEN (PlayStoreManager, PlatformStoreManager)
