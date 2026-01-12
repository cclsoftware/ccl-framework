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
// Filename    : ccl/security/credentialmanager.cpp
// Description : Credential Manager
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/security/credentialmanager.h"

#include "ccl/public/securityservices.h"

using namespace CCL;
using namespace Security;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Security Service APIs
////////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT ICredentialManager& CCL_API System::CCL_ISOLATED (GetCredentialManager) ()
{
	return CredentialManager::instance ();
}

//************************************************************************************************
// Credential
//************************************************************************************************

DEFINE_CLASS_HIDDEN (Credential, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

Credential::Credential (StringRef targetName, StringRef userName, Crypto::BlockRef srcData, StringRef groupName)
: targetName (targetName),
  userName (userName),
  groupName (groupName)
{
	if(srcData.length > 0)
		data = NEW Crypto::Material (srcData);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Credential::getTargetName (String& _targetName) const
{
	_targetName = this->targetName;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Credential::getUserName (String& _userName) const
{
	_userName = this->userName;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Credential::getGroupName (String& _groupName) const
{
	_groupName = groupName;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Credential::getDataReference (Crypto::Block& block) const
{
	if(data)
	{
		block = data->asBlock ();
		return kResultOk;
	}
	else
	{
		block = Crypto::Block ();
		return kResultFalse;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Credential::getPassword (String& password) const
{
	password.empty ();
	if(!data)
		return kResultFalse;

	Crypto::Block block = data->asBlock ();
	password.appendCString (Text::kUTF8, (char*)block.data, block.length);
	return kResultOk;
}

//************************************************************************************************
// CredentialStore
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (CredentialStore, Object)

//************************************************************************************************
// SimpleCredentialStore
//************************************************************************************************

SimpleCredentialStore::SimpleCredentialStore ()
{
	credentials.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult SimpleCredentialStore::add (Credential& credential)
{
	credential.retain ();
	credentials.add (&credential);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Credential* SimpleCredentialStore::get (StringRef targetName, StringRef /*groupName*/)
{
	ForEach (credentials, Credential, c)
		if(c->getTargetName () == targetName)
			return return_shared (c);
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult SimpleCredentialStore::remove (StringRef targetName)
{
	ForEach (credentials, Credential, c)
		if(c->getTargetName () == targetName)
		{
			credentials.remove (c);
			c->release ();
			return kResultOk;
		}
	EndFor
	return kResultFailed;
}

//************************************************************************************************
// CredentialManager
//************************************************************************************************

DEFINE_CLASS_HIDDEN (CredentialManager, Object)
DEFINE_SINGLETON (CredentialManager)

//////////////////////////////////////////////////////////////////////////////////////////////////

CredentialManager::CredentialManager ()
: store (nullptr)
{
	store = createNativeStore ();
	ASSERT (store != nullptr)
	if(store == nullptr)
		store = NEW SimpleCredentialStore;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CredentialManager::~CredentialManager ()
{
	store->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CredentialManager::addCredential (StringRef targetName, StringRef userName, const Crypto::Block& data, StringRef groupName)
{
	ASSERT (!targetName.isEmpty ())
	if(targetName.isEmpty ())
		return kResultInvalidArgument;

	AutoPtr<Credential> c = NEW Credential (targetName, userName, data, groupName);
	c->setTargetName (resolveName (targetName));
	CCL_PRINTF ("CredentialManager add '%s'\n", MutableCString (targetName).str ())
	return store->add (*c);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CredentialManager::addPassword (StringRef targetName, StringRef userName, StringRef _password, StringRef _groupName)
{
	ASSERT (!targetName.isEmpty ())
	if(targetName.isEmpty ())
		return kResultInvalidArgument;

	String usedGroupName = _groupName;
	if(_groupName.isEmpty ())
		usedGroupName = globalGroupName;
	
	MutableCString password (_password, Text::kUTF8);
	Crypto::Block dataBlock (password.str (), password.length ());
	return addCredential (targetName, userName, dataBlock, usedGroupName);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CredentialManager::addCredentialAlias (StringRef aliasName, StringRef targetName)
{
	ASSERT (!aliasName.isEmpty () && !targetName.isEmpty ())
	if(aliasName.isEmpty () || targetName.isEmpty ())
		return kResultInvalidArgument;

	VectorForEach (aliasNames, CredentialAlias, alias)
		if(alias.aliasName == aliasName && alias.targetName == targetName)
			return kResultOk;
	EndFor

	aliasNames.add (CredentialAlias (aliasName, targetName));
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CredentialManager::getCredential (ICredential*& credential, StringRef _targetName, StringRef groupName)
{	
	String targetName = resolveName (_targetName);
	ASSERT (!targetName.isEmpty ())
	if(targetName.isEmpty ())
		return kResultInvalidArgument;

	Credential* object = store->get (targetName, groupName);
	if(object)
		object->setTargetName (_targetName); // assign outer name
	credential = object;
	CCL_PRINTF ("CredentialManager get '%s' %s\n", MutableCString (targetName).str (), object ? "Ok" : "False")
	return credential ? kResultOk : kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CredentialManager::removeCredential (StringRef _targetName)
{
	String targetName = resolveName (_targetName);
	ASSERT (!targetName.isEmpty ())
	if(targetName.isEmpty ())
		return kResultInvalidArgument;

	CCL_PRINTF ("CredentialManager remove '%s'\n", MutableCString (targetName).str ())
	return store->remove (targetName);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CredentialManager::setSilentMode (tbool state)
{
	return store->setSilentMode (state != 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String CCL_API CredentialManager::resolveName (StringRef name) const
{
	String resolvedName;
	VectorForEach (aliasNames, CredentialAlias, alias)
		if(alias.aliasName == name)
		{
			resolvedName = alias.targetName;
			break;
		}
	EndFor

	if(resolvedName.isEmpty ())
		resolvedName = name;
	return resolvedName;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CredentialManager::setGlobalGroupName (StringRef _groupName)
{
	globalGroupName = _groupName;
}
