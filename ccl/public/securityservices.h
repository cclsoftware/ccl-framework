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
// Filename    : ccl/public/securityservices.h
// Description : Security Service APIs
//
//************************************************************************************************

#ifndef _ccl_securityservices_h
#define _ccl_securityservices_h

#include "ccl/public/cclexports.h"
#include "ccl/public/system/icryptor.h"
#include "ccl/public/security/icryptokeystore.h"
#include "ccl/public/security/iauthorizationmanager.h"
#include "ccl/public/security/icredentialmanager.h"

namespace CCL {
namespace System {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Security Service APIs
//////////////////////////////////////////////////////////////////////////////////////////////////

/** Get cryptographical key store singleton. */
CCL_EXPORT Security::Crypto::ICryptoKeyStore& CCL_API CCL_ISOLATED (GetCryptoKeyStore) ();
inline Security::Crypto::ICryptoKeyStore& GetCryptoKeyStore () { return CCL_ISOLATED (GetCryptoKeyStore) (); }

/** Get cryptographical factory singleton. */
CCL_EXPORT Security::Crypto::ICryptoFactory& CCL_API CCL_ISOLATED (GetCryptoFactory) ();
inline Security::Crypto::ICryptoFactory& GetCryptoFactory () { return CCL_ISOLATED (GetCryptoFactory) (); }

/** Get authorization manager singleton. */
CCL_EXPORT Security::IAuthorizationManager& CCL_API CCL_ISOLATED (GetAuthorizationManager) ();
inline Security::IAuthorizationManager& GetAuthorizationManager () { return CCL_ISOLATED (GetAuthorizationManager) (); }

/** Get credential manager singleton. */
CCL_EXPORT Security::ICredentialManager& CCL_API CCL_ISOLATED (GetCredentialManager) ();
inline Security::ICredentialManager& GetCredentialManager () { return CCL_ISOLATED (GetCredentialManager) (); }

/**	Generate name-based UUID (Type 5, RFC 4122) from name and namespace UUID.
	If 'nameSpace' is kNullUID, this will default to NameSpace_DNS to be used
	for a fully qualified domain name. */
CCL_EXPORT void CCL_API CCL_ISOLATED (CreateNameBasedUID) (UIDBytes& uid, StringRef name, UIDRef nameSpace);
inline void CreateNameBasedUID (UIDBytes& uid, StringRef name, UIDRef nameSpace) { CCL_ISOLATED (CreateNameBasedUID) (uid, name, nameSpace); }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace System
} // namespace CCL

#endif // _ccl_securityservices_h
