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
// Filename    : ccl/text/transform/encodings/cstringencoding.cpp
// Description : Transformation between 8bit-Encoding and UTF32
//
//************************************************************************************************

#include "ccl/text/transform/encodings/cstringencoding.h"

using namespace CCL;

//************************************************************************************************
// Latin1Decoder
//************************************************************************************************

int Latin1Decoder::decodeChar (uchar32& c, const unsigned char* sourceBuffer, int sourceSize)
{
	if(sourceSize > 0)
	{
		c = *sourceBuffer;
		return 1;
	}
	return kNotEnoughSourceData;
}

//************************************************************************************************
// Latin1Encoder
//************************************************************************************************

int Latin1Encoder::encodeChar (uchar32 c, unsigned char* destBuffer, int destSize)
{
	if(destSize > 0)
	{
		*destBuffer = c <= 255 ? (unsigned char)c : '?'; // TODO: map upper characters!
		return 1;
	}
	return kDestBufferTooSmall;
}

//************************************************************************************************
// ASCIIEncoder
//************************************************************************************************

int ASCIIEncoder::encodeChar (uchar32 c, unsigned char* destBuffer, int destSize)
{
	if(destSize > 0)
	{
		*destBuffer = c <= 127 ? (unsigned char)c : '?'; // TODO: map upper characters!
		return 1;
	}
	return kDestBufferTooSmall;
}
