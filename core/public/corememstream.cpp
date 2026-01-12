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
// Filename    : core/public/corememstream.cpp
// Description : Memory Stream
//
//************************************************************************************************

#include "core/public/corememstream.h"
#include "core/public/coreprimitives.h"

#ifndef CORE_MEMSTREAM_EXPONENTIAL_GROWTH
#define CORE_MEMSTREAM_EXPONENTIAL_GROWTH !CORE_PLATFORM_RTOS
#endif

using namespace Core;
using namespace IO;

//************************************************************************************************
// MemoryStream
//************************************************************************************************

MemoryStream::MemoryStream (uint32 memoryGrow)
: memoryGrow (memoryGrow),
  position (0),
  bytesWritten (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

MemoryStream::MemoryStream (void* buffer, uint32 size)
: memory (buffer, size, false),
  memoryGrow (0),
  position (0),
  bytesWritten (size)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

MemoryStream::MemoryStream (const MemoryStream& ms)
: memoryGrow (kDefaultGrow),
  position (0),
  bytesWritten (0)
{
	copyFrom (ms);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MemoryStream& MemoryStream::take (MemoryStream& ms)
{
	memory.take (ms.memory);
	memoryGrow = ms.memoryGrow;
	position = ms.position;
	bytesWritten = ms.bytesWritten;
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MemoryStream& MemoryStream::take (Buffer& buffer)
{
	memory.resize (0);
	memory.take (buffer);
	bytesWritten = static_cast<int> (memory.getSize ());
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MemoryStream::copyFrom (const MemoryStream& ms)
{
	if(memory.resize (ms.memory.getSize ()))
	{
		memcpy (memory.getAddress (), ms.memory.getAddress (), memory.getSize ());
		memoryGrow = ms.memoryGrow > 0 ? ms.memoryGrow : (uint32)kDefaultGrow;
		position = ms.position;
		bytesWritten = ms.bytesWritten;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MemoryStream::allocateMemory (uint32 size, bool initWithZero)
{
	if(memory.resize (size))
	{
		if(initWithZero)
			memory.zeroFill ();

		bytesWritten = 0;
		position = 0;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Buffer& MemoryStream::getBuffer () const
{
	return memory;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MemoryStream::setMemoryGrow (uint32 grow)
{
	memoryGrow = grow;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uint32 MemoryStream::getMemoryGrow () const
{
	return memoryGrow;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uint32 MemoryStream::getBytesWritten () const
{
	return bytesWritten;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MemoryStream::setBytesWritten (uint32 value)
{
	if(value <= memory.getSize ())
	{
		bytesWritten = value;
		ASSERT (position <= bytesWritten)
		if(position > bytesWritten)
			position = bytesWritten;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int MemoryStream::readBytes (void* buffer, int size)
{
	int maxRead = bytesWritten - position;

	int toRead = size < maxRead ? size : maxRead;
	if(toRead > 0)
	{
		memcpy (buffer, (char*)memory + position, toRead);
		position += toRead;
	}
	return toRead;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 MemoryStream::getPosition ()
{
	return (int64)position;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 MemoryStream::setPosition (int64 pos, int mode)
{
	switch(mode)
	{
	case kSeekSet :
		position = (int)pos;
		break;

	case kSeekEnd :
		position = bytesWritten + (int)pos; // <-- pos is negative!
		break;

	case kSeekCur :
		position += (int)pos;
		break;
	}

	if(position < 0)
		position = 0;
	else if(position > bytesWritten)
		position = bytesWritten;

	return (int64)position;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int MemoryStream::writeBytes (const void* buffer, int size)
{
	int maxWrite = memory.getSize () - position;

	if(size > maxWrite)
	{
		int newSize = position + size;

		ASSERT (memoryGrow > 0)
		if(memoryGrow == 0) // ctor with outer buffer has been used
			return -1;

		#if CORE_MEMSTREAM_EXPONENTIAL_GROWTH
		// grow by a factor of 1.5 (but at least by memoryGrow) if this is enough to hold the new size
		int exponentialGrowthSize = memory.getSize () + get_max (memory.getSize () / 2, memoryGrow);
		if(newSize <= exponentialGrowthSize)
			newSize = exponentialGrowthSize;
		#endif

		newSize = (((newSize - 1) / memoryGrow) + 1) * memoryGrow;

		if(!memory.resize (newSize))
			return -1;
	}

	if(size > 0)
	{
		memcpy ((char*)memory + position, buffer, size);
		position += size;
	}

	if(position > bytesWritten)
		bytesWritten = position;
	return size;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MemoryStream::moveBufferTo (Buffer& buffer)
{
	buffer.take (memory);
	buffer.setValidSize (bytesWritten);
	bytesWritten = 0;
	position = 0;
}
