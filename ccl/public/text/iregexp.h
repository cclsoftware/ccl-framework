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
// Filename    : ccl/public/text/iregexp.h
// Description : Regular Expression Interface
//
//************************************************************************************************

#ifndef _ccl_iregexp_h
#define _ccl_iregexp_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

//************************************************************************************************
// IRegularExpression
//************************************************************************************************

interface IRegularExpression: IUnknown
{
	/** Options. */
	enum Options
	{
		kCaseInsensitive = 1<<0,	///< perform case-insensitive match
		kMultiline = 1<<1,			///< make start/end metacharacters match at start/end of each line
		kDotMatchesAll = 1<<2		///< make dots match all characters, including line breaks
	};

	/** Construct with regular expression string. */
	virtual tresult CCL_API construct (StringRef expression, int options = 0) = 0;

	/** Check if expression matches the input string completely. */
	virtual tbool CCL_API isFullMatch (StringRef string) const = 0;

	/** Check if expression matches any substring of input string. */
	virtual tbool CCL_API isPartialMatch (StringRef string) const = 0;

	/** Replace first occurrence of expression in input string according to specified format.
		See http://ecma-international.org/ecma-262/5.1/#sec-15.5.4.11 for format spec. */
	virtual tbool CCL_API replace (String& string, StringRef format) const = 0;

	/** Replace all occurrences of expression in input string according to specified format.
		See http://ecma-international.org/ecma-262/5.1/#sec-15.5.4.11 for format spec. */
	virtual tbool CCL_API replaceAll (String& string, StringRef format) const = 0;

	DECLARE_IID (IRegularExpression)
};

DEFINE_IID (IRegularExpression, 0x809f43a7, 0x7ec6, 0x49f0, 0x80, 0x96, 0xe1, 0x6b, 0x5b, 0x4e, 0x56, 0xa6)

} // namespace CCL

#endif // _ccl_iregexp_h
