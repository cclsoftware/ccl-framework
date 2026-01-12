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
// Filename    : ccl/extras/stores/purchasemodel.cpp
// Description : Store Purchase Model
//
//************************************************************************************************

#include "ccl/extras/stores/purchasemodel.h"

#include "ccl/base/storage/file.h"
#include "ccl/base/storage/jsonarchive.h"

using namespace CCL;

//************************************************************************************************
// StoreProduct
//************************************************************************************************

DEFINE_CLASS_HIDDEN (StoreProduct, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

StoreProduct::StoreProduct (StringRef id)
: id (id)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StoreProduct::equals (const Object& obj) const
{
	auto other = ccl_cast<StoreProduct> (&obj);
	return other && other->id == id;
}

//************************************************************************************************
// StoreLicense
//************************************************************************************************

DEFINE_CLASS_HIDDEN (StoreLicense, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

StoreLicense::StoreLicense (StringRef productId)
: productId (productId),
  verificationResult (LicenseVerificationResult::kUnverified)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StoreLicense::equals (const Object& obj) const
{
	auto other = ccl_cast<StoreLicense> (&obj);
	return other && other->productId == productId;
}

//************************************************************************************************
// StoreTransaction
//************************************************************************************************

DEFINE_CLASS_HIDDEN (StoreTransaction, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

StoreTransaction::StoreTransaction ()
: state (PurchaseState::kFailed) 
{}

//************************************************************************************************
// ProductConfiguration
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ProductConfiguration, StoreProduct)
DEFINE_STRINGID_MEMBER_ (ProductConfiguration, kPlatformID, "platformId.")

//////////////////////////////////////////////////////////////////////////////////////////////////

ProductConfiguration::ProductConfiguration ()
: policyApplied (false),
  licenseValid (false),
  transactionPending (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ProductConfiguration::addPlatformID (StringID key, StringRef value)
{
	platformIds.set (key, value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String ProductConfiguration::getPlatformIDForStore (StringID storeId) const
{
	String platformId;
	ForEachAttribute (platformIds, key, value)
		if(key.endsWith (storeId))
		{
			platformId = value.asString ();
			break;
		}
	EndFor

	if(platformId.isEmpty ())
		platformId = id;
	return platformId;
}

//************************************************************************************************
// StoreConfiguration
//************************************************************************************************

DEFINE_CLASS_HIDDEN (StoreConfiguration, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

StoreConfiguration::StoreConfiguration ()
{
	productConfigurations.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StoreConfiguration::loadFromFile (UrlRef path)
{
	AutoPtr<IStream> file = File (path).open ();
	if(!file)
		return false;

	Attributes data;
	if(!JsonUtils::parse (data, *file))
		return false;

	IterForEach (data.newQueueIterator ("products", ccl_typeid<Attributes> ()), Attributes, productAttr)
		auto p = NEW ProductConfiguration;
		p->setID (productAttr->getString ("id"));
		p->setName (productAttr->getString ("name"));
		p->setPrice (productAttr->getString ("price"));
		p->setPolicyBase64 (productAttr->getString ("policy"));

		ForEachAttribute (*productAttr, key, value)
			if(key.startsWith (ProductConfiguration::kPlatformID))
				p->addPlatformID (key, value.asString ());
		EndFor

		productConfigurations.add (p);
	EndFor
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StoreConfiguration::loadBuiltInConfiguration ()
{
	return loadFromFile (ResourceUrl ("storeconfig.json"));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ObjectArray& StoreConfiguration::getProductConfigurations () const
{
	return productConfigurations;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ProductConfiguration* StoreConfiguration::find (StringRef productId) const
{
	for(auto c : iterate_as<ProductConfiguration> (productConfigurations))
		if(c->getID () == productId)
			return c;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String StoreConfiguration::getPlatformIDForProduct (StringRef canonicalProductId, StringID storeId) const
{
	for(auto p : iterate_as<ProductConfiguration> (productConfigurations))
	{
		if(p->getID () == canonicalProductId)
			return p->getPlatformIDForStore (storeId);
	}
	return String ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String StoreConfiguration::getCanonicalIDForProduct (StringRef platformProductId, StringID storeId) const
{
	for(auto p : iterate_as<ProductConfiguration> (productConfigurations))
	{
		if(p->getPlatformIDForStore (storeId) == platformProductId)
			return p->getID ();
	}
	return String ();
}
