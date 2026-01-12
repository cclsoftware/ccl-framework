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
// Filename    : ccl/public/text/cclstdstring.h
// Description : Standard String Conversion
//
//************************************************************************************************

#ifndef _ccl_cclstdstring_h
#define _ccl_cclstdstring_h

#include "ccl/public/text/cclstring.h"
#include "ccl/public/text/cstring.h"

#include <string>

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Standard String Conversion
//////////////////////////////////////////////////////////////////////////////////////////////////

/** Convert CCL::String (UTF-16) to std::string with given text encoding. */
inline std::string toStdString (StringRef string, TextEncoding encoding = Text::kUTF8)
{
	return static_cast<std::string> (MutableCString (string, encoding));
}

/** Convert std::string with given text encoding to CCL::String (UTF-16). */
inline String fromStdString (const std::string& string, TextEncoding encoding = Text::kUTF8)
{
	return String (encoding, string.c_str ());
}

} // namespace CCL

#endif // _ccl_cclstdstring_h
