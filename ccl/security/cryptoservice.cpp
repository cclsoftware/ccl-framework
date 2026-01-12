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
// Filename    : ccl/security/cryptoservice.cpp
// Description : Cryptographical Services
//
//************************************************************************************************
/*
	Modes for Block Cyphers
	=======================
	ECB... Electronic Codebook		(*)
	CBC... Cipher Block Chaining	(*)
	CFB... Cipher Feedback			(**)
	OFB... Output Feedback			(***)
	CTR... Counter					(***)

	(*)   size of plain data must be a multiple of block size
	(**)  size of plain data must be divisible by value smaller than block size
	(***) size of plain data _need not_ be a multiple of block size

	see: http://csrc.nist.gov/publications/nistpubs/800-38a/sp800-38a.pdf

	Other Acronyms
	==============

	PKCS...  Public Key Crypto System
	OAEP...  Optimal Asymmetric Encryption Padding
	ASN.1... Abstract Syntax Notation One
	BER...   Basic Encoding Rules
	DER...   Distinguished Encoding Rules

	File Formats
	============
	PEM...  (Privacy Enhanced Mail) base64-encoded ASN.1

			-----BEGIN CERTIFICATE-----
			xxxxxx
			-----END CERTIFICATE-----

			-----BEGIN RSA PRIVATE KEY-----
			xxxxxx
			-----END RSA PRIVATE KEY-----

	DER...	raw ASN.1 - a binary format

	see:	http://www.bo.infn.it/alice/introgrd/certmgr/node2.html
			http://de.wikipedia.org/wiki/X.509

	XML Signature
	=============
	see:	http://www.w3.org/2000/09/xmldsig#rsa-sha1
*/

#define DEBUG_LOG 0

#include "cryptoservice.h"
#include "cryptoppglue.h"

#include "ccl/public/base/buffer.h"
#include "ccl/public/base/istream.h"
#include "ccl/public/base/iprogress.h"
#include "ccl/public/base/memorystream.h"
#include "ccl/public/security/iasn1contenthandler.h"
#include "ccl/public/security/icryptointeger.h"
#include "ccl/public/securityservices.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
// Helper Macros
//////////////////////////////////////////////////////////////////////////////////////////////////

#define __BEGIN_CRYPTOPP_CALL__ \
	try {

#define __END_CTYPTOPP_CALL__ \
	} catch(...) { ASSERT (false) CCL_PRINTLN (" ### Crypto++ Exception!!!") return kResultFailed; }

#define CHECK_BLOCK_ARGUMENT(block, size) \
	ASSERT (block.length == size) \
	if(block.length != size) return kResultInvalidArgument;

namespace CCL {
namespace Security {
namespace Crypto {

//************************************************************************************************
// CryptoHandler Adapater Class
//************************************************************************************************

class CryptoHandler: public CryptoPP::ASN1ContentHandler
{
public:
	CryptoHandler (IASN1ContentHandler& _handler)
	: handler (_handler)
	{}
	
	// ASN1ContentHandler
	bool octetString (char* str, int length) override
	{
		MemoryStream stream (str, length);
		return handler.octetString (stream) == kResultOk;
	}
	
	bool sequence (Core::IO::MemoryStream& _sequence) override
	{
		MemoryStream sequence (const_cast<void*> (_sequence.getBuffer ().getAddress ()), _sequence.getBytesWritten ());
		return handler.sequence (sequence) == kResultOk;
	}
	
	bool set (Core::IO::MemoryStream& _set) override
	{
		MemoryStream set (const_cast<void*> (_set.getBuffer ().getAddress ()), _set.getBytesWritten ());
		return handler.set (set) == kResultOk;
	}

	bool integer (long i) override
	{
		return handler.integer (int(i)) == kResultOk;
	}
	
	bool asciiString (char* str, int length) override
	{
		String string;
		string.appendCString (Text::kASCII, str, length);
		return handler.string (string) == kResultOk;
	}
	
	bool utf8String (char* str, int length) override
	{
		String string;
		string.appendCString (Text::kUTF8, str, length);
		return handler.string (string) == kResultOk;
	}
	
	bool context (unsigned char& defaultTag, bool& _implicit, unsigned char contextTag) override
	{
		tbool implicit = false;
		tresult result = handler.context (defaultTag, implicit, contextTag);
		_implicit = implicit;
		return result == kResultOk;
	}
	
private:
	IASN1ContentHandler& handler;
};

//************************************************************************************************
// CryptoInteger
//************************************************************************************************

class CryptoInteger: public Object,
					 public IInteger
{
public:
	DECLARE_CLASS (CryptoInteger, Object)

	// IInteger
	tresult CCL_API fromCString (CStringPtr string, int base) override
	{
		__BEGIN_CRYPTOPP_CALL__
		
		return internal.fromString (string, base) ? kResultOk : kResultFailed;

		__END_CTYPTOPP_CALL__
	}

	tresult CCL_API toCString (MutableCString& string, int base = 16) const override
	{
		// TODO: determine required size...
		char buffer[STRING_STACK_SPACE_MAX] {};
		StringResult result (buffer, sizeof(buffer));

		__BEGIN_CRYPTOPP_CALL__

		if(!internal.toString (result, base))
			return kResultFailed;

		__END_CTYPTOPP_CALL__

		string.empty ();
		string.append (result.charBuffer);
		return kResultOk;
	}

	tresult CCL_API add (IInteger& result, const IInteger& value) override
	{
		auto* v = unknown_cast<CryptoInteger> (&value);
		auto* r = unknown_cast<CryptoInteger> (&result);
		if(!v || !r)
			return kResultInvalidArgument;
		
		__BEGIN_CRYPTOPP_CALL__

		internal.add (r->internal, v->internal);		
		return kResultOk;
		
		__END_CTYPTOPP_CALL__
	}
	
	tresult CCL_API substract (IInteger& result, const IInteger& value) override
	{
		auto* v = unknown_cast<CryptoInteger> (&value);
		auto* r = unknown_cast<CryptoInteger> (&result);
		if(!v || !r)
			return kResultInvalidArgument;

		__BEGIN_CRYPTOPP_CALL__

		internal.substract (r->internal, v->internal);
		return kResultOk;

		__END_CTYPTOPP_CALL__
	}

	tresult CCL_API multiply (IInteger& result, const IInteger& factor) override
	{
		auto* f = unknown_cast<CryptoInteger> (&factor);
		auto* r = unknown_cast<CryptoInteger> (&result);
		if(!f || !r)
			return kResultInvalidArgument;

		__BEGIN_CRYPTOPP_CALL__

		internal.multiply (r->internal, f->internal);
		return kResultOk;

		__END_CTYPTOPP_CALL__
	}
	
	tresult CCL_API divide (IInteger& result, const IInteger& factor) override
	{
		auto* f = unknown_cast<CryptoInteger> (&factor);
		auto* r = unknown_cast<CryptoInteger> (&result);
		if(!f || !r)
			return kResultInvalidArgument;

		__BEGIN_CRYPTOPP_CALL__

		internal.divide (r->internal, f->internal);
		return kResultOk;

		__END_CTYPTOPP_CALL__
	}
	
	tresult CCL_API expMod (IInteger& result, const IInteger& exp, const IInteger& mod) override
	{
		auto* e = unknown_cast<CryptoInteger> (&exp);
		auto* m = unknown_cast<CryptoInteger> (&mod);
		auto* r = unknown_cast<CryptoInteger> (&result);
		if(!e || !m || !r)
			return kResultInvalidArgument;

		__BEGIN_CRYPTOPP_CALL__

		internal.expMod (r->internal, e->internal, m->internal);
		return kResultOk;

		__END_CTYPTOPP_CALL__
	}
	
	tresult CCL_API modulo (IInteger& result, const IInteger& value) override
	{
		auto* v = unknown_cast<CryptoInteger> (&value);
		auto* r = unknown_cast<CryptoInteger> (&result);
		if(!v || !r)
			return kResultInvalidArgument;

		__BEGIN_CRYPTOPP_CALL__

		internal.modulo (r->internal, v->internal);
		return kResultOk;
	
		__END_CTYPTOPP_CALL__
	}
	
	CLASS_INTERFACE (IInteger, Object)

protected:
	CryptoPP::IntWrapper internal;
};

} // namespace Crypto
} // namespace Security
} // namespace CCL

using namespace CCL;
using namespace Security;
using namespace Crypto;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Security Service APIs
////////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT Internal::ICryptoService& CCL_API System::CCL_ISOLATED (GetCryptoService) ()
{
	static CryptoService theCryptoService;
	return theCryptoService;
}

//************************************************************************************************
// CryptoInteger
//************************************************************************************************

DEFINE_CLASS_HIDDEN (CryptoInteger, Object)

//************************************************************************************************
// CryptoService
//************************************************************************************************

DEFINE_CLASS_HIDDEN (CryptoService, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////
// RNG
//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CryptoService::RNG_generate (BlockRef randomData)
{
	__BEGIN_CRYPTOPP_CALL__

	CryptoPP::RNG_generate (randomData.data, randomData.length);

	__END_CTYPTOPP_CALL__
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Hash Algorithms
//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Hash> tresult CryptoService::calculateDigest (BlockRef digest, IStream& data, IProgressNotify* progress)
{
	__BEGIN_CRYPTOPP_CALL__

	Hash hash;
	unsigned int blockSize = hash.getOptimalBlockSize ();
	if(blockSize <= 1)
		blockSize = 1024;

	Buffer buffer (blockSize);
	int progressCounter = 0;
	while(true)
	{
		int numRead = data.read (buffer, blockSize);
		if(numRead == 0)
			break;
		if(numRead < 0) // stream error
			return kResultFailed;

		hash.update (buffer.getAddress (), numRead);

		if(progress)
		{
			progressCounter += numRead;
			if(progressCounter >= 0xFFFF) // every 64KB
			{
				progress->updateAnimated ();
				if(progress->isCanceled ())
					return kResultAborted;
				progressCounter = 0;
			}
		}
	}

	hash.finish (digest.data);

	__END_CTYPTOPP_CALL__
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CryptoService::MD5_calculate (BlockRef digest, IStream& data, IProgressNotify* progress)
{
	CHECK_BLOCK_ARGUMENT (digest, kMD5_DigestSize)

	return calculateDigest<CryptoPP::MD5Hash> (digest, data, progress);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CryptoService::SHA1_calculate (BlockRef digest, IStream& data, IProgressNotify* progress)
{
	CHECK_BLOCK_ARGUMENT (digest, kSHA1_DigestSize)

	return calculateDigest<CryptoPP::SHA1Hash> (digest, data, progress);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CryptoService::SHA256_calculate (BlockRef digest, IStream& data, IProgressNotify* progress)
{
	CHECK_BLOCK_ARGUMENT (digest, kSHA256_DigestSize)

	return calculateDigest<CryptoPP::SHA256Hash> (digest, data, progress);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// (Keyed-Hash) Message Authentication Code
//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CryptoService::HMAC_SHA1_sign (IStream& _signature, BlockRef key, IStream& _data)
{
	CoreStream signature (_signature);
	CoreStream data (_data);
	
	__BEGIN_CRYPTOPP_CALL__

	return CryptoPP::HMAC_SHA1_sign (signature, key.data, key.length, data) ? kResultOk : kResultFailed;

	__END_CTYPTOPP_CALL__
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CryptoService::HMAC_SHA256_sign (IStream& _signature, BlockRef key, IStream& _data)
{
	CoreStream signature (_signature);
	CoreStream data (_data);

	__BEGIN_CRYPTOPP_CALL__

	return CryptoPP::HMAC_SHA256_sign (signature, key.data, key.length, data) ? kResultOk : kResultFailed;
	
	__END_CTYPTOPP_CALL__
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// (HKDF) Key Function Derivation
//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CryptoService::HKDF_deriveKey (IStream& _derivedKey, uint32 derivedKeyLength, BlockRef secret, BlockRef salt, BlockRef info)
{
	CoreStream derivedKeyStream (_derivedKey);
	
	__BEGIN_CRYPTOPP_CALL__

	return CryptoPP::HKDF_deriveKey (derivedKeyStream, derivedKeyLength, secret.data, secret.length, salt.data, salt.length, info.data, info.length) ? kResultOk : kResultFailed;
	
	__END_CTYPTOPP_CALL__

	return kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// AES
//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CryptoService::AES_encrypt (IStream& _cipherData, BlockRef key, BlockRef iv, IStream& _plainData, BlockCipherMode mode)
{
	CHECK_BLOCK_ARGUMENT (key, kAES_DefaultKeySize)
	CHECK_BLOCK_ARGUMENT (iv, kAES_BlockSize)

	CoreStream cipherData (_cipherData);
	CoreStream plainData (_plainData);
	
	__BEGIN_CRYPTOPP_CALL__

	switch(mode)
	{
	case kBlockCipher_CTR : return CryptoPP::AES_CTR_encrypt (cipherData, key.data, key.length, iv.data, plainData) ? kResultOk : kResultFailed;
	case kBlockCipher_CBC : return CryptoPP::AES_CBC_encrypt (cipherData, key.data, key.length, iv.data, plainData) ? kResultOk : kResultFailed;
	default :
		CCL_NOT_IMPL ("Invalid block cipher mode!")
		return kResultInvalidArgument;
	}

	__END_CTYPTOPP_CALL__
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CryptoService::AES_decrypt (IStream& _plainData, BlockRef key, BlockRef iv, IStream& _cipherData, BlockCipherMode mode)
{
	CHECK_BLOCK_ARGUMENT (key, kAES_DefaultKeySize)
	CHECK_BLOCK_ARGUMENT (iv, kAES_BlockSize)

	CoreStream plainData (_plainData);
	CoreStream cipherData (_cipherData);
	
	__BEGIN_CRYPTOPP_CALL__

	switch(mode)
	{
	case kBlockCipher_CTR : return CryptoPP::AES_CTR_decrypt (plainData, key.data, key.length, iv.data, cipherData) ? kResultOk : kResultFailed;
	case kBlockCipher_CBC : return CryptoPP::AES_CBC_decrypt (plainData, key.data, key.length, iv.data, cipherData) ? kResultOk : kResultFailed;
	default :
		CCL_NOT_IMPL ("Invalid block cipher mode!")
		return kResultInvalidArgument;
	}

	__END_CTYPTOPP_CALL__
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// RSA
//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CryptoService::RSA_generateKeyPair (IStream& _privateKey, IStream& _publicKey, uint32 keyLength, BlockRef randomData)
{
	CoreStream privateKey (_privateKey);
	CoreStream publicKey (_publicKey);

	__BEGIN_CRYPTOPP_CALL__

	return CryptoPP::RSA_generateKeyPair (privateKey, publicKey, keyLength, randomData.data, randomData.length) ? kResultOk : kResultFailed;

	__END_CTYPTOPP_CALL__
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CryptoService::RSA_encrypt (IStream& _cipherData, IStream& _publicKey, BlockRef randomData, IStream& _plainData)
{
	CoreStream cipherData (_cipherData);
	CoreStream publicKey (_publicKey);
	CoreStream plainData (_plainData);

	__BEGIN_CRYPTOPP_CALL__

	return CryptoPP::RSA_encrypt (cipherData, publicKey, randomData.data, randomData.length, plainData) ? kResultOk : kResultFailed;

	__END_CTYPTOPP_CALL__
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CryptoService::RSA_decrypt (IStream& _plainData, IStream& _privateKey, IStream& _cipherData)
{
	CoreStream plainData (_plainData);
	CoreStream privateKey (_privateKey);
	CoreStream cipherData (_cipherData);

	__BEGIN_CRYPTOPP_CALL__

	return CryptoPP::RSA_decrypt (plainData, privateKey, cipherData) ? kResultOk : kResultFailed;

	__END_CTYPTOPP_CALL__
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CryptoService::RSA_sign (IStream& _signature, IStream& _privateKey, IStream& _data, Hash hash)
{
	CoreStream signature (_signature);
	CoreStream privateKey (_privateKey);
	CoreStream data (_data);

	__BEGIN_CRYPTOPP_CALL__
	switch(hash)
	{
	case kHashSHA1 : return CryptoPP::RSA_SHA1_sign (signature, privateKey, data) ? kResultOk : kResultFalse;
	case kHashSHA256 : return CryptoPP::RSA_SHA256_sign (signature, privateKey, data) ? kResultOk : kResultFalse;
	default :
		return kResultInvalidArgument;
	}
	__END_CTYPTOPP_CALL__
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CryptoService::RSA_verify (IStream& _data, IStream& _publicKey, IStream& _signature, Hash hash)
{
	CoreStream data (_data);
	CoreStream publicKey (_publicKey);
	CoreStream signature (_signature);

	__BEGIN_CRYPTOPP_CALL__
	switch(hash)
	{
	case kHashSHA1 : return CryptoPP::RSA_SHA1_verify (data, publicKey, signature) ? kResultOk : kResultFalse;
	case kHashSHA256 : return CryptoPP::RSA_SHA256_verify (data, publicKey, signature) ? kResultOk : kResultFalse;
	default :
		return kResultInvalidArgument;
	}
	__END_CTYPTOPP_CALL__
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// ASN.1
//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CryptoService::BER_decode (IASN1ContentHandler& _handler, IStream& _encodedData)
{
	CoreStream encodedData (_encodedData);
	CryptoHandler handler (_handler);
	
	__BEGIN_CRYPTOPP_CALL__
	
	return CryptoPP::BER_decode (handler, encodedData) ? kResultOk : kResultFailed;
	
	__END_CTYPTOPP_CALL__
}

//////////////////////////////////////////////////////////////////////////////////////////////////
///
tresult CCL_API CryptoService::DER_encode (IStream& _encodedData, int tag, IStream& _content)
{
	CoreStream encodedData (_encodedData);
	CoreStream content (_content);
	
	__BEGIN_CRYPTOPP_CALL__
	
	return CryptoPP::DER_encode (encodedData, tag, content) ? kResultOk : kResultFailed;
	
	__END_CTYPTOPP_CALL__
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// PKCS7
//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CryptoService::PKCS7_decodeData (IASN1ContentHandler& _handler, IStream& _encodedData)
{
	CoreStream encodedData (_encodedData);
	MemoryStream data;
	CryptoHandler handler (_handler);

	__BEGIN_CRYPTOPP_CALL__

	if(!CryptoPP::PKCS7_getData (data, encodedData))
		return kResultFailed;
	
	return CryptoPP::BER_decode (handler, data) ? kResultOk : kResultFailed;
	
	__END_CTYPTOPP_CALL__
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CryptoService::PKCS7_getCertificates (IStream& _certificates, IStream& _encodedData)
{
	CoreStream certificates (_certificates);
	CoreStream encodedData (_encodedData);
	
	__BEGIN_CRYPTOPP_CALL__
	
	return CryptoPP::PKCS7_getCertificates (certificates, encodedData)  ? kResultOk : kResultFailed;
	
	__END_CTYPTOPP_CALL__
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IInteger* CCL_API CryptoService::Integer_create ()
{
	return NEW CryptoInteger;
}
