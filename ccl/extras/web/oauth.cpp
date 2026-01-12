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
// Filename    : ccl/extras/web/oauth.cpp
// Description : OAuth (Secure API Authorization Protocol, see oauth.net)
//
//************************************************************************************************
// see http://hueniverse.com/2008/10/beginners-guide-to-oauth-part-iv-signing-requests/

#define DEBUG_LOG 0

#include "ccl/extras/web/oauth.h"

#include "ccl/base/storage/url.h"
#include "ccl/base/storage/urlencoder.h"
#include "ccl/base/collections/stringdictionary.h"
#include "ccl/base/security/cryptobox.h"
#include "ccl/base/security/cryptomaterial.h"

#include "ccl/public/collections/vector.h"
#include "ccl/public/network/web/httpstatus.h"

#include <time.h>
#include <math.h>

namespace CCL {
namespace Web {

//************************************************************************************************
// OAuth::ParamPair
//************************************************************************************************

namespace OAuth 
{
	struct ParamPair
	{
		MutableCString name;
		MutableCString value;

		ParamPair (StringID name = nullptr, StringID value = nullptr)
		: name (name),
		  value (value)
		{}

		int compare (const ParamPair& other) const;

		bool operator > (const ParamPair& other) const { return compare (other) > 0; }
	};
}

} // namespace Web
} // namespace CCL

using namespace CCL;
using namespace Web;

//////////////////////////////////////////////////////////////////////////////////////////////////
#if (0 && DEBUG)
CCL_KERNEL_INIT_LEVEL (OAuthTest, kFirstRun)
{
	CStringDictionary params;
	params.setEntry ("file", "vacation.jpg");
	params.setEntry ("size", "original");
	OAuth::prepare (params, "dpf43f3p2l4k3l03", "nnch734d00sl2jdk", OAuth::Parameters::kHMAC_SHA1);

	// override nonce + timestamp to match example
	params.setEntry (OAuth::Parameters::kNonce, "kllo9940pd9333jh");
	params.setEntry (OAuth::Parameters::kTimestamp, "1191242096");

	Url url ("http://photos.example.net/photos");
	MutableCString baseString = OAuth::normalize (HTTP::kGET, url, params);
	CCL_PRINTF ("base = %s\n", baseString.str ());
	const char* expectedBaseString = "GET&http%3A%2F%2Fphotos.example.net%2Fphotos&file%3Dvacation.jpg%26oauth_consumer_key%3Ddpf43f3p2l4k3l03%26oauth_nonce%3Dkllo9940pd9333jh%26oauth_signature_method%3DHMAC-SHA1%26oauth_timestamp%3D1191242096%26oauth_token%3Dnnch734d00sl2jdk%26oauth_version%3D1.0%26size%3Doriginal";
	ASSERT (baseString == expectedBaseString)

	MutableCString signature = OAuth::sign_HMAC_SHA1 (baseString, "kd94hf93k423kf44", "pfkkdhi9sl3r4s00");
	CCL_PRINTF ("signature = %s\n", signature.str ());
	const char* expectedSignature = "tR3+Ty81lMeYAr/Fid0kMTYa/WM=";
	ASSERT (signature == expectedSignature)

	params.setEntry (OAuth::Parameters::kSignature, signature);
	return true;
}
#endif

//************************************************************************************************
// OAuth::ParamPair
//************************************************************************************************

int OAuth::ParamPair::compare (const ParamPair& other) const
{
	// compare names
	int result = name.compare (other.name);
	if(result != 0)
		return result;

	// compare values
	return value.compare (other.value);
}

//************************************************************************************************
// OAuth::Parameters
//************************************************************************************************

DEFINE_STRINGID_MEMBER_ (OAuth::Parameters, kConsumerKey, "oauth_consumer_key")
DEFINE_STRINGID_MEMBER_ (OAuth::Parameters, kToken, "oauth_token")
DEFINE_STRINGID_MEMBER_ (OAuth::Parameters, kTokenSecret, "oauth_token_secret")
DEFINE_STRINGID_MEMBER_ (OAuth::Parameters, kNonce, "oauth_nonce")
DEFINE_STRINGID_MEMBER_ (OAuth::Parameters, kTimestamp, "oauth_timestamp")
DEFINE_STRINGID_MEMBER_ (OAuth::Parameters, kSignatureMethod, "oauth_signature_method")
DEFINE_STRINGID_MEMBER_ (OAuth::Parameters, kVersion, "oauth_version")
DEFINE_STRINGID_MEMBER_ (OAuth::Parameters, kSignature, "oauth_signature")
DEFINE_STRINGID_MEMBER_ (OAuth::Parameters, kCallback, "oauth_callback")
DEFINE_STRINGID_MEMBER_ (OAuth::Parameters, kVerifier, "oauth_verifier")
DEFINE_STRINGID_MEMBER_ (OAuth::Parameters, kVersion1_0, "1.0")
DEFINE_STRINGID_MEMBER_ (OAuth::Parameters, kHMAC_SHA1, "HMAC-SHA1")
DEFINE_STRINGID_MEMBER_ (OAuth::Parameters, kRSA_SHA1, "RSA-SHA1")
DEFINE_STRINGID_MEMBER_ (OAuth::Parameters, kPlaintext, "PLAINTEXT")

//************************************************************************************************
// OAuth
//************************************************************************************************

int64 OAuth::getSecondsSince1970 ()
{
	return ::time (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString OAuth::generateNonce ()
{
	static const char* alphabet = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
	static const int kAlphabetLength = (int)::strlen (alphabet);

	MutableCString result;
	CStringWriter<100> writer (result);

	int length = 15 + (int)floor (rand () * 16./(double)RAND_MAX);
	for(int i = 0; i < length; i++)
	{
		char c = alphabet[rand () % kAlphabetLength];
		writer.append (c);
	}

	writer.flush ();
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OAuth::prepare (ICStringDictionary& parameters, StringID consumerKey, StringID token, StringID signatureMethod)
{
	MutableCString nonce = generateNonce ();
	
	MutableCString timestamp;
	timestamp.appendFormat ("%" FORMAT_INT64 "d", getSecondsSince1970 ());

	parameters.setEntry (Parameters::kConsumerKey, consumerKey);
	parameters.setEntry (Parameters::kToken, token);
	parameters.setEntry (Parameters::kNonce, nonce);
	parameters.setEntry (Parameters::kTimestamp, timestamp);
	parameters.setEntry (Parameters::kSignatureMethod, signatureMethod);
	parameters.setEntry (Parameters::kVersion, Parameters::kVersion1_0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString OAuth::normalize (StringID _httpMethod, UrlRef url, const ICStringDictionary& parameters)
{
	UrlEncoder urlEncoder (UrlEncoder::kRFC3986, Text::kUTF8);

	// 1) sort by name and value
	Vector<ParamPair> candidates;
	for(int i = 0, count = parameters.countEntries (); i < count; i++)
	{
		StringID name = parameters.getKeyAt (i);
		StringID value = parameters.getValueAt (i);
		MutableCString encodedName = urlEncoder.encode (name);
		MutableCString encodedValue = urlEncoder.encode (value);
		candidates.addSorted (ParamPair (encodedName, encodedValue));
	}

	#if DEBUG_LOG
	CCL_PRINTLN ("Sorted OAuth params:");
	for(int i = 0; i < candidates.count (); i++)
	{
		const ParamPair& pair = candidates.at (i);
		CCL_PRINTF ("%d: %s = %s\n", i, pair.name.str (), pair.value.str ());
	}
	#endif

	// 3) concatenate into a single string
	MutableCString paramString;
	VectorForEach (candidates, ParamPair, pair)
		if(!paramString.isEmpty ())
			paramString += "&";
		paramString += pair.name;
		paramString += "=";
		paramString += pair.value;
	EndFor
	
	// 4) normalize URL
	MutableCString scheme (String (url.getProtocol ()).toLowercase (), Text::kUTF8); // must be lowercase
	MutableCString authority (String (url.getHostName ()).toLowercase (), Text::kUTF8); // must be lowercase
	MutableCString path (url.getPath (), Text::kUTF8); // case-sensitive
	MutableCString httpMethod (_httpMethod);
	httpMethod.toUppercase (); // must be uppercase

	// TODO: handle non-standard port numbers + URL parameters?
	MutableCString urlString;
	urlString += scheme;
	urlString += "://";
	urlString += authority;
	urlString += "/";
	urlString += path;

	// 5) complete the creation of the Signature Base String
	MutableCString baseString;
	baseString += httpMethod;
	baseString += "&";
	baseString += urlEncoder.encode (urlString);
	baseString += "&";
	baseString += urlEncoder.encode (paramString);

	return baseString;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString OAuth::sign_HMAC_SHA1 (StringID baseString, StringID consumerSecret, StringID tokenSecret)
{
	// construct HMAC-SHA1 algorithm key
	MutableCString key;
	UrlEncoder urlEncoder (UrlEncoder::kRFC3986, Text::kUTF8);
	key += urlEncoder.encode (consumerSecret);
	key += "&";
	key += urlEncoder.encode (tokenSecret);

	// calculate HMAC-SHA1
	Security::Crypto::Material signature;
	Security::Crypto::Block keyBlock (key.str (), key.length ());
	Security::Crypto::Material data (Security::Crypto::Block (baseString.str (), baseString.length ()));
	
	Security::Crypto::HMAC_SHA1::sign (signature, keyBlock, data);

	// base-64 encode
	MutableCString signatureString = signature.toCBase64 ();
	return signatureString;
}
