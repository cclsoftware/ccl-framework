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
// Filename    : ccl/extras/web/oauth2.h
// Description : OAuth2 (Secure API Authorization Protocol, see oauth.net/2)
//
//************************************************************************************************

#ifndef _ccl_oauth2_h
#define _ccl_oauth2_h

#include "ccl/public/text/cstring.h"
#include "ccl/public/text/cclstring.h"
#include "ccl/public/base/cclmacros.h"

namespace CCL {
namespace Web {

//************************************************************************************************
// OAuth2 Parameters
//************************************************************************************************

namespace OAuth2 
{
	/** OAuth2 parameter definitions. */
	class Parameters
	{
	public:
		DECLARE_STRINGID_MEMBER (kClientID)
		DECLARE_STRINGID_MEMBER (kClientSecret)
		DECLARE_STRINGID_MEMBER (kScope)
		DECLARE_STRINGID_MEMBER (kRedirectURI)
		DECLARE_STRINGID_MEMBER (kResponseType)
		DECLARE_STRINGID_MEMBER (kState)
		DECLARE_STRINGID_MEMBER (kGrantType)
		DECLARE_STRINGID_MEMBER (kCode)
				
		DECLARE_STRINGID_MEMBER (kError)
		DECLARE_STRINGID_MEMBER (kErrorDescription)

		DECLARE_STRINGID_MEMBER (kAuthorizationCode)
		DECLARE_STRINGID_MEMBER (kAccessToken)
		DECLARE_STRINGID_MEMBER (kIDToken)
		DECLARE_STRINGID_MEMBER (kRefreshToken)
		DECLARE_STRINGID_MEMBER (kPassword)
		DECLARE_STRINGID_MEMBER (kClientCredentials)
		DECLARE_STRINGID_MEMBER (kOAuth1Token)
		DECLARE_STRINGID_MEMBER (kExpiresIn)
		
		DECLARE_STRINGID_MEMBER (kCodeVerifier)
		DECLARE_STRINGID_MEMBER (kCodeChallenge)
		DECLARE_STRINGID_MEMBER (kCodeChallengeMethod)
		DECLARE_STRINGID_MEMBER (kCodeChallengeMethodSHA256)

		/** OAuth 2.1 Proof Key for Code Exchange (https://www.rfc-editor.org/rfc/rfc7636). */
		static bool generatePKCE (MutableCString& codeVerifier, MutableCString& codeChallenge);

		static void authenticate (IUrl& url, StringRef clientId, StringRef redirectURI,
								  StringRef responseType = nullptr, StringRef scope = nullptr,
								  StringRef codeChallenge = nullptr);
	};
}

//************************************************************************************************
// OAuth2Tokens
//************************************************************************************************

class OAuth2Tokens
{
public:
	OAuth2Tokens ();

	PROPERTY_MUTABLE_CSTRING (tokenType, TokenType)

	PROPERTY_STRING (idToken, IdToken)
	PROPERTY_STRING (refreshToken, RefreshToken)
	PROPERTY_STRING (accessToken, AccessToken)
	PROPERTY_VARIABLE (CCL::int64, expirationTime, ExpirationTime)

	void reset ();
	bool loadFromJson (IStream& jsonStream, int64 timestamp);
	bool expiresSoon () const;

	bool storeARTokens (StringRef credentialName) const;
	bool restoreARTokens (StringRef credentialName);
	void removeARTokens (StringRef credentialName);

protected:
	static const String kIdentifier;
	static const String kSeparator;
};

} // namespace Web
} // namespace CCL

#endif // _ccl_oauth2_h
