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
// Filename    : ccl/security/authorizationmanager.cpp
// Description : Authorization Manager
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/security/authorizationmanager.h"

#include "ccl/base/storage/attributes.h"
#include "ccl/base/storage/jsonarchive.h"
#include "ccl/base/security/signature.h"

#include "ccl/public/text/istringdict.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/securityservices.h"

namespace CCL {
namespace Security {

//////////////////////////////////////////////////////////////////////////////////////////////////
// CRC-8 checksum algorithm
//////////////////////////////////////////////////////////////////////////////////////////////////

static uint8 crc8 (uint8 data, uint8 crc)
{
	static const uint8 kCrc8Poly = 0x18;

	for(int i = 0; i < 8; i++)
	{
		if((crc & 0x80) ^ (data & 0x80))
			crc = (uint8)((crc << 1) ^ kCrc8Poly);
		else
			crc = (uint8)(crc << 1);
		data <<= 1;
	}
	return crc;
}

static uint8 crc8 (const uint8 data[], int length)
{
	uint8 crc = 0;
	for(int i = 0; i < length; i++)
		crc = crc8 (data[i], crc);
	return crc;
}

} // namespace Security
} // namespace CCL

using namespace CCL;
using namespace Security;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Security Service APIs
////////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT IAuthorizationManager& CCL_API System::CCL_ISOLATED (GetAuthorizationManager) ()
{
	return AuthorizationManager::instance ();
}

//************************************************************************************************
// AuthorizationManager
//************************************************************************************************

DEFINE_SINGLETON (AuthorizationManager)
DEFINE_CLASS_HIDDEN (AuthorizationManager, Object)

////////////////////////////////////////////////////////////////////////////////////////////////////

AuthorizationManager::AuthorizationManager ()
#if CCL_PLATFORM_WINDOWS
// As of CCL revision 16, Windows is using computer identifier v2
: platformComputerIdType (kComputerIDv2)
#else
: platformComputerIdType (kComputerIDv1)
#endif
{}

////////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AuthorizationManager::setAppIdentity (StringRef _appSid, StringRef _appSubSid)
{
	ASSERT (appSid.isEmpty ())
	if(!appSid.isEmpty ()) // must be called only once!
		return kResultFailed;

	appSid = _appSid;
	appSubSid = _appSubSid;
	return kResultOk;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API AuthorizationManager::getAppIdentity () const
{
	return appSid;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API AuthorizationManager::getAppSubIdentity () const
{
	return appSubSid;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AuthorizationManager::loadPolicy (IStream& stream, int mode)
{
	// main app policy must be signed!
	if(get_flag<int> (mode, kUnsignedPolicy) && policy == nullptr)
		return kResultInvalidArgument;

	AutoPtr<Authorization::Policy> p = parsePolicy (stream, mode);
	if(!p)
		return kResultFailed;

	if(policy == nullptr)
		policy = p.detach ();
	else
		policy->merge (*p);
	return kResultOk;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

Authorization::IPolicyItem* CCL_API AuthorizationManager::getPrivileges (StringRef resourceSid)
{
	if(!policy) // no policy loaded
		return nullptr;

	return ccl_const_cast (policy->getRoot ().lookup<Authorization::Resource> (resourceSid));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

IUnknownIterator* CCL_API AuthorizationManager::createIterator (StringRef dataSid)
{
	if(!policy) // no policy loaded
		return nullptr;

	ASSERT (!dataSid.isEmpty ())
	const Authorization::AssociatedData* data = policy->getRoot ().lookup<Authorization::AssociatedData> (dataSid);
	return data ? data->newIterator () : nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AuthorizationManager::revokePolicy (IStream& stream, int mode)
{
	if(!policy) // no policy loaded
		return kResultUnexpected;
		
	AutoPtr<Authorization::Policy> p = parsePolicy (stream, mode);
	if(!p)
		return kResultFailed;
	
	policy->revoke (*p);
	return kResultOk;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

Authorization::Policy* AuthorizationManager::parsePolicy (IStream& stream, int mode) const
{
	AutoPtr<Authorization::Policy> p = NEW Authorization::Policy;
	bool success = false;

	if(mode & kUnsignedPolicy)
	{
		// *** Plain XML/JSON policy ***
		AutoPtr<IMemoryStream> rawData;
		if(UnknownPtr<IMemoryStream> inMemStream = &stream)
			rawData.share (inMemStream);
		else
			rawData = System::GetFileUtilities ().createStreamCopyInMemory (stream);
		
		if(rawData)
		{
			bool isJson = JsonArchive::isJson (rawData->getMemoryAddress (), rawData->getBytesWritten ());
			success = isJson ? p->loadFromJsonStream (*rawData) : p->loadFromStream (*rawData);
		}
	}
	else
	{
		// *) Init AES Cipher
		AutoPtr<Crypto::Cipher> cipher = NEW Crypto::Cipher;
		cipher->setFromKeyStore (KeyID::kAuthPolicy);

		// *) Read Signed/Encrypted Policy
		Crypto::SignedXmlMessage signedPolicy (CSTR ("SignedAuthorizationPolicy"));
		signedPolicy.setCipher (cipher);
		if(!signedPolicy.loadFromStream (stream))
			return nullptr;

		// *) Decrypt Public Key
		Crypto::Material encryptedPublicKey;
		System::GetCryptoKeyStore ().getMaterial (encryptedPublicKey, KeyID::kAuthPolicy, Crypto::kPublicKey);
		Crypto::Material publicKey;
		cipher->decrypt (publicKey, encryptedPublicKey);

		// *) Verify Signature
		Crypto::Verifier verifier;
		verifier.setPublicKey (publicKey);
		if(!verifier.verify (signedPolicy))
			return nullptr;

		// *) Load Plain Policy
		success = signedPolicy.getObjectFromData (*p);
	}

	ASSERT (success == true)
	return success ? p.detach () : nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AuthorizationManager::getComputerIdentity (String& computerSid, ComputerIDType type)
{
	if(savedComputerSidV1.isEmpty () || savedComputerSidV2.isEmpty ())
	{
		#if CCL_PLATFORM_DESKTOP
		updateHardwareHashes ();
		#else // use simplified identifiers on mobile platforms
		Attributes computerInfo;
		System::GetSystem ().getComputerInfo (computerInfo);
		savedComputerSidV1 = computerInfo.getString (System::kDeviceIdentifier);
		savedComputerSidV2 = savedComputerSidV1;
		#endif
	}

	switch(type)
	{
	case kComputerIDv1 : computerSid = savedComputerSidV1; break; 
	case kComputerIDv2 : computerSid = savedComputerSidV2; break; 
	default : // kDefaultComputerID
		if(platformComputerIdType == kComputerIDv2)
			computerSid = savedComputerSidV2;
		else
			computerSid = savedComputerSidV1;
		break;
	}
	return kResultOk;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void AuthorizationManager::updateHardwareHashes ()
{
	CCL_PRINTLN ("Gathering computer information...")
	ASSERT (System::IsInMainThread ())

	BasicComputerInformation info;
	getBasicComputerInformation (info);

	CCL_PRINTF ("### Basic Computer Information ###\n"
				"CPU Speed: %d (Model: %s)\n"
				"Physical Memory: %.2lf MB\n"
				"Disk: %s\n"
				"MAC Address: %s (Adapter: %s)\n"
                "DeviceModelSubtype: %s\n",
				info.cpuClockSpeed,
				MutableCString (info.cpuIdentifier).str (),
				(double)info.physicalMemoryAmount / (1024. * 1024.),
				MutableCString (info.diskIdentifier).str (),
				MutableCString (info.macAddress).str (),
				MutableCString (info.adapterName).str (),
                MutableCString (info.deviceModelSubtype).str ())

	// *** Hardware Hash V1 ***
	// Based on CPU model and MAC address with fallback to disk model.
	// Works fine on macOS, problematic on Windows because of MAC address.
	
	ComputerFingerprint fingerprintV1;
    if(info.cpuClockSpeed == 0 || info.processIsTranslated)
    {
        // Apple Silicon
        fingerprintV1.data.fields.partByte1 = (uint8)(info.deviceModelSubtype.getHashCode () % 0xFF); // e.g. "MacBookPro17,1"
        fingerprintV1.data.fields.partHash2 = info.cpuModel.getHashCode (); // e.g. "Apple M1"
    }
    else
    {
        fingerprintV1.data.fields.partByte1 = (uint8)(info.cpuClockSpeed % 0xFF);
        fingerprintV1.data.fields.partHash2 = info.cpuIdentifier.getHashCode ();
    }
    
	if(!info.macAddress.isEmpty ())
		fingerprintV1.data.fields.partHash1 = info.macAddress.getHashCode ();
	else
		fingerprintV1.data.fields.partHash1 = info.diskIdentifier.getHashCode ();
	
	savedComputerSidV1 = finish (fingerprintV1);

	// *** Hardware Hash V2 ***
	// Based on CPU model and disk serial number with fallback to volume serial.
	// Currently Windows only to get rid of the MAC address.
		
	bool hasInfoV2 = !info.diskSerialNumber.isEmpty () || !info.volumeSerialNumber.isEmpty ();
	if(hasInfoV2 == true)
	{
		ComputerFingerprint fingerprintV2;
		fingerprintV2.data.fields.partByte1 = fingerprintV1.data.fields.partByte1; // same as V1
		if(!info.diskSerialNumber.isEmpty ())
			fingerprintV2.data.fields.partHash1 = info.diskSerialNumber.getHashCode ();
		else
			fingerprintV2.data.fields.partHash1 = info.volumeSerialNumber.getHashCode ();
		fingerprintV2.data.fields.partHash2 = fingerprintV1.data.fields.partHash2; // same as V1

		savedComputerSidV2 = finish (fingerprintV2);
	}
	else
		savedComputerSidV2 = savedComputerSidV1;

	CCL_PRINTF ("==> Computer ID v1: %s\n\n", MutableCString (savedComputerSidV1).str ())
	CCL_PRINTF ("==> Computer ID v2: %s\n\n", MutableCString (savedComputerSidV2).str ())
}

////////////////////////////////////////////////////////////////////////////////////////////////////

String AuthorizationManager::finish (ComputerFingerprint& fingerprint) const
{
	ASSERT (fingerprint.data.fields.partByte1 != 0)
	ASSERT (fingerprint.data.fields.partHash1 != 0)
	ASSERT (fingerprint.data.fields.partHash2 != 0)

	if(fingerprint.data.fields.partByte1 == 0)
		fingerprint.data.fields.partByte1 = 0xff;
	if(fingerprint.data.fields.partHash1 == 0)
		fingerprint.data.fields.partHash1 = 0xbaadf00d;
	if(fingerprint.data.fields.partHash2 == 0)
		fingerprint.data.fields.partHash2 = 0xbaadf00d;

	ASSERT (fingerprint.data.fields.checkByte == 0)
	fingerprint.data.fields.checkByte = crc8 (fingerprint.data.bytes, ComputerFingerprint::kSize);
	ASSERT (verify (fingerprint) == true)

	String base32 = Crypto::Material (Crypto::Block (fingerprint.data.bytes, ComputerFingerprint::kSize)).toBase32 ();
	ASSERT (base32.length () == 16)

	String result;
	StringWriter<128> writer (result);
	for(int i = 0; i < 16; i++)
	{
		if(i > 0 && i % 4 == 0)
			writer.append ('-');

		writer.append (base32[i]);
	}
	writer.flush ();
	return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool AuthorizationManager::verify (const ComputerFingerprint& fingerprint) const
{
	// boilerplate code for computer fingerprint verification
	ComputerFingerprint fingerprint2 (fingerprint);
	fingerprint2.data.fields.checkByte = 0;
	uint8 checkByte = crc8 (fingerprint2.data.bytes, ComputerFingerprint::kSize);
	return checkByte == fingerprint.data.fields.checkByte;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void AuthorizationManager::getBasicComputerInformation (BasicComputerInformation& info) const
{
	Attributes attributes;
	System::GetSystem ().getComputerInfo (attributes, System::kQueryExtendedComputerInfo);

	info.cpuClockSpeed = attributes.getInt (System::kCPUSpeed);
	info.physicalMemoryAmount = attributes.getInt64 (System::kPhysicalRAMSize);
	info.cpuIdentifier = attributes.getString (System::kCPUIdentifier);
    info.cpuModel = attributes.getString (System::kCPUModelHumanReadable);
	info.diskIdentifier = attributes.getString (System::kDiskModelHumanReadable);
	info.diskSerialNumber = attributes.getString (System::kDiskSerialNumber);
	info.volumeSerialNumber = attributes.getString (System::kVolumeSerialNumber);
	info.macAddress = attributes.getString (System::kMACAddress);
	info.adapterName = attributes.getString (System::kEthernetAdapter);
    info.deviceModelSubtype = attributes.getString (System::kDeviceModelSubtype);
    info.processIsTranslated = attributes.getBool (System::kProcessIsTranslated);
}
