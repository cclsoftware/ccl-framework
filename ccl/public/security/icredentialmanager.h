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
// Filename    : ccl/public/security/icredentialmanager.h
// Description : Credential Manager Interface
//
//************************************************************************************************

#ifndef _ccl_icredentialmanager_h
#define _ccl_icredentialmanager_h

#include "ccl/public/base/istream.h"

namespace CCL {
namespace Security {

namespace Crypto {
struct Block; }
 
//************************************************************************************************
// ICredential
//************************************************************************************************

interface ICredential: IUnknown
{
	/** Get target name (describes what the credential is used for). */
	virtual tresult CCL_API getTargetName (String& targetName) const = 0;

	/** Get associated user name. */
	virtual tresult CCL_API getUserName (String& userName) const = 0;
	
	/** Get the name of the group the credential belongs to, if any. */
	virtual tresult CCL_API getGroupName (String& groupName) const = 0;

	/** Get reference to associated data (password, certificate, etc.). */
	virtual tresult CCL_API getDataReference (Crypto::Block& block) const = 0;

	/** Get data as (password) string. */
	virtual tresult CCL_API getPassword (String& password) const = 0;

	DECLARE_IID (ICredential)
};

DEFINE_IID (ICredential, 0x52a29848, 0xcc87, 0x42b6, 0x93, 0xc5, 0x2a, 0x55, 0xb7, 0xf0, 0xed, 0x59)

//************************************************************************************************
// ICredentialManager
/**	Credential management interface. 
	Note that calls might return kResultAccessDenied in case of missing permissions. */
//************************************************************************************************

interface ICredentialManager: IUnknown
{
	/** Add credential with data block, overrides existing. */
	virtual tresult CCL_API addCredential (StringRef targetName, StringRef userName, const Crypto::Block& data, StringRef groupName = nullptr) = 0;

	/** Add credential with password string, overrides existing. */
	virtual tresult CCL_API addPassword (StringRef targetName, StringRef userName, StringRef password, StringRef groupName = nullptr) = 0;

	/** Add alias for target name. */
	virtual tresult CCL_API addCredentialAlias (StringRef aliasName, StringRef targetName) = 0;

	/** Get credential by target name, has to be released by caller. */
	virtual tresult CCL_API getCredential (ICredential*& credential, StringRef targetName, StringRef groupName = nullptr) = 0;

	/** Remove credential with given target name. */
	virtual tresult CCL_API removeCredential (StringRef targetName) = 0;

	/** Suppress any user interaction, silently fail on errors. Returns old state. */
	virtual tbool CCL_API setSilentMode (tbool state) = 0;

	/** Resolve alias to target name. */
	virtual String CCL_API resolveName (StringRef name) const = 0;
	
	/** Set global group name. */
	virtual void CCL_API setGlobalGroupName (StringRef groupName) = 0;

	struct SilentMode
	{
		ICredentialManager& manager;
		tbool oldState;

		SilentMode (ICredentialManager& manager, bool state)
		: manager (manager),
		  oldState (manager.setSilentMode (state))
		{}

		~SilentMode ()
		{ manager.setSilentMode (oldState); }
	};

	DECLARE_IID (ICredentialManager)
};

DEFINE_IID (ICredentialManager, 0xb38a0d51, 0x7ad, 0x4d3c, 0xa9, 0x49, 0xd, 0xff, 0x43, 0x15, 0xee, 0x8d)

} // namespace Security
} // namespace CCL

#endif // _ccl_icredentialmanager_h
