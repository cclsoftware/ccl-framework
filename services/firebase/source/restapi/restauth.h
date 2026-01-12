//************************************************************************************************
//
// Firebase Service
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
// Filename    : restauth.h
// Description : Firebase Auth class using REST API
//
//************************************************************************************************

#ifndef _firebase_restauth_h
#define _firebase_restauth_h

#include "ccl/base/object.h"

#include "ccl/extras/firebase/iauth.h"

namespace CCL {
class Attributes;

namespace Firebase {
class RESTApp;

namespace Auth {
class RESTAuth;

//************************************************************************************************
// Firebase::Auth::RESTUser
//************************************************************************************************

class RESTUser: public Object,
				public IUser
{
public:
	DECLARE_CLASS_ABSTRACT (RESTUser, Object)

	RESTUser (RESTAuth& auth);

	PROPERTY_STRING (idToken, IDToken)
	PROPERTY_STRING (refreshToken, RefreshToken)
	PROPERTY_VARIABLE (int64, expirationDate, ExpirationDate)

	// IUser
	IAsyncOperation* CCL_API getToken (tbool forceRefresh = false) override;

	CLASS_INTERFACE (IUser, Object)

protected:
	/**how long before expiration does the idToken get refreshed? (in seconds)*/
	const static int kRefreshTokenSafetyInterval = 5;
	RESTAuth& auth;
};

//************************************************************************************************
// Firebase::Auth::RESTAuth
//************************************************************************************************

class RESTAuth: public Object,
				public IAuth
{
public:
	DECLARE_CLASS_ABSTRACT (RESTAuth, Object)

	RESTAuth (RESTApp& app);

	RESTUser* getUserInternal () const { return currentUser; }
	StringRef getUserIDToken () { return currentUser ? currentUser->getIDToken () : String::kEmpty; }

	IAsyncOperation* reauthenticate ();
	void onSignInCompleted (const Attributes& jsonResult);
	void onReauthenticationCompleted (const Attributes& jsonResult);
	void onDisconnect ();
	
	// IAuth
	IApp& CCL_API getApp () const override;
	IAsyncOperation* CCL_API signInWithCustomToken (StringRef customToken) override;
	void CCL_API signOut () override;
	IUser* CCL_API getCurrentUser () const override;

	CLASS_INTERFACE (IAuth, Object)
	
protected:
	class SignInOperation;
	class ReauthenticationOperation;

	RESTApp& app;
	SharedPtr<RESTUser> currentUser;
};

} // namespace Auth
} // namespace Firebase
} // namespace CCL

#endif // _firebase_restauth_h
