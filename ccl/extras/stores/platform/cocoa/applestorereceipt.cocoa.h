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
// Filename    : ccl/extras/stores/platform/cocoa/applestorereceipt.cocoa.h
// Description : Evaluates receipts from the iOS and Mac app stores
//
//************************************************************************************************

#ifndef _ccl_applestorereceipt_cocoa_h
#define _ccl_applestorereceipt_cocoa_h

#include "ccl/base/collections/objectarray.h"
#include "ccl/base/security/cryptomaterial.h"

#include "ccl/public/text/cclstring.h"

@class NSURL;

namespace CCL {

//************************************************************************************************
// AppleStoreReceipt
//************************************************************************************************

class AppleStoreReceipt
{
public:
	AppleStoreReceipt (NSURL* _receiptUrl = nullptr);
	bool isValid ();
	bool getPurchases (ObjectArray& licenses);

	PROPERTY_STRING (bundleID, BundleID)
	PROPERTY_STRING (appVersion, AppVersion)
	ObjectArray& getInAppPurchases () { return inAppPurchases; }
	Security::Crypto::Material& getReceiptHash () { return receiptHash; }
	Security::Crypto::Material& getOpaqueValue () { return opaquaValue; }
	Security::Crypto::Material& getRawBundleID () { return rawBundleID; }

protected:
	ObjectArray inAppPurchases;
	Security::Crypto::Material receiptHash;
	Security::Crypto::Material opaquaValue;
	Security::Crypto::Material rawBundleID;

	NSURL* receiptUrl;
	
	bool readFromFile ();
	bool verifyCertificate (MemoryStream& cert);
	void calculateDeviceHash (Security::Crypto::Material& deviceHash);
};

} // namespace CCL

#endif // _ccl_applestorereceipt_cocoa_h
