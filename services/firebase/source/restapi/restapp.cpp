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
// Filename    : restapp.cpp
// Description : Firebase App class using REST API
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "restapp.h"
#include "restauth.h"
#include "restfirestore.h"

#include "ccl/base/storage/attributes.h"
#include "ccl/base/storage/jsonarchive.h"
#include "ccl/public/base/istream.h"
#include "ccl/public/network/web/iwebrequest.h"

using namespace CCL;
using namespace Firebase;

//************************************************************************************************
// Firebase::RESTApp
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (RESTApp, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

RESTApp::RESTApp (const AppOptions& options, IClassAllocator* _allocator)
: options (options),
  auth (nullptr),
  firestore (nullptr)
{
	allocator.share (_allocator);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

RESTApp::~RESTApp ()
{
	safe_release (auth);
	safe_release (firestore);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IClassAllocator& RESTApp::getAllocator ()
{
	if(!allocator)
		allocator = NEW Attributes;
	return *allocator;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Auth::RESTAuth& RESTApp::getAuth ()
{
	if(!auth)
		auth = NEW Auth::RESTAuth (*this);
	return *auth;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Firestore::RESTFirestore& RESTApp::getFirestore ()
{
	if(!firestore)
		firestore = NEW Firestore::RESTFirestore (*this);
	return *firestore;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const AppOptions& CCL_API RESTApp::getOptions () const
{
	return options;
}

//************************************************************************************************
// Firebase::RESTOperation
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (RESTOperation, Web::AsyncXHROperation)

//////////////////////////////////////////////////////////////////////////////////////////////////

RESTOperation::RESTOperation (Web::IXMLHttpRequest* httpRequest)
: AsyncXHROperation (httpRequest)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RESTOperation::onHttpRequestFinished ()
{
	auto& jsonResult = getJsonResult ();
	if(auto stream = httpRequest->getResponseStream ())
		JsonUtils::parse (jsonResult, *stream);
	DateTime responseDate;
	if(auto responseHeaders = httpRequest->getAllResponseHeaders ())
		responseHeaders->parseDate (responseDate); // If this fails we keep the uninitialized DateTime object
	setResponseTimestamp (responseDate);

	#if DEBUG_LOG
	jsonResult.dump ();
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Attributes& RESTOperation::getJsonResult ()
{
	auto jsonResult = unknown_cast<Attributes> (getResult ().asUnknown ());
	if(!jsonResult)
	{
		jsonResult = NEW Attributes;
		setResult (Variant ().takeShared (jsonResult->asUnknown ()));
		jsonResult->release ();
	}
	return *jsonResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RESTOperation::hasError ()
{
	return getState () == kFailed || getJsonResult ().contains ("error");
}

//************************************************************************************************
// Firebase::RESTVoidOperation
//************************************************************************************************

void RESTVoidOperation::onHttpRequestFinished ()
{
	RESTOperation::onHttpRequestFinished ();
	if(!hasError ())
		setResult (Variant ()); // clear result if not an error
}
