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
// Filename    : ccl/extras/web/cognito.h
// Description : Cognito (Amazon Cognito Authentication Service)
//
//************************************************************************************************

#ifndef _ccl_cognito_h
#define _ccl_cognito_h

#include "ccl/public/text/cstring.h"
#include "ccl/public/text/cclstring.h"

namespace CCL {   

class Attributes;
interface IAsyncOperation;

namespace Web {

interface IXMLHttpRequest;
class OAuth2Tokens;

//************************************************************************************************
// Cognito
//************************************************************************************************

namespace Cognito
{
	const CString kTokenType = "cognito";

	const String kRegionUSEast = "us-east-1";
	
	// Errors
	const String kUsernameExists = "UsernameExistsException";
	const String kNotAuthorized = "NotAuthorizedException";
	const String kUserNotFound = "UserNotFoundException";

	/**
		Initiate sign-in for user in the Amazon Cognito user directory.
		Action is InitiateAuth, AuthFlow is USER_PASSWORD_AUTH.
	*/
	IXMLHttpRequest* createSignInRequest (StringRef region, 
										  StringRef clientId, 
										  StringRef userName,
										  StringRef password);

	/**
		Initiate secure sign-in for user in the Amazon Cognito user directory.
		Action is InitiateAuth, AuthFlow is USER_SRP_AUTH.
	*/
	IAsyncOperation* signInSRP (StringRef region, 
								StringRef clientId, 
								StringRef userName,
								StringRef password, 
								StringRef userPoolId);

	/** 
		Request new ID and access tokens for given refresh token.
		Action is InitiateAuth, AuthFlow is REFRESH_TOKEN_AUTH.
	*/
	IXMLHttpRequest* createTokenRefreshRequest (StringRef region,
												StringRef clientId, 
												StringRef refreshToken);

	/**
		Load ID, access, and refresh token from sign-in or refresh response. 
	*/
	bool loadAuthenticationResult (OAuth2Tokens& tokens,
								   IStream& jsonStream,
								   int64 timestamp);

	/**
		Get user attributes for the currently signed-in user. 
		Action is GetUser.
	*/
	IXMLHttpRequest* createGetUserRequest (StringRef region,
										   StringRef accessToken);

	/**
		Load user attributes from response. 
	*/
	bool loadGetUserResponse (String& userName,
							  Attributes& userAttributes,
							  IStream& jsonStream);

	/**
		Register new user with an app client in the Amazon Cognito user directory.
		Action is SignUp.
	*/
	IXMLHttpRequest* createSignUpRequest (StringRef region,
										  StringRef clientId, 
										  StringRef userName, 
										  StringRef password, 
										  const Attributes& userAttributes);

	/**
		Load error type.
	*/
	bool loadErrorType (String& errorType, IStream& jsonStream);
}

} // namespace Web
} // namespace CCL

#endif // _ccl_cognito_h
