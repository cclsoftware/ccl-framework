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
// Filename    : ccl/extras/stores/storepurchasehandler.cpp
// Description : In-App Purchase Handler
//
//************************************************************************************************

#include "ccl/extras/stores/storepurchasehandler.h"
#include "ccl/extras/stores/platformstoremanager.h"

#include "ccl/base/message.h"
#include "ccl/base/signalsource.h"
#include "ccl/base/collections/objectarray.h"

#include "ccl/public/securityservices.h"
#include "ccl/public/guiservices.h"

#include "ccl/public/text/translation.h"

#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/iapplication.h"
#include "ccl/public/gui/framework/iuserinterface.h"
#include "ccl/public/gui/framework/ialert.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("StorePurchaseHandler")
	XSTRING (RestoreError, "Could not restore purchases. Please try again in a few minutes.")
	XSTRING (PurchaseError, "Could not complete the purchase.")
END_XSTRINGS

//////////////////////////////////////////////////////////////////////////////////////////////////
// Tags
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum StoreProductComponentTags
	{
		kName = 100,
		kPrice,
		kBuy
	};

	enum StorePurchaseHandlerTags
	{
		kRestorePurchases = 100
	};
}

//************************************************************************************************
// StoreProductComponent
//************************************************************************************************

DEFINE_CLASS_HIDDEN (StoreProductComponent, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

StoreProductComponent::StoreProductComponent (StringRef name)
: Component (name)
{
	paramList.addString ("name", Tag::kName)->setReadOnly (true);
	paramList.addString ("price", Tag::kPrice)->setReadOnly (true);
	paramList.addParam ("buy", Tag::kBuy)->enable (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StoreProductComponent::updateDetails (const StoreProduct& data)
{
	paramList.byTag (Tag::kName)->fromString (data.getName ());
	paramList.byTag (Tag::kPrice)->fromString (data.getPrice ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StoreProductComponent::setPurchaseEnabled (bool state)
{
	bool wasEnabled = paramList.byTag (Tag::kBuy)->isEnabled ();
	if(state != wasEnabled)
	{
		paramList.byTag (Tag::kBuy)->enable (state);
		
		signalPropertyChanged ("licensed");
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StoreProductComponent::isLicensed () const
{
	auto handler = StorePurchaseHandler::getInstance (this);
	ASSERT (handler)
	return handler && handler->isProductLicensed (getName ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API StoreProductComponent::paramChanged (IParameter* param)
{
	switch(param->getTag ())
	{
	case Tag::kBuy :
		{
			auto handler = StorePurchaseHandler::getInstance (this);
			ASSERT (handler)
			if(handler)
			{
				param->enable (false);
				handler->purchaseProduct (getName ());
			}
		}
		break;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API StoreProductComponent::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "licensed")
	{
		var = isLicensed ();
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//************************************************************************************************
// StoreProductCollection
//************************************************************************************************

DEFINE_CLASS_HIDDEN (StoreProductCollection, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

StoreProductCollection::StoreProductCollection (StringRef name)
: Component (name)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API StoreProductCollection::getSubItems (IUnknownList& items, ItemIndexRef index)
{
	for(auto child : getChildArray ())
		items.add (child->asUnknown (), true);
	return true;
}

//************************************************************************************************
// StorePurchaseHandler
//************************************************************************************************

StorePurchaseHandler* StorePurchaseHandler::getInstance (const Component* component)
{
	for(ObjectNode* n = const_cast<Component*> (component); n; n = n->getParentNode ())
		if(auto handler = ccl_cast<StorePurchaseHandler> (n))
			return handler;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (StorePurchaseHandler, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

StorePurchaseHandler::StorePurchaseHandler ()
: Component ("StoreHandler"),
  productCollection (nullptr)
{
	addComponent (productCollection = NEW StoreProductCollection ("Products"));
	addObject ("products", productCollection->asUnknown ());
	
	paramList.addParam ("restorePurchases", Tag::kRestorePurchases);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API StorePurchaseHandler::initialize (IUnknown* context)
{
	configuration.loadBuiltInConfiguration ();

	for(auto c : iterate_as<ProductConfiguration> (configuration.getProductConfigurations ()))
		productCollection->addComponent (NEW StoreProductComponent (c->getID ()));
	
	signalSlots.advise (&PlatformStoreManager::instance (), PlatformStoreManager::kTransactionsChanged, 
						this, &StorePurchaseHandler::onTransactionsChanged);
	signalSlots.advise (&PlatformStoreManager::instance (), PlatformStoreManager::kLocalLicensesChanged, 
						this, &StorePurchaseHandler::onLocalLicensesChanged);

	ISubject::addObserver (&System::GetGUI (), this);

	Promise (PlatformStoreManager::instance ().startup ()).then ([this] (IAsyncOperation& op)
	{
		if(op.getState () == IAsyncInfo::kCompleted)
			onStartupCompleted ();
	});

	return SuperClass::initialize (context);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API StorePurchaseHandler::terminate ()
{
	ISubject::removeObserver (&System::GetGUI (), this);
	
	signalSlots.unadvise (&PlatformStoreManager::instance ());

	return SuperClass::terminate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StorePurchaseHandler::purchaseProduct (StringRef productId)
{
	StringID storeId = PlatformStoreManager::instance ().getID ();
	String platformId = configuration.getPlatformIDForProduct (productId, storeId);

	Promise p (PlatformStoreManager::instance ().purchaseProduct (platformId));
	p.then ([](IAsyncOperation& op)
	{
		if(op.getState () == IAsyncOperation::kFailed)
			Promise (Alert::errorAsync (XSTR (PurchaseError)));

		// If there is no error, the platform store issues license change notification.
	});
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StorePurchaseHandler::isProductLicensed (StringRef productId) const
{
	auto c = configuration.find (productId);
	return c != nullptr && c->isLicenseValid ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StorePurchaseHandler::updateProductDetails ()
{
	Vector<String> productIds;
	StringID storeId = PlatformStoreManager::instance ().getID ();
	for(auto p : iterate_as<ProductConfiguration> (configuration.getProductConfigurations ()))
		productIds.add (p->getPlatformIDForStore (storeId));

	Promise p = PlatformStoreManager::instance ().requestProducts (productIds);
	p.then ([this, storeId] (IAsyncOperation& op)
	{
		if(op.getState () == IAsyncInfo::kCompleted)
		{
			// update price, etc. of existing product components
			if(auto result = unknown_cast<Container> (op.getResult ()))
				for(auto data : iterate_as<StoreProduct> (*result))
				{
					String id = configuration.getCanonicalIDForProduct (data->getID (), storeId);
					auto component = productCollection->getComponent<StoreProductComponent> (id);
					ASSERT (component)
					if(component)
						component->updateDetails (*data);
				}

			updateProductPurchasesEnabled ();
		}
	});
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StorePurchaseHandler::updateProductLicenseStates ()
{
	Promise p = PlatformStoreManager::instance ().getLocalLicenses ();
	p.then ([this] (IAsyncOperation& op)
	{
		if(op.getState () == IAsyncInfo::kCompleted)
		{
			Vector<String> validIds;
			bool policyChanged = false;
				
			StringID storeId = PlatformStoreManager::instance ().getID ();
			if(auto result = unknown_cast<Container> (op.getResult ()))
			{
				for(auto license : iterate_as<StoreLicense> (*result))
				{
					String id = configuration.getCanonicalIDForProduct (license->getProductID (), storeId);
					auto c = configuration.find (id);
					ASSERT (c)
					if(!c)
						continue;

					if(license->getVerificationResult () != LicenseVerificationResult::kValid)
						continue;

					validIds.addOnce (id);
					c->setLicenseValid (true);

					if(!c->isPolicyApplied () && !c->getPolicyBase64 ().isEmpty ())
					{
						Security::Crypto::Material m;
						m.fromBase64 (c->getPolicyBase64 ());
						System::GetAuthorizationManager ().loadPolicy (m.asStream (), Security::IAuthorizationManager::kUnsignedPolicy);
						c->setPolicyApplied (true);
						policyChanged = true;
					}
				}
			}

			for(auto c : iterate_as<ProductConfiguration> (configuration.getProductConfigurations ()))
				if(!validIds.contains (c->getID ())) // no license found for this product
				{
					c->setLicenseValid (false);
						
					if(c->isPolicyApplied () && !c->getPolicyBase64 ().isEmpty ())
					{
						Security::Crypto::Material m;
						m.fromBase64 (c->getPolicyBase64 ());
						System::GetAuthorizationManager ().revokePolicy (m.asStream (), Security::IAuthorizationManager::kUnsignedPolicy);
						c->setPolicyApplied (false);
						policyChanged = true;
					}
				}

			if(policyChanged)
				SignalSource (Signals::kAuthorization).signal (Message (Signals::kAuthorizationPolicyChanged));

			updateProductPurchasesEnabled ();
		}
	});
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StorePurchaseHandler::updateProductPurchasesEnabled ()
{
	for(auto c : iterate_as<ProductConfiguration> (configuration.getProductConfigurations ()))
	{
		auto component = productCollection->getComponent<StoreProductComponent> (c->getID ());
		ASSERT (component)
		if(component)
			component->setPurchaseEnabled (!c->isLicenseValid () && !c->isTransactionPending ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StorePurchaseHandler::checkTransactions ()
{
	Promise p = PlatformStoreManager::instance ().getTransactions ();
	p.then ([this] (IAsyncOperation& op)
	{
		if(op.getState () == IAsyncInfo::kCompleted)
		{
			StringID storeId = PlatformStoreManager::instance ().getID ();
			bool transactionsChanged = false;

			Vector<String> pendingIds;
			if(auto container = unknown_cast<Container> (op.getResult ()))
				for(auto transaction : iterate_as<StoreTransaction> (*container))
				{
					String id = configuration.getCanonicalIDForProduct (transaction->getProductID (), storeId);
					auto c = configuration.find (id);
					ASSERT (c)
					if(!c)
						continue;

					if(transaction->getState () == PurchaseState::kInProgress || 
					   transaction->getState () == PurchaseState::kDeferred)
					{
						pendingIds.add (id);
						if(!c->isTransactionPending ())
						{
							c->setTransactionPending (true);
							transactionsChanged = true;
						}
					}
				}

			for(auto c : iterate_as<ProductConfiguration> (configuration.getProductConfigurations ()))
				if(!pendingIds.contains (c->getID ()))
				{
					if(c->isTransactionPending ())
					{
						c->setTransactionPending (false);
						transactionsChanged = true;
					}
				}

			if(transactionsChanged)
				updateProductPurchasesEnabled ();
		}
	});
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StorePurchaseHandler::onStartupCompleted ()
{
	// On app startup, we need to do the following:
	// - get product details from store (price, etc.)
	// - check which licenses are locally available
	// - check which transactions are currently pending
	
	updateProductDetails ();
	updateProductLicenseStates ();
	checkTransactions ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StorePurchaseHandler::onAppActivated ()
{
	// On app activation, we need to do the following:
	// - manually check licenses as they might have been revoked due to a refund
	// - check which transactions are currently pending
	
	updateProductLicenseStates ();
	checkTransactions ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StorePurchaseHandler::onTransactionsChanged (MessageRef msg)
{
	checkTransactions ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StorePurchaseHandler::onLocalLicensesChanged (MessageRef msg)
{
	updateProductLicenseStates ();
	signalPropertyChanged ("licensedProductsAvailable");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API StorePurchaseHandler::paramChanged (IParameter* param)
{
	if(param->getTag () == Tag::kRestorePurchases)
	{
		param->enable (false);
		Promise p = PlatformStoreManager::instance ().restorePurchases ();
		p.then ([param] (IAsyncOperation& op)
		{
			if(op.getState () == IAsyncOperation::kFailed)
				Promise (Alert::errorAsync (XSTR (RestoreError)));

			param->enable (true);
		});
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API StorePurchaseHandler::notify (ISubject* subject, MessageRef msg)
{
	if(msg == IApplication::kAppActivated)
	{
		onAppActivated ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API StorePurchaseHandler::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "licensedProductsAvailable")
	{
		bool licenseFound = false;
		for(auto p : iterate_as<ProductConfiguration> (configuration.getProductConfigurations ()))
		{
			if(p->isLicenseValid ())
			{
				licenseFound = true;
				break;
			}
		}

		var = licenseFound;
		return true;
	}

	return SuperClass::getProperty (var, propertyId);
}
