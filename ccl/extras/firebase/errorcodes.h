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
// Filename    : ccl/extras/firebase/errorcodes.h
// Description : Firestore Error Codes
//
//************************************************************************************************

#ifndef _ccl_firestore_errorcodes_h
#define _ccl_firestore_errorcodes_h

#include "ccl/public/text/cstring.h"
#include "ccl/public/text/cclstring.h"

namespace CCL {
namespace Firebase {
namespace Firestore {

//************************************************************************************************
// Firebase::Firestore::Error
//************************************************************************************************

struct Error
{
	int errorCode = 0;
	String message;
	MutableCString status;

	Error () {}
	Error (VariantRef result) { fromResult (result); }

	Error& fromResult (VariantRef result);
	void println ();
};

//************************************************************************************************
// Firebase::Firestore::ErrorID
//************************************************************************************************

namespace ErrorID
{
	const CStringPtr kNotFound = "NOT_FOUND";
}

} // namespace Firestore
} // namespace Firebase
} // namespace CCL

#endif // _ccl_firestore_errorcodes_h
