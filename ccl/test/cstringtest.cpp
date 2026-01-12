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
// Filename    : cstringtest.cpp
// Description : Unit tests for C-String
//
//************************************************************************************************

#include "ccl/base/unittest.h"

#include "ccl/public/text/cstring.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (CStringTest, testCopyOnWrite)
{
	static const char* theCString = "Hello World!";

	MutableCString s1 = theCString;
	CCL_TEST_ASSERT (s1.str () != theCString); // address must be different!

	MutableCString s2 = s1;
	CCL_TEST_ASSERT (s1.str () == s2.str ()); // address must be equal!
	
	String unicodeString;
	s2.toUnicode (unicodeString);
	CCL_TEST_ASSERT (unicodeString == String (theCString));
	
	MutableCString s3 = unicodeString;
	CCL_TEST_ASSERT (s3 == s2);

	s2 += "123";
	CCL_TEST_ASSERT (s2 != theCString);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (CStringTest, testConstantString)
{
	const CString str1 = CSTR ("Hello World!");
	const CString str2 = CSTR ("Hello World!");
	CCL_TEST_ASSERT (str1.str () == str2.str ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (CStringTest, testSubString)
{
	const CString str1 = CSTR ("Hello World!");
	MutableCString str2 = str1.subString (1, 4);
	CCL_TEST_ASSERT (str2 == "ello");
}
