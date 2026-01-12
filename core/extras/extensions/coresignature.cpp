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
// Filename    : core/extras/extensions/coresignature.h
// Description : Digital Signature
//
//************************************************************************************************

//#define CORE_PROFILE 1

#include "core/extras/extensions/coresignature.h"

#include "core/public/coremacros.h"
#include "core/system/coredebug.h"

#include "mbedtls/include/mbedtls/pk.h"
#include "mbedtls/include/mbedtls/sha256.h"

using namespace Core::Security;

//************************************************************************************************
// SignatureVerifierRS256
//************************************************************************************************

bool SignatureVerifierRS256::verifySignature (MaterialRef data, MaterialRef publicKey, MaterialRef signature) const
{
	//CORE_PROFILE_START (verifySignature);

	uint8 hash[32] = {0};
	::mbedtls_sha256_ret (data.as<unsigned char> (), data.getSize (), hash, 0);

	mbedtls_pk_context pk;
	::mbedtls_pk_init (&pk);

	unsigned char* begin = const_cast<unsigned char*> (publicKey.as<unsigned char> ());
	const unsigned char* end = begin + publicKey.getSize ();
	int result = ::mbedtls_pk_parse_subpubkey (&begin, end, &pk);
	if(result != 0)
	{
		::mbedtls_pk_free (&pk);
		return false;
	}

	result = ::mbedtls_pk_verify (&pk, MBEDTLS_MD_SHA256, hash, 0, signature.as<unsigned char> (), signature.getSize ());
	if(result != 0)
	{
		::mbedtls_pk_free (&pk);
		return false;
	}

	::mbedtls_pk_free (&pk);

	//CORE_PROFILE_STOP (verifySignature, "signature");
	return true;
};

