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
// Filename    : ccl/extras/firebase/errorcodes.cpp
// Description : Firestore Error Codes
//
//************************************************************************************************

#include "ccl/extras/firebase/errorcodes.h"

#include "ccl/public/storage/iattributelist.h"
#include "ccl/public/base/debug.h"

using namespace CCL;
using namespace Firebase;
using namespace Firestore;

//************************************************************************************************
// Firebase::Firestore::Error
//************************************************************************************************

Error& Error::fromResult (VariantRef result)
{
	if(UnknownPtr<IAttributeList> resultAttr = result.asUnknown ())
		if(UnknownPtr<IAttributeList> errorAttr = AttributeReadAccessor (*resultAttr).getUnknown ("error"))
		{
			AttributeReadAccessor reader (*errorAttr);
			errorCode = reader.getInt ("code");
			message = reader.getString ("message");
			status = reader.getString ("status");
		}
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Error::println ()
{
	Debugger::printf ("Firestore Error %s : %s\n", status.str (), MutableCString (message).str ());
}
