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
// Filename    : ccl/extras/stores/platform/cocoa/applestorereceipt.cocoa.mm
// Description : Evaluates receipts from the iOS and Mac app stores
//
//************************************************************************************************

#define DEBUG_LOG 0

#define COPY_RECEIPT_FILE 0 && CCL_PLATFORM_IOS // disable before releasing a product to the general public!

#include "ccl/extras/stores/platform/cocoa/applestorereceipt.cocoa.h"

#include "ccl/extras/stores/purchasemodel.h"
#include "ccl/base/security/cryptobox.h"
#include "ccl/public/security/iasn1contenthandler.h"

#include "ccl/platform/cocoa/macutils.h"

#if CCL_PLATFORM_MAC
#include "ccl/public/base/ccldefpush.h"
#import <Foundation/Foundation.h>
#import <IOKit/network/IONetworkLib.h>
#import <IOKit/network/IONetworkInterface.h>
#import <IOKit/network/IONetworkController.h>

//************************************************************************************************
// Helpers to generate a device hash on the Mac
//************************************************************************************************

namespace CCL {
namespace MacUtils {

io_service_t ioService (const char* name, BOOL wantBuiltIn)
{
	io_iterator_t iterator = IO_OBJECT_NULL;
	mach_port_t default_port = kIOMasterPortDefault;
	io_service_t service = IO_OBJECT_NULL;
	
	if (KERN_SUCCESS != IOMasterPort (MACH_PORT_NULL, &default_port))
	{
		return IO_OBJECT_NULL;
	}
	
	CFMutableDictionaryRef matchingDict = IOBSDNameMatching (default_port, 0, name);
	if(matchingDict == NULL)
	{
		return IO_OBJECT_NULL;
	}
	
	if(KERN_SUCCESS != IOServiceGetMatchingServices (default_port, matchingDict, &iterator))
	{
		return IO_OBJECT_NULL;
	}
	
	if(iterator != IO_OBJECT_NULL)
	{
		io_service_t candidate = IOIteratorNext (iterator);
		while(candidate != IO_OBJECT_NULL)
		{
			CFTypeRef isBuiltIn = IORegistryEntryCreateCFProperty (candidate, CFSTR (kIOBuiltin), kCFAllocatorDefault, 0);
			if(isBuiltIn != NULL && CFGetTypeID (isBuiltIn) == CFBooleanGetTypeID ())
			{
				if(wantBuiltIn == CFBooleanGetValue (static_cast<CFBooleanRef> (isBuiltIn)))
				{
					service = candidate;
					break;
				}
			}
			IOObjectRelease (candidate);
			candidate = IOIteratorNext (iterator);
		}
		IOObjectRelease (iterator);
	}
	
	return service;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CFDataRef copyMacAddress ()
{
	CFDataRef macAddress = NULL;
	io_service_t service = ioService ("en0", true);
	
	if(service == IO_OBJECT_NULL)
		service = ioService ("en1", true);
	
	if(service == IO_OBJECT_NULL)
		service = ioService ("en0", false);
	
	if(service != IO_OBJECT_NULL)
	{
		CFTypeRef property = IORegistryEntrySearchCFProperty (service, kIOServicePlane, CFSTR (kIOMACAddress), kCFAllocatorDefault, kIORegistryIterateRecursively|kIORegistryIterateParents);
		if(property != NULL)
		{
			if(CFGetTypeID (property) == CFDataGetTypeID ())
				macAddress = static_cast<CFDataRef> (property);
			else
				CFRelease(property);
		}
		IOObjectRelease(service);
	}
	
	return macAddress;
}

} // namespace MacUtils
} // namespace CCL

#endif

using namespace CCL;
using namespace Security;
using namespace Crypto;

//************************************************************************************************
// LicenseReader
// Base class for parsing data from the Apple store
//************************************************************************************************

class LicenseReader: public Unknown,
					 public IASN1ContentHandler
{
public:
	LicenseReader ();
	
	enum AppStoreTypes
	{
		kBundleIdentifier = 2,
		kAppVersion = 3,
		kOpaqueValue = 4,
		kSHA1Hash = 5,
		kReceiptCreationDate = 12,
		kInAppPurchaseReceipt = 17,
		kOriginalApplicationVersion = 19,
		kReceiptExpirationDate = 21,
		kQuantity = 1701,
		kProductIdentifier = 1702,
		kTransactionIdentifier = 1703,
		kPurchaseDate = 1704,
		kOriginalTransactionIdentifier = 1705,
		kOriginalPurchaseDate = 1706,
		kSubscriptionExpirationDate = 1708,
		kCancellationDate = 1712,
		kSubscriptionIntroductoryPricePeriod = 1719
	};
	
	// IASN1ContentHandler
	tresult CCL_API integer (int i) override;
	tresult CCL_API sequence (IStream& data) override;
	tresult CCL_API set (IStream& data) override;
	tresult CCL_API context (unsigned char& defaultTag, tbool& implicit, unsigned char contextTag) override;

	CLASS_INTERFACE (IASN1ContentHandler, Unknown);
	
protected:
	int attributeType;
	int attributeVersion;
};

//************************************************************************************************
// ReceiptReader
//************************************************************************************************

class ReceiptReader: public LicenseReader
{
public:
	ReceiptReader (AppleStoreReceipt& receipt);

	// LicenseReader
	tresult CCL_API octetString (IStream& data) override;
	tresult CCL_API string (StringRef data) override;
	
	CLASS_INTERFACE (IASN1ContentHandler, LicenseReader);

protected:
	AppleStoreReceipt& receipt;
	
	tresult addPurchase (IStream& data);
};

//************************************************************************************************
// PurchaseReader
//************************************************************************************************

class PurchaseReader: public LicenseReader
{
public:
	PurchaseReader (StoreLicense& license);

	// LicenseReader
	tresult CCL_API octetString (IStream& data) override;
	tresult CCL_API string (StringRef data) override;
	
	CLASS_INTERFACE (IASN1ContentHandler, LicenseReader);
	
protected:
	StoreLicense& license;
};

//************************************************************************************************
// LicenseReader
//************************************************************************************************

LicenseReader::LicenseReader ()
: attributeType (0),
  attributeVersion (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API LicenseReader::integer (int i)
{
	// Apple's attributes are a sequence of type, version and value (octect string)
	if(attributeType == 0)
	{
		attributeType = i;
		return kResultOk;
	}
	
	if(attributeVersion == 0)
	{
		attributeVersion = i;
		return kResultOk;
	}
				
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API LicenseReader::sequence (IStream& data)
{
	attributeType = 0;
	attributeVersion = 0;
	BER::decode (*this, data);
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API LicenseReader::set (IStream& data)
{
	BER::decode (*this, data);
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API LicenseReader::context (unsigned char& defaultTag, tbool& implicit, unsigned char contextTag)
{
	// Apple's data currently uses no context dependent tags, if that should change: ignore
	return kResultOk;
}

//************************************************************************************************
// ReceiptReader
//************************************************************************************************

ReceiptReader::ReceiptReader (AppleStoreReceipt& _receipt)
: receipt (_receipt)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ReceiptReader::octetString (IStream& data)
{
	switch(attributeType)
	{
	case kBundleIdentifier : // bundleID as UTF8STRING wrapped in an OCTET_STRING
		receipt.getRawBundleID ().copyFrom (data);
		return BER::decode (*this, data) ? kResultOk : kResultFalse;
	
	case kAppVersion : // appVersion as UTF8STRING wrapped in an OCTET_STRING
		return BER::decode (*this, data) ? kResultOk : kResultFalse;

	case kOpaqueValue : // series of bytes
		receipt.getOpaqueValue ().copyFrom (data);
		break;
	
	case kSHA1Hash :
		receipt.getReceiptHash ().copyFrom (data);
		break;

	case kInAppPurchaseReceipt :
		return addPurchase (data);
	}
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult ReceiptReader::addPurchase (IStream& data)
{
	StoreLicense* license = NEW StoreLicense ();
	license->setVerificationResult (LicenseVerificationResult::kValid);
	PurchaseReader reader (*license);
	if(BER::decode (reader, data) == false)
		return kResultFailed;
	
	if(!license->getProductID ().isEmpty ())
		receipt.getInAppPurchases ().add (license);
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ReceiptReader::string (StringRef data)
{
	if(attributeType == kBundleIdentifier)
		receipt.setBundleID (data);
	
	else if (attributeType == kAppVersion)
		receipt.setAppVersion (data);
		
	return kResultOk;
}

//************************************************************************************************
// PurchaseReader
//************************************************************************************************

PurchaseReader::PurchaseReader (StoreLicense& _license)
: license (_license)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PurchaseReader::octetString (IStream& data)
{
	switch(attributeType)
	{
	case kProductIdentifier : // product identifier as UTF8STRING wrapped in an OCTET_STRING
		return BER::decode (*this, data) ? kResultOk : kResultFalse;
	
	case kCancellationDate : // IA5STRING, interpreted as an RFC 3339 date, wrapped in an OCTET_STRING
		return BER::decode (*this, data) ? kResultOk : kResultFalse;
	}
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PurchaseReader::string (StringRef data)
{
	switch(attributeType)
	{
	case kProductIdentifier :
		license.setProductID (data);
		return kResultOk;
			
	case kCancellationDate :
		if(data.isEmpty () == false)
			license.setVerificationResult (LicenseVerificationResult::kExpired);
		return kResultOk;
	}
	
	return kResultOk;
}

//************************************************************************************************
// AppleStoreReceipt
//************************************************************************************************

AppleStoreReceipt::AppleStoreReceipt (NSURL* _receiptUrl)
: receiptUrl (nil),
  receiptHash (SHA1::kDigestSize)
{
	if(_receiptUrl == nil)
		receiptUrl = [[NSBundle mainBundle] appStoreReceiptURL];
	else
		receiptUrl = _receiptUrl;

	inAppPurchases.objectCleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AppleStoreReceipt::isValid ()
{
	return readFromFile ();
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

bool AppleStoreReceipt::getPurchases (ObjectArray& licenses)
{
	bool success = readFromFile ();
	if(!success)
		return false;

	licenses.add (inAppPurchases, Container::kShare);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AppleStoreReceipt::readFromFile ()
{
	inAppPurchases.removeAll ();
	NSMutableData* receiptData = [[[NSMutableData alloc] initWithData:[NSData dataWithContentsOfURL:receiptUrl]] autorelease];
	
	#if COPY_RECEIPT_FILE
	NSArray* paths = NSSearchPathForDirectoriesInDomains (NSDocumentDirectory, NSUserDomainMask, YES);
	[receiptData writeToURL:[NSURL fileURLWithPath:@"receipt" relativeToURL:[NSURL fileURLWithPath:paths.firstObject]] options:NSDataWritingAtomic error:nil];
	#endif
	
	MemoryStream pkcs7 (receiptData.mutableBytes, static_cast <unsigned int> (receiptData.length));

	MemoryStream certificates;
	if(PKCS7::getCertificates (certificates, pkcs7) == false)
		return false;
		
	if(verifyCertificate (certificates) == false)
		return false;

	ReceiptReader receiver (*this);
	if(PKCS7::decodeData (receiver, pkcs7) == false)
		return false;

	Material deviceHash (SHA1::kDigestSize);
	calculateDeviceHash (deviceHash);
	if(receiptHash.equals (deviceHash) == false)
		return false;

	if(NSString* infoEntry = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleIdentifier"])
	{
		String origBundleID;
		origBundleID.appendNativeString (infoEntry);
		if(bundleID.compare (origBundleID) != Text::kEqual)
			return false;
	}
	
	NSString* infoEntry = nil;
	#if CCL_PLATFORM_MAC
	infoEntry = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleShortVersionString"];
	#else // CCL_PLATFORM_IOS
	infoEntry = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleVersion"];
	#endif
	if(infoEntry)
	{
		String origAppVersion;
		origAppVersion.appendNativeString (infoEntry);
		if(appVersion.compare (origAppVersion) != Text::kEqual)
			return false;
	}
	else
		return false;
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AppleStoreReceipt::calculateDeviceHash (Material& deviceHash)
{
	Material bytes;

	#if CCL_PLATFORM_MAC
	CFObj<CFDataRef> macData = MacUtils::copyMacAddress ();
	if(macData)
		bytes.append (Block (CFDataGetBytePtr (macData), static_cast<uint32> (CFDataGetLength (macData))));
	#else // CCL_PLATFORM_IOS
	unsigned char uuidBytes[16];
	[[[UIDevice currentDevice] identifierForVendor] getUUIDBytes:uuidBytes];
	bytes.append (Block (uuidBytes, 16));
	#endif
	
	bytes.append (opaquaValue);
	bytes.append (rawBundleID);
	
	SHA1::calculate (deviceHash, bytes);
	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AppleStoreReceipt::verifyCertificate (MemoryStream& cert)
{
	class CertificateReader: public Unknown,
					 		 public IASN1ContentHandler
	{
	public:
		CertificateReader (NSMutableArray* _certificates)
		: certificates (_certificates)
		{}
		
		// IASN1ContentHandler
		tresult CCL_API sequence (IStream& data) override
		{
			MemoryStream container;
			DER::encode (container, DER::kSequence | DER::kConstructed, data);
			Material m;
			m.copyFrom (container);
			CFObj<CFDataRef> certificateData = CFDataCreate (CFAllocatorGetDefault (), static_cast<const UInt8*> (m.asBlock ().data), m.getSize ());
			CFObj<SecCertificateRef> certificate = SecCertificateCreateWithData (CFAllocatorGetDefault (), certificateData);
			ASSERT (certificate)
			if(certificate)
				[certificates addObject:(__bridge id)((SecCertificateRef)certificate)];
			
			return kResultOk;
		};
		tresult CCL_API integer (int i) override { return kResultOk; }
		tresult CCL_API set (IStream& data) override { return kResultOk; }
		tresult CCL_API octetString (IStream& data) override { return kResultOk; }
		tresult CCL_API string (StringRef data) override { return kResultOk; }
		tresult CCL_API context (unsigned char& defaultTag, tbool& implicit, unsigned char contextTag) override { return kResultOk; }
		
		CLASS_INTERFACE (IASN1ContentHandler, Unknown);
		
	protected:
		NSMutableArray* certificates;
	};
	
	static StringRef kAppleRootFingerprint (CCLSTR ("B0B1730ECBC7FF4505142C49F1295E6EDA6BCAED7E2C68C5BE91B5A11001F024"));

	NSMutableArray* certArray = [NSMutableArray arrayWithCapacity:3];
	CertificateReader reader (certArray);
	BER::decode (reader, cert);

	CFTypeRef policyTemplate = NULL;
	#if DEBUG
	policyTemplate = kSecPolicyAppleX509Basic;
	#else
	policyTemplate = kSecPolicyMacAppStoreReceipt;
	#endif
	CFObj<SecPolicyRef> policy = SecPolicyCreateWithProperties (policyTemplate, NULL);
	SecTrustRef t;
	OSStatus status = SecTrustCreateWithCertificates ((__bridge CFTypeRef)certArray, policy, &t);
	if(status != errSecSuccess)
		return false;
	CFObj<SecTrustRef> trust (t);

	NSString* certFileName = nil;
	#if RELEASE
	certFileName = @"Apple Root CA";
	#else
	certFileName = @"StoreKitTestCertificate";
	#endif
	CFDataRef rootCertificateData = rootCertificateData = (CFDataRef)[NSData dataWithContentsOfURL:[[NSBundle mainBundle] URLForResource:certFileName withExtension:@"cer"]];
	if(rootCertificateData == NULL)
		return false;
	
	CFObj<SecCertificateRef> rootCert = SecCertificateCreateWithData (CFAllocatorGetDefault (), rootCertificateData);
	if(rootCert == NULL)
		return false;
	#if RELEASE
	Material fingerprint (SHA256::kDigestSize);
	MemoryStream certificateStream (const_cast <void*> (static_cast<const void*> (CFDataGetBytePtr (rootCertificateData))), static_cast<unsigned int> (CFDataGetLength (rootCertificateData)));
	SHA256::calculate (fingerprint, certificateStream);
	Material appleRootFingerprint (SHA256::kDigestSize);
	appleRootFingerprint.fromHex (kAppleRootFingerprint);
	if(fingerprint.equals (appleRootFingerprint) == false)
		return false;
	#endif
	
	NSArray* rootCertArray = @[ (__bridge id)((SecCertificateRef)rootCert) ];
	status = SecTrustSetAnchorCertificates (trust, (__bridge CFArrayRef)rootCertArray);
	if(status != errSecSuccess)
		return false;

	CFErrorRef error = NULL;
	if(SecTrustEvaluateWithError (trust, &error) == NO)
		return false;
	
	return true;
}
