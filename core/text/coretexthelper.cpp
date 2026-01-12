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

#include "coretexthelper.h"

using namespace Core;
using namespace Text;

//************************************************************************************************
// BufferedTextInput
//************************************************************************************************

BufferedTextInput::BufferedTextInput (IO::Stream* stream)
: stream (stream),
  count (0),
  readPos (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

char BufferedTextInput::readChar ()
{
	if(readPos < count)
		return buffer[readPos++];
		
	count = stream->readBytes (buffer, kTextBufferSize);
	if(count <= 0)
		return 0;

	readPos = 0;
	return buffer[readPos++];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

char BufferedTextInput::readPreviousChar ()
{
	if(readPos > 0)
	{
		readPos--;
		return buffer[readPos--];
	}
	
	readPos = 0;
	return buffer[readPos];
}

//************************************************************************************************
// BufferedTextOutput
//************************************************************************************************

BufferedTextOutput::BufferedTextOutput (IO::Stream* stream)
: stream (stream),
  count (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BufferedTextOutput::flush ()
{
	if(count > 0)
	{
		if(stream->writeBytes (buffer, count) != count)
			return false;
		count = 0;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BufferedTextOutput::writeChar (char c)
{
	buffer[count] = c;
	if(++count >= kTextBufferSize)
		return flush ();
	return true;
}

//************************************************************************************************
// TextBuffer
//************************************************************************************************

TextBuffer::TextBuffer ()
: numChars (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

TextBuffer::~TextBuffer ()
{
	empty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr TextBuffer::getBuffer () const
{
	return heapBuffer.isNull () ? (CStringPtr)stackBuffer : (CStringPtr)heapBuffer.getAddress ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TextBuffer& TextBuffer::append (char c)
{
	if(numChars < static_cast<int> (kTextBufferSize))
	{
		stackBuffer[numChars++] = c;
	}
	else if(numChars < heapBuffer.getSize ())
	{
		heapBuffer[numChars++] = c;
	}
	else
	{
		if(heapBuffer.isNull ())
		{
			if(!heapBuffer.resize (2 * kTextBufferSize))
				return *this;

			heapBuffer.copyFrom (stackBuffer, numChars);
		}
		else
		{
			if(!heapBuffer.resize (heapBuffer.getSize () + kTextBufferSize))
				return *this;
		}

		heapBuffer[numChars++] = c;
	}
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TextBuffer& TextBuffer::appendNull ()
{
	return append ('\0');
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TextBuffer& TextBuffer::empty ()
{
	numChars = 0;
	heapBuffer.resize (0);
	return *this;
}

//************************************************************************************************
// TextParser
//************************************************************************************************

TextParser::TextParser (IO::Stream* stream)
: textInput (stream),
  peekChar (0),
  bytePosition (0)
{}

//************************************************************************************************
// TextWriter
//************************************************************************************************

TextWriter::TextWriter (IO::Stream* stream)
: textOutput (stream),
  indent (0),
  suppressWhitespace (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextWriter::flush ()
{
	return textOutput.flush ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextWriter::writeChar (char c)
{
	return textOutput.writeChar (c);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextWriter::writeString (CStringPtr text, bool newline)
{
	if(text)
		while(char c = *text++)
			if(!writeChar (c))
				return false;

	return newline ? writeNewline () : true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextWriter::writeNewline ()
{
	return suppressWhitespace || writeString (ENDLINE);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextWriter::writeIndent ()
{
	if(suppressWhitespace == false)
		for(int i = 0; i < indent; i++)
			if(!writeChar ('\t'))
				return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextWriter::writeLine (CStringPtr text)
{
	if(indent > 0)
		writeIndent ();

	return writeString (text, true);
}
