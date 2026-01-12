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
// Filename    : restauth.cpp
// Description : Firebase App class using REST API
//
//************************************************************************************************

#include "restauth.h"
#include "restapp.h"

#include "ccl/base/message.h"
#include "ccl/base/storage/url.h"
#include "ccl/base/storage/attributes.h"
#include "ccl/base/storage/jsonarchive.h"

#include "ccl/public/base/istream.h"
#include "ccl/public/base/memorystream.h"
#include "ccl/public/text/istringdict.h"
#include "ccl/public/network/web/httpstatus.h"
#include "ccl/public/network/web/iwebrequest.h"
#include "ccl/public/network/web/iwebservice.h"
#include "ccl/public/plugservices.h"

using namespace CCL;
using namespace Web;
using namespace Firebase;
using namespace Auth;

//************************************************************************************************
// Firebase::Auth::RESTAuth::SignInOperation
//************************************************************************************************

class RESTAuth::SignInOperation: public RESTOperation
{
public:
	SignInOperation (RESTAuth& auth, IXMLHttpRequest* httpRequest)
	: RESTOperation (httpRequest),
	  auth (auth)
	{}

	void onHttpRequestFinished () override
	{
		RESTOperation::onHttpRequestFinished ();
		
		if(!hasError ())
			auth.onSignInCompleted (getJsonResult ());

		setResult (Variant ().takeShared (auth.getCurrentUser ()));
	}

protected:
	RESTAuth& auth;
};

//************************************************************************************************
// Firebase::Auth::RESTAuth::ReauthenticationOperation
//************************************************************************************************

class RESTAuth::ReauthenticationOperation: public RESTOperation
{
public:
	ReauthenticationOperation (RESTAuth& auth, IXMLHttpRequest* httpRequest)
	: RESTOperation (httpRequest),
	  auth (auth)
	{}

	void onHttpRequestFinished () override
	{
		RESTOperation::onHttpRequestFinished ();
		
		if(!hasError ())
			auth.onReauthenticationCompleted (getJsonResult ());

		setResult (Variant ().takeShared (auth.getCurrentUser ()));
	}

protected:
	RESTAuth& auth;
};

//************************************************************************************************
// Firebase::Auth::RESTAuth
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (RESTAuth, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

RESTAuth::RESTAuth (RESTApp& app)
: app (app)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* RESTAuth::reauthenticate ()
{
	Url url ("https://securetoken.googleapis.com/v1/token");
	url.getParameters ().setEntry ("key", getApp ().getOptions ().apiKey);

	auto request = ccl_new<IXMLHttpRequest> (ClassID::XMLHttpRequest);
	request->open (HTTP::kPOST, url);
	request->setRequestHeader (Meta::kContentType, Meta::kFormContentType);

	AutoPtr<IMemoryStream> data (NEW MemoryStream);
	MutableCString parameters ("grant_type=refresh_token&refresh_token=");
	data->write (parameters.str (), parameters.length ());
	MutableCString token (currentUser->getRefreshToken ());
	data->write (token.str (), token.length ());
	request->send (static_cast<IStream*> (data));

	auto operation = NEW ReauthenticationOperation (*this, request);
	operation->setState (IAsyncInfo::kStarted);
	return operation;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RESTAuth::onSignInCompleted (const Attributes& jsonResult)
{
	if(jsonResult.isEmpty ())
		return;
	ASSERT (!currentUser)
	AutoPtr<RESTUser> user = NEW RESTUser (*this);
	user->setIDToken (jsonResult.getString ("idToken"));
	user->setRefreshToken (jsonResult.getString ("refreshToken"));
	int expiresIn = jsonResult.getVariant ("expiresIn").parseInt ();
	user->setExpirationDate (UnixTime::getTime () + expiresIn);
	currentUser = user;
	signal (Message (kAuthStateChanged));
	signal (Message (kIDTokenChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RESTAuth::onReauthenticationCompleted (const Attributes& jsonResult)
{
	ASSERT (currentUser)
	currentUser->setIDToken (jsonResult.getString ("id_token"));
	currentUser->setRefreshToken (jsonResult.getString ("refresh_token"));
	int expiresIn = jsonResult.getVariant ("expires_in").parseInt ();
	currentUser->setExpirationDate (UnixTime::getTime () + expiresIn);
	signal (Message (kIDTokenChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RESTAuth::onDisconnect ()
{
	currentUser = nullptr;
	signal (Message (kAuthStateChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IApp& CCL_API RESTAuth::getApp () const
{
	return app;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* CCL_API RESTAuth::signInWithCustomToken (StringRef customToken)
{
	Attributes a;
	a.set ("token", customToken);
	a.set ("returnSecureToken", true, Variant::kBoolFormat);
	AutoPtr<IStream> jsonData = JsonUtils::serialize (a);

	Url url ("https://identitytoolkit.googleapis.com/v1/accounts:signInWithCustomToken");
	url.getParameters ().setEntry ("key", app.getOptions ().apiKey);

	auto request = ccl_new<IXMLHttpRequest> (ClassID::XMLHttpRequest);
	request->open (HTTP::kPOST, url);
	request->setRequestHeader (Meta::kContentType, JsonArchive::kMimeType);
	request->send (static_cast<IStream*> (jsonData));

	auto operation = NEW SignInOperation (*this, request);
	operation->setState (IAsyncInfo::kStarted);
	return operation;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API RESTAuth::signOut ()
{
	if(currentUser)
	{
		currentUser.release ();
		signal (Message (kAuthStateChanged));
		signal (Message (kIDTokenChanged));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUser* CCL_API RESTAuth::getCurrentUser () const
{
	return currentUser;
}

//************************************************************************************************
// Firebase::Auth::RESTUser
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (RESTUser, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

RESTUser::RESTUser (RESTAuth& auth)
: auth (auth),
  expirationDate (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* CCL_API RESTUser::getToken (tbool forceRefresh)
{
	if(expirationDate + kRefreshTokenSafetyInterval < UnixTime::getTime () || forceRefresh)
	{
		// what if reauthentication is already in progress?
		auto result = Promise (auth.reauthenticate ()).then ([this] (IAsyncOperation& op)
		{
			if(op.getState () != IAsyncOperation::kCompleted)
			{
				if(expirationDate < UnixTime::getTime ())
					auth.onDisconnect ();
			}
		});
		result.then ([this] (IAsyncOperation& op)
		{
			op.setResult (getIDToken ());
		});
		return return_shared<IAsyncOperation> (result);
	}
	return AsyncOperation::createCompleted (idToken);
}
