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
// Filename    : ccl/security/cryptoservice.h
// Description : Cryptographical Services
//
//************************************************************************************************

#ifndef _ccl_cryptoservice_h
#define _ccl_cryptoservice_h

#include "ccl/base/object.h"

#include "ccl/public/security/icryptoservice.h"

namespace CCL {
namespace Security {
namespace Crypto {

//************************************************************************************************
// CryptoService
//************************************************************************************************

class CryptoService: public Object,
					 public Internal::ICryptoService
{
public:
	DECLARE_CLASS (CryptoService, Object)

	// ICryptoService
	tresult CCL_API RNG_generate (BlockRef randomData) override;
	tresult CCL_API MD5_calculate (BlockRef digest, IStream& data, IProgressNotify* progress = nullptr) override;
	tresult CCL_API SHA1_calculate (BlockRef digest, IStream& data, IProgressNotify* progress = nullptr) override;
	tresult CCL_API SHA256_calculate (BlockRef digest, IStream& data, IProgressNotify* progress = nullptr) override;
	tresult CCL_API HMAC_SHA1_sign (IStream& signature, BlockRef key, IStream& data) override;
	tresult CCL_API HMAC_SHA256_sign (IStream& signature, BlockRef key, IStream& data) override;
	tresult CCL_API HKDF_deriveKey (IStream& _derivedKey, uint32 derivedKeyLength, BlockRef secret, BlockRef salt, BlockRef info) override;
	tresult CCL_API AES_encrypt (IStream& cipherData, BlockRef key, BlockRef iv, IStream& plainData, BlockCipherMode mode) override;
	tresult CCL_API AES_decrypt (IStream& plainData, BlockRef key, BlockRef iv, IStream& cipherData, BlockCipherMode mode) override;
	tresult CCL_API RSA_generateKeyPair (IStream& privateKey, IStream& publicKey, uint32 keyLength, BlockRef randomData) override;
	tresult CCL_API RSA_encrypt (IStream& cipherData, IStream& publicKey, BlockRef randomData, IStream& plainData) override;
	tresult CCL_API RSA_decrypt (IStream& plainData, IStream& privateKey, IStream& cipherData) override;
	tresult CCL_API RSA_sign (IStream& signature, IStream& privateKey, IStream& data, Hash hash) override;
	tresult CCL_API RSA_verify (IStream& data, IStream& publicKey, IStream& signature, Hash hash) override;
	tresult CCL_API BER_decode (IASN1ContentHandler& handler, IStream& encodedData) override;
	tresult CCL_API DER_encode (IStream& encodedData, int tag, IStream& content) override;
	tresult CCL_API PKCS7_decodeData (IASN1ContentHandler& handler, IStream& encodedData) override;
	tresult CCL_API PKCS7_getCertificates (IStream& certificates, IStream& encodedData) override;
	IInteger* CCL_API Integer_create () override;
	
	CLASS_INTERFACE (ICryptoService, Object)

protected:
	template<class Hash> tresult calculateDigest (BlockRef digest, IStream& data, IProgressNotify* progress);
};

} // namespace Crypto
} // namespace Security
} // namespace CCL

#endif // _ccl_cryptoservice_h
