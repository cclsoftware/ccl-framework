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
// Filename    : ccl/extras/firebase/iauth.h
// Description : Firebase Auth Interfaces
//
//************************************************************************************************

#ifndef _ccl_firebase_iauth_h
#define _ccl_firebase_iauth_h

#include "ccl/public/text/cstring.h"
#include "ccl/public/text/cclstring.h"
#include "ccl/public/base/iasyncoperation.h"

namespace CCL {
namespace Firebase {
interface IApp;

namespace Auth {
interface IUser;

//************************************************************************************************
// Firebase::Auth::IAuth
//************************************************************************************************

interface IAuth: IUnknown
{
	/** Get app this object belongs to. */
	virtual IApp& CCL_API getApp () const = 0;

	/** Result: IUser. */
	virtual IAsyncOperation* CCL_API signInWithCustomToken (StringRef customToken) = 0;
	
	/** Removes existing authentication credentials. */
	virtual void CCL_API signOut () = 0;
	
	/** Get cached current user. */
	virtual IUser* CCL_API getCurrentUser () const = 0;
		
	DECLARE_STRINGID_MEMBER (kAuthStateChanged)
	DECLARE_STRINGID_MEMBER (kIDTokenChanged)

	DECLARE_IID (IAuth)
};

DEFINE_IID (IAuth, 0xed97e5d1, 0x2631, 0x4d58, 0x93, 0x48, 0xe9, 0x7b, 0x81, 0xfc, 0xb6, 0x40)
DEFINE_STRINGID_MEMBER (IAuth, kAuthStateChanged, "authStateChanged")
DEFINE_STRINGID_MEMBER (IAuth, kIDTokenChanged, "idTokenChanged")

//************************************************************************************************
// Firebase::Auth::IUser
//************************************************************************************************

interface IUser: IUnknown
{
	/** Result: token string. */
	virtual IAsyncOperation* CCL_API getToken (tbool forceRefresh = false) = 0;
	
	DECLARE_IID (IUser)
};

DEFINE_IID (IUser, 0x989bf80d, 0x258d, 0x4b30, 0xa7, 0xf6, 0x11, 0xf3, 0x8e, 0xc6, 0xbe, 0x61)

} // namespace Auth
} // namespace Firebase
} // namespace CCL

#endif // _ccl_firebase_iauth_h
