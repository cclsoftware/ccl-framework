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
// Filename    : ccl/system/packaging/bufferedstream.cpp
// Description : Buffered Stream
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/system/packaging/bufferedstream.h"
#include "ccl/public/system/inativefilesystem.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

#if DEBUG_LOG
#define LOG_BUFFER CCL_PRINTF(" -- streamPos: %"FORMAT_INT64"d;  buffer [start: %"FORMAT_INT64"d, %d bytes, pos: %d]\n", bufferStart, filled, bufferPos, streamPos)
#define VERIFY_STREAMPOS ASSERT (streamPos == stream->tell ())
#else
#define LOG_BUFFER
#define VERIFY_STREAMPOS
#endif

//************************************************************************************************
// BufferedStream
//************************************************************************************************

BufferedStream::BufferedStream (IStream* stream, unsigned int bufferSize)
: buffer (bufferSize, false),
  bufferStart (0),
  streamPos (0),
  bufferPos (0),
  filled (0),
  hotStart (0),
  hotEnd (0)
{
	setStream (stream);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BufferedStream::~BufferedStream ()
{
	if(stream)
		flush ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BufferedStream::setStream (IStream* newStream)
{
	if(stream)
		flush ();

	stream.share (newStream);
	bufferStart	= streamPos = stream->tell ();
	bufferPos	= 0;
	filled		= 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BufferedStream::setStreamOptions (int options)
{
	UnknownPtr<INativeFileStream> s (stream);
	if(s)
		s->setOptions (options);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BufferedStream::seekStream (int64 pos)
{
	if(streamPos != pos)
	{
		streamPos = stream->seek (pos, kSeekSet);
		CCL_PRINTF ("   seek stream: %" FORMAT_INT64"d\n", pos)
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API BufferedStream::read (void* data, int size)
{
	CCL_PRINTF ("BufferedStream::read (%d)\n", size)

	char* outBuffer = (char*)data;
	unsigned int toRead = size;

	// copy data from buffer
	int maxCopy = int (filled) - int (bufferPos);
	ASSERT (maxCopy >= 0)
	int toCopy = ccl_min<int> (toRead, maxCopy);
	if(toCopy > 0)
	{
		memcpy (outBuffer, buffer + bufferPos, toCopy);

		bufferPos += toCopy;
		outBuffer += toCopy;
		toRead    -= toCopy;
		CCL_PRINTF ("   copy %d bytes from buffer\n", toCopy);
	}

	ASSERT (bufferPos == filled || toRead == 0) // buffer is exhausted or we are finished

	if(toRead)
	{
		flush (); // write out any changes before we move forward in the stream

		if(toRead >= buffer.getSize ())
		{
			// read directly from stream, bypassing the buffer
			seekStream (bufferStart + filled);
			int bytesRead = stream->read (outBuffer, toRead);
			if(bytesRead > 0)
				streamPos += bytesRead;

			CCL_PRINTF ("   read directly %d bytes (toRead %d)", bytesRead, toRead);

			// (empty) buffer starts after read block
			bufferStart = streamPos;
			filled = 0;
			bufferPos = 0;
			LOG_BUFFER
			VERIFY_STREAMPOS
			return size - (toRead - bytesRead);
		}

		while(toRead)
		{
			// refill buffer
			bufferStart += filled;
			bufferPos = 0;
			filled = 0;
			seekStream (bufferStart);

			int bytesRead = stream->read (buffer, buffer.getSize ());
			if(bytesRead <= 0)
				break;

			streamPos += bytesRead;
			filled = bytesRead;

			// copy from buffer
			int toCopy = ccl_min (toRead, filled);
			memcpy (outBuffer, buffer, toCopy);
			
			bufferPos = toCopy;
			outBuffer += toCopy;
			toRead    -= toCopy;

			CCL_PRINTF ("   refilled %d bytes, delivered %d", bytesRead, toCopy);
			LOG_BUFFER
			VERIFY_STREAMPOS
		}
	}
	return size - toRead;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API BufferedStream::write (const void* data, int size)
{
	CCL_PRINTF ("BufferedStream::write (%d)\n", size)

	const char* source = (const char*)data;
	unsigned int toWrite = (unsigned int)size;

	if(toWrite >= buffer.getSize ())
	{
		flush ();

		// bypass the buffer
		seekStream (bufferStart + bufferPos);
		int written = stream->write (source, toWrite);
		CCL_PRINTF ("   write directly %d bytes (toWrite %d)", written, toWrite);
		ccl_lower_limit (written, 0);

		streamPos += written;
		toWrite   -= written;

		// (empty) buffer will start after written block
		bufferStart = streamPos;
		filled = 0;
		bufferPos = 0;
		LOG_BUFFER
		VERIFY_STREAMPOS
	}
	else
	{
		// copy to buffer at current position
		unsigned int toCopy = ccl_min (toWrite, buffer.getSize () - bufferPos);
		if(toCopy > 0)
		{
			memcpy (buffer + bufferPos, source, toCopy);

			ccl_upper_limit (hotStart, bufferPos);
			bufferPos += toCopy;
			toWrite   -= toCopy;
			source    += toCopy;
			ccl_lower_limit (hotEnd, bufferPos);
			ccl_lower_limit (filled, hotEnd);

			CCL_PRINTF ("   write %d bytes to buffer", toCopy);
			LOG_BUFFER
		}

		if(toWrite > 0)
		{
			// still something to write (buffer is full)
			ASSERT (toWrite <= buffer.getSize ())
			flush ();
			bufferStart += bufferPos;

			// copy data into (empty) buffer
			memcpy (buffer, source, toWrite);
			bufferPos = toWrite;
			filled = bufferPos;
			hotEnd = toWrite;
			CCL_PRINTF ("   write %d bytes to empty buffer", toWrite);
			LOG_BUFFER
			return size;
		}
	}
	return size - toWrite;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BufferedStream::flush ()
{
	if(hotEnd > 0)
	{
		CCL_PRINTF ("   flush: [%d, %d]\n", hotStart, hotEnd);
		ASSERT (hotStart < hotEnd)
		VERIFY_STREAMPOS

		int64 writePos = bufferStart + hotStart;
		int toWrite = hotEnd - hotStart;
		seekStream (writePos);

		stream->write (buffer + hotStart, toWrite);
		streamPos += toWrite;

		VERIFY_STREAMPOS
		hotStart = hotEnd = 0;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 CCL_API BufferedStream::tell ()
{
	return bufferStart + bufferPos;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API BufferedStream::isSeekable () const
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 CCL_API BufferedStream::seek (int64 pos, int mode)
{
	CCL_PRINTF ("BufferedStream::seek (%d) [%d]\n", pos, mode)

	int64 currentPos = tell ();
	int64 newPos = 0;
	switch(mode)
	{
		case kSeekSet:
			newPos = pos;
			break;
		case kSeekCur:
			newPos = currentPos + pos;
			break;
		case kSeekEnd:
		{
			// determine end position
			stream->seek (0, kSeekEnd);
			int64 streamEnd = stream->tell ();
			newPos = streamEnd + pos; // <-- pos is negative!
			if(streamEnd != streamPos)
				stream->seek (streamPos, kSeekSet);
			VERIFY_STREAMPOS
			break;
		}
	}

	if(newPos != currentPos)
	{
		ccl_lower_limit (newPos, (int64)0);

		if(newPos >= bufferStart && newPos < bufferStart + filled)
		{
			// seek inside buffer
			bufferPos = int(newPos - bufferStart);
		}
		else
		{
			// seek in stream
			flush ();
			seekStream (newPos);

			// (empty) buffer now starts there
			bufferStart = streamPos;
			bufferPos = 0;
			filled = 0;
		}
	}
	return bufferStart + bufferPos;
}

