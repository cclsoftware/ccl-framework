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
// Filename    : ccl/platform/shared/host/platformcredentialmanager.cpp
// Description : Platform Credential Manager
//
//************************************************************************************************

#include "ccl/security/credentialmanager.h"
#include "ccl/public/cclversion.h"

#include "ccl/platform/shared/interfaces/platformsecurity.h"
#include "ccl/platform/shared/host/iplatformintegrationloader.h"

#include "core/public/corememstream.h"

namespace CCL {
namespace PlatformIntegration {

//************************************************************************************************
// PlatformCredentialStore
//************************************************************************************************

class PlatformCredentialStore: public Security::CredentialStore
{
public:
	PlatformCredentialStore ();
	
	// CredentialStore
	tresult add (Security::Credential& credential) override;
	Security::Credential* get (StringRef targetName, StringRef groupName) override;
	tresult remove (StringRef targetName) override;
	
protected:
	PlatformImplementationPtr<IPlatformCredentialStore> platformStore;

	virtual bool unlock ();
};

} // namespace PlatformIntegration
} // namespace CCL

using namespace CCL;
using namespace PlatformIntegration;
using namespace Security;

//************************************************************************************************
// CredentialManager
//************************************************************************************************

CredentialStore* CredentialManager::createNativeStore ()
{
	return NEW PlatformCredentialStore;
}

//************************************************************************************************
// PlatformCredentialStore
//************************************************************************************************

PlatformCredentialStore::PlatformCredentialStore ()
: platformStore (CCLSECURITY_PACKAGE_ID)
{
	platformStore.load ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlatformCredentialStore::unlock ()
{
	if(platformStore == nullptr)
		return false;

	return platformStore->unlock (silent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult PlatformCredentialStore::add (Credential& src)
{
	if(!unlock ())
		return kResultFailed;

	MutableCString targetName (src.getTargetName (), Text::kUTF8);
	MutableCString userName (src.getUserName (), Text::kUTF8);
	Crypto::Material data = *src.getData ();
	bool succeeded = platformStore->setCredentials (targetName.str (), userName.str (), data.asBlock ().data, data.asBlock ().length);
	
	return succeeded ? kResultOk : kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Credential* PlatformCredentialStore::get (StringRef targetName, StringRef /*groupName*/)
{
	if(!unlock ())
		return nullptr;

	AutoPtr<Credential> credential;
	
	Core::IO::MemoryStream username;
	Core::IO::MemoryStream data;
	
	if(platformStore->getCredentials (username, data, MutableCString (targetName, Text::kUTF8).str ()))
	{
		credential = NEW Credential;
		credential->setTargetName (targetName);
		credential->setUserName  (String (Text::kUTF8, username.getBuffer ()));
		AutoPtr<Crypto::Material> password = NEW Crypto::Material (data.getBytesWritten ());
		::memcpy (password->asBlock ().data, data.getBuffer (), password->asBlock ().length);
		credential->setData (password);
	}
	
	return credential.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult PlatformCredentialStore::remove (StringRef targetName)
{
	if(!unlock ())
		return kResultFailed;

	bool succeeded = platformStore->removeCredentials (MutableCString (targetName, Text::kUTF8).str ());
	
	return succeeded ? kResultOk : kResultFailed;
}
