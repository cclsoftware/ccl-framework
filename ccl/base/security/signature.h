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
// Filename    : ccl/base/security/signature.h
// Description : Cryptographical Signature
//
//************************************************************************************************

#ifndef _ccl_signature_h
#define _ccl_signature_h

#include "ccl/base/security/cipher.h"

namespace CCL {
namespace Security {
namespace Crypto {

//************************************************************************************************
// Crypto::SignedMessage
//************************************************************************************************

class SignedMessage: public StorableObject
{
public:
	DECLARE_CLASS (SignedMessage, StorableObject)

	/** Signed data. */
	Material& getData ();

	/** Signature value. */
	Material& getSignature ();

	/** Key information. */
	Material& getKeyInfo ();

	void setDataWithObject (const StorableObject& object);
	bool getObjectFromData (StorableObject& object);

protected:
	Material data;
	Material signature;
	Material keyInfo;
};

//************************************************************************************************
// Crypto::SignedXmlMessage
//************************************************************************************************

class SignedXmlMessage: public SignedMessage
{
public:
	DECLARE_CLASS (SignedXmlMessage, SignedMessage)

	SignedXmlMessage (StringID rootName = nullptr);

	/** Name of XML root tag. */
	PROPERTY_MUTABLE_CSTRING (rootName, RootName)

	/** Optional cipher object for symmetric encryption. */
	PROPERTY_SHARED_AUTO (Cipher, cipher, Cipher)

	// SignedMessage
	tbool CCL_API save (IStream& stream) const override;
	tbool CCL_API load (IStream& stream) override;
};

//************************************************************************************************
// Crypto::Signer
//************************************************************************************************

class Signer: public StorableObject
{
public:
	DECLARE_CLASS (Signer, StorableObject)

	Signer (Hash hash = kHashSHA1);

	PROPERTY_VARIABLE (Hash, hash, Hash)
	PROPERTY_OBJECT (Material, privateKey, PrivateKey)
	PROPERTY_OBJECT (Material, keyInfo, KeyInfo)

	bool sign (SignedMessage& message);
	bool sign (IStream& signature, IStream& data);

	// StorableObject
	tbool CCL_API save (IStream& stream) const override;
	tbool CCL_API load (IStream& stream) override;
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;
};

//************************************************************************************************
// Crypto::Verifier
//************************************************************************************************

class Verifier: public StorableObject
{
public:
	DECLARE_CLASS (Verifier, StorableObject)

	Verifier (Hash hash = kHashSHA1);

	PROPERTY_VARIABLE (Hash, hash, Hash)
	PROPERTY_OBJECT (Material, publicKey, PublicKey)

	bool setFromKeyStore (StringID keyName, bool detectKeyEncryption = true);

	bool verify (SignedMessage& message);
	bool verify (IStream& data, IStream& signature);

	// StorableObject
	tbool CCL_API save (IStream& stream) const override;
	tbool CCL_API load (IStream& stream) override;
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;
};

} // namespace Crypto
} // namespace Security
} // namespace CCL

#endif // _ccl_signature_h
