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
// Filename    : ccl/base/security/cipher.h
// Description : Cipher
//
//************************************************************************************************

#ifndef _ccl_cipher_h
#define _ccl_cipher_h

#include "ccl/base/security/cryptomaterial.h"

namespace CCL {
namespace Security {
namespace Crypto {

//************************************************************************************************
// Crypto::Cipher
//************************************************************************************************

class Cipher: public StorableObject
{
public:
	DECLARE_CLASS (Cipher, StorableObject)

	Cipher (Algorithm algorithm = kAlgorithmAES,
			BlockCipherMode mode = kBlockCipher_CTR);

	PROPERTY_VARIABLE (Algorithm, algorithm, Algorithm)
	PROPERTY_VARIABLE (BlockCipherMode, mode, Mode)

	PROPERTY_OBJECT (Material, initialVector, InitialVector)
	PROPERTY_OBJECT (Material, secretKey, SecretKey)

	bool setFromKeyStore (StringID keyName);

	bool encrypt (IStream& cipherData, IStream& plainData);
	bool decrypt (IStream& plainData, IStream& cipherData);

	// StorableObject
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;
};

} // namespace Crypto
} // namespace Security
} // namespace CCL

#endif // _ccl_cipher_h
