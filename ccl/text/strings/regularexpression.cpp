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
// Filename    : ccl/text/strings/regularexpression.cpp
// Description : Regular Expression
//
//************************************************************************************************

#include "ccl/text/strings/regularexpression.h"

#include "ccl/public/text/cclstring.h"

#include "pcre2.h"

using namespace CCL;

//************************************************************************************************
// RegularExpression
//************************************************************************************************

RegularExpression::RegularExpression ()
: reFull (nullptr),
  rePartial (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

RegularExpression::~RegularExpression ()
{
	cleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RegularExpression::cleanup ()
{
	if(reFull)
		pcre2_code_free ((pcre2_code*)reFull);
	reFull = nullptr;
	if(rePartial)
		pcre2_code_free ((pcre2_code*)rePartial);
	rePartial = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API RegularExpression::construct (StringRef string, int options)
{
	cleanup ();

	int opt = PCRE2_UTF;
	if(options & kCaseInsensitive)
		opt |= PCRE2_CASELESS;
	if(options & kMultiline)
		opt |= PCRE2_MULTILINE;
	if(options & kDotMatchesAll)
		opt |= PCRE2_DOTALL;

	int errorCode = 0;
	size_t errorOffset = 0;
	rePartial = pcre2_compile ((PCRE2_SPTR)(const uchar*)StringChars (string), PCRE2_ZERO_TERMINATED, opt, &errorCode, &errorOffset, nullptr);
	if(rePartial)
		reFull = pcre2_compile ((PCRE2_SPTR)(const uchar*)StringChars (String ("(?:").append (string).append (")\\z")), PCRE2_ZERO_TERMINATED, opt, &errorCode, &errorOffset, nullptr);
	else
		return kResultFailed;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API RegularExpression::isFullMatch (StringRef string) const
{
	ASSERT (rePartial != nullptr) // yes, check rePartial here and reFull below
	if(reFull == nullptr)
		return false;

	pcre2_match_data* matchData = pcre2_match_data_create_from_pattern ((pcre2_code*)reFull, nullptr);
	int result = pcre2_match ((pcre2_code*)reFull, (PCRE2_SPTR)(const uchar*)StringChars (string), string.length (), 0, 0, matchData, nullptr);
	pcre2_match_data_free (matchData);
	if(result < 0)
		return false;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API RegularExpression::isPartialMatch (StringRef string) const
{
	ASSERT (rePartial != nullptr)
	if(rePartial == nullptr)
		return false;

	pcre2_match_data* matchData = pcre2_match_data_create_from_pattern ((pcre2_code*)rePartial, nullptr);
	int result = pcre2_match ((pcre2_code*)rePartial, (PCRE2_SPTR)(const uchar*)StringChars (string), string.length (), 0, 0, matchData, nullptr);
	pcre2_match_data_free (matchData);
	if(result < 0)
		return false;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API RegularExpression::replace (String& string, StringRef format) const
{
	return replace (string, format, false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API RegularExpression::replaceAll (String& string, StringRef format) const
{
	return replace (string, format, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RegularExpression::replace (String& string, StringRef format, bool all) const
{
	ASSERT (rePartial != nullptr)
	if(rePartial == nullptr)
		return false;

	// string constants used during replace
	static StringRef strPlaceholderPre = CCLSTR ("$`");
	static StringRef strPlaceholderPost = CCLSTR ("$'");
	static StringRef strPlaceholderMatch = CCLSTR ("$&");
	static StringRef strDollar = CCLSTR ("$");
	static StringRef strEscapeDollar = CCLSTR ("$$");
	static StringRef strTemporaryDollar = CCLSTR ("$^");

	// process string in a loop, one match at a time
	String output;
	int position = 0;
	int length = string.length ();
	pcre2_match_data* matchData = pcre2_match_data_create_from_pattern ((pcre2_code*)rePartial, nullptr);
	do
	{
		int result = pcre2_match ((pcre2_code*)rePartial, (PCRE2_SPTR)(const uchar*)StringChars (string), length, position, 0, matchData, nullptr);
		if(result < -1)
		{
			pcre2_match_data_free (matchData);
			return false;
		}

		// no additional matches
		if(result == PCRE2_ERROR_NOMATCH)
			break;

		// get match offsets
		PCRE2_SIZE* offsets = pcre2_get_ovector_pointer (matchData);

		// helper to get n-th match
		auto match = [&] (int n)
		{
			String match = string.subString (int (offsets[2 * n]), int (offsets[2 * n + 1] - offsets[2 * n]));
			match.replace (strDollar, strTemporaryDollar);
			return match;
		};

		// process matches, replacing placeholders
		String replaced = format;
		replaced.replace (strEscapeDollar, strTemporaryDollar);

		for(int i = 1; i < result; i++) // $nn
			replaced.replace (String (strDollar).appendIntValue (i, 2), match (i));

		for(int i = 1; i < ccl_min (result, 10); i++) // $n
			replaced.replace (String (strDollar).appendIntValue (i), match (i));

		replaced.replace (strPlaceholderPre, string.subString (0, int (offsets[0])));
		replaced.replace (strPlaceholderPost, string.subString (int (offsets[1])));
		replaced.replace (strPlaceholderMatch, match (0));
		replaced.replace (strTemporaryDollar, strDollar);

		output.append (string.subString (position, int (offsets[0] - position)));
		output.append (replaced);

		position = int (offsets[1]);
	}
	while(all && position < length);

	pcre2_match_data_free (matchData);

	// append rest of input after matching is complete
	output.append (string.subString (position));

	string = output;
	return true;
}
