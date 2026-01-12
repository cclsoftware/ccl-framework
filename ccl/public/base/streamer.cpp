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
// Filename    : ccl/public/base/streamer.cpp
// Description : Streamer
//
//************************************************************************************************

#include "ccl/public/base/streamer.h"
#include "ccl/public/base/primitives.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/text/cclstring.h"

using namespace CCL;

//************************************************************************************************
// Streamer
//************************************************************************************************

Streamer::Streamer (IStream& stream, int byteOrder)
: BinaryAccessor (byteOrder),
  stream (stream)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Streamer::read (void* buffer, int size)
{
	return stream.read (buffer, size); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Streamer::write (const void* buffer, int size)
{
	return stream.write (buffer, size); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename CharType>
bool Streamer::writeWithLength (const CharType* string, uint32 length)
{
	// write length
	if(!writeVarLen (length))
		return false;

	// write characters (non-terminated)
	return writeElements<CharType> (string, length) == length;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class Str, typename CharType> 
bool Streamer::readWithLength (Str& string)
{
	// read length
	uint32 length = 0;
	if(!readVarLen (length))
		return false;

	// read characters
	const int kStringBufferCount = 512;
	CharType buffer[kStringBufferCount];

	int remaining = length;
	while(remaining > 0)
	{
		int toRead = ccl_min<int> (remaining, kStringBufferCount);
		int numRead = readElements<CharType> (buffer, toRead);
		ASSERT (numRead == toRead)
		if(numRead != toRead)
			return false;

		string.append (buffer, numRead);
		remaining -= numRead;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Streamer::writeWithLength (CStringRef string)
{
	return writeWithLength<char> (string.str (), string.length ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Streamer::readCString (MutableCString& string)
{
	CStringWriter<512> writer (string);
	while(1)
	{
		char c = 0;
		if(!read (c))
			return false;
		if(c == 0)
			break;
		writer.append (c);
	}
	writer.flush ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Streamer::readWithLength (MutableCString& string)
{
	return readWithLength<MutableCString, char> (string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Streamer::writeLine (StringRef line)
{
	if(!writeString (line, false))
		return false;

	static const String strEndline = CCLSTR (ENDLINE);
	return writeString (strEndline, false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Streamer::writeString (StringRef string, bool terminate)
{
	StringChars chars (string);
	int length = string.length ();
	if(terminate)
		length++;

	for(int i = 0; i < length; i++)
		if(!writeChar (chars[i]))
			return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Streamer::writeWithLength (StringRef string)
{
	StringChars chars (string);
	return writeWithLength<uchar> (chars, string.length ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Streamer::readWithLength (String& string)
{
	return readWithLength<String, uchar> (string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Streamer::writeString (const uchar* chars, bool terminate)
{
	if(!chars)
		return false;

	while(true)
	{
		uchar c = *chars++;
		if(c == 0)
			break;

		if(!writeChar (c))
			return false;
	}
	
	if(terminate)
		if(!writeChar (0))
			return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Streamer::readString (String& string)
{
	uchar c = 0;
	bool result = true;
	StringWriter<512> writer (string);

	while((result = readChar (c)) == true)
	{
		if(c == 0)
			break;

		writer.append (c);
	}

	writer.flush ();
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Streamer::writeChar (uchar c)
{
	return write ((int16)c);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Streamer::readChar (uchar& c)
{
	return read ((int16&)c);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Streamer::write (FOURCC fcc)
{
	return write (fcc.bytes, 4) == 4;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Streamer::read (FOURCC& fcc)
{
	return read (fcc.bytes, 4) == 4;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Streamer::write (UIDRef uid)
{
	write (uid.data1);
	write (uid.data2);
	write (uid.data3);
	return write (uid.data4, 8) == 8;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Streamer::read (UIDBytes& uid)
{
	read (uid.data1);
	read (uid.data2);
	read (uid.data3);
	return read (uid.data4, 8) == 8;
}

//************************************************************************************************
// StreamPacketizer
//************************************************************************************************

StreamPacketizer::StreamPacketizer (IStream& stream, int _packetSize)
: stream (stream),
  packetSize (ccl_max (_packetSize, 1))
{
	ASSERT (_packetSize > 1)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int StreamPacketizer::readPackets (void* buffer, int packetCount, int bufferSize)
{
	int byteSize = packetCount * packetSize;
	ASSERT (bufferSize >= byteSize)
	int bytesRead = stream.read (buffer, ccl_min (byteSize, bufferSize));
	return bytesRead / packetSize;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int StreamPacketizer::writePackets (const void* buffer, int packetCount)
{
	int byteSize = packetCount * packetSize;
	int bytesWritten = stream.write (buffer, byteSize);
	return bytesWritten / packetSize;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 StreamPacketizer::seekPacket (int64 packetOffset, int mode)
{
	ASSERT (stream.isSeekable ())
	int64 byteOffset = packetOffset * packetSize;
	int64 byteResult = stream.seek (byteOffset, mode);
	return byteResult / packetSize;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 StreamPacketizer::getPacketPosition () const
{
	return stream.tell () / packetSize;
}
