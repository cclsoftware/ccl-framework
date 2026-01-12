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
// Filename    : ccl/text/transform/encodings/textencoding.cpp
// Description : Base classes for text encoders/decoders
//
//************************************************************************************************

#include "ccl/text/transform/encodings/textencoding.h"

using namespace CCL;

//************************************************************************************************
// TextDecoder
//************************************************************************************************

TextDecoder::TextDecoder ()
: isOpen (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API TextDecoder::open (int sourceSize, int destSize)
{
	if(isOpen)
		close ();

	isOpen = true;
	return kResultTrue;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TextDecoder::close ()
{
	isOpen = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API TextDecoder::transform (const TransformData& data, int& sourceUsed, int& destUsed)
{
	const unsigned char* sourceBuffer = (const unsigned char*)data.sourceBuffer;
	uchar32* destBuffer = (uchar32*)data.destBuffer;
	int sourceRemaining = data.sourceSize;
	int destRemaining   = data.destSize;

	uchar32 c;
	while(destRemaining >= sizeof(uchar32))
	{
		int bytesConsumed = decodeChar (c, sourceBuffer, sourceRemaining);
		if(bytesConsumed <= 0)
			break; // todo: distinguish real errors from eg. kNotEnoughSourceData

		sourceBuffer    += bytesConsumed;
		sourceRemaining -= bytesConsumed;

		*destBuffer++ = c;
		destRemaining -= sizeof(uchar32);
	}

	sourceUsed = data.sourceSize - sourceRemaining;
	destUsed   = data.destSize   - destRemaining;
	return kResultTrue;
}

//************************************************************************************************
// TextEncoder
//************************************************************************************************

TextEncoder::TextEncoder ()
: isOpen (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API TextEncoder::open (int sourceSize, int destSize)
{
	if(isOpen)
		close ();

	isOpen = true;
	return kResultTrue;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TextEncoder::close ()
{
	isOpen = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API TextEncoder::transform (const TransformData& data, int& sourceUsed, int& destUsed)
{
	unsigned char* destBuffer = (unsigned char*)data.destBuffer;
	const uchar32* sourceBuffer = (const uchar32*)data.sourceBuffer;
	int sourceRemaining = data.sourceSize;
	int destRemaining   = data.destSize;

	tresult result = kResultTrue;
	while(sourceRemaining >= sizeof(uchar32))
	{
		int bytesWritten = encodeChar (*sourceBuffer, destBuffer, destRemaining);
		if(bytesWritten <= 0)
		{
			if(bytesWritten == kNoUnicodeChar)
			{
				// encode the unicode "replacement character" instead
				bytesWritten = encodeChar (0xFFFD, destBuffer, destRemaining);
				ASSERT (bytesWritten > 0)
				if(bytesWritten <= 0)
				{
					result = kResultFailed;
					break;
				}
			}
			else // e.g. kDestBufferTooSmall
			{
				result = kResultFailed;
				break;
			}
		}

		destBuffer    += bytesWritten;
		destRemaining -= bytesWritten;

		sourceRemaining -= sizeof(uchar32);
		sourceBuffer++;
	}

	sourceUsed = data.sourceSize - sourceRemaining;
	destUsed   = data.destSize   - destRemaining;
	return result;
}

