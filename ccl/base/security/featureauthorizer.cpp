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
// Filename    : ccl/base/security/featureauthorizer.cpp
// Description : Feature Authorizer
//
//************************************************************************************************

#include "ccl/base/security/featureauthorizer.h"

#include "ccl/base/singleton.h"

#include "ccl/public/system/ikeyprovider.h"
#include "ccl/public/securityservices.h"

using namespace CCL;
using namespace Security;

//************************************************************************************************
// PolicyAccessor
//************************************************************************************************

bool PolicyAccessor::getDataFromPolicy (String& outData, StringRef dataSid, StringRef dataPrefix)
{
	IterForEachUnknown (System::GetAuthorizationManager ().createIterator (dataSid), unk)
		if(UnknownPtr<Security::Authorization::IPolicyItem> item = unk)
			if(item->getItemSID ().startsWith (dataPrefix))
			{
				outData = item->getItemSID ().subString (dataPrefix.length ());
				outData.trimWhitespace ();
				return true;
			}
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PolicyAccessor::getEncryptionKeyFromPolicy (String& key, StringRef dataSid)
{
	static const String kKeyPrefix ("key=");
	return getDataFromPolicy (key, dataSid, kKeyPrefix);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IEncryptionKeyProvider* PolicyAccessor::getEncryptionKeyProvider ()
{
	class KeyProvider: public Object,
					   public IEncryptionKeyProvider,
					   public StaticSingleton<KeyProvider>
	{
	public:
		// IEncryptionKeyProvider
		tresult CCL_API getEncryptionKey (String& key, StringRef keyId) override
		{
			return getEncryptionKeyFromPolicy (key, keyId) ? kResultOk : kResultFailed;
		}

		CLASS_INTERFACE (IEncryptionKeyProvider, Object)
	};

	return &KeyProvider::instance ();
}

//************************************************************************************************
// FeatureAuthorizer
//************************************************************************************************

const String FeatureAuthorizer::kAny ("*");
const String FeatureAuthorizer::kDefaultItemSid ("default");

////////////////////////////////////////////////////////////////////////////////////////////////////

String FeatureAuthorizer::getFullAppId ()
{
	StringRef appId = System::GetAuthorizationManager ().getAppIdentity ();
	return makeFullAppId (appId);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

String FeatureAuthorizer::makeFullAppId (StringRef appId)
{
	StringRef appSubId = System::GetAuthorizationManager ().getAppSubIdentity ();
	if(appSubId.isEmpty ())
		return appId;

	String fullAppId (appId);
	fullAppId << "." << appSubId;
	return fullAppId;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FeatureAuthorizer::FeatureAuthorizer (StringRef resourceSid, StringRef _clientSid, int mode)
: resource (System::GetAuthorizationManager ().getPrivileges (resourceSid)),
  clientSid (_clientSid.isEmpty () ? System::GetAuthorizationManager ().getAppIdentity () : _clientSid),
  mode (mode)
{}
