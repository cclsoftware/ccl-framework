//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : urltest.cpp
// Description : Unit tests for Urls
//
//************************************************************************************************

#include "ccl/base/unittest.h"
#include "ccl/base/storage/url.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (UrlTest, testAbsolutePathDetection)
{
	// Absolute
	CCL_TEST_ASSERT (UrlUtils::isCanonicalAbsolutePath ("/folder/file.txt"));
	CCL_TEST_ASSERT (UrlUtils::isCanonicalAbsolutePath ("c:/folder/file.txt"));
	CCL_TEST_ASSERT (UrlUtils::isCanonicalAbsolutePath ("/c:/folder/file.txt"));
	CCL_TEST_ASSERT (UrlUtils::isCanonicalAbsolutePath ("c:\\folder\\file.txt"));

	// Relative
	CCL_TEST_ASSERT (UrlUtils::isCanonicalAbsolutePath ("file.txt") == false);
	CCL_TEST_ASSERT (UrlUtils::isCanonicalAbsolutePath ("folder/file.txt") == false);
	CCL_TEST_ASSERT (UrlUtils::isCanonicalAbsolutePath ("./file.txt") == false);
	CCL_TEST_ASSERT (UrlUtils::isCanonicalAbsolutePath ("../folder/file.txt") == false);
}
