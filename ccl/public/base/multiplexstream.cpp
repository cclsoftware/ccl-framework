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
// Filename    : ccl/public/base/multiplexstream.cpp
// Description : Multiplex Stream
//
//************************************************************************************************

#include "ccl/public/base/multiplexstream.h"

using namespace CCL;

//************************************************************************************************
// MultiplexStream
//************************************************************************************************

MultiplexStream::MultiplexStream ()
: totalSize (0),
  readPosition (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 MultiplexStream::getTotalSize () const
{
	return totalSize;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MultiplexStream::addStream (IStream* data, int64 size)
{
	ASSERT (data->isSeekable ())
	ASSERT (size > 0)

	streams.add (StreamPart (totalSize, size, data));
	totalSize += size;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MultiplexStream::findStream (StreamPart& result, int64 position)
{
	VectorForEach (streams, StreamPart, part)
		if(position >= part.start && position < part.start + part.size)
		{
			result = part;
			return true;
		}
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API MultiplexStream::read (void* buffer, int size)
{
	int numRead = 0;

	int toRead = size;
	while(toRead > 0)
	{
		StreamPart part;
		if(!findStream (part, readPosition))
			break;

		IStream* src = part.stream;
		ASSERT (src)
		if(src == nullptr)
			return -1;
		
		int64 localOffset = readPosition - part.start;
		if(src->tell () != localOffset)
		{
			int64 result = src->seek (localOffset, IStream::kSeekSet);
			ASSERT (result == localOffset)
			if(result != localOffset) // stream error
				return -1;
		}

		int maxRead = (int)(part.size - localOffset);
		int count = ccl_min<int> (maxRead, toRead);

		char* dst = (char*)buffer + numRead;
		int result = src->read (dst, count);
		if(result < 0) // stream error
			return -1;

		numRead += result;
		toRead -= result;
		readPosition += result;
	}

	return numRead;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API MultiplexStream::write (const void* buffer, int size)
{
	CCL_NOT_IMPL ("MultiplexStream::write() not implemented!")
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 CCL_API MultiplexStream::tell ()
{
	return readPosition;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API MultiplexStream::isSeekable () const
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 CCL_API MultiplexStream::seek (int64 pos, int mode)
{
	switch(mode)
	{
	case kSeekSet : readPosition = pos; break;
	case kSeekEnd : readPosition = totalSize + pos; break; // pos is negative!
	case kSeekCur : readPosition += pos; break;
	}

	readPosition = ccl_bound<int64> (readPosition, 0, totalSize);
	return readPosition;
}
