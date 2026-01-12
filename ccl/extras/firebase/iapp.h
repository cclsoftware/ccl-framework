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
// Filename    : ccl/extras/firebase/iapp.h
// Description : Firebase App Interfaces
//
//************************************************************************************************

#ifndef _ccl_firebase_iapp_h
#define _ccl_firebase_iapp_h

#include "ccl/public/text/cclstring.h"

namespace CCL {
namespace Firebase {

//************************************************************************************************
// Firebase::AppOptions
//************************************************************************************************

struct AppOptions
{
	String apiKey;
	String projectId;
};

//************************************************************************************************
// Firebase::IApp
//************************************************************************************************

interface IApp: IUnknown
{
	/** Get options app was created with. */
	virtual const AppOptions& CCL_API getOptions () const = 0;

	DECLARE_IID (IApp)
};

DEFINE_IID (IApp, 0x5fb1014d, 0xc0f8, 0x4c74, 0xaa, 0xbf, 0xae, 0x33, 0xea, 0x65, 0x63, 0xeb)

} // namespace Firebase
} // namespace CCL

#endif // _ccl_firebase_iapp_h
