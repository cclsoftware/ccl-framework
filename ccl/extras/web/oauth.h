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
// Filename    : ccl/extras/web/oauth.h
// Description : OAuth (Secure API Authorization Protocol, see oauth.net)
//
//************************************************************************************************

#ifndef _ccl_oauth_h
#define _ccl_oauth_h

#include "ccl/public/text/cstring.h"
#include "ccl/public/text/istringdict.h"

namespace CCL {
namespace Web {

//************************************************************************************************
// OAuth
//************************************************************************************************

namespace OAuth 
{
	/** OAuth parameter definitions. */
	class Parameters
	{
	public:
		DECLARE_STRINGID_MEMBER (kConsumerKey)
		DECLARE_STRINGID_MEMBER (kToken)
		DECLARE_STRINGID_MEMBER (kTokenSecret)
		DECLARE_STRINGID_MEMBER (kNonce)
		DECLARE_STRINGID_MEMBER (kTimestamp)
		DECLARE_STRINGID_MEMBER (kSignatureMethod)
		DECLARE_STRINGID_MEMBER (kVersion)
		DECLARE_STRINGID_MEMBER (kSignature)
		DECLARE_STRINGID_MEMBER (kCallback)
		DECLARE_STRINGID_MEMBER (kVerifier)

		DECLARE_STRINGID_MEMBER (kVersion1_0)

		DECLARE_STRINGID_MEMBER (kHMAC_SHA1)
		DECLARE_STRINGID_MEMBER (kRSA_SHA1)
		DECLARE_STRINGID_MEMBER (kPlaintext)
	};

	/**
		Returns number of seconds since January 1, 1970 00:00.
	*/
	int64 getSecondsSince1970 ();

	/**
		Generate a random string between 15 and 32 chars length.
	*/
	MutableCString generateNonce ();

	/**
		Prepare OAuth parameters (add timestamp, nonce, etc.). 
	*/
	void prepare (ICStringDictionary& parameters, 
				  StringID consumerKey,
				  StringID token, 
				  StringID signatureMethod);

	/**
		Normalize OAuth Request.
		All parameters in dictionary have to be UTF-8 encoded!
	*/
	MutableCString normalize (StringID httpMethod, 
							  UrlRef url, 
							  const ICStringDictionary& parameters);

	/**
		Sign OAuth Request using HMAC-SHA1 method.
		Consumer secret and token secret have to be UTF-8 encoded!
	*/
	MutableCString sign_HMAC_SHA1 (StringID baseString, 
								   StringID consumerSecret, 
								   StringID tokenSecret);
}

} // namespace Web
} // namespace CCL

#endif // _ccl_oauth_h
