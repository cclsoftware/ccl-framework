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
// Filename    : core/portable/coresimplereader.cpp
// Description : Simple Text Stream Reader
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "coresimplereader.h"
#include "corefile.h"

#include "core/system/coredebug.h"

using namespace Core;
using namespace Portable;

//************************************************************************************************
// TextReader
//************************************************************************************************

/*static*/void TextReader::getValueForKey (ValueString& value, CStringPtr key, CStringPtr fileName)
{
	if(!FileUtils::fileExists (fileName))
		return;
	
	FileStream fileStream;
	if(!fileStream.open (fileName))
	{
		CORE_PRINTF ("TextReader::getValueForKey: File exists but couldn't be opened: %s\n", fileName);
		return;
	}
	
	TextReader reader (fileStream);
	reader.getValueForKey (value, key);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TextReader::TextReader (IO::Stream& stream, bool rewind)
: stream (stream)
{
	if(rewind == true)
		stream.setPosition (0, IO::kSeekSet);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextReader::skipBOM ()
{
	unsigned char bom[3] = {};
	int numRead = stream.readBytes (bom, 3);
	if(numRead != 3 || bom[0] != 0xEF || bom[1] != 0xBB || bom[2] != 0xBF)
		stream.setPosition (0, IO::kSeekSet);
	return numRead > 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextReader::getNextLine (LineString& lineString) const
{
	lineString.empty ();
	int counter = 0;
	while(counter++ < lineString.getSize ())
	{
		char byte = 0;
		int numRead = stream.readBytes (&byte, 1);
		if(numRead != 1)
			return !lineString.isEmpty ();
		
		if(byte == '\n')
			return true;
		
		if(byte != '\r') // skip carriage returns
			lineString.append (byte);
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
	
bool TextReader::advanceToNextWord (CStringPtr word) const
{
	CStringPtr wordpos = word;
	int64 firstpos = 0;
	int64 pos = 0;
	
	bool isFirstChar = true;
	while(wordpos)
	{
		char thechar = *wordpos++;
		if(!thechar)
			return true;
			
		firstpos = stream.getPosition ();
		if(advanceToNextChar (thechar) == false)
			return false;

		// make sure it is a consecutive char
		pos = stream.getPosition ();
		if(!isFirstChar && firstpos + 1 != pos) // we skipped more than one char
		{
			wordpos	= word; // start over
			stream.setPosition (firstpos + 1, 0);
		}
		isFirstChar = false;
	}
	return true;	
}

//////////////////////////////////////////////////////////////////////////////////////////////////
	
bool TextReader::advanceToNextChar (char test) const
{
	int counter = 0;
	while(counter++ < kMaxBytesToAdvance)
	{
		char byte = 0;
		int numRead = stream.readBytes (&byte, 1);
		if(numRead != 1)
			return false;
		
		if(byte == test)
			return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextReader::getNextWord (ValueString& wordString, char delimiter) const
{
	wordString.empty ();
	int counter = 0;
	while(counter++ < wordString.getSize ())
	{
		char byte = 0;
		int numRead = stream.readBytes (&byte, 1);
		if(numRead != 1)
			return false;
		
		if(byte == delimiter)
		{
			// skip leading tabs
			if(wordString.isEmpty ())
				continue;
			
			return true;
		}
		
		if(byte == '\r') // skip carriage returns
			continue;
		
		if(byte == '\n')
			return true;

		wordString.append (byte);
	}
	return false;
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////
	
bool TextReader::getValueForKey (ValueString& value, CStringPtr key) const
{
	ValueString wordString;
	bool result = getNextWord (wordString, '=');
	if(!result)
		return false; // EOF
	
	if(wordString != key)
		return true;
	
	getNextWord (wordString);
	if(wordString.isEmpty ())
	{
		CORE_PRINTF ("TextReader::getValueForKey didn't find a name after the %s tag\n", key);
		return true;
	}
	
	value = wordString;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextReader::findValueForKey (ValueString& value, CStringPtr key) const
{
	// first, reset the stream.
	stream.setPosition (0, IO::kSeekSet);	
	while(true)
	{
		if(!getValueForKey (value, key))
			break; // EOF
		
		if(!value.isEmpty ())
			break;
	}
	return !value.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 TextReader::getPosition () const 
{ 
	return stream.getPosition (); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextReader::setPosition (int64 pos) 
{  
	stream.setPosition (pos, IO::kSeekSet); 
}
