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
// Filename    : core/public/corebuffer.h
// Description : Buffer class
//
//************************************************************************************************

#include "core/public/corebuffer.h"

using namespace Core;
using namespace IO;

//************************************************************************************************
// Buffer
//************************************************************************************************

Buffer::Buffer (void* _buffer, uint32 _size, bool copy)
: buffer (nullptr),
  size (0),
  alignment (0),
  ownMemory (true) // if initial size is zero
{
	if(!_buffer || !_size)
		return;

	if(copy)
	{
		buffer = core_malloc (_size);
		if(buffer)
		{
			size = _size;
			memcpy (buffer, _buffer, size);
		}
	}
	else
	{
		buffer = _buffer;
		size = _size;
		ownMemory = false;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Buffer::Buffer (uint32 _size, bool initWithZero)
: buffer (nullptr),
  size (0),
  alignment (0),
  ownMemory (true)
{
	resize (_size);

	if(initWithZero)
		zeroFill ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Buffer::~Buffer ()
{
	if(buffer && ownMemory)
		core_free (buffer);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Buffer& Buffer::take (Buffer& other)
{
	buffer = other.buffer;
	size = other.size;
	alignment = other.alignment;
	ownMemory = other.ownMemory;
	
	other.buffer = nullptr;
	other.size = 0;
	other.ownMemory = false;
	return *this;	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Buffer::resize (uint32 newSize)
{
	if(ownMemory)
	{
		if(newSize == 0)
		{
			if(buffer)
				core_free (buffer),
				buffer = nullptr;
			size = 0;
			return true;
		}
		else
		{
			void* ptr = core_realloc (buffer, newSize + alignment);
			if(ptr)
			{
				buffer = ptr;
				size = newSize;
				return true;
			}
		}
	}
	else
	{
		if(newSize == 0) // allow resize to zero
		{
			buffer = nullptr;
			size = 0;
			return true;
		}

		ASSERT (0)
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Buffer::setAlignment (uint32 _alignment)
{	
	alignment = _alignment;
	if(ownMemory && size)
		resize (size);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Buffer::setValidSize (uint32 newSize)
{
	ASSERT (newSize <= size)
	if(newSize > size)
		return false;

	size = newSize;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void* align_pointer (void* buffer, uint32 alignment)
{
	if(buffer)
	{
		if(alignment)
			return (void*)(((UIntPtr)buffer + ((uint64)alignment - 1)) & ~((uint64)alignment - 1));
		return buffer;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const void* Buffer::getAddressAligned () const
{
	return align_pointer (buffer, alignment);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void* Buffer::getAddressAligned ()
{
	return align_pointer (buffer, alignment);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Buffer::zeroFill ()
{
	if(buffer)
		memset (buffer, 0, size);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Buffer::byteFill (uint8 value)
{
	if(buffer)
		memset (buffer, value, size);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uint32 Buffer::copyFrom (const void* src, uint32 srcSize)
{
	ASSERT (src != nullptr)
	if(!src)
		return 0;

	uint32 count = size < srcSize ? size : srcSize;
	if(count > 0)
		memcpy (buffer, src, count);
	return count;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uint32 Buffer::copyFrom (uint32 dstOffset, const void* src, uint32 srcSize)
{
	ASSERT (src != nullptr)
	if(!src)
		return 0;

	int maxCopy = (int)size - (int)dstOffset;
	if(maxCopy <= 0)
		return 0;

	if(srcSize > (uint32)maxCopy)
		srcSize = maxCopy;

	memcpy ((char*)buffer + dstOffset, src, srcSize);
	return srcSize;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uint32 Buffer::copyTo (void* dst, uint32 dstSize) const
{
	ASSERT (dst != nullptr)
	if(!dst)
		return 0;

	uint32 count = size < dstSize ? size : dstSize;
	if(count > 0)
		memcpy (dst, buffer, count);
	return count;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Buffer::operator == (const Buffer& other) const
{
	if(size == other.size)
	{
		if(size == 0) // both buffers empty
			return true;

		if(buffer && other.buffer)
			return memcmp (buffer, other.buffer, size) == 0;
	}
	return false;
}
