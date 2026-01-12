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
// Filename    : ccl/base/security/jsonwebsecurity.h
// Description : JSON Web Encryption (JWE), JSON Web Signature (JWS), JSON Web Token (JWT)
//
//************************************************************************************************

#ifndef _jsonwebsecurity_h
#define _jsonwebsecurity_h

#include "ccl/base/storage/attributes.h"
#include "ccl/base/storage/jsonarchive.h"
#include "ccl/base/security/signature.h"
#include "ccl/base/security/cryptobox.h"

#include "ccl/public/base/buffer.h"
#include "ccl/public/base/streamer.h"

#include "core/public/corejsonsecurity.h"

namespace CCL {
namespace Security {

namespace JOSE
{
	// import shared definitions
	using namespace Core::Security::JOSE;
}

//************************************************************************************************
// JWProtectedObject
/** Base class for JWE and JWS. */
//************************************************************************************************

class JWProtectedObject
{
public:
	PROPERTY_OBJECT (CCL::Attributes, protectedHeader, ProtectedHeader)
	CCL::Attributes& getProtectedHeader () { return protectedHeader; }

	JOSE::Algorithm getAlgorithm () const
	{
		return JOSE::getAlgorithm (protectedHeader.getCString (JOSE::kAlgorithm));
	}

	void setAlgorithm (JOSE::Algorithm algorithm)
	{
		protectedHeader.set (JOSE::kAlgorithm, JOSE::getAlgorithmName (algorithm));
	}

	bool isKnownAlgorithm () const
	{
		return getAlgorithm () != JOSE::kUnknownAlgorithm;
	}

	bool isDirect () const
	{
		return getAlgorithm () == JOSE::kDirect;
	}

	MutableCString getKeyID () const
	{
		return protectedHeader.getCString (JOSE::kKeyID);
	}

	void setKeyID (StringID keyId)
	{
		protectedHeader.set (JOSE::kKeyID, keyId);
	}

	MutableCString getType () const
	{
		return protectedHeader.getCString (JOSE::kType);
	}

	void setType (StringID type)
	{
		protectedHeader.set (JOSE::kType, type);
	}

	bool isJWT () const
	{
		return JOSE::isJWT (getType ());
	}

protected:
	String serializeProtectedHeader () const
	{
		Crypto::Material serializedHeader;
		JsonArchive (serializedHeader.asStream ()).saveAttributes (nullptr, protectedHeader);
		return serializedHeader.toBase64URL ();
	}
};

//************************************************************************************************
// JWEObject
/** JSON Web Encryption (JWE) - https://tools.ietf.org/html/rfc7516. */
//************************************************************************************************

class JWEObject: public JWProtectedObject
{
public:
	PROPERTY_OBJECT (Crypto::Material, encryptedKey, EncryptedKey)
	PROPERTY_OBJECT (Crypto::Material, initializationVector, InitializationVector)
	PROPERTY_OBJECT (Crypto::Material, ciphertext, Ciphertext)
	PROPERTY_OBJECT (Crypto::Material, authenticationTag, AuthenticationTag)
	PROPERTY_OBJECT (Crypto::Material, additionalAuthenticatedData, AdditionalAuthenticatedData) // BASE64URL(UTF8(JWE Protected Header))

	JOSE::Encryption getEncryption () const
	{
		return JOSE::getEncryption (protectedHeader.getCString (JOSE::kEncryption));
	}

	void setEncryption (JOSE::Encryption encryption)
	{
		protectedHeader.set (JOSE::kEncryption, JOSE::getEncryptionName (encryption));
	}

	bool isKnownEncryption () const
	{
		return getEncryption () != JOSE::kUnknownEncryption;
	}

	JWEObject& fromCompactSerialization (StringRef compactSerialization)
	{
		/*
			BASE64URL(UTF8(JWE Protected Header)) || '.' ||
			BASE64URL(JWE Encrypted Key) || '.' ||
			BASE64URL(JWE Initialization Vector) || '.' ||
			BASE64URL(JWE Ciphertext) || '.' ||
			BASE64URL(JWE Authentication Tag)
		*/

		int counter = 0;
		ForEachStringTokenWithFlags (compactSerialization, CCLSTR ("."), part, Text::kPreserveEmptyToken)
			switch(counter)
			{
			case 0 : additionalAuthenticatedData.append (part, Text::kUTF8);
					 JsonArchive (Crypto::Material ().fromBase64URL (part).asStream ()).loadAttributes (nullptr, protectedHeader); break;
			case 1 : encryptedKey.fromBase64URL (part); break;
			case 2 : initializationVector.fromBase64URL (part); break;
			case 3 : ciphertext.fromBase64URL (part); break;
			case 4 : authenticationTag.fromBase64URL (part); break;
			}
			counter++;
		EndFor
		return *this;
	}

	String toCompactSerialization () const
	{
		String result;
		result << serializeProtectedHeader () << ".";
		result << encryptedKey.toBase64URL () << ".";
		result << initializationVector.toBase64URL () << ".";
		result << ciphertext.toBase64URL () << ".";
		result << authenticationTag.toBase64URL ();
		return result;
	}

	bool decrypt (Crypto::Material& plainText, const Crypto::Material& key)
	{
		switch(getEncryption ())
		{
		case JOSE::kAES_128_CBC_HMAC_SHA_256 :
			{
				ASSERT (key.getBitCount () == 256)

				// compare tags
				Crypto::Material tag;
				calculateTag (tag, key);
				if(!tag.equals (authenticationTag))
					return false;
								                 
				// second half of 256 bit key is used as AES encryption key
				int subKeySize = key.getSize () / 2;
				Crypto::Material encKey;
				encKey.copyPart (key, subKeySize, subKeySize);

				// decrypt
				Crypto::Cipher cipher;
				cipher.setAlgorithm (Crypto::kAlgorithmAES);
				cipher.setMode (Crypto::kBlockCipher_CBC);
				cipher.setInitialVector (initializationVector);
				cipher.setSecretKey (encKey);
				return cipher.decrypt (plainText, ciphertext);
			}

		case JOSE::kAES_128_GCM :
			{
				ASSERT (key.getBitCount () == 128)

				Crypto::Cipher cipher;
				cipher.setAlgorithm (Crypto::kAlgorithmAES);
				cipher.setMode (Crypto::kBlockCipher_CTR); // GCM = Galois/Counter Mode
				cipher.setInitialVector (initializationVector);
				cipher.setSecretKey (key);
				return cipher.decrypt (plainText, ciphertext);
			}

		default :
			CCL_NOT_IMPL ("Encryption not implemented!\n")
			break;
		}
		return false;
	}

	bool encrypt (const Crypto::Material& plainText, const Crypto::Material& key)
	{
		switch(getEncryption ())
		{
		case JOSE::kAES_128_CBC_HMAC_SHA_256 :
			{
				ASSERT (key.getBitCount () == 256)
								                 
				// second half of 256 bit key is used as AES encryption key
				int subKeySize = key.getSize () / 2;
				Crypto::Material encKey;
				encKey.copyPart (key, subKeySize, subKeySize);

				// encrypt
				Crypto::Cipher cipher;
				cipher.setAlgorithm (Crypto::kAlgorithmAES);
				cipher.setMode (Crypto::kBlockCipher_CBC);
				cipher.setInitialVector (initializationVector);
				cipher.setSecretKey (encKey);
				if(!cipher.encrypt (ciphertext, const_cast<Crypto::Material&> (plainText).asStream ()))
					return false;

				// update additional data
				additionalAuthenticatedData.empty ();
				additionalAuthenticatedData.append (serializeProtectedHeader (), Text::kASCII);

				// calculate tag
				authenticationTag.empty ();
				return calculateTag (authenticationTag, key);
			}

		case JOSE::kAES_128_GCM :
			{
				ASSERT (key.getBitCount () == 128)

				Crypto::Cipher cipher;
				cipher.setAlgorithm (Crypto::kAlgorithmAES);
				cipher.setMode (Crypto::kBlockCipher_CTR); // GCM = Galois/Counter Mode
				cipher.setInitialVector (initializationVector);
				cipher.setSecretKey (key);
				return cipher.encrypt (ciphertext, const_cast<Crypto::Material&> (plainText).asStream ());
			}

		default :
			CCL_NOT_IMPL ("Encryption not implemented!\n")
			break;
		}
		return false;
	}

protected:
	bool calculateTag (Crypto::Material& tag, const Crypto::Material& key) const
	{
		ASSERT (getEncryption () == JOSE::kAES_128_CBC_HMAC_SHA_256)
		ASSERT (!additionalAuthenticatedData.isEmpty ())
		ASSERT (key.getBitCount () == 256)

		// first half of 256 bit key is used as HMAC authentication key
		int subKeySize = key.getSize () / 2;
		Security::Crypto::Material hmacKey;
		hmacKey.copyPart (key, 0, subKeySize);
				
		// calculate tag				
		Crypto::Material hmacInput;
		hmacInput.append (additionalAuthenticatedData);
		hmacInput.append (initializationVector);
		hmacInput.append (ciphertext);

		Crypto::Material uint64Buffer;
		Streamer (uint64Buffer.asStream (), kBigEndian).write ((uint64)additionalAuthenticatedData.getBitCount ());
		hmacInput.append (uint64Buffer);
				
		Crypto::Material signedData;
		if(!Crypto::HMAC_SHA256::sign (signedData.asStream (), hmacKey, hmacInput))
			return false;

		tag.copyPart (signedData, 0, subKeySize);
		return true;
	}
};

//************************************************************************************************
// JWSObject
/** JSON Web Signature (JWS) - https://tools.ietf.org/html/rfc7515. */
//************************************************************************************************

class JWSObject: public JWProtectedObject
{
public:
	PROPERTY_OBJECT (Crypto::Material, payload, Payload)
	PROPERTY_OBJECT (Crypto::Material, signingInput, SigningInput) // BASE64URL(UTF8(JWS Protected Header)) || '.' || BASE64URL(JWS Payload)
	PROPERTY_OBJECT (Crypto::Material, signature, Signature)

	JWSObject& fromCompactSerialization (StringRef compactSerialization)
	{
		/*
			BASE64URL(UTF8(JWS Protected Header)) || '.' ||
			BASE64URL(JWS Payload) || '.' ||
			BASE64URL(JWS Signature)
		*/

		int counter = 0;
		signingInput.empty ();
		ForEachStringTokenWithFlags (compactSerialization, CCLSTR ("."), part, Text::kPreserveEmptyToken)
			switch(counter)
			{
			case 0 : 
				signingInput.append (part, Text::kASCII);
				JsonArchive (Crypto::Material ().fromBase64URL (part).asStream ()).loadAttributes (nullptr, protectedHeader); 
				break;

			case 1 : 
				signingInput.append (".");
				signingInput.append (part, Text::kASCII);
				payload.fromBase64URL (part); 
				break;

			case 2 : signature.fromBase64URL (part); break;
			}
			counter++;
		EndFor
		return *this;
	}

	String toCompactSerialization () const
	{
		String result;
		result << serializeProtectedHeader () << ".";
		result << payload.toBase64URL () << ".";
		result << signature.toBase64URL ();
		return result;
	}

	INLINE bool verify (const Crypto::Material& publicKey)
	{
		switch(getAlgorithm ())
		{
		case JOSE::kRS256 :
			{
				Crypto::Verifier verifier (Crypto::kHashSHA256);
				verifier.setPublicKey (publicKey);
				return verifier.verify (signingInput, signature);
			}
		}
		return false;
	}

	INLINE bool sign (const Crypto::Material& privateKey)
	{
		switch(getAlgorithm ())
		{
		case JOSE::kRS256 :
			{
				// update signing input
				signingInput.empty ();
				signingInput.append (serializeProtectedHeader (), Text::kASCII);
				signingInput.append (".");
				signingInput.append (payload.toBase64URL (), Text::kASCII);

				// sign
				Crypto::Signer signer (Crypto::kHashSHA256);
				signer.setPrivateKey (privateKey);
				signature.empty ();
				return signer.sign (signature, signingInput.asStream ());
			}
		}
		return false;
	}
};

//************************************************************************************************
// JWTObject
/** JSON Web Token (JWT) - https://tools.ietf.org/html/rfc7519. */
//************************************************************************************************

class JWTObject
{
public:
	PROPERTY_OBJECT (CCL::Attributes, claims, Claims)
	CCL::Attributes& getClaims () { return claims; }
	
	JWTObject& fromSignature (const JWSObject& jws)
	{
		JsonArchive (const_cast<Crypto::Material&> (jws.getPayload ()).asStream ()).loadAttributes (nullptr, claims);
		return *this;
	}

	void toSignature (JWSObject& jws) const
	{
		Crypto::Material m;
		JsonArchive (m.asStream ()).saveAttributes (nullptr, claims);
		jws.setPayload (m);
	}

	JWTObject& fromAttributes (const CCL::Attributes& other)
	{
		claims.copyFrom (other);
		return *this;
	}

	String getSubject () const
	{
		return claims.getString (JOSE::kSubject);
	}

	void setSubject (StringRef subject)
	{
		claims.set (JOSE::kSubject, subject);
	}

	String getAudience () const
	{
		return claims.getString (JOSE::kAudience);
	}

	void setAudience (StringRef audience)
	{
		claims.set (JOSE::kAudience, audience);
	}

	bool getIssuedAt (int64& time) const
	{
		return getTimestamp (time, JOSE::kIssuedAt);
	}

	void setIssuedAt (int64 time)
	{
		claims.set (JOSE::kIssuedAt, time);
	}

	bool getNotBefore (int64& time) const
	{
		return getTimestamp (time, JOSE::kNotBefore);
	}

	void setNotBefore (int64 time)
	{
		claims.set (JOSE::kNotBefore, time);
	}

	bool getExpirationTime (int64& time) const
	{
		return getTimestamp (time, JOSE::kExpirationTime);
	}

	void setExpirationTime (int64 time)
	{
		claims.set (JOSE::kExpirationTime, time);
	}

protected:
	bool getTimestamp (int64& time, CStringPtr id) const
	{
		Variant value;
		if(claims.getAttribute (value, id) && value.isInt ())
		{
			time = value.asLargeInt ();
			return true;
		}
		return false;
	}
};

} // namespace Security
} // namespace CCL

#endif // _jsonwebsecurity_h
