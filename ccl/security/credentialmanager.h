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
// Filename    : ccl/security/credentialmanager.h
// Description : Credential Manager
//
//************************************************************************************************

#ifndef _ccl_credentialmanager_h
#define _ccl_credentialmanager_h

#include "ccl/base/singleton.h"
#include "ccl/base/security/cryptomaterial.h"
#include "ccl/base/collections/objectarray.h"

#include "ccl/public/security/icredentialmanager.h"

namespace CCL {
namespace Security {

//************************************************************************************************
// Credential
//************************************************************************************************

class Credential: public Object,
				  public ICredential
{
public:
	DECLARE_CLASS (Credential, Object)

	Credential (StringRef targetName = nullptr, StringRef userName = nullptr, Crypto::BlockRef data = Crypto::Block (), StringRef groupName = nullptr);

	PROPERTY_STRING (targetName, TargetName)
	PROPERTY_STRING (userName, UserName)
	PROPERTY_STRING (groupName, GroupName)
	PROPERTY_SHARED_AUTO (Crypto::Material, data, Data)

	// ICredential
	tresult CCL_API getTargetName (String& targetName) const override;
	tresult CCL_API getUserName (String& userName) const override;
	tresult CCL_API getGroupName (String& groupName) const override;
	tresult CCL_API getDataReference (Crypto::Block& block) const override;
	tresult CCL_API getPassword (String& password) const override;

	CLASS_INTERFACE (ICredential, Object)
};

//************************************************************************************************
// CredentialStore
//************************************************************************************************

class CredentialStore: public Object
{
public:
	DECLARE_CLASS_ABSTRACT (CredentialStore, Object)

	CredentialStore ()
	: silent (false)
	{}

	virtual bool setSilentMode (bool state)
	{
		bool oldState = silent;
		silent = state;
		return oldState;
	}

	virtual tresult add (Credential& credential) = 0;

	virtual Credential* get (StringRef targetName, StringRef groupName = nullptr) = 0;

	virtual tresult remove (StringRef targetName) = 0;

protected:
	bool silent;
};

//************************************************************************************************
// SimpleCredentialStore
//************************************************************************************************

class SimpleCredentialStore: public CredentialStore
{
public:
	SimpleCredentialStore ();

	// CredentialStore
	tresult add (Credential& credential) override;
	Credential* get (StringRef targetName, StringRef groupName) override;
	tresult remove (StringRef targetName) override;

protected:
	ObjectArray credentials;
};

//************************************************************************************************
// CredentialManager
//************************************************************************************************

class CredentialManager: public Object,
						 public ICredentialManager,
						 public Singleton<CredentialManager>
{
public:
	DECLARE_CLASS (CredentialManager, Object)

	CredentialManager ();
	~CredentialManager ();

	// ICredentialManager
	tresult CCL_API addCredential (StringRef targetName, StringRef userName, const Crypto::Block& data, StringRef groupName = nullptr) override;
	tresult CCL_API addPassword (StringRef targetName, StringRef userName, StringRef password, StringRef groupName = nullptr) override;
	tresult CCL_API addCredentialAlias (StringRef aliasName, StringRef targetName) override;
	tresult CCL_API getCredential (ICredential*& credential, StringRef targetName, StringRef groupName = nullptr) override;
	tresult CCL_API removeCredential (StringRef targetName) override;
	tbool CCL_API setSilentMode (tbool state) override;
	String CCL_API resolveName (StringRef name) const override;
	void CCL_API setGlobalGroupName (StringRef groupName) override;

	CLASS_INTERFACE (ICredentialManager, Object)

protected:
	struct CredentialAlias
	{
		String aliasName;
		String targetName;

		CredentialAlias (StringRef aliasName = nullptr,
						 StringRef targetName = nullptr)
		: aliasName (aliasName),
		  targetName (targetName)
		{}
	};

	CredentialStore* store;
	Vector<CredentialAlias> aliasNames;
	String globalGroupName;

	static CredentialStore* createNativeStore ();
};

} // namespace Security
} // namespace CCL

#endif // _ccl_credentialmanager_h
