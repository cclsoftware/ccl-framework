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
// Filename    : ccl/base/storage/textparser.cpp
// Description : Simple text parser class
//
//************************************************************************************************

#include "ccl/base/storage/textparser.h"

#include "ccl/public/base/istream.h"
#include "ccl/public/text/cclstring.h"
#include "ccl/public/math/mathprimitives.h"

using namespace CCL;

//************************************************************************************************
// Parser
//************************************************************************************************

TextParser::TextParser (IStream& stream)
: stream (stream),
  whitespaces (" \t\r\n"),
  identChars ("_")
{
	advance ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TextParser::~TextParser ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextParser::addWhitespace (uchar c)
{ 
	ASSERT (c <= 255)
	whitespaces += c;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextParser::addIdentifierChar (uchar c)
{ 
	ASSERT (c <= 255)
	identChars += c;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextParser::isWhitespace (uchar c) const
{
	return c > 0 && whitespaces.index (c) != -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextParser::isIdentifierChar (uchar c) const
{
	return Unicode::isAlphaNumeric (c) || (c > 0 && identChars.index (c) != -1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextParser::isIdentifierChar (char c) const
{
	// ASCII order: 0-9, A-Z, a-z
	// small characters are assumed most likely
	return (c <= 'z' && (c >= 'a' || (c <= 'Z' && (c >= 'A' || (c <= '9' && c >= '0')))))
		|| (c > 0 && identChars.index (c) != -1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextParser::isAlpha (char c)
{
	return c <= 'z' && (c >= 'a' || (c <= 'Z' && c >= 'A'));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uchar TextParser::peek () const
{ 
	return peekChar;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextParser::advance ()
{
	if(stream.read (&peekChar, sizeof(uchar)) == sizeof(uchar))
		return true;
	peekChar = 0;
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uchar TextParser::read ()
{
	uchar c = peekChar;
	advance ();
	return c;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextParser::read (uchar c)
{
	if(peekChar != c)
		return false;

	if(stream.read (&peekChar, sizeof(uchar)) != sizeof(uchar))
		peekChar = 0; // end of stream
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String& TextParser::readIdentifier (String& ident)
{
	StringWriter<512> writer (ident);
	if(!Unicode::isAlpha (peekChar))
		return ident;

	writer.append (peekChar);
	advance ();

	while(isIdentifierChar (peekChar))
	{
		writer.append (peekChar);
		advance ();
	}

	writer.flush ();
	return ident;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextParser::readIdentifier (char* ident, int bufferSize)
{
	char c = (char)peekChar;
	if(!isAlpha (c))
	{
		ident[0] = 0;
	}
	else
	{
		ident[0] = (char)c;
		advance ();
		c = (char)peekChar;

		int i = 1;
		while(i < bufferSize - 1 && isIdentifierChar (c))
		{
			ident[i++] = (char)c;
			advance ();
			c = (char)peekChar;
		}
		ident[i] = 0;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextParser::readPropertyPath (MutableCString& string)
{
	static CString extraChars ("/.:");

	CStringWriter<512> writer (string);
	while(Unicode::isAlphaNumeric (peekChar) || (peekChar && extraChars.contains ((char)peekChar)))
	{
		writer.append (peekChar);
		advance ();
	}

	writer.flush ();
	return !string.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String& TextParser::readStringLiteral (String& string, uchar quote)
{
	StringWriter<512> writer (string);
	if(read (quote))
	{
		while(peekChar)
		{
			if(peekChar == quote)
			{
				advance ();
				writer.flush ();
				return string;
			}

			writer.append (peekChar);
			advance ();
		}
		// closing " not found, eos reached: how to handle??
	}
	return string;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Float>
void TextParser::tryExponent (Float& value)
{
	if(read ('E') || read ('e'))
	{
		int exponent = 0;
		parseInt (exponent);
		value *= Math::Functions<Float>::pow (10, (Float)exponent);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Float>
bool TextParser::parseFloat (Float& value, bool withExponent)
{
	Float sign = 1.;
	if(read ('-'))
		sign = -1.;
	else
		read ('+'); // (ignore)

	if(peek () != '.' && !::isdigit (peek ()))
		return false;

	value = 0;

	while(1)
	{
		if(peek () == '.')
		{
			advance ();

			// continue with fractional part
			float fractFactor = 1;
			while(::isdigit (peek ()))
			{
				fractFactor /= 10.f;
				value += (peek () - '0') * fractFactor;
				advance ();
			}
			break; // no more digits
		}
		else if(::isdigit (peek ()))
		{
			// digit before '.'
			value *= 10;
			value += (peek () - '0');
			advance ();
		}
		else
			break; // no more digits while parsing the int part
	}

	value *= sign;

	if(withExponent)
		tryExponent (value);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Int>
bool TextParser::parseInt (Int& value)
{
	Int sign = 1;
	if(read ('-'))
		sign = -1;
	else
		read ('+'); // (ignore)

	if(!::isdigit (peek ()))
		return false;

	value = peek () - '0';
	advance ();

	while(::isdigit (peek ()))
	{
		value *= 10;
		value += (peek () - '0');
		advance ();
	}
	value *= sign;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextParser::readFloat (float& value, bool withExponent)	{ return parseFloat (value, withExponent); }
bool TextParser::readFloat (double& value, bool withExponent)	{ return parseFloat (value, withExponent); }
bool TextParser::readInt (int& value)							{ return parseInt (value); }
bool TextParser::readInt (int64& value)							{ return parseInt (value); }

//////////////////////////////////////////////////////////////////////////////////////////////////

int TextParser::read (String& string, int length)
{
	StringWriter<512> writer (string);
	int numRead  = 0;
	while(peekChar && (numRead < length))
	{
		writer.append (peekChar);
		advance ();
		numRead++;
	}
	writer.flush ();
	return numRead;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextParser::readUntil (uchar delimiter, String& string)
{
	uchar temp[2] = {delimiter, 0};
	String del (temp);
	return readUntil (del, string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextParser::readUntil (CStringPtr _delimiters, String& string)
{
	StringWriter<64> writer (string, false);
	
	CString delimiters (_delimiters);
	while(!delimiters.contains ((char)peekChar))
	{
		if(!peekChar)
			return false; // eof, none of the delimiters

		// read one more char
		writer.append (peekChar);
		advance ();
	}

	// found a delimiter
	writer.flush ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextParser::readUntil (StringRef delimiter, String& string)
{
	// try to read enough chars to compare with delimiter
	read (string, delimiter.length ());

	int comparePos = 0;
	while(string.lastIndex (delimiter) != comparePos)
	{
		if(!peekChar)
			return false; // eof, delimiter not found completely

		// read one more char
		string.append (&peekChar, 1); // <--- not very efficient!!!
		advance ();
		comparePos++; // always compare the last strlen(delimiter) chars
	}

	// found the delimiting text, cut the result string before it
	string.truncate (comparePos);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextParser::skipWhite ()
{
	while(isWhitespace (peekChar))
		if(!advance ())
			break;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextParser::skipLine ()
{
	while(peekChar != '\n' && peekChar != '\r')
	{
		if(!advance ())
			return false; // end reached
	}

	advance ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextParser::readLine (String& line)
{
	if(peekChar == 0)
		return false;

	StringWriter<512> writer (line);
	while(peekChar != '\n' && peekChar != '\r')
	{
		writer.append (peekChar);
		if(!advance ())
		{
			writer.flush ();
			return !line.isEmpty (); // end of stream before linefeed: success if we have read anything
		}
	}
	read ('\r');
	read ('\n');
	writer.flush ();
	return true;
}
