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
// Filename    : ccl/extras/firebase/ifirebase.h
// Description : Firebase Interfaces
//
//************************************************************************************************

#ifndef _ccl_ifirebase_h
#define _ccl_ifirebase_h

#include "ccl/extras/firebase/iapp.h"
#include "ccl/extras/firebase/iauth.h"
#include "ccl/extras/firebase/ifirestore.h"

namespace CCL {
namespace Firebase {

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID
{
	DEFINE_CID (FirebaseStatics, 0x9dd8f2c1, 0x2ced, 0x42b1, 0xab, 0x14, 0x54, 0x5f, 0x9c, 0x80, 0x86, 0xc8)
}

//************************************************************************************************
// Firebase::IFirebaseStatics
//************************************************************************************************

interface IFirebaseStatics: IUnknown
{
	virtual IApp* CCL_API createApp (const AppOptions& options, IClassAllocator* allocator = nullptr) = 0;
	
	virtual Auth::IAuth* CCL_API getAuth (IApp* app) = 0;
	
	virtual Firestore::IFirestore* CCL_API getFirestore (IApp* app) = 0;

	DECLARE_IID (IFirebaseStatics)
};

DEFINE_IID (IFirebaseStatics, 0x8e12ca27, 0xda05, 0x4889, 0x9e, 0x18, 0x2e, 0x9a, 0xd1, 0x75, 0x42, 0xcc)

} // namespace Firebase
} // namespace CCL

#endif // _ccl_ifirebase_h
