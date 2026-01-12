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
// Filename    : ccl/public/text/stringbuilder.cpp
// Description : String Builder
//
//************************************************************************************************

#include "ccl/public/text/stringbuilder.h"

#include "ccl/public/base/uid.h"
#include "ccl/public/base/primitives.h"

using namespace CCL;

//************************************************************************************************
// WideCharString
//************************************************************************************************

#if CCL_UCHAR_COMPATIBLE_WITH_WCHAR_T
WideCharString::WideCharString (StringRef string)
: chars (string)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

WideCharString::~WideCharString ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

const wchar_t* WideCharString::str () const
{
	return static_cast<const uchar*> (chars);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

#else
WideCharString::WideCharString (StringRef string)
{
	StringChars chars (string);
	int length = string.length ();
	buffer = NEW wchar_t[length + 1];
	for(int i = 0; i < length; i++)
		buffer[i] = chars[i];
	buffer[length] = '\0';
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WideCharString::~WideCharString ()
{
	delete [] buffer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const wchar_t* WideCharString::str () const
{
	return buffer;
}
#endif // !CCL_UCHAR_COMPATIBLE_WITH_WCHAR_T

//************************************************************************************************
// UIDString
//************************************************************************************************

String UIDString::generate ()
{
	UID uid;
	bool result = uid.generate ();
	ASSERT (result == true)
	return UIDString (uid);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UIDString::verify (StringRef uidString)
{
	UID uid;
	uid.fromString (uidString);
	return uid.isValid ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UIDString::UIDString (UIDRef uid)
{
	UID (uid).toString (*this);
}

//************************************************************************************************
// UIDCString
//************************************************************************************************

MutableCString UIDCString::generate ()
{
	UID uid;
	bool result = uid.generate ();
	ASSERT (result == true)
	return UIDCString (uid);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UIDCString::UIDCString (UIDRef uid)
{
	UID (uid).toCString (*this);
}

//************************************************************************************************
// FourCCString
//************************************************************************************************

FourCCString::FourCCString (uint32 fourCC)
{
	appendFourCC (fourCC);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uint32 FourCCString::getFourCC () const
{
	int count = ccl_min (4, length ());
	StringChars chars (*this);

	uint32 result = 0;
	for(int i = 0; i < count; i++)
	{
		result <<= 8;
		result |= (chars[i] & 0xff);
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FourCCString::appendFourCC (uint32 fourCC)
{
	fourCC = MAKE_BIG_ENDIAN (fourCC);
	char* ptr = reinterpret_cast<char*>(&fourCC);

	StringWriter<4> writer (*this, false);
	for(int i = 0; i < 4; i++)
		if(ptr[i] != '\0')
			writer.append (ptr[i]);

	writer.flush ();
}

//************************************************************************************************
// StringBuilder
//************************************************************************************************

StringBuilder::StringBuilder (String& string)
: string (string),
  itemSeparator (CCLSTR ("\n")),
  moreItemsMarker (CCLSTR ("...")),
  maxItems (20),
  numItems (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StringBuilder::isLimitReached () const
{
	return numItems >= maxItems;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringBuilder::addItem (StringRef text)
{
	if(numItems <= maxItems)
	{
		if(numItems == maxItems)
			string << itemSeparator << moreItemsMarker;
		else
		{
			if(numItems > 0)
				string << itemSeparator;

			string << text;
		}
	}
	numItems++;
}

//************************************************************************************************
// StringUtils
//************************************************************************************************

bool StringUtils::getLastIntValue (StringRef string, int64& value, String* prefix, StringRef delimiters)
{
	int length = string.length ();
	if(Unicode::isDigit (string.at (length - 1)))
	{
		// ends with digit: find index of first digit
		int firstDigitIndex = length - 1;
		while(firstDigitIndex > 0 && Unicode::isDigit (string.at (firstDigitIndex - 1)))
			firstDigitIndex--;

		int prefixLength = firstDigitIndex;

		if(!delimiters.isEmpty ())
		{
			// previous character must match any of the delimiters
			String prevCharacter (string.subString (firstDigitIndex - 1, 1));
			if(!delimiters.contains (prevCharacter))
				return false;

			prefixLength--;
		}

		String number (string.subString (firstDigitIndex));
		int64 n;
		if(number.getIntValue (n))
		{
			value = n;
			if(prefix)
				*prefix = string.subString (0, prefixLength);

			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StringUtils::getLastIntValue (StringRef string, int& value, String* prefix, StringRef delimiters)
{
	int64 v = -1;
	bool result = getLastIntValue (string, v, prefix, delimiters);
	if(result)
		value = (int)v;
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StringUtils::isDigitsOnly (StringRef string)
{
	int length = string.length ();
	if(length == 0)
		return false;

	StringChars chars (string);
	for(int i = 0; i < length; i++)
		if(Unicode::isDigit (chars[i]) == false)
			return false;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String StringUtils::strip (StringRef string, bool (*filter) (uchar c))
{
	String result;
	StringWriter<64> resultWriter (result);
	StringChars chars (string);
	const uchar* stringPtr = chars;
	for(int i = 0; stringPtr[i] != 0; i++)
	{
		if(filter (stringPtr[i]))
			resultWriter.append (stringPtr[i]);
	}
	resultWriter.flush ();
	return result;
}

//************************************************************************************************
// StringUtils::IndexedNameBuilder
//************************************************************************************************

StringUtils::IndexedNameBuilder::IndexedNameBuilder (StringRef requestedName, StringRef originalName, int startNumber, StringRef separator)
: separator (separator),
  number (-1),
  useBrackets (false)
{
	if(!originalName.isEmpty () && requestedName.startsWith (originalName))
	{
		int originalNameNumber = -1;
		if(getLastIntValue (originalName, originalNameNumber, nullptr, String::kEmpty))
		{
			// original name ends with a number (e.g. year): avoid counting it up, append index in brackets instead
			stemName = originalName;
			number = startNumber;
			useBrackets = true;

			// try to extract a trailing number in brackets
			String remainder (requestedName.subString (originalName.length ()));

			int bracketIndex = remainder.lastIndex (")");
			if(bracketIndex >= 0)
				if(getLastIntValue (remainder.truncate (remainder.length () - 1), number))
					number++;
		}
	}

	if(number < 0)
	{
		// default: no brackets
		stemName = requestedName;
		number = startNumber;

		// try to extract a trailing number
		if(getLastIntValue (requestedName, number, &stemName, separator))
			number++;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringUtils::IndexedNameBuilder::nextName (String& name)
{
	name.empty ();

	if(useBrackets)
		name << stemName << separator << "(" << number++ << ")";
	else
		name << stemName << separator << number++;
}

//************************************************************************************************
// StringParser
//************************************************************************************************

const String StringParser::kWhitespace (" \t");

//////////////////////////////////////////////////////////////////////////////////////////////////

StringParser::StringParser (StringRef string)
: string (string), 
  position (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StringParser::advance (int numChars)
{
	position += numChars;
	ASSERT (position <= string.length ())
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StringParser::isEndOfString () const
{
	if(position >= string.length ())
	{
		ASSERT (position == string.length ())
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uchar StringParser::peek () const
{
	return string[position];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uchar StringParser::read ()
{
	uchar next = peek ();
	if(next != 0)
		advance ();

	return next;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StringParser::read (uchar c)
{
	uchar next = peek ();
	if(next == c)
	{
		if(next != 0)
			advance ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StringParser::readUntil (String& string, StringRef delimiters)
{
	int delimiterCount = delimiters.length ();
	StringChars delimiterChars (delimiters);
	
	auto isDelimiter = [&] (uchar c)
	{
		for(int i = 0; i < delimiterCount; i++)
			if(delimiterChars[i] == c)
				return true;
		return false;
	};

	StringWriter<256> writer (string, true);
	while(!isEndOfString ())
	{
		uchar c = read ();
		if(c == 0 || isDelimiter (c))
			break;
		writer.append (c);
	}
	writer.flush ();
	return !string.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StringParser::readUntilWhitespace (String& string)
{
	return readUntil (string, kWhitespace);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StringParser::skipUntil (StringRef token)
{
	int index = string.subString (position).index (token);
	if(index >= 0)
	{
		position += index + token.length ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StringParser::readToken (StringRef token)
{
	int len = token.length ();
	if(string.subString (position, len).startsWith (token))
	{
		position += len;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StringParser::peekToken (StringRef token) const
{
	int len = token.length ();
	return string.subString (position, len).startsWith (token);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringParser::skipAny (StringRef characters)
{
	while(characters.contains (string.subString (position, 1)))
		advance ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringParser::skipWhitepace ()
{
	skipAny (kWhitespace);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StringParser::skipEmptyLine ()
{
	skipWhitepace ();
	return skipLineEnding ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StringParser::skipLineEnding ()
{
	// consume following cr, lf or cr-lf
	constexpr uchar lineEndings[] = {'\r', '\n'};

	bool result = false;
	for(uchar lineEnd : lineEndings)
		if(read (lineEnd))
			result = true;

	return result;
}
