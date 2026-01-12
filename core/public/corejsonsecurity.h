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
// Filename    : core/public/corejsonsecurity.h
// Description : JOSE (Javascript Object Signing and Encryption) Definitions
//
//************************************************************************************************

#ifndef _corejsonsecurity_h
#define _corejsonsecurity_h

#include "core/public/coretypes.h"

namespace Core {
namespace Security {

//////////////////////////////////////////////////////////////////////////////////////////////////
// JOSE (Javascript Object Signing and Encryption) Definitions
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace JOSE
{
	const CStringPtr kAlgorithm = "alg";

	enum Algorithm
	{
		kUnknownAlgorithm = -1,
		kDirect,
		kRS256
	};

	inline Algorithm getAlgorithm (CStringPtr algorithmName)
	{
		if(::strcmp (algorithmName, "dir") == 0)
			return kDirect;
		else if(::strcmp (algorithmName, "RS256") == 0)
			return kRS256;
		else
			return kUnknownAlgorithm;
	}

	inline CStringPtr getAlgorithmName (Algorithm algorithm)
	{
		switch(algorithm)
		{
		case kDirect : return "dir";
		case kRS256 : return "RS256";
		}
		return nullptr;
	}

	const CStringPtr kEncryption = "enc";

	enum Encryption
	{
		kUnknownEncryption = -1,
		kAES_128_CBC_HMAC_SHA_256, ///< https://tools.ietf.org/html/rfc7518#section-5.2.3
		kAES_128_GCM ///< AES using 128-bit key (GCM = Galois/Counter Mode)
	};

	inline Encryption getEncryption (CStringPtr encryptionName)
	{
		if(::strcmp (encryptionName, "A128CBC-HS256") == 0)
			return kAES_128_CBC_HMAC_SHA_256;
		else if(::strcmp (encryptionName, "A128GCM") == 0)
			return kAES_128_GCM;
		else
			return kUnknownEncryption;
	}

	inline CStringPtr getEncryptionName (Encryption encryption)
	{
		switch(encryption)
		{
		case kAES_128_CBC_HMAC_SHA_256 : return "A128CBC-HS256";
		case kAES_128_GCM : return "A128GCM";
		}
		return nullptr;
	}

	const CStringPtr kType = "typ";
	const CStringPtr kJWT = "JWT";

	inline bool isJWT (CStringPtr type)
	{
		return ::strcmp (type, kJWT) == 0;
	}

	const CStringPtr kKeyID = "kid";
	const CStringPtr kKey = "key";
	const CStringPtr kKeyType = "kty";

	const CStringPtr kSubject = "sub";
	const CStringPtr kAudience = "aud";

	const CStringPtr kIssuedAt = "iat";
	const CStringPtr kNotBefore = "nbf";
	const CStringPtr kExpirationTime = "exp";
}

} // namespace Security
} // namespace Core

#endif // _corejsonsecurity_h
