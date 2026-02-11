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
// Filename    : testsource.cpp
// Description : Test sources file for xstring functional test
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "some/include.h"

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("Test strings")
	XSTRING (First, "First string")
	XSTRING (Second, "Second string")
	XSTRING (Third, "Third string")
END_XSTRINGS

namespace CCL {

//************************************************************************************************
// SomeClass
//************************************************************************************************

class SomeClass
{
};

}



