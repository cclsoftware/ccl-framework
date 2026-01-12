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
// Filename    : core/public/corestreamaccessor.cpp
// Description : Stream Accessor
//
//************************************************************************************************

#include "core/public/corestreamaccessor.h"
#include "core/public/coreprimitives.h"
#include "core/public/corestringbuffer.h"

using namespace Core;
using namespace IO;

//************************************************************************************************
// BinaryAccessor
//************************************************************************************************

const uint16 BinaryAccessor::kByteOrderMark = 0xFEFF;

//////////////////////////////////////////////////////////////////////////////////////////////////

BinaryAccessor::BinaryAccessor (int byteOrder)
: byteOrder (byteOrder)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BinaryAccessor::writeByteOrder () 
{ 
	ASSERT (byteOrder == CORE_NATIVE_BYTEORDER)
	return write (&kByteOrderMark, 2) == 2; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BinaryAccessor::readByteOrder ()
{
	uint16 mark = 0;
	if(!read (mark))
		return false;

	if(!(mark == kByteOrderMark || mark == byte_swap (kByteOrderMark)))
		return false;

	if(mark != kByteOrderMark)
		setByteOrder (CORE_NATIVE_BYTEORDER == CORE_LITTLE_ENDIAN ? CORE_BIG_ENDIAN : CORE_LITTLE_ENDIAN);
	else
		setByteOrder (CORE_NATIVE_BYTEORDER);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BinaryAccessor::write (int64 ll)
{
	if(isByteSwap ())
		ll = byte_swap (ll);

	return write (&ll, sizeof(int64)) == sizeof(int64);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BinaryAccessor::read (int64& ll)
{
	if(read (&ll, sizeof(int64)) != sizeof(int64))
		return false;

	if(isByteSwap ())
		ll = byte_swap (ll);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BinaryAccessor::write (int32 l)
{
	if(isByteSwap ())
		l = byte_swap (l);

	return write (&l, sizeof(int32)) == sizeof(int32);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BinaryAccessor::read (int32& l)
{
	if(read (&l, sizeof(int32)) != sizeof(int32))
		return false;

	if(isByteSwap ())
		l = byte_swap (l);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BinaryAccessor::write (int16 s)
{
	if(isByteSwap ())
		s = byte_swap (s);

	return write (&s, sizeof(int16)) == sizeof(int16);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BinaryAccessor::read (int16& s)
{
	if(read (&s, sizeof(int16)) != sizeof(int16))
		return false;

	if(isByteSwap ())
		s = byte_swap (s);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BinaryAccessor::writeVarLen (uint32 value)
{
	uint32 buffer = value & 0x7F;

	while((value >>= 7))
	{
		buffer <<= 8;
		buffer |= ((value & 0x7F) | 0x80);
	}

	while(1)
	{
		if(!write ((uint8)(buffer & 0xFF)))
			return false;

		if (buffer & 0x80)
			buffer >>= 8;
		else
			break;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BinaryAccessor::readVarLen (uint32& value)
{
    uint8 c = 0;
	if(read (c) == false)
		return false;

	int bytesRead = 1;
    if((value = c) & 0x80)
    {
       value &= 0x7F;
       do
       {
			if(!read (c))
				return false;

			value = (value << 7) + (c & 0x7F);

			// we allow max. 4 bytes
			if(++bytesRead == 4)
				break;
       } while(c & 0x80);
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BinaryAccessor::write (float f)
{
	if(isByteSwap ())
		f = byte_swap (f);

	return write (&f, sizeof(float)) == sizeof(float);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BinaryAccessor::read (float& f)
{
	if(read (&f, sizeof(float)) != sizeof(float))
		return false;

	if(isByteSwap ())
		f = byte_swap (f);
	return true;
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

bool BinaryAccessor::write (double f)
{
	if(isByteSwap ())
		f = byte_swap (f);

	return write (&f, sizeof(double)) == sizeof(double);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BinaryAccessor::read (double& f)
{
	if(read (&f, sizeof(double)) != sizeof(double))
		return false;

	if(isByteSwap ())
		f = byte_swap (f);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BinaryAccessor::writeCString (CStringPtr _string, bool terminate)
{
	ConstString string (_string);
	int size = string.length ();
	if(terminate)
		size++;

	return write (string.str (), size) == size;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BinaryAccessor::readCStringBuffer (char* charBuffer, int bufferSize)
{
	char* dst = charBuffer;
	int length = 0;
	while(1)
	{
		ASSERT (length < bufferSize)
		if(length >= bufferSize)
			return false;
		char c = 0;
		if(!read (c))
			return false;
		dst[length++] = c;
		if(c == 0)
			break;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BinaryAccessor::writeFCC (int32 _fcc)
{
	int32 fcc = MAKE_BIG_ENDIAN (_fcc);
	char* bytes = reinterpret_cast<char*> (&fcc);
	return write (bytes, 4) == 4;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BinaryAccessor::readFCC (int32& fcc)
{
	char* bytes = reinterpret_cast<char*> (&fcc);
	if(read (bytes, 4) != 4)
		return false;

	fcc = MAKE_BIG_ENDIAN (fcc);
	return true;
}
