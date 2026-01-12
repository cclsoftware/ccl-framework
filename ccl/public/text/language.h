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
// Filename    : ccl/public/text/language.h
// Description : Language Definitions
//
//************************************************************************************************

#ifndef _ccl_language_h
#define _ccl_language_h

#include "ccl/public/text/cstring.h"

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Language Codes
/**	\ingroup ccl_text */
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace LanguageCode
{
	DEFINE_STRINGID (English,    "en")
	DEFINE_STRINGID (German,     "de")
	DEFINE_STRINGID (French,     "fr")
	DEFINE_STRINGID (Spanish,    "es")
	DEFINE_STRINGID (Italian,    "it")
	DEFINE_STRINGID (Portuguese, "pt")
	DEFINE_STRINGID (Japanese,   "ja")
	DEFINE_STRINGID (Chinese,    "zh")

	DEFINE_STRINGID (EnglishUS,  "en-US")
	DEFINE_STRINGID (GermanDE,   "de-DE")

	DEFINE_STRINGID (Neutral,    "neutral")
}

} // namespace CCL

#endif // _ccl_language_h
