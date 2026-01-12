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
// Filename    : ccl/extras/stores/purchasemodel.h
// Description : Store Purchase Model
//
//************************************************************************************************

#ifndef _ccl_purchasemodel_h
#define _ccl_purchasemodel_h

#include "ccl/base/storage/attributes.h"
#include "ccl/base/security/cryptomaterial.h"

namespace CCL {

//************************************************************************************************
// LicenseVerificationResult
//************************************************************************************************

enum class LicenseVerificationResult
{
	kUnverified = -1,
	kValid,
	kExpired,
	kInvalid
};

//************************************************************************************************
// PurchaseState
//************************************************************************************************

enum class PurchaseState
{
	kInProgress,	///< purchase is in progress
	kDeferred,		///< waiting for external approval
	kCompleted,		///< completed (restored)
	kCanceled,		///< canceled by user
	kFailed			///< failed transaction
};

//************************************************************************************************
// StoreProduct
//************************************************************************************************

class StoreProduct: public Object
{
public:
	DECLARE_CLASS (StoreProduct, Object)

	StoreProduct (StringRef id = nullptr);

	PROPERTY_STRING (id, ID)
	PROPERTY_STRING (name, Name)
	PROPERTY_STRING (price, Price)

	// Object
	bool equals (const Object& obj) const override;
};

//************************************************************************************************
// StoreLicense
//************************************************************************************************

class StoreLicense: public Object
{
public:
	DECLARE_CLASS (StoreLicense, Object)

	StoreLicense (StringRef productId = nullptr);

	PROPERTY_STRING (productId, ProductID)
	PROPERTY_OBJECT (Security::Crypto::Material, receiptData, ReceiptData)
	PROPERTY_VARIABLE (LicenseVerificationResult, verificationResult, VerificationResult)

	// Object
	bool equals (const Object& obj) const override;
};

//************************************************************************************************
// StoreTransaction
//************************************************************************************************

class StoreTransaction: public Object
{
public:
	DECLARE_CLASS (StoreTransaction, Object)

	StoreTransaction ();

	PROPERTY_STRING (transactionId, TransactionID)
	PROPERTY_STRING (productId, ProductID)
	PROPERTY_VARIABLE (PurchaseState, state, State)
};

//************************************************************************************************
// ProductConfiguration
//************************************************************************************************

class ProductConfiguration: public StoreProduct
{
public:
	DECLARE_CLASS (ProductConfiguration, StoreProduct)

	ProductConfiguration ();

	void addPlatformID (StringID key, StringRef value);
	String getPlatformIDForStore (StringID storeId) const;

	PROPERTY_STRING (policyBase64, PolicyBase64)
	
	// cached states used at runtime
	PROPERTY_BOOL (policyApplied, PolicyApplied)
	PROPERTY_BOOL (licenseValid, LicenseValid)
	PROPERTY_BOOL (transactionPending, TransactionPending)

	DECLARE_STRINGID_MEMBER (kPlatformID)

protected:
	Attributes platformIds;
};

//************************************************************************************************
// StoreConfiguration
//************************************************************************************************

class StoreConfiguration: public Object
{
public:
	DECLARE_CLASS (StoreConfiguration, Object)

	StoreConfiguration ();

	bool loadFromFile (UrlRef path);
	bool loadBuiltInConfiguration ();

	const ObjectArray& getProductConfigurations () const;
	ProductConfiguration* find (StringRef productId) const;

	String getPlatformIDForProduct (StringRef canonicalProductId, StringID storeId) const;
	String getCanonicalIDForProduct (StringRef platformProductId, StringID storeId) const;

protected:
	ObjectArray productConfigurations;
};

} // namespac CCL

#endif // _ccl_purchasemodel_h
