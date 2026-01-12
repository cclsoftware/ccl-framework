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
// Filename    : ccl/public/system/cryptotypes.h
// Description : Basic cryptographical data types
//
//************************************************************************************************

#ifndef _ccl_cryptotypes_h
#define _ccl_cryptotypes_h

#include "ccl/public/base/platform.h"

namespace CCL {
namespace Security {
namespace Crypto {

//************************************************************************************************
// Crypto::Algorithm
//************************************************************************************************

DEFINE_ENUM (Algorithm)
{
	kAlgorithmAES
};

//************************************************************************************************
// Crypto::BlockCipherMode
//************************************************************************************************

DEFINE_ENUM (BlockCipherMode)
{
	kBlockCipher_CTR,		///< Counter
	kBlockCipher_CBC		///< Cipher Block Chaining (input < blocksize)
};

//************************************************************************************************
// Crypto::Hash
//************************************************************************************************

DEFINE_ENUM (Hash)
{
	kHashSHA1,
	kHashSHA256
};

//************************************************************************************************
// Definitions
//************************************************************************************************

enum Definitions
{
	kMD5_DigestSize = 16,			///< [MD5] 128 Bit Digest
	kSHA1_DigestSize = 20,			///< [SHA1] 160 Bit Digest
	kSHA256_DigestSize = 32,		///< [SHA256] 256 Bit Digest

	kAES_DefaultKeySize = 16,		///< [AES] Default Key Size (128 Bit)
	kAES_BlockSize = 16,			///< [AES] 128 Bit Block Size

	kRSA_DefaultKeyLength = 2048	///< [RSA] Default Key Length (2048 Bit)
};

//************************************************************************************************
// Crypto::Block
//************************************************************************************************

struct Block
{
	uint8* data;
	uint32 length;

	Block (const void* data = nullptr, uint32 length = 0)
	: data ((uint8*)data),
	  length (length)
	{}
};

typedef const Block& BlockRef;

} // namespace Crypto
} // namespace Security
} // namespace CCL

#endif // _ccl_cryptotypes_h
