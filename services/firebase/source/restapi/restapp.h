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
// Filename    : restapp.h
// Description : Firebase App class using REST API
//
//************************************************************************************************

#ifndef _firebase_restapp_h
#define _firebase_restapp_h

#include "ccl/public/base/datetime.h"

#include "ccl/extras/web/webxhroperation.h"

#include "ccl/extras/firebase/iapp.h"

namespace CCL {
class Attributes;

namespace Firebase {

namespace Auth {
class RESTAuth; }

namespace Firestore {
class RESTFirestore; }

//************************************************************************************************
// Firebase::RESTApp
//************************************************************************************************

class RESTApp: public Object,
			   public IApp
{
public:
	DECLARE_CLASS_ABSTRACT (RESTApp, Object)
	
	RESTApp (const AppOptions& options, IClassAllocator* allocator);
	~RESTApp ();
	
	IClassAllocator& getAllocator ();
	Auth::RESTAuth& getAuth ();
	Firestore::RESTFirestore& getFirestore ();
	
	// IApp
	const AppOptions& CCL_API getOptions () const override;

	CLASS_INTERFACE (IApp, Object)
	
protected:
	AppOptions options;
	AutoPtr<IClassAllocator> allocator;
	Auth::RESTAuth* auth;
	Firestore::RESTFirestore* firestore;
};

//************************************************************************************************
// Firebase::RESTOperation
//************************************************************************************************

class RESTOperation: public Web::AsyncXHROperation
{
public:
	DECLARE_CLASS_ABSTRACT (RESTOperation, Web::AsyncXHROperation)

	RESTOperation (Web::IXMLHttpRequest* httpRequest);

	PROPERTY_OBJECT (DateTime, responseTimestamp, ResponseTimestamp)

	Attributes& getJsonResult ();
	bool hasError ();

	// AsyncXHROperation
	void onHttpRequestFinished () override;
};

//************************************************************************************************
// Firebase::RESTVoidOperation
//************************************************************************************************

class RESTVoidOperation: public RESTOperation
{
public:
	using RESTOperation::RESTOperation;

	// RESTOperation
	void onHttpRequestFinished () override;
};

} // namespace Firebase
} // namespace CCL

#endif // _firebase_restapp_h
