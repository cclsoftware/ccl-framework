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
// Filename    : ccl/text/strings/unicode-cross-platform/ucharfunctions.cpp
// Description : Platform-independent Unicode functions
//
//************************************************************************************************

#include "ccl/text/strings/unicode-cross-platform/ucharfunctions.h"

#include "core/public/coremacros.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// UTF16 Character Tables
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace UCharTables {

static const uchar charset_lowercase[] = 
{
	#include "charset-lowercase.inc"
};

static const uchar charset_uppercase[] = 
{
	#include "charset-uppercase.inc"
};

static const uchar charset_numeric[] = 
{
	#include "charset-numeric.inc"
};

static const uchar charset_whitespace[] = 
{
	#include "charset-whitespace.inc"
};

static const UCharMapping::Item ascii_encode[] =
{
	#include "ascii-encode.inc"
};

static const UCharMapping::Item latin1_encode[] =
{
	#include "latin1-encode.inc"
};

static const UCharMapping::Item cp437_encode[] =
{
	#include "cp437-encode.inc"
};

static const UCharMapping::Item cp437_decode[] =
{
	#include "cp437-decode.inc"
};

} // namespace UCharTables

//************************************************************************************************
// UCharFunctions
//************************************************************************************************

const UCharSet UCharFunctions::lowercaseCharacterSet (UCharTables::charset_lowercase, ARRAY_COUNT (UCharTables::charset_lowercase));
const UCharSet UCharFunctions::uppercaseCharacterSet (UCharTables::charset_uppercase, ARRAY_COUNT (UCharTables::charset_uppercase));
const UCharSet UCharFunctions::numericCharacterSet (UCharTables::charset_numeric, ARRAY_COUNT (UCharTables::charset_numeric));
const UCharSet UCharFunctions::whitespaceCharacterSet (UCharTables::charset_whitespace, ARRAY_COUNT (UCharTables::charset_whitespace));
const UCharMapping UCharFunctions::encodingTableASCII (UCharTables::ascii_encode, ARRAY_COUNT (UCharTables::ascii_encode));
const UCharMapping UCharFunctions::encodingTableLatin1 (UCharTables::latin1_encode, ARRAY_COUNT (UCharTables::latin1_encode));
const UCharMapping UCharFunctions::encodingTableCP437 (UCharTables::cp437_encode, ARRAY_COUNT (UCharTables::cp437_encode));
const UCharMapping UCharFunctions::decodingTableCP437 (UCharTables::cp437_decode, ARRAY_COUNT (UCharTables::cp437_decode));

//////////////////////////////////////////////////////////////////////////////////////////////////

int UCharFunctions::compareStringsNumerically (const uchar* a, const uchar* b, bool ignoreCase)
{
	struct CompareHelper
	{
		static int compareRight (const uchar* a, const uchar* b)
		{
			int bias = 0;
			for(;; a++, b++)
			{
				bool aIsDigit = isDigit (*a);
				bool bIsDigit = isDigit (*b);

				if(!aIsDigit && !bIsDigit)
					return bias;
				else if(!aIsDigit)
					return -1;
				else if(!bIsDigit)
					return +1;
				else if(*a < *b)
				{
					if(!bias)
						bias = -1;
				}
				else if(*a > *b)
				{
					if(!bias)
						bias = +1;
				}
				else if(!*a && !*b)
					return bias;
			}
			return 0;
		}

		static int compareLeft (const uchar* a, const uchar* b)
		{
			for(;; a++, b++)
			{
				bool aIsDigit = isDigit (*a);
				bool bIsDigit = isDigit (*b);

				if(!aIsDigit && !bIsDigit)
					return 0;
				else if(!aIsDigit)
					return -1;
				else if(!bIsDigit)
					return +1;
				else if(*a < *b)
					return -1;
				else if(*a > *b)
					return +1;
			}
			return 0;
		}
	};

	int aIndex = 0;
	int bIndex = 0;

	while(true)
	{
		uchar ca = a[aIndex];
		uchar cb = b[bIndex];

		while(isWhitespace (ca))
			ca = a[++aIndex];

		while(isWhitespace (cb))
			cb = b[++bIndex];

		if(isDigit (ca) && isDigit (cb))
		{
			bool fractional = (ca == '0' || cb == '0');

			int result = fractional ? CompareHelper::compareLeft (a + aIndex, b + bIndex)
									: CompareHelper::compareRight (a + aIndex, b + bIndex);
			if(result != 0)
				return result;
		}

		if(!ca && !cb)
			return 0;

		if(ignoreCase)
		{
			ca = toUppercase (ca);
			cb = toUppercase (cb);
		}

		if(ca < cb)
			return -1;

		if(ca > cb)
			return +1;

		++aIndex;
		++bIndex;
	}
}

