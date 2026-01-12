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
// Filename    : core/text/coretextcodec.cpp
// Description : Text Format Conversion
//
//************************************************************************************************

#include "coreutfcodec.h"

namespace Core { 
namespace Text {
namespace UTFCodec {

//////////////////////////////////////////////////////////////////////////////////////////////////

inline uchar32 get16Bit (const unsigned char* sourceBuffer, int byteOrder)
{
	if(byteOrder == CORE_BIG_ENDIAN)
		return (sourceBuffer[0] << 8) + sourceBuffer[1];
	else
		return (sourceBuffer[1] << 8) + sourceBuffer[0];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void write16Bit (uint16 w, unsigned char* destBuffer, int byteOrder)
{
	if(byteOrder == CORE_BIG_ENDIAN)
	{
		destBuffer[0] = (unsigned char)(w >> 8);
		destBuffer[1] = (unsigned char) w;
	}
	else
	{
		destBuffer[0] = (unsigned char) w;
		destBuffer[1] = (unsigned char)(w >> 8);
	}
}

} // namespace UTFCodec
} // namespace Text
} // namespace Core

using namespace Core;
using namespace Text;

//************************************************************************************************
// UTFCodec
//************************************************************************************************

int UTFCodec::decodeUTF8 (uchar32& c, const unsigned char* sourceBuffer, int sourceSize)
{
	if(sourceSize <= 0)
		return 0;

	// algorithm: see RFC 2279
	unsigned char s0 = sourceBuffer[0];

	if(s0 < 0x80)
	{
		c = s0;
		return 1;
	}
	else if(s0 < 0xc2)
	{
		return kIllegalInput;
	}
	else if(s0 < 0xe0)
	{
		if(sourceSize < 2)
			return kBufferTooSmall;
		if(!((sourceBuffer[1] ^ 0x80) < 0x40))
			return kIllegalInput;
		c = ((uchar32) (s0 & 0x1f) << 6) | (uchar32) (sourceBuffer[1] ^ 0x80);
		return 2;
	}
	else if(s0 < 0xf0)
	{
		if(sourceSize < 3)
			return kBufferTooSmall;
		if(! ((sourceBuffer[1] ^ 0x80) < 0x40
			&& (sourceBuffer[2] ^ 0x80) < 0x40
			&& (s0 >= 0xe1 || sourceBuffer[1] >= 0xa0)))
			return kIllegalInput;
		c =	(  (uchar32) (s0 & 0x0f) << 12)
			| ((uchar32) (sourceBuffer[1] ^ 0x80) << 6)
			|  (uchar32) (sourceBuffer[2] ^ 0x80);
		return 3;
	}
	else if(s0 < 0xf8)
	{
		if(sourceSize < 4)
			return kBufferTooSmall;
		if(!  ((sourceBuffer[1] ^ 0x80) < 0x40
			&& (sourceBuffer[2] ^ 0x80) < 0x40
			&& (sourceBuffer[3] ^ 0x80) < 0x40
			&& (s0 >= 0xf1 || sourceBuffer[1] >= 0x90)))
			return kIllegalInput;

		c = ((uchar32) (s0 & 0x07) << 18)
			| ((uchar32) (sourceBuffer[1] ^ 0x80) << 12)
			| ((uchar32) (sourceBuffer[2] ^ 0x80) << 6)
			|  (uchar32) (sourceBuffer[3] ^ 0x80);
		return 4;
	}
	else if(s0 < 0xfc)
	{
		if(sourceSize < 5)
			return kBufferTooSmall;
		if(!  ((sourceBuffer[1] ^ 0x80) < 0x40
			&& (sourceBuffer[2] ^ 0x80) < 0x40
			&& (sourceBuffer[3] ^ 0x80) < 0x40
			&& (sourceBuffer[4] ^ 0x80) < 0x40
			&& (s0 >= 0xf9 || sourceBuffer[1] >= 0x88)))
				return kIllegalInput;
		c = (  (uchar32) (s0 & 0x03) << 24)
			| ((uchar32) (sourceBuffer[1] ^ 0x80) << 18)
			| ((uchar32) (sourceBuffer[2] ^ 0x80) << 12)
			| ((uchar32) (sourceBuffer[3] ^ 0x80) << 6)
			|  (uchar32) (sourceBuffer[4] ^ 0x80);
		return 5;
	}
	else if(s0 < 0xfe)
	{
		if(sourceSize < 6)
			return kBufferTooSmall;
		if(!  ((sourceBuffer[1] ^ 0x80) < 0x40
			&& (sourceBuffer[2] ^ 0x80) < 0x40
			&& (sourceBuffer[3] ^ 0x80) < 0x40
			&& (sourceBuffer[4] ^ 0x80) < 0x40
			&& (sourceBuffer[5] ^ 0x80) < 0x40
			&& (s0 >= 0xfd || sourceBuffer[1] >= 0x84)))
			return kIllegalInput;
		c =   ((uchar32) (s0 & 0x01) << 30)
			| ((uchar32) (sourceBuffer[1] ^ 0x80) << 24)
			| ((uchar32) (sourceBuffer[2] ^ 0x80) << 18)
			| ((uchar32) (sourceBuffer[3] ^ 0x80) << 12)
			| ((uchar32) (sourceBuffer[4] ^ 0x80) << 6)
			|  (uchar32) (sourceBuffer[5] ^ 0x80);
		return 6;
	}
	return kIllegalInput;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int UTFCodec::encodeUTF8 (uchar32 c, unsigned char* destBuffer, int destSize)
{
	// algorithm: see RFC 2279
	int bytesNeeded;

	if(c < 0x80)
		bytesNeeded = 1;
	else if(c < 0x800)
		bytesNeeded = 2;
	else if(c < 0x10000)
		bytesNeeded = 3;
	else if(c < 0x200000)
		bytesNeeded = 4;
	else if(c < 0x4000000)
		bytesNeeded = 5;
	else if(c <= 0x7fffffff)
		bytesNeeded = 6;
	else
		return kIllegalInput;

	if(destSize < bytesNeeded)
		return kBufferTooSmall;

	switch(bytesNeeded)
	{
		// fall through!
		case 6: destBuffer[5] = 0x80 | (c & 0x3f); c = c >> 6; c |= 0x4000000; CORE_FALLTHROUGH
		case 5: destBuffer[4] = 0x80 | (c & 0x3f); c = c >> 6; c |= 0x200000; CORE_FALLTHROUGH
		case 4: destBuffer[3] = 0x80 | (c & 0x3f); c = c >> 6; c |= 0x10000; CORE_FALLTHROUGH
		case 3: destBuffer[2] = 0x80 | (c & 0x3f); c = c >> 6; c |= 0x800; CORE_FALLTHROUGH
		case 2: destBuffer[1] = 0x80 | (c & 0x3f); c = c >> 6; c |= 0xc0; CORE_FALLTHROUGH
		case 1: destBuffer[0] = (unsigned char)c;
	}
	return bytesNeeded;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UTFCodec::isHighSurrogateUTF16 (uchar c)
{
	return c >= 0xD800 && c <= 0xDBFF;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UTFCodec::isLowSurrogateUTF16 (uchar c)
{
	return c >= 0xDC00 && c <= 0xDFFF;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uchar32 UTFCodec::makeSurrogatePairUTF16 (uchar high, uchar low)
{
	return 0x10000 + ((high - 0xd800) << 10) + (low - 0xdc00);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int UTFCodec::decodeUTF16 (uchar32& c, const unsigned char* sourceBuffer, int sourceSize, int byteOrder)
{
	// algorithm: see RFC 2781
	if(sourceSize >= 2)
	{
		uchar32 w1 = get16Bit (sourceBuffer, byteOrder);
		if(w1 < 0xD800 || w1 > 0xDFFF)
		{
			c = w1;
			return 2;
		}
		else if(w1 >= 0xD800 && w1 <= 0xDBFF)
		{
			// surrogate pair
			if(sourceSize >= 4)
			{
				uchar32 w2 = get16Bit (sourceBuffer + 2, byteOrder);
				if(w2 >= 0xDC00 && w2 <= 0xDFFF)
				{
					c = makeSurrogatePairUTF16 (static_cast<uchar> (w1), static_cast<uchar> (w2));
					return 4;
				}
				else
					return kIllegalInput;
			}
			else
				return kBufferTooSmall; // second word may follow in next call
		}
		else
			return kIllegalInput;
	}
	return kBufferTooSmall;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int UTFCodec::encodeUTF16 (uchar32 c, unsigned char* destBuffer, int destSize, int byteOrder)
{
	// algorithm: see RFC 2781
	if(!(c >= 0xd800 && c < 0xe000))
	{
		if(c < 0x10000)
		{
			if(destSize >= 2)
			{
				write16Bit ((uint16)c, destBuffer, byteOrder);
				return 2;
			}
			else
				return kBufferTooSmall;
		}
		else if(c < 0x110000)
		{
			if(destSize >= 4)
			{
				write16Bit ((uint16)(0xd800 + ((c - 0x10000) >> 10)),  destBuffer,     byteOrder);
				write16Bit ((uint16)(0xdc00 + ((c - 0x10000) & 0x3ff)), destBuffer + 2, byteOrder);
				return 4;
			}
			else
				return kBufferTooSmall;
		}
	}
	return kIllegalInput;
}
