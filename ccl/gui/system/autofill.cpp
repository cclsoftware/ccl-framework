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
// Filename    : ccl/gui/system/autofill.cpp
// Description : Autofill Support
//
//************************************************************************************************

#include "ccl/gui/system/autofill.h"

#define PLATFORM_AUTOFILLMANAGER_AVAILABLE CCL_PLATFORM_IOS // work in progress...

namespace CCL {

//************************************************************************************************
// NullAutofillManager
//************************************************************************************************	

class NullAutofillManager: public AutofillManager
{
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// NullAccessibilityManager
//************************************************************************************************	

#if !PLATFORM_AUTOFILLMANAGER_AVAILABLE
DEFINE_EXTERNAL_SINGLETON (AutofillManager, NullAutofillManager)
#endif

//************************************************************************************************
// IAutofillClient
//************************************************************************************************

DEFINE_IID_ (IAutofillClient, 0x6509140, 0xe559, 0x4cd0, 0xa7, 0x4e, 0xc2, 0xe2, 0x55, 0x6f, 0x2, 0x38)

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_STYLEDEF (IAutofillClient::types)
	{"none",		Styles::kAutofillTypeNone},
	{"username",	Styles::kAutofillTypeUsername},
	{"email",		Styles::kAutofillTypeEmail},
	{"password",	Styles::kAutofillTypePassword},
	{"newpassword",	Styles::kAutofillTypeNewPassword},
	{"firstname",	Styles::kAutofillTypeFirstName},
	{"lastname",	Styles::kAutofillTypeLastName},
	{"country",		Styles::kAutofillTypeCountry},
END_STYLEDEF

//************************************************************************************************
// AutofillManager
//************************************************************************************************

void AutofillManager::addClient (IAutofillClient* client)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AutofillManager::removeClient (IAutofillClient* client)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AutofillManager::updateClient (IAutofillClient* client)
{}
