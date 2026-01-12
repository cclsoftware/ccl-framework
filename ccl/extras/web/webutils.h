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
// Filename    : ccl/extras/web/webutils.h
// Description : Web Utilities
//
//************************************************************************************************

#ifndef _ccl_webutils_h
#define _ccl_webutils_h

#include "ccl/public/text/cclstring.h"

namespace CCL {
namespace Web {

//************************************************************************************************
// WebUtils
//************************************************************************************************

namespace WebUtils
{
	bool isValidEmail (StringRef email);

	enum PasswordFlags
	{
		kPasswordNumbers = 1<<0,
		kPasswordUppercaseLetters = 1<<1,
		kPasswordLowercaseLetters = 1<<2
	};

	bool isValidPassword (StringRef password, int flags, int minLength = 0);
}

} // namespace Web
} // namespace CCL

#endif // _ccl_webutils_h
