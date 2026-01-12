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
// Filename    : ccl/security/cryptokeystore.h
// Description : Cryptographical Key Store
//
//************************************************************************************************

#ifndef _ccl_cryptokeystore_h
#define _ccl_cryptokeystore_h

#include "ccl/base/security/cryptomaterial.h"
#include "ccl/base/collections/objectarray.h"

#include "ccl/public/security/icryptokeystore.h"

namespace CCL {
namespace Security {
namespace Crypto {

//************************************************************************************************
// CryptoKeyStore
//************************************************************************************************

class CryptoKeyStore: public Object,
					  public ICryptoKeyStore
{
public:
	DECLARE_CLASS (CryptoKeyStore, Object)

	CryptoKeyStore ();

	static CryptoKeyStore& instance ();

	// ICryptoKeyStore
	tresult CCL_API addMaterial (StringID name, MaterialType type, IStream& data) override;
	tresult CCL_API addMaterial (StringID name, MaterialType type, const void* data, uint32 length) override;
	tresult CCL_API getMaterial (IStream& data, StringID name, MaterialType type) const override;
	tresult CCL_API removeMaterial (StringID name, MaterialType type) override;
	tresult CCL_API removeMaterial (StringID name) override;

	CLASS_INTERFACE (ICryptoKeyStore, Object)

protected:
	ObjectArray entries;

	class Entry: public Material
	{
	public:
		Entry (): hashValue (0), type (kRandomData) {}

		PROPERTY_VARIABLE (uint32, hashValue, HashValue)
		PROPERTY_VARIABLE (MaterialType, type, Type)
	};

	Entry* lookup (StringID name, MaterialType type) const;
};

} // namespace Crypto
} // namespace Security
} // namespace CCL

#endif // _ccl_cryptokeystore_h
