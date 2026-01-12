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
// Filename    : ccl/text/transform/encodings/baseencoding.cpp
// Description : Base 16/32/64 Encoding
//
//************************************************************************************************

#define DEBUG_LOG 0//DEBUG

#include "ccl/text/transform/encodings/baseencoding.h"

#include "ccl/public/base/buffer.h"

using namespace CCL;

/*
	Base 16 (2^4):	4 Bit/char	=>	1 Byte	=>	 8 Bit => 2 chars
	Base 32 (2^5):	5 Bit/char	=>	5 Bytes	=>	40 Bit => 8 chars
	Base 64 (2^6):	6 Bit/char	=>	3 Bytes	=>	24 Bit => 4 chars
*/

//************************************************************************************************
// BaseTransformer
//************************************************************************************************

const char* BaseTransformer::kAlphabet16 = "=0123456789ABCDEF";
const char* BaseTransformer::kAlphabet32 = "=ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
const char* BaseTransformer::kAlphabet64 = "=ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class Transformer> 
Transformer* BaseTransformer::createInstance (UIDRef cid)
{
	if(cid == ClassID::Base16Encoding)
		return NEW Transformer (kBase16);
	if(cid == ClassID::Base32Encoding)
		return NEW Transformer (kBase32);
	if(cid == ClassID::Base64Encoding)
		return NEW Transformer (kBase64);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BaseTransformer::BaseTransformer (Type type)
: bitsPerChar (0),
  blockSize (0),
  charsPerBlock (0),
  alphabet (nullptr),
  alphabetLength (0)
{
	switch(type)
	{
	case kBase16 : 
		bitsPerChar = 4; 
		alphabet = kAlphabet16;
		break;

	case kBase32 :
		bitsPerChar = 5;
		alphabet = kAlphabet32;
		break;

	case kBase64 :
		bitsPerChar = 6; 
		alphabet = kAlphabet64;
		break;
	}

	alphabet++; // use alphabet[-1] for padding
	alphabetLength = (int)::strlen (alphabet);

	blockSize = 1;
	while((blockSize * 8) % bitsPerChar)
		blockSize++;

	charsPerBlock = (blockSize * 8) / bitsPerChar;

	CCL_PRINTF ("BaseTransformer: bitsPerChar = %d blockSize = %d charsPerBlock = %d alphabet = \"%s\" length = %d\n", 
				bitsPerChar, blockSize, charsPerBlock, alphabet, alphabetLength)
}

//************************************************************************************************
// BaseEncoder
//************************************************************************************************

BaseEncoder* BaseEncoder::createInstance (UIDRef cid)
{
	return BaseTransformer::createInstance<BaseEncoder> (cid);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BaseEncoder::BaseEncoder (Type type)
: BaseTransformer (type),
  inputBuffer (nullptr),
  inputCount (0)
{
#if DEBUG_LOG // terminated for debug output
	inputBuffer = NEW char[blockSize + 1];
	inputBuffer[blockSize] = 0;
#else
	inputBuffer = NEW char[blockSize];
#endif

	::memset (inputBuffer, 0, blockSize);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BaseEncoder::~BaseEncoder ()
{
	delete [] inputBuffer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API BaseEncoder::transform (const TransformData& data, int& sourceUsed, int& destUsed)
{
	const char* src = (const char*)data.sourceBuffer;
	char* dst = (char*)data.destBuffer;

	// feed input
	while(inputCount < blockSize && sourceUsed < data.sourceSize)
		inputBuffer[inputCount++] = src[sourceUsed++];

	// check padding
	int firstPaddingBit = 0xFFFF;
	if(data.flush)
	{
		int missing = blockSize - inputCount;
		if(missing < blockSize) // pad partial block only
		{
			firstPaddingBit = inputCount * 8;

			for(int i = 0; i < missing; i++)
				inputBuffer[inputCount++] = 0;

			CCL_PRINTF ("BaseEncoder: Missing %d bytes (firstPaddingBit = %d)\n", missing, firstPaddingBit)
		}
	}

	// produce output
	if(inputCount == blockSize)
	{
		ASSERT (data.destSize >= charsPerBlock)
		inputCount = 0; // reset

		CCL_PRINTF ("BaseEncoder: Encoding buffer [%s]\n", inputBuffer)

		BitAccessor accessor (inputBuffer, blockSize, true); // reversed!
		int inputBitIndex = 0;
		for(int charIndex = 0; charIndex < charsPerBlock; charIndex++)
		{
			int value = 0;

			if(inputBitIndex >= firstPaddingBit)
			{
				value = -1;
				inputBitIndex += bitsPerChar;
			}
			else
			{
				for(int bitIndex = bitsPerChar-1; bitIndex >= 0; bitIndex--, inputBitIndex++)
				{
					bool b = accessor.getBit (inputBitIndex);
					if(b)
						value |= 1 << bitIndex;
				}
			}

			ASSERT (value >= -1 && value < alphabetLength)
			char c = alphabet[value];

			CCL_PRINTF ("BaseEncoder: value = %d (0x%x) char = %c\n", value, value, c)
			dst[destUsed++] = c;
		}
	}

	return kResultOk;
}

//************************************************************************************************
// BaseDecoder
//************************************************************************************************

BaseDecoder* BaseDecoder::createInstance (UIDRef cid)
{
	return BaseTransformer::createInstance<BaseDecoder> (cid);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BaseDecoder::BaseDecoder (Type type)
: BaseTransformer (type),
  charBuffer (nullptr),
  charCount (0),
  outputBuffer (nullptr),
  outputCount (0),
  outputValid (0)
{
#if DEBUG_LOG // terminated for debug output
	charBuffer = NEW char[charsPerBlock + 1];
	charBuffer[charsPerBlock] = 0;
	outputBuffer = NEW char[blockSize + 1];
	outputBuffer[blockSize] = 0;
#else
	charBuffer = NEW char[charsPerBlock];
	outputBuffer = NEW char[blockSize];
#endif

	::memset (charBuffer, 0, charsPerBlock);
	::memset (outputBuffer, 0, blockSize);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BaseDecoder::~BaseDecoder ()
{
	delete [] charBuffer;
	delete [] outputBuffer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API BaseDecoder::transform (const TransformData& data, int& sourceUsed, int& destUsed)
{
	const char* src = (const char*)data.sourceBuffer;
	char* dst = (char*)data.destBuffer;

	// feed input
	while(charCount < charsPerBlock && sourceUsed < data.sourceSize)
		charBuffer[charCount++] = src[sourceUsed++];

	// decode if enough input available and no more output pending
	if(charCount == charsPerBlock && outputCount == 0)
	{
		charCount = 0; // reset
		outputCount = outputValid = blockSize;

		CCL_PRINTF ("BaseDecoder: Input buffer [%s]\n", charBuffer)

		BitAccessor accessor (outputBuffer, blockSize, true); // reversed!
		int outputBitIndex = 0;
		for(int charIndex = 0; charIndex < charsPerBlock; charIndex++)
		{
			char c = charBuffer[charIndex];
			int value = getCharValue (c);
			
			// check for padding
			if(value == -1)
			{
				if(outputValid == blockSize)
				{
					outputCount = outputValid = (charIndex * bitsPerChar)/8;

					CCL_PRINTF ("BaseDecoder: Truncating block from %d to %d bytes\n", blockSize, outputCount)
				}
				
				value = 0;
			}

			for(int bitIndex = bitsPerChar-1; bitIndex >= 0; bitIndex--, outputBitIndex++)
			{
				bool b = (value & (1 << bitIndex)) != 0;
				accessor.setBit (outputBitIndex, b);
			}
		}

		CCL_PRINTF ("BaseDecoder: Output buffer [%s]\n", outputBuffer)
	}

	// deliver pending output
	while(outputCount > 0 && destUsed < data.destSize)
	{
		dst[destUsed++] = outputBuffer[outputValid-outputCount];
		outputCount--;
	}

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int BaseDecoder::getCharValue (char c) const
{
	if(c == alphabet[-1]) // padding
		return -1;

	if(alphabetLength < 64) // handle Base16/32 case-insensitive
		c = (char)::toupper (c);

	for(int i = 0; i < alphabetLength; i++)
		if(c == alphabet[i])
			return i;

	SOFT_ASSERT (0, "Invalid character!")
	return 0;
}
