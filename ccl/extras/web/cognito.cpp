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
// Filename    : ccl/extras/web/cognito.cpp
// Description : Cognito (Amazon Cognito Authentication Service)
//
//************************************************************************************************

#include "ccl/extras/web/cognito.h"
#include "ccl/extras/web/oauth2.h"

#include "ccl/base/asyncoperation.h"
#include "ccl/base/boxedtypes.h"
#include "ccl/base/storage/attributes.h"
#include "ccl/base/storage/jsonarchive.h"
#include "ccl/base/storage/url.h"
#include "ccl/base/security/cryptomaterial.h"
#include "ccl/base/security/cryptobox.h"

#include "ccl/public/base/istream.h"
#include "ccl/public/base/datetime.h"
#include "ccl/public/base/streamer.h"
#include "ccl/public/text/istringdict.h"
#include "ccl/public/text/language.h"

#include "ccl/public/network/web/httpstatus.h"
#include "ccl/public/network/web/iwebrequest.h"
#include "ccl/public/network/web/ixmlhttprequest.h"
#include "ccl/public/network/web/iwebcredentials.h"

#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/ilocalemanager.h"
#include "ccl/public/system/formatter.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/plugservices.h"

#include "ccl/public/security/icryptointeger.h"
#include "ccl/public/securityservices.h"

#include "ccl/extras/web/webxhroperation.h"

namespace CCL {
namespace Web {

//************************************************************************************************
// Cognito
//************************************************************************************************

namespace Cognito 
{
	String makeCognitoUrl (StringRef region)
	{
		return String () << "https://cognito-idp." << region << ".amazonaws.com";
	}

	const CString kAwsTargetHeader = "X-Amz-Target"; // specifies which action to perform
	const CString kAwsJsonContentType = "application/x-amz-json-1.1"; // specifies which JSON version to use

	// Actions
	const CString kRequestTypeInitiateAuth = "AWSCognitoIdentityProviderService.InitiateAuth";
	const CString kRequestTypeGetUser = "AWSCognitoIdentityProviderService.GetUser";
	const CString kRequestTypeSignUp = "AWSCognitoIdentityProviderService.SignUp";
	const CString kRequestTypeRespondToAuthChallenge = "AWSCognitoIdentityProviderService.RespondToAuthChallenge";

	void fromNameValue (Attributes& attributes, const AttributeQueue& cognitoQueue);
	void toNameValue (AttributeQueue& cognitoQueue, const Attributes& attributes);

	//////////////////////////////////////////////////////////////////////////////////////////////
	// SRP (Secure Remote Password protocol)
	//////////////////////////////////////////////////////////////////////////////////////////////

	// A large safe prime (2048-bit) used as the modulus in SRP key exchange
	const CString kSRPN = "FFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD129024E088A67CC74020BBEA63B139B22514A08798E3404DDEF9519B3CD3A431B302B0A6DF25F14374FE1356D6D51C245E485B576625E7EC6F44C42E9A637ED6B0BFF5CB6F406B7EDEE386BFB5A899FA5AE9F24117C4B1FE649286651ECE45B3DC2007CB8A163BF0598DA48361C55D39A69163FA8FD24CF5F83655D23DCA3AD961C62F356208552BB9ED529077096966D670C354E4ABC9804F1746C08CA18217C32905E462E36CE3BE39E772C180E86039B2783A2EC07A28FB5C55DF06F4C52C9DE2BCBF6955817183995497CEA956AE515D2261898FA051015728E5A8AAAC42DAD33170D04507A33A85521ABDF1CBA64ECFB850458DBEF0A8AEA71575D060C7DB3970F85A6E1E4C7ABF5AE8CDB0933D71E8C94E04A25619DCEE3D2261AD2EE6BF12FFA06D98A0864D87602733EC86A64521F2B18177B200CBBE117577A615D6C770988C0BAD946E208E24FA074E5AB3143DB5BFCE0FD108E4B82D120A93AD2CAFFFFFFFFFFFFFFFF";
	const CString kSRPg = "2"; // generator number used for BigInt operations in SRP key exchange
	const CString kDerivedKeyInfo = "Caldera Derived Key";

	struct SRPInfo
	{
		String username;
		MutableCString srpb;
		MutableCString salt;
		MutableCString secretBlock;
		String userId;
	};

	/** Initiate SRP Authentication. Action is InitiateAuth, AuthFlow is USER_SRP_AUTH. */
	IXMLHttpRequest* createSRPAuthRequest (MutableCString& SRPA, MutableCString& a, StringRef region, StringRef clientId, StringRef userName);

	/** Extract SRP Info from server response. */
	bool parseSRPResponse (SRPInfo& srpInfo, VariantRef serverResponse);

	/** Respond to SRP Authentication Challenge. Action is InitiateAuth, AuthFlow is USER_SRP_AUTH. */
	IXMLHttpRequest* createSRPChallengeResponse (StringRef region,
												 StringRef clientId,
												 StringRef userName,
												 StringRef password,
												 CStringRef userPoolId,
												 CStringRef SRPA,
												 CStringRef a,
												 const SRPInfo& srpInfo);

	/** Compute SRPA as g^a mod N. */
	MutableCString computeSRPA (CStringRef a);

	/** Compute x = SHA256_HASH (salt + (poolname + username + ":" + password)) */
	MutableCString computeX (StringRef password, CStringRef userPoolId, const SRPInfo& srpInfo);

	/** Compute k = SHA256_HASH ("00" + N + "0" + g) */
	MutableCString computeK ();

	/** Compute u = SHA256_HASH (SRP_A + SRP_B) */
	MutableCString computeU (CStringRef SRPA, CStringRef SRPB);

	/** Compute S = (B - k * g^x) ^ (a + u * x) mod N */
	MutableCString computeS (CStringRef x,
							 CStringRef k,
							 CStringRef u,
							 CStringRef a,
							 CStringRef srpb);

	/** Compute hkdf = HKDF (S, u) */
	MutableCString computeHKDF (CStringRef S, CStringRef u);

	/** Compute Password Claim as Base64(HMAC_SHA(hkdf, UserPoolId + PASSWORD_CLAIM_SECRET_BLOCK + TIMESTAMP))) */
	MutableCString computePasswordClaimSignature (CStringRef hkdf,
												  CStringRef userPoolId,
												  CStringRef timeStamp,
												  const SRPInfo& srpInfo);

	/** Format the timestamp according to Cognito: EEE MMM d HH:mm:ss UTC yyyy */
	String getSRPTimestamp ();

	/** Pad hex string */
	MutableCString padHex (CStringRef hexValue);
}

} // namespace Web
} // namespace CCL

using namespace CCL;
using namespace Web;

//************************************************************************************************
// Cognito
//************************************************************************************************

void Cognito::fromNameValue (Attributes& attributes, const AttributeQueue& cognitoQueue)
{
	for(auto* attr : iterate_as<Attribute> (cognitoQueue))
	{
		if(Attributes* cognitoAttributes = unknown_cast<Attributes> (attr->getValue ().asUnknown ()))
		{
			MutableCString name;
			name = cognitoAttributes->getCString ("Name");
			Variant value;
			cognitoAttributes->getAttribute (value, "Value");

			attributes.setAttribute (name, value);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Cognito::toNameValue (AttributeQueue& cognitoQueue, const Attributes& attributes)
{
	ForEachAttribute (attributes, name, value)
		Attributes infoAttributes;
		infoAttributes.set ("Name", name);
		infoAttributes.setAttribute ("Value", value);
		
		cognitoQueue.addAttributes (&infoAttributes, Attributes::kTemp);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IXMLHttpRequest* Cognito::createSignInRequest (StringRef region, StringRef clientId, StringRef userName, StringRef password)
{
	Url url (makeCognitoUrl (region));

	Attributes params;
	params.set ("AuthFlow", "USER_PASSWORD_AUTH");
	params.set ("ClientId", clientId);

	Attributes authParameters;
	authParameters.set ("USERNAME", userName);
	authParameters.set ("PASSWORD", password);
	params.setAttribute ("AuthParameters", authParameters.asUnknown ());

	AutoPtr<IStream> jsonData = JsonUtils::serialize (params);	

	auto* httpPostRequest = ccl_new<IXMLHttpRequest> (ClassID::XMLHttpRequest);
	httpPostRequest->open (HTTP::kPOST, url);
	httpPostRequest->setRequestHeader (kAwsTargetHeader, kRequestTypeInitiateAuth);
	httpPostRequest->setRequestHeader (Meta::kContentType, kAwsJsonContentType);
	httpPostRequest->send (jsonData);
	
	return httpPostRequest;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IXMLHttpRequest* Cognito::createTokenRefreshRequest (StringRef region, StringRef clientId, StringRef refreshToken)
{
	Url url (makeCognitoUrl (region));
	
	Attributes params;
	params.set ("AuthFlow", "REFRESH_TOKEN_AUTH");
	params.set ("ClientId", clientId);
	
	Attributes authParameters;
	authParameters.set ("REFRESH_TOKEN", refreshToken);
	params.setAttribute ("AuthParameters", authParameters.asUnknown ());
	
	AutoPtr<IStream> jsonData = JsonUtils::serialize (params);
	
	auto* httpPostRequest = ccl_new<IXMLHttpRequest> (ClassID::XMLHttpRequest);
	httpPostRequest->open (HTTP::kPOST, url);
	httpPostRequest->setRequestHeader (Meta::kContentType, kAwsJsonContentType);
	httpPostRequest->setRequestHeader (kAwsTargetHeader, kRequestTypeInitiateAuth);
	httpPostRequest->send (jsonData);
	
	return httpPostRequest;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Cognito::loadAuthenticationResult (OAuth2Tokens& tokens, IStream& jsonStream, int64 timestamp)
{
	Attributes response;
	if(!JsonUtils::parse (response, jsonStream))
		return false;
	
	if(Attributes* authenticationResult = response.getAttributes ("AuthenticationResult"))
	{
		tokens.setAccessToken (authenticationResult->getString ("AccessToken"));
		tokens.setIdToken (authenticationResult->getString ("IdToken"));
		tokens.setRefreshToken (authenticationResult->getString ("RefreshToken"));
		
		StringRef expiresIn = authenticationResult->getString ("ExpiresIn");
		if(!expiresIn.isEmpty ())
		{
			int64 tokenExpiration = 0;
			expiresIn.getIntValue (tokenExpiration);
			
			tokens.setExpirationTime (timestamp + tokenExpiration);
		}
		
		tokens.setTokenType (kTokenType);		
		return true;
	}
	
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IXMLHttpRequest* Cognito::createGetUserRequest (StringRef region, StringRef accessToken)
{
	Url url (makeCognitoUrl (region));

	Attributes params;
	params.set ("AccessToken", accessToken);

	AutoPtr<IStream> jsonData = JsonUtils::serialize (params);
	
	auto* httpPostRequest = ccl_new<IXMLHttpRequest> (ClassID::XMLHttpRequest);
	httpPostRequest->open (HTTP::kPOST, url, true, String::kEmpty, accessToken, String (Meta::kBearer));
	httpPostRequest->setRequestHeader (kAwsTargetHeader, kRequestTypeGetUser);
	httpPostRequest->setRequestHeader (Meta::kContentType, kAwsJsonContentType);
	httpPostRequest->send (jsonData);
	
	return httpPostRequest;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Cognito::loadGetUserResponse (String& userName, Attributes& userAttributes, IStream& jsonStream)
{
	Attributes response;
	if(!JsonUtils::parse (response, jsonStream))
		return false;

	if(AttributeQueue* queue = response.getObject<AttributeQueue> ("UserAttributes"))
		fromNameValue (userAttributes, *queue);
	
	userName = response.getString ("Username");
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IXMLHttpRequest* Cognito::createSignUpRequest (StringRef region, StringRef clientId, 
											   StringRef userName, StringRef password, 
											   const Attributes& userAttributes)
{
	Url url (makeCognitoUrl (region));
	
	Attributes params;
	params.set ("ClientId", clientId);
	params.set ("Username", userName);
	params.set ("Password", password);
	
	AttributeQueue queue;
	toNameValue (queue, userAttributes);	
	params.set ("UserAttributes", queue.asUnknown ());

	AutoPtr<IStream> jsonData = JsonUtils::serialize (params);
	
	auto* httpPostRequest = ccl_new<IXMLHttpRequest> (ClassID::XMLHttpRequest);
	httpPostRequest->open (HTTP::kPOST, url);
	httpPostRequest->setRequestHeader (kAwsTargetHeader, kRequestTypeSignUp);
	httpPostRequest->setRequestHeader (Meta::kContentType, kAwsJsonContentType);
	httpPostRequest->send (jsonData);
	
	return httpPostRequest;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Cognito::loadErrorType (String& errorType, IStream& jsonStream)
{
	Attributes response;
	if(!JsonUtils::parse (response, jsonStream))
		return false;

	response.get (errorType, "__type");
	return !errorType.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IXMLHttpRequest* Cognito::createSRPAuthRequest (MutableCString& SRPA, MutableCString& a, StringRef region, StringRef clientId, StringRef userName)
{
	Url url (makeCognitoUrl (region));

	Attributes params;
	params.set ("AuthFlow", "USER_SRP_AUTH");
	params.set ("ClientId", clientId);

	// generate random number a
	Security::Crypto::Material aMaterial (128);
	Security::Crypto::RandomPool::generate (aMaterial);
	a = aMaterial.toCHex ();
	
	SRPA = computeSRPA (a);
	
	Attributes authParameters;
	authParameters.set ("USERNAME", userName);
	authParameters.set ("SRP_A", SRPA);
	params.setAttribute ("AuthParameters", authParameters.asUnknown ());

	AutoPtr<IStream> jsonData = JsonUtils::serialize (params);

	auto* httpPostRequest = ccl_new<IXMLHttpRequest> (ClassID::XMLHttpRequest);
	httpPostRequest->open (HTTP::kPOST, url);
	httpPostRequest->setRequestHeader (kAwsTargetHeader, kRequestTypeInitiateAuth);
	httpPostRequest->setRequestHeader (Meta::kContentType, kAwsJsonContentType);
	httpPostRequest->send (jsonData);
	
	return httpPostRequest;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString Cognito::computeSRPA (CStringRef a)
{
	AutoPtr<Security::Crypto::IInteger> aInteger = Security::Crypto::IntegerStatics::create ();
	tresult result = aInteger->fromCString (a, 16);
	ASSERT (result == kResultOk)
	
	AutoPtr<Security::Crypto::IInteger> gInteger = Security::Crypto::IntegerStatics::create ();
	result = gInteger->fromCString (kSRPg, 16);
	ASSERT (result == kResultOk)
	
	AutoPtr<Security::Crypto::IInteger> NInteger = Security::Crypto::IntegerStatics::create ();
	result = NInteger->fromCString (kSRPN, 16);
	ASSERT (result == kResultOk)
	
	// compute SRP_A = g^a mod N
	AutoPtr<Security::Crypto::IInteger> resultInteger = Security::Crypto::IntegerStatics::create ();
	result = gInteger->expMod (*resultInteger, *aInteger, *NInteger);
	ASSERT (result == kResultOk)
	
	MutableCString SRPA;
	resultInteger->toCString (SRPA);
	return SRPA;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Cognito::parseSRPResponse (SRPInfo& srpInfo, VariantRef serverResponse)
{
	UnknownPtr<IStream> stream (serverResponse.asUnknown ());
	if(stream.isValid ())
	{
		Attributes response;
		if(!JsonUtils::parse (response, *stream))
			return false;
		
		if(response.getString ("ChallengeName") != "PASSWORD_VERIFIER")
			return false;
		
		if(Attributes* challengeParameters = response.getAttributes ("ChallengeParameters"))
		{
			srpInfo.username = challengeParameters->getString ("USERNAME");
			srpInfo.srpb = challengeParameters->getCString ("SRP_B");
			srpInfo.salt = challengeParameters->getString ("SALT");
			srpInfo.secretBlock = challengeParameters->getString ("SECRET_BLOCK");
			srpInfo.userId = challengeParameters->getString ("USER_ID_FOR_SRP");
			return true;
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IXMLHttpRequest* Cognito::createSRPChallengeResponse (StringRef region, StringRef clientId, StringRef userName,
													  StringRef password, CStringRef userPoolId, CStringRef SRPA,
													  CStringRef a, const SRPInfo& srpInfo)
{
	String timeStamp = getSRPTimestamp ();

	MutableCString xValue = computeX (password, userPoolId, srpInfo);
	MutableCString kValue = computeK ();
	
	MutableCString uValue = computeU (SRPA, srpInfo.srpb);
	Security::Crypto::Material uHash;
	uHash.fromHex (uValue);
	
	MutableCString S = computeS (xValue, kValue, uValue, a, srpInfo.srpb);
	
	MutableCString hkdf = computeHKDF (S, uValue);
	
	MutableCString passwordClaimSignature = computePasswordClaimSignature (hkdf, userPoolId, MutableCString (timeStamp), srpInfo);
	
	Url url (makeCognitoUrl (region));

	Attributes params;
	params.set ("ChallengeName", "PASSWORD_VERIFIER");
	params.set ("ClientId", clientId);

	Attributes authParameters;
	authParameters.set ("USERNAME", srpInfo.username);
	authParameters.set ("PASSWORD_CLAIM_SECRET_BLOCK", srpInfo.secretBlock);
	authParameters.set ("TIMESTAMP", timeStamp);
	authParameters.set ("PASSWORD_CLAIM_SIGNATURE", passwordClaimSignature);
	params.setAttribute ("ChallengeResponses", authParameters.asUnknown ());

	AutoPtr<IStream> jsonData = JsonUtils::serialize (params);

	auto* httpPostRequest = ccl_new<IXMLHttpRequest> (ClassID::XMLHttpRequest);
	httpPostRequest->open (HTTP::kPOST, url);
	httpPostRequest->setRequestHeader (kAwsTargetHeader, kRequestTypeRespondToAuthChallenge);
	httpPostRequest->setRequestHeader (Meta::kContentType, kAwsJsonContentType);
	httpPostRequest->send (jsonData);
	
	return httpPostRequest;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString Cognito::computeX (StringRef password, CStringRef userPoolId, const SRPInfo& srpInfo)
{
	//	x = SHA256_HASH (salt + FULL_PASSWORD)
	MutableCString fullPassword (userPoolId);
	fullPassword.append (srpInfo.username);
	fullPassword.append (":");
	Security::Crypto::Material fullPasswordMaterial;
	fullPasswordMaterial.append (fullPassword);
	fullPasswordMaterial.append (password, Text::kUTF8);
	Security::Crypto::Material fullPasswordHash (Security::Crypto::SHA256::kDigestSize);
	Security::Crypto::SHA256::calculate (fullPasswordHash, fullPasswordMaterial);
	fullPassword = fullPasswordHash.toCHex ();
	
	// Left-pad with zeros if necessary
	if(fullPassword.length () < 64)
	{
		int padLen = 64 - fullPassword.length ();
		for(int i = 0; i < padLen; i++)
			fullPassword.insert (0, "0");
	}
	
	MutableCString xString = padHex (srpInfo.salt);
	Security::Crypto::Material xMaterial;
	xMaterial.fromHex (xString);
	xMaterial.append (fullPasswordHash);
	Security::Crypto::Material xHash (Security::Crypto::SHA256::kDigestSize);
	Security::Crypto::SHA256::calculate (xHash, xMaterial);
	return xHash.toCHex ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString Cognito::computeK ()
{
	//	k = "00" + N + "0" + g
	MutableCString kString ("00");
	kString.append (kSRPN);
	kString.append ("0");
	kString.append(kSRPg);
	
	Security::Crypto::Material kMaterial;
	kMaterial.fromHex (kString);
	Security::Crypto::Material kHash (Security::Crypto::SHA256::kDigestSize);
	Security::Crypto::SHA256::calculate (kHash, kMaterial);
	return kHash.toCHex ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString Cognito::computeU (CStringRef SRPA, CStringRef SRPB)
{
	//	u = SHA256_HASH(SRP_A + SRP_B)
	MutableCString uString = padHex (SRPA);
	MutableCString SRPBPadded = padHex (SRPB);
	uString.append (SRPBPadded);

	Security::Crypto::Material uMaterial;
	uMaterial.fromHex (uString);

	Security::Crypto::Material uHash (Security::Crypto::SHA256::kDigestSize);
	Security::Crypto::SHA256::calculate (uHash, uMaterial);
	return uHash.toCHex ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString Cognito::computeS (CStringRef x, CStringRef k, CStringRef u, CStringRef a, CStringRef srpb)
{
	// computes S = (B - k * g^x) ^ (a + u * x) mod N
	// Step 1: g^x mod N
	AutoPtr<Security::Crypto::IInteger> gInteger = Security::Crypto::IntegerStatics::create ();
	tresult result = gInteger->fromCString (kSRPg, 16);
	ASSERT (result == kResultOk)
	
	AutoPtr<Security::Crypto::IInteger> NInteger = Security::Crypto::IntegerStatics::create ();
	result = NInteger->fromCString (kSRPN, 16);
	ASSERT (result == kResultOk)
	
	AutoPtr<Security::Crypto::IInteger> xInteger = Security::Crypto::IntegerStatics::create ();
	result = xInteger->fromCString (x, 16);
	ASSERT (result == kResultOk)
	
	// g^x mod N
	AutoPtr<Security::Crypto::IInteger> gXNInteger = Security::Crypto::IntegerStatics::create ();
	result = gInteger->expMod (*gXNInteger, *xInteger, *NInteger);
	ASSERT (result == kResultOk)
	
	// Step 2: B - k * (g^x)
	AutoPtr<Security::Crypto::IInteger> BInteger = Security::Crypto::IntegerStatics::create ();
	result = BInteger->fromCString (srpb, 16);
	ASSERT (result == kResultOk)

	// k * (g^x)
	AutoPtr<Security::Crypto::IInteger> kInteger = Security::Crypto::IntegerStatics::create ();
	result = kInteger->fromCString (k, 16);
	ASSERT (result == kResultOk)

	AutoPtr<Security::Crypto::IInteger> kgxResult = Security::Crypto::IntegerStatics::create ();
	result = kInteger->multiply (*kgxResult, *gXNInteger);
	ASSERT (result == kResultOk)
	
	// B - k * (g^x)
	AutoPtr<Security::Crypto::IInteger> baseInteger = Security::Crypto::IntegerStatics::create ();
	result = BInteger->substract (*baseInteger, *kgxResult);
	ASSERT (result == kResultOk)
	
	// Step 3: Exponent = a + u * x
	AutoPtr<Security::Crypto::IInteger> aInteger = Security::Crypto::IntegerStatics::create ();
	result = aInteger->fromCString (a, 16);
	ASSERT (result == kResultOk)
	
	AutoPtr<Security::Crypto::IInteger> uInteger = Security::Crypto::IntegerStatics::create ();
	result = uInteger->fromCString (u, 16);
	ASSERT (result == kResultOk)
	
	// u * x
	AutoPtr<Security::Crypto::IInteger> uxResult = Security::Crypto::IntegerStatics::create ();
	result = uInteger->multiply (*uxResult, *xInteger);
	ASSERT (result == kResultOk)
	
	// a + u * x
	AutoPtr<Security::Crypto::IInteger> expInteger = Security::Crypto::IntegerStatics::create ();
	result = aInteger->add (*expInteger, *uxResult);
	ASSERT (result == kResultOk)
	
	// Step 4: (B - k * g^x) ^ (a + ux) mod N
	AutoPtr<Security::Crypto::IInteger> resInteger = Security::Crypto::IntegerStatics::create ();
	result = baseInteger->expMod (*resInteger, *expInteger, *NInteger);
	ASSERT (result == kResultOk)

	// Step 5: result mod N
	AutoPtr<Security::Crypto::IInteger> SInteger = Security::Crypto::IntegerStatics::create ();
	result = resInteger->modulo (*SInteger, *NInteger);
	ASSERT (result == kResultOk)
	
	MutableCString S;
	SInteger->toCString (S);
	return S;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString Cognito::computeHKDF (CStringRef S, CStringRef u)
{
	Security::Crypto::Material hkdfMaterial;
	MutableCString paddedS = padHex (S);
	Security::Crypto::Material SMaterial;
	SMaterial.fromHex (paddedS);
	MutableCString paddedU = padHex (u);
	Security::Crypto::Material uMaterial;
	uMaterial.fromHex (paddedU);
	MutableCString hkdfInfo (kDerivedKeyInfo);
	Security::Crypto::Block infoBlock (hkdfInfo, hkdfInfo.length ());
	Security::Crypto::HKDF::deriveKey (hkdfMaterial, Security::Crypto::HKDF::kKeyLen16, SMaterial, uMaterial, infoBlock);
	return hkdfMaterial.toCHex ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString Cognito::computePasswordClaimSignature (CStringRef hkdf, CStringRef userPoolId, CStringRef timeStamp, const SRPInfo& srpInfo)
{
	// convert key to bytes
	Security::Crypto::Material signature;
	Security::Crypto::Material keyMaterial;
	keyMaterial.fromHex (hkdf);
	
	// data = (poolid + username + secretBlockDecoded + timeStamp) as bytes
	MutableCString poolIdUserName;
	poolIdUserName.append (userPoolId);
	poolIdUserName.append (srpInfo.username);
	Security::Crypto::Block poolIdUserNameBlock (reinterpret_cast<const unsigned char *>(poolIdUserName.str ()), poolIdUserName.length ());
	
	Security::Crypto::Material passwordClaimMaterial (poolIdUserNameBlock);
	
	Security::Crypto::Material secretBlockDecodedMaterial;
	secretBlockDecodedMaterial.fromBase64 (srpInfo.secretBlock);
	Security::Crypto::Block secretBlockDecoded = secretBlockDecodedMaterial.asBlock ();
	
	passwordClaimMaterial.append (Security::Crypto::Material (secretBlockDecoded));
	
	Security::Crypto::Material timeMaterial (Security::Crypto::Block (reinterpret_cast<const unsigned char *>(timeStamp.str ()), timeStamp.length ()));
	
	passwordClaimMaterial.append (timeMaterial);
	
	Security::Crypto::HMAC_SHA256::sign (signature.asStream (), keyMaterial.asBlock (), passwordClaimMaterial);

	return signature.toCBase64 ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String Cognito::getSRPTimestamp ()
{
	const ILocaleInfo* locale = System::GetLocaleManager ().getLocale (LanguageCode::English);
	ASSERT (locale)
	if(!locale)
		return String::kEmpty;

	String timestamp;

	// e.g. Sat May 3 09:27:03 UTC 2025
	DateTime localDateTime;
	System::GetSystem ().getLocalTime (localDateTime);
	DateTime utcDateTime;
	System::GetSystem ().convertLocalTimeToUTC (utcDateTime, localDateTime);
	Date date = utcDateTime.getDate ();
	Time time = utcDateTime.getTime ();
	
	int dayNo = locale->getDayOfWeek (date);
	String dayName = locale->getWeekdayName (dayNo);
	timestamp.append (dayName.subString (0, 3));
	
	int monthNo = date.getMonth ();
	String month = locale->getMonthName (monthNo);
	timestamp.append (" ");
	timestamp.append (month.subString (0, 3));
	timestamp.append (" ");
	timestamp.appendIntValue (date.getDay ());
	timestamp.append (" ");
	
	int hour = time.getHour ();
	if(hour < 10)
		timestamp.append ("0");
	timestamp.appendIntValue (time.getHour ());
	timestamp.append (":");
	int min = time.getMinute ();
	if(min < 10)
		timestamp.append ("0");
	timestamp.appendIntValue (min);
	timestamp.append (":");
	int sec = time.getSecond ();
	if(sec < 10)
		timestamp.append ("0");
	timestamp.appendIntValue (sec);

	timestamp.append (" UTC ");
	timestamp.appendIntValue (date.getYear ());
	return timestamp;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString Cognito::padHex (CStringRef hexValue)
{
	MutableCString result;
	MutableCString characters ("89ABCDEFabcdef");
	
	if(hexValue.length () % 2 == 1)
		result.append ("0");
	else if(hexValue.length () > 0 && characters.contains (hexValue[0]))
		result.append ("00");

	result.append (hexValue);

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* Cognito::signInSRP (StringRef region, StringRef clientId, StringRef userName, StringRef password, StringRef userPoolId)
{
	AutoPtr<AsyncSequence> asyncs = NEW AsyncSequence ();
	asyncs->setCancelOnError (true);
	
	AutoPtr<Boxed::Variant> SRPA = NEW Boxed::Variant;
	AutoPtr<Boxed::Variant> SRPa = NEW Boxed::Variant;
	
	asyncs->add ([region, clientId, userName, SRPA, SRPa] ()
	{
		MutableCString A, a;
		IXMLHttpRequest* request = createSRPAuthRequest (A, a, region, clientId, userName);
		SRPA->assign (Variant (A));
		SRPa->assign (Variant (a));
		return NEW AsyncXHROperation (request);
	});

	AutoPtr<Boxed::Variant> parsingResult = NEW Boxed::Variant;
	asyncs->then ([parsingResult] (IAsyncOperation& op)
	{
		if(op.getState () == IAsyncInfo::kCompleted)
		{
			UnknownPtr<IXMLHttpRequest> httpRequest (&op);
			ASSERT (httpRequest.isValid ())
			if(httpRequest)
			{
				if(IStream* responseStream = httpRequest->getResponseStream ())
				{
					Variant data (responseStream);
					parsingResult->assign (Variant ().takeShared (data));
				}
			}
		}
	});

	AutoPtr<Boxed::Variant> srpParsingResult = NEW Boxed::Variant;
	asyncs->add ([srpParsingResult, parsingResult, region, clientId, userName, password, userPoolId, SRPA, SRPa] () -> IAsyncOperation*
	{
		SRPInfo srpInfo;
		srpParsingResult->assign (Variant (parseSRPResponse (srpInfo, parsingResult->asVariant ())));
		if(!srpParsingResult->asVariant ().asBool ())
			return AsyncOperation::createCompleted (parsingResult->asVariant ());
		
		MutableCString A, a;
		SRPA->asVariant ().toCString (A);
		SRPa->asVariant ().toCString (a);

		if(!srpInfo.username.isEmpty ())
			return NEW AsyncXHROperation (createSRPChallengeResponse (region, clientId, userName, password, MutableCString (userPoolId), A, a, srpInfo));
		
		return AsyncOperation::createFailed ();
	});
	
	asyncs->then ([srpParsingResult, parsingResult] (IAsyncOperation& op)
	{
		if(op.getState () == IAsyncInfo::kCompleted)
		{
			if(srpParsingResult->asVariant ().asBool ())
			{
				UnknownPtr<IXMLHttpRequest> httpRequest (&op);
				ASSERT (httpRequest.isValid ())
				if(httpRequest)
				{
					if(IStream* responseStream = httpRequest->getResponseStream ())
					{
						Variant data (responseStream);
						op.setResult (data);
					}
				}
			}
		}
	});

	return return_shared<IAsyncOperation> (asyncs->start ());
}
