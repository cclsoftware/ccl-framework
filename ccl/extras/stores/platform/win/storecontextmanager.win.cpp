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
// Filename    : ccl/extras/stores/platform/win/storecontextmanager.win.cpp
// Description : Microsoft Store Manager using StoreContext API
//
//************************************************************************************************

#include "ccl/extras/stores/platformstoremanager.h"

#include "ccl/base/message.h"

#include "ccl/public/gui/framework/iwindow.h"
#include "ccl/public/guiservices.h"

#include "ccl/platform/win/system/cclcppwinrt.h"

#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Services.Store.h>

using namespace winrt;

using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Services;
using namespace Windows::Services::Store;

namespace CCL {
namespace CppWinRT {

//************************************************************************************************
// StoreContextManager
//************************************************************************************************

class StoreContextManager: public PlatformStoreManager
{
public:
	DECLARE_CLASS (StoreContextManager, PlatformStoreManager)

	StoreContextManager ();

	// PlatformStoreManager
	StringID getID () const override { return PlatformStoreID::kMicrosoftStore; }
	IAsyncOperation* startup () override;
	IAsyncOperation* requestProducts (const ConstVector<String>& productIds) override;
	IAsyncOperation* purchaseProduct (StringRef productId) override;
	IAsyncOperation* getTransactions () override;
	IAsyncOperation* getLocalLicenses () override;

private:
	StoreContext context = StoreContext::GetDefault ();
	AutoPtr<ObjectArray> transactions = NEW ObjectArray;

	void offlineLicensesChanged (const StoreContext& sender, const IInspectable& args);
};

} // namespace CppWinRT
} // namespace CCL

using namespace CCL;
using namespace CppWinRT;

//************************************************************************************************
// StoreContextManager
//************************************************************************************************

StoreContextManager::StoreContextManager ()
{
	transactions->objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL::IAsyncOperation* StoreContextManager::startup ()
{
	IDesktop& desktop = System::GetDesktop ();
	IWindow* appWindow = desktop.getApplicationWindow ();
	ASSERT (appWindow) // main window must exist prior to initializing the store
	if(!appWindow)
		return AsyncOperation::createFailed ();

	context.as<IInitializeWithWindow> ()->Initialize ((HWND)appWindow->getSystemWindow ());
	context.OfflineLicensesChanged (TypedEventHandler<StoreContext, IInspectable> (this, &StoreContextManager::offlineLicensesChanged));

	return AsyncOperation::createCompleted ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StoreContextManager::offlineLicensesChanged (const StoreContext& sender, const IInspectable& args)
{
	deferSignal (NEW Message (kLocalLicensesChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL::IAsyncOperation* StoreContextManager::requestProducts (const ConstVector<String>& productIds)
{
	std::vector<hstring> productKinds = { L"Application", L"Durable" };
	std::vector<hstring> storeIds;
	for(StringRef id : productIds)
		storeIds.push_back ((const uchar*) StringChars (id));

	return NEW AsyncOperationWrapper<StoreProductQueryResult> (context.GetStoreProductsAsync (std::move (productKinds), std::move (storeIds)), [](const StoreProductQueryResult& result)
	{
		AutoPtr<ObjectArray> products = NEW ObjectArray;
		products->objectCleanup (true);

		for(IKeyValuePair<hstring, Store::StoreProduct>& item : result.Products ())
		{
			Store::StoreProduct storeProduct = item.Value ();
			StoreProduct* product = NEW StoreProduct;

			product->setID (storeProduct.StoreId ().data ());
			product->setName (storeProduct.Title ().data ());
			product->setPrice (storeProduct.Price ().FormattedPrice ().data ());

			products->add (product);
		}

		return Variant (products->asUnknown (), true);
	});
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL::IAsyncOperation* StoreContextManager::purchaseProduct (StringRef productId)
{
	static int nextTransactionId = 0;

	AutoPtr<StoreTransaction> transaction = NEW StoreTransaction;
	transaction->setTransactionID (String (productId).append ("-").appendIntValue (nextTransactionId++));
	transaction->setProductID (productId);
	transaction->setState (PurchaseState::kInProgress);

	transactions->add (return_shared<StoreTransaction> (transaction));
	deferSignal (NEW Message (kTransactionsChanged));

	Promise promise = Promise (NEW AsyncOperationWrapper<StorePurchaseResult> (context.RequestPurchaseAsync ((const uchar*)StringChars (productId)), [transaction](const StorePurchaseResult& result)
	{
		switch(result.Status ())
		{
		case StorePurchaseStatus::Succeeded :
		case StorePurchaseStatus::AlreadyPurchased :
			transaction->setState (PurchaseState::kCompleted);
			break;
		case StorePurchaseStatus::NotPurchased :
			transaction->setState (PurchaseState::kCanceled);
			break;
		case StorePurchaseStatus::NetworkError :
		case StorePurchaseStatus::ServerError :
			transaction->setState (PurchaseState::kFailed);
			break;
		default:
			transaction->setState (PurchaseState::kDeferred);
			break;
		}

		return Variant (transaction->asUnknown (), true);
	})).then ([](IAsyncOperation& op)
	{
		PlatformStoreManager::instance ().deferSignal (NEW Message (PlatformStoreManager::kTransactionsChanged));
	});

	return return_shared<IAsyncOperation> (promise);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL::IAsyncOperation* StoreContextManager::getTransactions ()
{
	return AsyncOperation::createCompleted (Variant (transactions->asUnknown (), true), true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL::IAsyncOperation* StoreContextManager::getLocalLicenses ()
{
	return NEW AsyncOperationWrapper<StoreAppLicense> (context.GetAppLicenseAsync (), [](const StoreAppLicense& result)
	{
		AutoPtr<ObjectArray> licenses = NEW ObjectArray;
		licenses->objectCleanup (true);

		for(IKeyValuePair<hstring, Store::StoreLicense>& item : result.AddOnLicenses ())
		{
			Store::StoreLicense storeLicense = item.Value ();
			StoreLicense* license = NEW StoreLicense;

			license->setProductID (String (storeLicense.SkuStoreId ().data ()).subString (0, 12)); // SkuStoreId is formatted as <storeId{12}>/<skuId{4}>
			license->setVerificationResult (storeLicense.IsActive () ? LicenseVerificationResult::kValid : LicenseVerificationResult::kInvalid);

			licenses->add (license);
		}

		return Variant (licenses->asUnknown (), true);
	});
}

DEFINE_CLASS_HIDDEN (StoreContextManager, PlatformStoreManager)
#if !CCL_DEMO_STORE_MANAGER_ENABLED
DEFINE_EXTERNAL_SINGLETON (PlatformStoreManager, StoreContextManager)
#endif
