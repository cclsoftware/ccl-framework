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
// Filename    : ccl/public/security/icryptoservice.h
// Description : Cryptographical Service Interface
//
//************************************************************************************************

#ifndef _ccl_icryptoservice_h
#define _ccl_icryptoservice_h

#include "ccl/public/cclexports.h"
#include "ccl/public/base/iunknown.h"

#include "ccl/public/system/cryptotypes.h"

namespace CCL {

interface IStream;
interface IProgressNotify;

namespace Security {
namespace Crypto {
	
interface IASN1ContentHandler;
interface IInteger;

namespace Internal {

//************************************************************************************************
// Crypto::Internal::ICryptoService
/** Helper methods for crypto class implementation. Do not use this interface directly. 
	\ingroup base_io */
//************************************************************************************************

interface ICryptoService: IUnknown
{
	//////////////////////////////////////////////////////////////////////////////////////////////
	// Random Number Generation
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Generate random data. */
	virtual tresult CCL_API RNG_generate
							(BlockRef randomData) = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Hash Algorithms
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** [MD5] Calculate digest from data stream - DEPRECATED. */
	virtual tresult CCL_API MD5_calculate 
							(BlockRef digest, IStream& data, IProgressNotify* progress = nullptr) = 0;

	/** [SHA1] Calculate digest from data stream. */
	virtual tresult CCL_API SHA1_calculate 
							(BlockRef digest, IStream& data, IProgressNotify* progress = nullptr) = 0;

	/** [SHA256] Calculate digest from data stream. */
	virtual tresult CCL_API SHA256_calculate 
							(BlockRef digest, IStream& data, IProgressNotify* progress = nullptr) = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// (Keyed-Hash) Message Authentication Code
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** [HMAC-SHA1] Sign data. */
	virtual tresult CCL_API HMAC_SHA1_sign
							(IStream& signature, BlockRef key, IStream& data) = 0;

	/** [HMAC-SHA256] Sign data. */
	virtual tresult CCL_API HMAC_SHA256_sign
							(IStream& signature, BlockRef key, IStream& data) = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////////
	// (HKDF) Key Function Derivation
	//////////////////////////////////////////////////////////////////////////////////////////////////

	virtual tresult CCL_API HKDF_deriveKey
 							(IStream& _derivedKey, uint32 derivedKeyLength, BlockRef secret, BlockRef salt, BlockRef info) = 0;
	
	//////////////////////////////////////////////////////////////////////////////////////////////
	// Symmetric Encryption Algorithms
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** [AES] Encrypt data. */
	virtual tresult CCL_API AES_encrypt 
							(IStream& cipherData, BlockRef key, BlockRef iv, IStream& plainData, BlockCipherMode mode) = 0;

	/** [AES] Decrypt data. */
	virtual tresult CCL_API AES_decrypt 
							(IStream& plainData, BlockRef key, BlockRef iv, IStream& cipherData, BlockCipherMode mode) = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Public Key Algorithms
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** [RSA] Generate private/public key pair. */
	virtual tresult CCL_API RSA_generateKeyPair 
							(IStream& privateKey, IStream& publicKey, uint32 keyLength, BlockRef randomData) = 0;

	/** [RSA] Encrypt data. */
	virtual tresult CCL_API RSA_encrypt
							(IStream& cipherData, IStream& publicKey, BlockRef randomData, IStream& plainData) = 0;

	/** [RSA] Decrypt data. */
	virtual tresult CCL_API RSA_decrypt 
							(IStream& plainData, IStream& privateKey, IStream& cipherData) = 0;
		
	/** [RSA] Sign data. */
	virtual tresult CCL_API RSA_sign 
							(IStream& signature, IStream& privateKey, IStream& data, Hash hash) = 0;
		
	/** [RSA] Verify data. */
	virtual tresult CCL_API RSA_verify 
							(IStream& data, IStream& publicKey, IStream& signature, Hash hash) = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// ASN.1
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Parse ASN.1 encoded data (Basic Encoding Rules) */
	virtual tresult CCL_API BER_decode (IASN1ContentHandler& reader, IStream& encodedData) = 0;

	/** Write ASN.1 encoded data (Distinguished Encoding Rules) */
	virtual tresult CCL_API DER_encode (IStream& encodedData, int tag, IStream& content) = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// PKCS #7 Digital Envelopes
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Parse the ASN.1 payload from a PKCS #7 container */
	virtual tresult CCL_API PKCS7_decodeData (IASN1ContentHandler& reader, IStream& encodedData) = 0;

	/** Get an X.509 certificate (chain) from a PKCS #7 container */
	virtual tresult CCL_API PKCS7_getCertificates (IStream& certficates, IStream& encodedData) = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Integer
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Create new integer instance. */
	virtual IInteger* CCL_API Integer_create () = 0;

	DECLARE_IID (ICryptoService)
};

DEFINE_IID (ICryptoService, 0xb7ec4a26, 0x344c, 0x479d, 0xb3, 0xdd, 0xb9, 0x82, 0xd, 0x6d, 0x9, 0x3a)

} // namespace Internal
} // namespace Crypto
} // namespace Security

namespace System {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Security Service APIs
//////////////////////////////////////////////////////////////////////////////////////////////////

/** Get cryptographical service singleton (internal). */
CCL_EXPORT Security::Crypto::Internal::ICryptoService& CCL_API CCL_ISOLATED (GetCryptoService) ();
inline Security::Crypto::Internal::ICryptoService& GetCryptoService () { return CCL_ISOLATED (GetCryptoService) (); }

} // namespace System
} // namespace CCL

#endif // _ccl_icryptoservice_h
