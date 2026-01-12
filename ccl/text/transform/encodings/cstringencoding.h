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
// Filename    : ccl/text/transform/encodings/cstringencoding.h
// Description : Transformation between 8bit-Encodings and UTF32
//
//************************************************************************************************

#ifndef _ccl_cstringencoding_h
#define _ccl_cstringencoding_h

#include "ccl/text/transform/encodings/textencoding.h"

namespace CCL {

//************************************************************************************************
// Latin1Decoder
//************************************************************************************************

class Latin1Decoder: public TextDecoder
{
public:
	// TextDecoder
	int decodeChar (uchar32& c, const unsigned char* sourceBuffer, int sourceSize) override;
};

//************************************************************************************************
// Latin1Encoder
//************************************************************************************************

class Latin1Encoder: public TextEncoder
{
public:
	// TextEncoder
	int encodeChar (uchar32 c, unsigned char* destBuffer, int destSize) override;
};

//************************************************************************************************
// ASCIIDecoder
//************************************************************************************************

typedef Latin1Decoder ASCIIDecoder;

//************************************************************************************************
// ASCIIEncoder
//************************************************************************************************

class ASCIIEncoder: public TextEncoder
{
public:
	// TextEncoder
	int encodeChar (uchar32 c, unsigned char* destBuffer, int destSize) override;
};

} // namespace CCL

#endif // _ccl_cstringencoding_h
