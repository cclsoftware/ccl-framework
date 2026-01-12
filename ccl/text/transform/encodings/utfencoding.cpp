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
// Filename    : ccl/text/transform/encodings/utfencoding.cpp
// Description : Transformation between UTF-8/UTF-16 and UTF-32
//
//************************************************************************************************

#include "ccl/text/transform/encodings/utfencoding.h"
#include "core/text/coreutfcodec.h"

using namespace CCL;

//************************************************************************************************
// UTF8Decoder
//************************************************************************************************

UTF8Decoder::UTF8Decoder ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

int UTF8Decoder::decodeChar (uchar32& c, const unsigned char* sourceBuffer, int sourceSize)
{
	int result = Core::Text::UTFCodec::decodeUTF8 (c, sourceBuffer, sourceSize);
	switch (result)
	{
	case Core::Text::UTFCodec::kBufferTooSmall: return kNotEnoughSourceData;
	case Core::Text::UTFCodec::kIllegalInput: return kIllegalSequence;		
	}
	return result;	
}

//************************************************************************************************
// UTF8Encoder
//************************************************************************************************

UTF8Encoder::UTF8Encoder ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

int UTF8Encoder::encodeChar (uchar32 c, unsigned char* destBuffer, int destSize)
{
	int result = Core::Text::UTFCodec::encodeUTF8 (c, destBuffer, destSize);
	switch (result)
	{
	case Core::Text::UTFCodec::kBufferTooSmall: return kDestBufferTooSmall;
	case Core::Text::UTFCodec::kIllegalInput: return kNoUnicodeChar;		
	}
	return result;	
}

//************************************************************************************************
// UTF16Decoder
//************************************************************************************************

UTF16Decoder::UTF16Decoder (ByteOrder byteOrder)
: byteOrder (byteOrder)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

int UTF16Decoder::decodeChar (uchar32& c, const unsigned char* sourceBuffer, int sourceSize)
{
	int result = Core::Text::UTFCodec::decodeUTF16 (c, sourceBuffer, sourceSize, (int) byteOrder);
	switch (result)
	{
	case Core::Text::UTFCodec::kBufferTooSmall: return kNotEnoughSourceData;
	case Core::Text::UTFCodec::kIllegalInput: return kIllegalSequence;		
	}
	return result;	
}

//************************************************************************************************
// UTF16Encoder
//************************************************************************************************

UTF16Encoder::UTF16Encoder (ByteOrder byteOrder)
: byteOrder (byteOrder)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

int UTF16Encoder::encodeChar (uchar32 c, unsigned char* destBuffer, int destSize)
{
	int result = Core::Text::UTFCodec::encodeUTF16 (c, destBuffer, destSize, (int) byteOrder);
	switch (result)
	{
	case Core::Text::UTFCodec::kBufferTooSmall: return kDestBufferTooSmall;
	case Core::Text::UTFCodec::kIllegalInput: return kNoUnicodeChar;		
	}
	return result;
}
