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
// Filename    : ccl/extras/stores/storepurchasehandler.h
// Description : In-App Purchase Handler
//
//************************************************************************************************

#ifndef _ccl_storepurchasehandler_h
#define _ccl_storepurchasehandler_h

#include "ccl/extras/stores/purchasemodel.h"

#include "ccl/app/component.h"

#include "ccl/public/gui/framework/iitemmodel.h"
#include "ccl/public/base/iasyncoperation.h"

namespace CCL {

//************************************************************************************************
// StoreProductComponent
//************************************************************************************************

class StoreProductComponent: public Component
{
public:
	DECLARE_CLASS (StoreProductComponent, Component)

	StoreProductComponent (StringRef name = nullptr);

	void updateDetails (const StoreProduct& data);
	void setPurchaseEnabled (bool state);
	bool isLicensed () const;

	// Component
	tbool CCL_API paramChanged (IParameter* param) override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
};

//************************************************************************************************
// StoreProductCollection
//************************************************************************************************

class StoreProductCollection: public Component,
							  public AbstractItemModel
{
public:
	DECLARE_CLASS (StoreProductCollection, Component)

	StoreProductCollection (StringRef name = nullptr);

	// IItemModel
	tbool CCL_API getSubItems (IUnknownList& items, ItemIndexRef index) override;

	CLASS_INTERFACE (IItemModel, Component)
};

//************************************************************************************************
// StorePurchaseHandler
//************************************************************************************************

class StorePurchaseHandler: public Component
{
public:
	DECLARE_CLASS (StorePurchaseHandler, Component)

	StorePurchaseHandler ();

	static StorePurchaseHandler* getInstance (const Component* component);

	void purchaseProduct (StringRef productId);
	bool isProductLicensed (StringRef productId) const;

	// Component
	tresult CCL_API initialize (IUnknown* context = nullptr) override;
	tresult CCL_API terminate () override;
	tbool CCL_API paramChanged (IParameter* param) override;
	void CCL_API notify (ISubject* subject, CCL::MessageRef msg) override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;

protected:
	StoreConfiguration configuration;
	StoreProductCollection* productCollection;

	void updateProductDetails ();
	void updateProductLicenseStates ();
	void updateProductPurchasesEnabled ();
	void checkTransactions ();

	void onStartupCompleted ();
	void onAppActivated ();
	void onTransactionsChanged (MessageRef msg);
	void onLocalLicensesChanged (MessageRef msg);
};

} // namespac CCL

#endif // _ccl_storepurchasehandler_h
