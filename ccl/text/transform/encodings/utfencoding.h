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
// Filename    : ccl/text/transform/encodings/utfencoding.h
// Description : Transformation between UTF-8/UTF-16 and UTF-32
//
//************************************************************************************************

#ifndef _ccl_utf8encoding_h
#define _ccl_utf8encoding_h

#include "ccl/text/transform/encodings/textencoding.h"

namespace CCL {

//************************************************************************************************
// UTF8Decoder
//************************************************************************************************

class UTF8Decoder: public TextDecoder
{
public:
	UTF8Decoder ();

	// TextDecoder
	int decodeChar (uchar32& c, const unsigned char* sourceBuffer, int sourceSize) override;
};

//************************************************************************************************
// UTF8Encoder
//************************************************************************************************

class UTF8Encoder: public TextEncoder
{
public:
	UTF8Encoder ();

	// TextEncoder
	int encodeChar (uchar32 c, unsigned char* destBuffer, int destSize) override;
};

//************************************************************************************************
// UTF16Decoder
//************************************************************************************************

class UTF16Decoder: public TextDecoder
{
public:
	UTF16Decoder (ByteOrder byteOrder);

	// TextDecoder
	int decodeChar (uchar32& c, const unsigned char* sourceBuffer, int sourceSize) override;

protected:
	ByteOrder byteOrder;
};

//************************************************************************************************
// UTF16Encoder
//************************************************************************************************

class UTF16Encoder: public TextEncoder
{
public:
	UTF16Encoder (ByteOrder byteOrder);

	// TextEncoder
	int encodeChar (uchar32 c, unsigned char* destBuffer, int destSize) override;

protected:
	ByteOrder byteOrder;
};

} // namespace CCL

#endif // _ccl_utf8encoding_h
