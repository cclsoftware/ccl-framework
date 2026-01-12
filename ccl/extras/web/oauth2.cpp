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
// Filename    : ccl/extras/web/oauth2.cpp
// Description : OAuth2 (Secure API Authorization Protocol, see oauth.net/2)
//
//************************************************************************************************

#include "ccl/extras/web/oauth2.h"

#include "ccl/base/storage/attributes.h"
#include "ccl/base/storage/jsonarchive.h"

#include "ccl/base/security/cryptobox.h"
#include "ccl/base/security/cryptomaterial.h"

#include "ccl/public/base/datetime.h"

#include "ccl/public/storage/iurl.h"
#include "ccl/public/text/istringdict.h"
#include "ccl/public/securityservices.h"

using namespace CCL;
using namespace Web;

//************************************************************************************************
// OAuth2::Parameters
//************************************************************************************************

DEFINE_STRINGID_MEMBER_ (OAuth2::Parameters, kClientID, "client_id")
DEFINE_STRINGID_MEMBER_ (OAuth2::Parameters, kClientSecret, "client_secret")
DEFINE_STRINGID_MEMBER_ (OAuth2::Parameters, kScope, "scope")
DEFINE_STRINGID_MEMBER_ (OAuth2::Parameters, kRedirectURI, "redirect_uri")
DEFINE_STRINGID_MEMBER_ (OAuth2::Parameters, kResponseType, "response_type")
DEFINE_STRINGID_MEMBER_ (OAuth2::Parameters, kState, "state")
DEFINE_STRINGID_MEMBER_ (OAuth2::Parameters, kGrantType, "grant_type")
DEFINE_STRINGID_MEMBER_ (OAuth2::Parameters, kCode, "code")

DEFINE_STRINGID_MEMBER_ (OAuth2::Parameters, kError, "error")
DEFINE_STRINGID_MEMBER_ (OAuth2::Parameters, kErrorDescription, "error_description")

DEFINE_STRINGID_MEMBER_ (OAuth2::Parameters, kAuthorizationCode, "authorization_code")
DEFINE_STRINGID_MEMBER_ (OAuth2::Parameters, kAccessToken, "access_token")
DEFINE_STRINGID_MEMBER_ (OAuth2::Parameters, kIDToken, "id_token")
DEFINE_STRINGID_MEMBER_ (OAuth2::Parameters, kRefreshToken, "refresh_token")
DEFINE_STRINGID_MEMBER_ (OAuth2::Parameters, kPassword, "password")
DEFINE_STRINGID_MEMBER_ (OAuth2::Parameters, kClientCredentials, "client_credentials")
DEFINE_STRINGID_MEMBER_ (OAuth2::Parameters, kOAuth1Token, "oauth1_token")
DEFINE_STRINGID_MEMBER_ (OAuth2::Parameters, kExpiresIn, "expires_in")

// OAuth 2.1 PKCE
DEFINE_STRINGID_MEMBER_ (OAuth2::Parameters, kCodeVerifier, "code_verifier")
DEFINE_STRINGID_MEMBER_ (OAuth2::Parameters, kCodeChallenge, "code_challenge")
DEFINE_STRINGID_MEMBER_ (OAuth2::Parameters, kCodeChallengeMethod, "code_challenge_method")
DEFINE_STRINGID_MEMBER_ (OAuth2::Parameters, kCodeChallengeMethodSHA256, "S256")

//////////////////////////////////////////////////////////////////////////////////////////////////

bool OAuth2::Parameters::generatePKCE (MutableCString& codeVerifier, MutableCString& codeChallenge)
{
	constexpr int kCodeVerifierLength = 32;
	Security::Crypto::Material codeVerifierMaterial (kCodeVerifierLength);
	if(!Security::Crypto::RandomPool::generate (codeVerifierMaterial))
		return false; // code verifier must be reliable random value

	codeVerifier = codeVerifierMaterial.toCBase64URL ();

	Security::Crypto::Material codeChallengeMaterial;
	codeChallengeMaterial.copyFrom (codeVerifier);
	Security::Crypto::Material codeChallengeHash (Security::Crypto::SHA256::kDigestSize);
	Security::Crypto::SHA256::calculate (codeChallengeHash, codeChallengeMaterial);
	codeChallenge = codeChallengeHash.toCBase64URL ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OAuth2::Parameters::authenticate (IUrl& url, StringRef clientId, StringRef redirectURI, StringRef responseType, StringRef scope, StringRef codeChallenge)
{
	url.getParameters ().setEntry (String (kClientID), clientId);
	url.getParameters ().setEntry (String (kRedirectURI), redirectURI);
	if(!responseType.isEmpty ())
		url.getParameters ().setEntry (String (kResponseType), responseType);
	if(!scope.isEmpty ())
		url.getParameters ().setEntry (String (kScope), scope);
	if(!codeChallenge.isEmpty ())
	{
		url.getParameters ().setEntry (String (kCodeChallenge), codeChallenge);
		url.getParameters ().setEntry (String (kCodeChallengeMethod), String (kCodeChallengeMethodSHA256));
	}
}

//************************************************************************************************
// OAuth2Tokens
//************************************************************************************************

const String OAuth2Tokens::kIdentifier ("OAuth2Tokens");
const String OAuth2Tokens::kSeparator ("&");

//////////////////////////////////////////////////////////////////////////////////////////////////

OAuth2Tokens::OAuth2Tokens ()
: expirationTime (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OAuth2Tokens::reset ()
{
	setIdToken (String::kEmpty);
	setRefreshToken (String::kEmpty);
	setAccessToken (String::kEmpty);
	setExpirationTime (0);
	setTokenType (CString::kEmpty);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool OAuth2Tokens::loadFromJson (IStream& jsonStream, int64 timestamp)
{
	Attributes a;
	if(!JsonUtils::parse (a, jsonStream))
		return false;

	setRefreshToken (a.getString (OAuth2::Parameters::kRefreshToken));			
	setIdToken (a.getString (OAuth2::Parameters::kIDToken));
	setAccessToken (a.getString (OAuth2::Parameters::kAccessToken));

	StringRef expiresIn = a.getString (OAuth2::Parameters::kExpiresIn);
	if(!expiresIn.isEmpty ())
	{
		int64 tokenExpiration = 0;
		expiresIn.getIntValue (tokenExpiration);
					
		setExpirationTime (timestamp + tokenExpiration);
	}
	
	setTokenType (CString::kEmpty);			
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool OAuth2Tokens::expiresSoon () const
{
	static const int64 kSixMinutes = 6 * Time::kSecondsPerMinute;
	
	int64 now = UnixTime::getTime ();
	return (expirationTime - now) < kSixMinutes;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool OAuth2Tokens::storeARTokens (StringRef credentialName) const
{
	ASSERT (!credentialName.isEmpty ())

	String password;
	if(!accessToken.isEmpty ())
		password = accessToken;
	if(!refreshToken.isEmpty ())
		password << kSeparator << refreshToken;
	if(!tokenType.isEmpty ())
	{
		if(!password.contains (kSeparator))
			password << kSeparator;
		password << kSeparator << tokenType;
	}

	return System::GetCredentialManager ().addPassword (credentialName, kIdentifier, password) == kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool OAuth2Tokens::restoreARTokens (StringRef credentialName)
{
	ASSERT (!credentialName.isEmpty ())

	AutoPtr<Security::ICredential> c;
	System::GetCredentialManager ().getCredential (c, credentialName);
	if(c == nullptr)
		return false;

	String userName;
	c->getUserName (userName);
	if(userName != kIdentifier)
		return false;
		
	String password;
	c->getPassword (password);
	int index = password.index (kSeparator);
	String accessToken = password.subString (0, index);
	int secondIndex = password.lastIndex (kSeparator);
	String refreshToken, type;
	if(secondIndex != index)
	{
		refreshToken = password.subString (index + 1, secondIndex - index - 1);
		type = password.subString (secondIndex + 1);
	}
	else
		refreshToken = password.subString (index + 1);
	
	if(accessToken.isEmpty () && refreshToken.isEmpty ())
		return false;

	setAccessToken (accessToken);
	setRefreshToken (refreshToken);
	setTokenType (MutableCString (type));	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OAuth2Tokens::removeARTokens (StringRef credentialName)
{
	ASSERT (!credentialName.isEmpty ())

	System::GetCredentialManager ().removeCredential (credentialName);

	reset ();
}
