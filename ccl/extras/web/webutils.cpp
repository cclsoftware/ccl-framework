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
// Filename    : ccl/extras/web/webutils.cpp
// Description : Web Utilities
//
//************************************************************************************************

#include "ccl/extras/web/webutils.h"

#include "ccl/public/text/iregexp.h"
#include "ccl/public/base/primitives.h"
#include "ccl/public/systemservices.h"

using namespace CCL;
using namespace Web;

//************************************************************************************************
// WebUtils
//************************************************************************************************

bool WebUtils::isValidEmail (StringRef email)
{
	AutoPtr<IRegularExpression> emailRegExp = System::CreateRegularExpression ();
	emailRegExp->construct ("^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$");
	return emailRegExp->isFullMatch (email);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WebUtils::isValidPassword (StringRef password, int flags, int minLength)
{
	if(password.length () < minLength)
		return false;

	if(get_flag<int> (flags, kPasswordNumbers))
	{
		AutoPtr<IRegularExpression> numberRegExp = System::CreateRegularExpression ();
		numberRegExp->construct ("[0-9]");
		if(!numberRegExp->isPartialMatch (password))
			return false;
	}

	if(get_flag<int> (flags, kPasswordUppercaseLetters))
	{
		AutoPtr<IRegularExpression> upperCaseStringExp = System::CreateRegularExpression ();
		upperCaseStringExp->construct ("[A-Z]");
		if(!upperCaseStringExp->isPartialMatch (password))
			return false;
	}

	if(get_flag<int> (flags, kPasswordLowercaseLetters))
	{
		AutoPtr<IRegularExpression> lowerCaseStringExp = System::CreateRegularExpression ();
		lowerCaseStringExp->construct ("[a-z]");
		if(!lowerCaseStringExp->isPartialMatch (password))
			return false;
	}
	
	return true;
}
