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
// Filename    : restfirebase.cpp
// Description : Firebase class using REST API
//
//************************************************************************************************

#include "restfirebase.h"
#include "restapp.h"
#include "restauth.h"
#include "restfirestore.h"

using namespace CCL;
using namespace Firebase;

//************************************************************************************************
// Firebase::RESTFirebaseStatics
//************************************************************************************************

DEFINE_CLASS (RESTFirebaseStatics, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

IApp* CCL_API RESTFirebaseStatics::createApp (const AppOptions& options, IClassAllocator* allocator)
{
	return NEW RESTApp (options, allocator);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Auth::IAuth* CCL_API RESTFirebaseStatics::getAuth (IApp* _app)
{
	auto* app = unknown_cast<RESTApp> (_app);
	return app ? &app->getAuth () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Firestore::IFirestore* CCL_API RESTFirebaseStatics::getFirestore (IApp* _app)
{
	auto* app = unknown_cast<RESTApp> (_app);
	return app ? &app->getFirestore () : nullptr;
}
