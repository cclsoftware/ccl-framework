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
// Filename    : ccl/public/security/iauthorizationmanager.h
// Description : Authorization Manager Interface
//
//************************************************************************************************

#ifndef _ccl_iauthorizationmanager_h
#define _ccl_iauthorizationmanager_h

#include "ccl/public/text/cstring.h"
#include "ccl/public/text/cclstring.h"

namespace CCL {
interface IStream;
interface IUnknownIterator;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Authorization Signals
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Signals
{
	DEFINE_STRINGID (kAuthorization, "Authorization")
	
		DEFINE_STRINGID (kAuthorizationPolicyChanged, "AuthorizationPolicyChanged")
}

namespace Security {

namespace Authorization {
interface IPolicyItem; }

//************************************************************************************************
// KeyID
//************************************************************************************************

/** Common key identifiers. */
namespace KeyID
{
	DEFINE_STRINGID (kApplication, "application")
	DEFINE_STRINGID (kExtensions, "extensions")
	DEFINE_STRINGID (kDeveloper, "developer")
	DEFINE_STRINGID (kSubscription, "subscription")
	DEFINE_STRINGID (kAuthPolicy, "authpolicy")
	DEFINE_STRINGID (kCredentials, "credentials")
}

//************************************************************************************************
// IAuthorizationManager
//************************************************************************************************

interface IAuthorizationManager: IUnknown
{
	//////////////////////////////////////////////////////////////////////////////////////////////
	// Security Identifiers
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Set security identifiers of hosting application. Can be called only once. */
	virtual tresult CCL_API setAppIdentity (StringRef appSid, StringRef appSubSid = nullptr) = 0;

	/** Get primary security identifier of hosting application. */
	virtual StringRef CCL_API getAppIdentity () const = 0;

	/** Get secondary security identifier of hosting application (can be empty). */
	virtual StringRef CCL_API getAppSubIdentity () const = 0;

	DEFINE_ENUM (ComputerIDType)
	{
		kDefaultComputerID = 0,
		kComputerIDv1,
		kComputerIDv2
	};

	/** Get hardware hash identifying the local computer. */
	virtual tresult CCL_API getComputerIdentity (String& computerSid, ComputerIDType type = kDefaultComputerID) = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Authorization Policy
	//////////////////////////////////////////////////////////////////////////////////////////////

	enum PolicyMode 
	{
		kSignedPolicy = 0,
		kUnsignedPolicy = 1<<0 
	};

	/**	Load authorization policy from stream. 
		Host authorization policy needs to be encrypted and signed with KeyID::kAuthPolicy from key store. 
		Additonal policy snippets can be unsigned (kUnsignedPolicy mode). */
	virtual tresult CCL_API loadPolicy (IStream& stream, int mode = 0) = 0;

	/** Get privileges of given resource based on the active authorization policy. */
	virtual Authorization::IPolicyItem* CCL_API getPrivileges (StringRef resourceSid) = 0;

	/** Create iterator for data associated with the active authorization policy. */
	virtual IUnknownIterator* CCL_API createIterator (StringRef dataSid) = 0;

	/** Revoke given snippet from policy. Note that this might revoke more than expeced in case items have been merged.  */
	virtual tresult CCL_API revokePolicy (IStream& stream, int mode = 0) = 0;

	DECLARE_IID (IAuthorizationManager)
};

DEFINE_IID (IAuthorizationManager, 0x2f87bd9b, 0x30e2, 0x45e6, 0xb5, 0x5d, 0x5, 0xa3, 0xbe, 0x46, 0xa, 0xdc)

} // namespace Security
} // namespace CCL

#endif // _ccl_iauthorizationmanager_h
