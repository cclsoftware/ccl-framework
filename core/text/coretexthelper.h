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
// Filename    : core/text/coretexthelper.h
// Description : Text Helper classes
//
//************************************************************************************************

#ifndef _coretexthelper_h
#define _coretexthelper_h

#include "core/public/corebuffer.h"
#include "core/public/corestream.h"
#include "core/public/corestringbuffer.h"

namespace Core {
namespace Text {

static const int kTextBufferSize = STRING_STACK_SPACE_MAX;

//************************************************************************************************
// BufferedTextInput
//************************************************************************************************

class BufferedTextInput
{
public:
	BufferedTextInput (IO::Stream* stream);

	char readChar ();
	char readPreviousChar ();

protected:
	IO::Stream* stream;
	char buffer[kTextBufferSize];
	int count;
	int readPos;
};

//************************************************************************************************
// BufferedTextOutput
//************************************************************************************************

class BufferedTextOutput
{
public:
	BufferedTextOutput (IO::Stream* stream);

	bool writeChar (char c);
	bool flush ();

protected:
	IO::Stream* stream;
	char buffer[kTextBufferSize];
	int count;
};

//************************************************************************************************
// TextBuffer
//************************************************************************************************

class TextBuffer
{
public:
	TextBuffer ();
	~TextBuffer ();

	CStringPtr getBuffer () const;

	TextBuffer& append (char c);
	TextBuffer& appendNull ();
	TextBuffer& empty ();

	void setNumChars (uint32 num); // careful, only in stackBuffer mode! 

private:
	char stackBuffer[kTextBufferSize];
	IO::Buffer heapBuffer;
	uint32 numChars;
};

//************************************************************************************************
// TextParser
//************************************************************************************************

class TextParser
{
public:
	TextParser (IO::Stream* stream);

	char readChar ();
	char readPreviousChar ();

protected:
	BufferedTextInput textInput;
	char peekChar;
	int64 bytePosition;
};

//************************************************************************************************
// StringParser
// Simple parser working directly on a string buffer
//************************************************************************************************

class StringParser
{
public:
	StringParser (CStringPtr string);

	bool advance ();
	void skip (const ConstString& characters);
	void skip (char c);
	bool read (char c);

	template<class Int> bool parseInt (Int& value);
	bool parseHexByte (int& value); ///< reads up to 2 characters

	static int getHexValue (char c);

protected:
	CStringPtr string;
	char peekChar;

	static bool isDigit (char c);
};

//************************************************************************************************
// TextWriter
//************************************************************************************************

class TextWriter
{
public:
	TextWriter (IO::Stream* stream);

	bool flush ();
	bool writeChar (char c);
	bool writeString (CStringPtr text, bool newline = false);
	bool writeLine (CStringPtr text);
	bool writeIndent ();
	bool writeNewline ();

	void incIndent () { indent++; }
	void decIndent () { indent--; }
	int currentIndent () const { return indent; }

	void setSuppressWhitespace (bool state) { suppressWhitespace = state; }

protected:
	BufferedTextOutput textOutput;
	int indent;
	bool suppressWhitespace;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline char TextParser::readChar ()
{
	char c = peekChar;
	peekChar = textInput.readChar ();
	bytePosition++;
	return c;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline char TextParser::readPreviousChar ()
{
	bytePosition--;
	peekChar = textInput.readPreviousChar ();
	char c = peekChar;
	return c;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline StringParser::StringParser (CStringPtr str)
: string (str)
{
	ASSERT (string)
	peekChar = *str;
	string++;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool StringParser::advance ()
{
	if(peekChar == 0)
		return false;
	peekChar = *string++;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void StringParser::skip (const ConstString& characters)
{
	while(peekChar && characters.index (peekChar) != -1)
		advance ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void StringParser::skip (char c)
{
	while(peekChar == c)
		advance ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool inline StringParser::read (char c)
{
	if(peekChar != c)
		return false;

	advance ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool StringParser::isDigit (char c)
{
	return c >= '0' && c <= '9';
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Int>
inline bool StringParser::parseInt (Int& value)
{
	Int sign = 1;
	if(read ('-'))
		sign = -1;

	if(!isDigit (peekChar))
		return false;

	value = peekChar - '0';
	advance ();

	while(isDigit (peekChar))
	{
		value *= 10;
		value += (peekChar - '0');
		advance ();
	}
	value *= sign;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline int StringParser::getHexValue (char c)
{
	if(isDigit (c))
		return c - '0';
	else if(c >= 'a' && c <= 'f')
		return 10 + c - 'a';
	else if(c >= 'A' && c <= 'F')
		return 10 + c - 'A';
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool StringParser::parseHexByte (int& value)
{
	int v1 = getHexValue (peekChar);
	if(v1 >= 0)
	{
		advance ();
		int v2 = getHexValue (peekChar);
		if(v2 >= 0)
		{
			advance ();
			value = (v1 << 4) + v2;
		}
		else
			value = v1;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void TextBuffer::setNumChars (uint32 num)
{
	ASSERT (numChars < static_cast<int> (kTextBufferSize) && num < static_cast<int> (kTextBufferSize))
	numChars = num;
}

} // namespace Text
} // namespace Core

#endif // _coretexthelper_h
