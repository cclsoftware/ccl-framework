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
// Filename    : restfirebase.h
// Description : Firebase class using REST API
//
//************************************************************************************************

#ifndef _restfirebase_h
#define _restfirebase_h

#include "ccl/base/object.h"

#include "ccl/extras/firebase/ifirebase.h"

namespace CCL {
namespace Firebase {

//************************************************************************************************
// Firebase::RESTFirebaseStatics
//************************************************************************************************

class RESTFirebaseStatics: public Object,
						   public IFirebaseStatics
{
public:
	DECLARE_CLASS (RESTFirebaseStatics, Object)

	// IFirebaseStatics
	IApp* CCL_API createApp (const AppOptions& options, IClassAllocator* allocator) override;
	Auth::IAuth* CCL_API getAuth (IApp* app) override;
	Firestore::IFirestore* CCL_API getFirestore (IApp* app) override;

	CLASS_INTERFACE (IFirebaseStatics, Object)
};

} // namespace Firebase
} // namespace CCL

#endif // _restfirebase_h
