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
// Filename    : ccl/extras/stores/demostoremanager.cpp
// Description : Demo Store Manager
//
//************************************************************************************************

#define DEMO_PURCHASE_BEHAVIOR_ENABLED DEBUG // simulate purchase, for development only

#include "ccl/extras/stores/platformstoremanager.h"

#include "ccl/base/message.h"

#include "ccl/public/gui/framework/ialert.h"
#include "ccl/public/guiservices.h"

#include "ccl/public/text/translation.h"

namespace CCL {

//************************************************************************************************
// DemoStoreManager
//************************************************************************************************

class DemoStoreManager: public PlatformStoreManager
{
public:
	DECLARE_CLASS (DemoStoreManager, PlatformStoreManager)

	DemoStoreManager ();
	~DemoStoreManager ();

	// PlatformStoreManager
	StringID getID () const override { return PlatformStoreID::kDemo; }
	IAsyncOperation* startup () override;
	IAsyncOperation* requestProducts (const ConstVector<String>& productIds) override;
	IAsyncOperation* purchaseProduct (StringRef productId) override;
	IAsyncOperation* getTransactions () override;
	IAsyncOperation* getLocalLicenses () override;
	IAsyncOperation* restorePurchases () override;

protected:
	StoreConfiguration configuration;
	ObjectArray demoLicenses;
	ObjectArray demoTransactions;
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

// XSTRINGS_OFF	hint for xstring tool to skip this section
BEGIN_XSTRINGS ("DemoStore")
	XSTRING (ProductNotFound, "Product not found.")
	XSTRING (AlreadyOwnProduct, "You already own this product.")
	XSTRING (ConfirmProductPurchase, "Please confirm your purchase:\n\n%(1) %(2)")
END_XSTRINGS

//************************************************************************************************
// DemoStoreManager
//************************************************************************************************

DEFINE_CLASS_HIDDEN (DemoStoreManager, PlatformStoreManager)
#if CCL_DEMO_STORE_MANAGER_ENABLED
DEFINE_EXTERNAL_SINGLETON (PlatformStoreManager, DemoStoreManager)
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////

DemoStoreManager::DemoStoreManager ()
{
	demoLicenses.objectCleanup (true);
	demoTransactions.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DemoStoreManager::~DemoStoreManager ()
{
	cancelSignals ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* DemoStoreManager::startup ()
{
	configuration.loadBuiltInConfiguration ();

	return AsyncOperation::createCompleted ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* DemoStoreManager::requestProducts (const ConstVector<String>& productIds)
{
	AutoPtr<ObjectArray> result = NEW ObjectArray;
	result->objectCleanup (true);

	for(auto p : iterate_as<ProductConfiguration> (configuration.getProductConfigurations ()))
		if(productIds.contains (p->getID ()))
			result->add (return_shared (p));
	
	return AsyncOperation::createCompleted (Variant ().takeShared (result->asUnknown ()), true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* DemoStoreManager::purchaseProduct (StringRef productId)
{
#if DEMO_PURCHASE_BEHAVIOR_ENABLED
	AutoPtr<StoreTransaction> transaction = NEW StoreTransaction;
	static int nextTransactionId = 1000;
	transaction->setTransactionID (String ().appendIntValue (nextTransactionId++));
	transaction->setProductID (productId);

	auto product = static_cast<StoreProduct*> (configuration.getProductConfigurations ().findEqual (StoreProduct (productId)));
	auto license = static_cast<StoreLicense*> (demoLicenses.findEqual (StoreLicense (productId)));
	if(!product || license)
	{
		String message;
		message = !product ? XSTR (ProductNotFound) : XSTR (AlreadyOwnProduct);
		Promise p = Alert::infoAsync (message);
		return return_shared<IAsyncOperation> (p.then ([this, transaction] (IAsyncOperation& op)
			{
				transaction->setState (PurchaseState::kFailed);
				demoTransactions.add (transaction);
				transaction->retain ();
				deferSignal (NEW Message (kTransactionsChanged));
				
				op.setResult (Variant ().takeShared (transaction->asUnknown ()));
			}));
	}
	
	String message;
	message.appendFormat (XSTR (ConfirmProductPurchase), product->getName (), product->getPrice ());
	Promise p = Alert::askAsync (message);
	return return_shared<IAsyncOperation> (p.then ([this, transaction] (IAsyncOperation& op)
		{
			if(op.getResult ().asInt () == Alert::kYes)
			{
				transaction->setState (PurchaseState::kCompleted);
				
				auto license = NEW StoreLicense (transaction->getProductID ());
				license->setVerificationResult (LicenseVerificationResult::kValid);
				demoLicenses.add (license);
				deferSignal (NEW Message (kLocalLicensesChanged));
			}
			else
				transaction->setState (PurchaseState::kCanceled);

			demoTransactions.add (transaction);
			transaction->retain ();
			deferSignal (NEW Message (kTransactionsChanged));

			op.setResult (Variant ().takeShared (transaction->asUnknown ()));
		}));
#else
	return AsyncOperation::createFailed ();
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* DemoStoreManager::getTransactions ()
{
	AutoPtr<ObjectArray> result = NEW ObjectArray;
	result->objectCleanup (true);
	result->add (demoTransactions, Container::kShare);

	return AsyncOperation::createCompleted (Variant ().takeShared (result->asUnknown ()), true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* DemoStoreManager::getLocalLicenses ()
{
	AutoPtr<ObjectArray> result = NEW ObjectArray;
	result->objectCleanup (true);
	result->add (demoLicenses, Container::kShare);

	return AsyncOperation::createCompleted (Variant ().takeShared (result->asUnknown ()), true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* DemoStoreManager::restorePurchases ()
{
	return AsyncOperation::createCompleted ();
}
