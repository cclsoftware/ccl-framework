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
// Filename    : ccl/base/security/cryptobox.h
// Description : Cryptographical Toolbox
//
//************************************************************************************************

#ifndef _ccl_cryptobox_h
#define _ccl_cryptobox_h

#include "ccl/public/system/cryptotypes.h"

namespace CCL {

interface IStream;
interface IProgressNotify;

namespace Security {
namespace Crypto {

interface IInteger;
interface IASN1ContentHandler;

//************************************************************************************************
// Crypto::RandomPool
//************************************************************************************************

class RandomPool
{
public:
	static bool generate (BlockRef block);
};

//************************************************************************************************
// Crypto::MD5
//************************************************************************************************

class MD5
{
public:
	enum Constants 
	{
		kDigestSize = kMD5_DigestSize
	};

	static bool calculate (BlockRef digest, IStream& data, IProgressNotify* progress = nullptr);
};

//************************************************************************************************
// Crypto::SHA1
//************************************************************************************************

class SHA1
{
public:
	enum Constants 
	{
		kDigestSize = kSHA1_DigestSize
	};

	static bool calculate (BlockRef digest, IStream& data, IProgressNotify* progress = nullptr);
};

//************************************************************************************************
// Crypto::SHA256
//************************************************************************************************

class SHA256
{
public:
	enum Constants 
	{
		kDigestSize = kSHA256_DigestSize
	};

	static bool calculate (BlockRef digest, IStream& data, IProgressNotify* progress = nullptr);
};

//************************************************************************************************
// Crypto::HMAC_SHA1
//************************************************************************************************

class HMAC_SHA1
{
public:
	static bool sign (IStream& signature, BlockRef key, IStream& data);
};

//************************************************************************************************
// Crypto::HMAC_SHA256
//************************************************************************************************

class HMAC_SHA256
{
public:
	static bool sign (IStream& signature, BlockRef key, IStream& data);
};

//************************************************************************************************
// Crypto::HKDF
//************************************************************************************************

class HKDF
{
public:
	enum Constants
	{
		kKeyLen16 = 16,
		kKeyLen24 = 24,
		kKeyLen32 = 32
	};
	
	static bool deriveKey (IStream& derivedKey, uint32 derivedKeyLength, BlockRef secret, BlockRef salt, BlockRef info);
};

//************************************************************************************************
// Crypto::AES
//************************************************************************************************

class AES
{
public:
	enum Constants 
	{
		kDefaultKeySize = kAES_DefaultKeySize,
		kBlockSize = kAES_BlockSize
	};

	static bool encrypt (IStream& cipherData, BlockRef key, BlockRef iv, IStream& plainData, BlockCipherMode mode = kBlockCipher_CTR);
	static bool decrypt (IStream& plainData, BlockRef key, BlockRef iv, IStream& cipherData, BlockCipherMode mode = kBlockCipher_CTR);
};

//************************************************************************************************
// Crypto::RSA
//************************************************************************************************

class RSA
{
public:
	enum Constants 
	{
		kDefaultKeyLength = kRSA_DefaultKeyLength
	};

	static bool generateKeyPair (IStream& privateKey, IStream& publicKey, BlockRef randomData = Block (), uint32 keyLength = kDefaultKeyLength);
	static bool encrypt (IStream& cipherData, IStream& publicKey, IStream& plainData, BlockRef randomData = Block ());
	static bool decrypt (IStream& plainData, IStream& privateKey, IStream& cipherData);	
	static bool sign (IStream& signature, IStream& privateKey, IStream& data, Hash hash = kHashSHA1);
	static bool verify (IStream& data, IStream& publicKey, IStream& signature, Hash hash = kHashSHA1);
};

//************************************************************************************************
// Crypto::BER
//************************************************************************************************

class BER
{
public:
	static bool decode (IASN1ContentHandler& sink, IStream& encodedData);
};

//************************************************************************************************
// Crypto::DER
//************************************************************************************************

class DER
{
public:
	enum ASNTags
	{
		kBoolean			= 0x01,
		kInteger			= 0x02,
		kBitString			= 0x03,
		kOctetString		= 0x04,
		kTagNull			= 0x05,
		kObjectIdentifier	= 0x06,
		kObjectDescriptor	= 0x07,
		kExternal			= 0x08,
		kReal				= 0x09,
		kEnumerated			= 0x0a,
		kUTF8String			= 0x0c,
		kSequence			= 0x10,
		kSet				= 0x11,
		kNumericString		= 0x12,
		kPrintableString	= 0x13,
		kT61String			= 0x14,
		kVideotextString	= 0x15,
		kIA5String			= 0x16,
		kUTCTime			= 0x17,
		kGeneralizedTime	= 0x18,
		kGraphicString		= 0x19,
		kVisibleString		= 0x1a,
		kGeneralString		= 0x1b,
		kUniversalString	= 0x1c,
		kBMPString			= 0x1e
	};
	
	enum ASNFlags
	{
		kUniversal			= 0x00,
		kPrimitive			= 0x00,
		kConstructed		= 0x20,
		kApplication		= 0x40,
		kContextSpecific	= 0x80,
		kPrivate			= 0xc0
	};

	static bool encode (IStream& encodedData, int tag, IStream& content);
};

//************************************************************************************************
// Crypto::PKCS7
//************************************************************************************************

class PKCS7
{
public:
	static bool decodeData (IASN1ContentHandler& reader, IStream& encodedData);
	static bool getCertificates (IStream& certificates, IStream& encodedData);
};

//************************************************************************************************
// Crypto::IntegerStatics
//************************************************************************************************

class IntegerStatics
{
public:
	static IInteger* create ();
};

} // namespace Crypto
} // namespace Security
} // namespace CCL

#endif // _ccl_cryptobox_h
