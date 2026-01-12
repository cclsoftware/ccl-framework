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
// Filename    : ccl/public/plugins/versionnumber.cpp
// Description : Version Number
//
//************************************************************************************************

#include "ccl/public/plugins/versionnumber.h"

#include "ccl/public/text/cstring.h"

using namespace CCL;

//************************************************************************************************
// VersionNumber
//************************************************************************************************

String VersionNumber::print (Format format) const
{
	MutableCString cString;
	toCString (cString, format);
	return String (cString);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

VersionNumber& VersionNumber::scan (StringRef string)
{
	MutableCString cString (string);
	fromCString (cString);
	return *this;
}
