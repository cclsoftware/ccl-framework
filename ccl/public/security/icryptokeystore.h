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
// Filename    : ccl/public/security/icryptokeystore.h
// Description : Cryptographical Key Store Interface
//
//************************************************************************************************

#ifndef _ccl_icryptokeystore_h
#define _ccl_icryptokeystore_h

#include "ccl/public/base/iunknown.h"

namespace CCL {
interface IStream;

namespace Security {
namespace Crypto {

//************************************************************************************************
// Crypto::MaterialType
//************************************************************************************************

/** Type of cryptographical material. */
DEFINE_ENUM (MaterialType)
{
	kRandomData,		///< material describes random data
	kInitialVector,		///< material describes an initialization vector
	kSecretKey,			///< material describes a secret key of a symmetric cipher
	kPublicKey,			///< material describes the public key of a pair
	kPrivateKey			///< material describes the private key of a pair
};

//************************************************************************************************
// Crypto::ICryptoKeyStore
//************************************************************************************************

interface ICryptoKeyStore: IUnknown
{
	/** Add material. */
	virtual tresult CCL_API addMaterial (StringID name, MaterialType type, IStream& data) = 0;

	/** Add material. */
	virtual tresult CCL_API addMaterial (StringID name, MaterialType type, const void* data, uint32 length) = 0;

	/** Get material. */
	virtual tresult CCL_API getMaterial (IStream& data, StringID name, MaterialType type) const = 0;

	/** Remove material. */
	virtual tresult CCL_API removeMaterial (StringID name, MaterialType type) = 0;

	/** Remove material. */
	virtual tresult CCL_API removeMaterial (StringID name) = 0;

	DECLARE_IID (ICryptoKeyStore)
};

DEFINE_IID (ICryptoKeyStore, 0x4b2f9520, 0x852d, 0x4ca1, 0xa4, 0xc2, 0xee, 0x1d, 0xfd, 0x87, 0x87, 0x62)

} // namespace Crypto
} // namespace Security
} // namespace CCL

#endif // _ccl_icryptokeystore_h
