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
// Filename    : ccl/text/strings/unicodestringbuffer.impl.h
// Description : Implementation with platform-independent Unicode functions
//
//************************************************************************************************

int Text::convertToCString (char* cString, int cStringSize, TextEncoding encoding, const uchar* uString, int uStringLength)
{
	if(uStringLength == -1)
		uStringLength = getLength (uString);

	switch(encoding)
	{
	case kASCII :
		return UCharFunctions::encodeASCII (cString, cStringSize, uString, uStringLength);
	case kISOLatin1 :
	case kSystemEncoding : // use ISO Latin 1 as system encoding
		return UCharFunctions::encodeISOLatin1 (cString, cStringSize, uString, uStringLength);
	case kDOSLatinUS :
		return UCharFunctions::encodeDOSLatinUS (cString, cStringSize, uString, uStringLength);
	case kUTF8 :
		return UCharFunctions::encodeUTF8String (cString, cStringSize, uString, uStringLength);
	}

	ASSERT (0) // not implemented!
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Text::convertToUnicode (uchar* uString, int uStringSize, TextEncoding encoding, const char* cString, int cStringLength)
{
	if(cStringLength == -1)
		cStringLength = getLength (cString);

	switch(encoding)
	{
	case kASCII :
		return UCharFunctions::decodeASCII (uString, uStringSize, cString, cStringLength);
	case kISOLatin1 :
	case kSystemEncoding : // use ISO Latin 1 as system encoding
		return UCharFunctions::decodeISOLatin1 (uString, uStringSize, cString, cStringLength);
	case kDOSLatinUS :
		return UCharFunctions::decodeDOSLatinUS (uString, uStringSize, cString, cStringLength);
	case kUTF8 :
		return UCharFunctions::decodeUTF8String (uString, uStringSize, cString, cStringLength);
	}

	ASSERT (0) // not implemented!
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Text::compareStrings (const uchar* s1, int l1, const uchar* s2, int l2, int flags)
{
	if(flags & kCompareNumerically)
		return UCharFunctions::compareStringsNumerically (s1, s2, (flags & kIgnoreCase) != 0);

	if(l1 == -1)
		l1 = getLength (s1);
	if(l2 == -1)
		l2 = getLength (s2);

	return UCharFunctions::compareStrings (s1, l1, s2, l2, (flags & kIgnoreCase) != 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const uchar* Text::findString (const uchar* source, const uchar* value, int flags)
{
	int sourceLength = getLength (source);
	int valueLength = getLength (value);

	bool ignoreCase = (flags & kIgnoreCase) != 0;
	if(flags & kReverseFind)
		return UCharFunctions::findStringReverse (source, sourceLength, value, valueLength, ignoreCase);
	else
		return UCharFunctions::findString (source, sourceLength, value, valueLength, ignoreCase);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Text::toUppercase (uchar* s)
{
	UCharFunctions::toUppercase (s, getLength (s));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Text::toLowercase (uchar* s)
{
	UCharFunctions::toLowercase (s, getLength (s));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Text::capitalize (uchar* s)
{
	UCharFunctions::capitalize (s, getLength (s));
}
