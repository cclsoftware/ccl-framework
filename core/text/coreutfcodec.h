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
// Filename    : core/text/coreutfcodec.h
// Description : Text Format Conversion
//
//************************************************************************************************

#ifndef _coreutfcodec_h
#define _coreutfcodec_h

#include "core/public/coretypes.h"

namespace Core {
namespace Text {

//************************************************************************************************
// UTFCodec
//************************************************************************************************

namespace UTFCodec 
{
	// special return values
	static const int kBufferTooSmall = 0;
	static const int kIllegalInput = -1;
			
	int decodeUTF8 (uchar32& c, const unsigned char* sourceBuffer, int sourceSize);
	int encodeUTF8 (uchar32 c, unsigned char* destBuffer, int destSize);

	bool isHighSurrogateUTF16 (uchar c);	// check if the given 16 bit value is the first half of a UTF-16 surrogate pair
	bool isLowSurrogateUTF16 (uchar c);		// check if the given 16 bit value is the second half of a UTF-16 surrogate pair
	uchar32 makeSurrogatePairUTF16 (uchar high, uchar low);

	int decodeUTF16 (uchar32& c, const unsigned char* sourceBuffer, int sourceSize, int byteOrder);
	int encodeUTF16 (uchar32 c, unsigned char* destBuffer, int destSize, int byteOrder);

	inline int decodeUTF16 (uchar32& c, const unsigned char* sourceBuffer, int sourceSize)
	{
		return decodeUTF16 (c, sourceBuffer, sourceSize, CORE_NATIVE_BYTEORDER);
	}

	inline int encodeUTF16 (uchar32 c, unsigned char* destBuffer, int destSize)
	{
		return encodeUTF16 (c, destBuffer, destSize, CORE_NATIVE_BYTEORDER);
	}

	typedef int (*DecodeFunction) (uchar32& c, const unsigned char* sourceBuffer, int sourceSize);
	typedef int (*EncodeFunction) (uchar32 c, unsigned char* destBuffer, int destSize);
}

//************************************************************************************************
// UTFReader
//************************************************************************************************

template <UTFCodec::DecodeFunction decode>
struct UTFReader
{
	UTFReader (const unsigned char* source, int numBytes)
	: source (source),
	  numBytes (numBytes)
	{}

	uchar32 getNext (int* bytesUsed = nullptr)
	{
		if(numBytes > 0)
		{
			uchar32 c;
			int used = decode (c, source, numBytes);
			if(used <= 0)
				return 0;
			source += used;
			numBytes -= used;
			if(bytesUsed)
				*bytesUsed = used;
			return c;
		}
		return 0;
	}

private:
	const unsigned char* source;
	int numBytes;
};

//************************************************************************************************
// UTFWriter
//************************************************************************************************

template <UTFCodec::EncodeFunction encode, int maxOutput>
struct UTFWriter
{
	UTFWriter (unsigned char* dest, int maxBytes)
	: dest (dest),
	  maxBytes (maxBytes),
	  numBytes (0)
	{}
	
	bool writeNext (uchar32 c)
	{
		int used = encode (c, output, maxOutput);
		if(used <= 0)
			return false;
		
		if(dest)
		{
			if(numBytes + used > maxBytes)
				return false;
			
			::memcpy (dest, output, used);
			dest += used;
		}
		
		numBytes += used;
		return true;
	}

protected:
	unsigned char* dest;
	int maxBytes;
	int numBytes;
	unsigned char output[maxOutput];
};

//************************************************************************************************
// UTF8Reader
//************************************************************************************************

struct UTF8Reader: UTFReader<UTFCodec::decodeUTF8>
{
	UTF8Reader (CStringPtr string, int length)
	: UTFReader<UTFCodec::decodeUTF8>((const unsigned char*)string, length)
	{}
};

//************************************************************************************************
// UTF8Writer
//************************************************************************************************

struct UTF8Writer: UTFWriter<UTFCodec::encodeUTF8, 6>
{
	UTF8Writer (char* dest, int maxDestLength)
	: UTFWriter<UTFCodec::encodeUTF8, 6> ((unsigned char*)dest, maxDestLength)
	{}
	
	int getLength () const { return numBytes; }

	void finish ()
	{
		if(dest == nullptr)
			numBytes++;
		else if(numBytes < maxBytes)
		{
			*dest = 0;
			numBytes++;
		}
	}
};

//************************************************************************************************
// UTF16Reader
//************************************************************************************************

struct UTF16Reader: UTFReader<UTFCodec::decodeUTF16>
{
	UTF16Reader (const uchar* string, int length)
	: UTFReader<UTFCodec::decodeUTF16> ((const unsigned char*)string, length << 1)
	{}
};

//************************************************************************************************
// UTF16Writer
//************************************************************************************************

struct UTF16Writer: UTFWriter<UTFCodec::encodeUTF16, 4>
{
	UTF16Writer (uchar* dest, int maxDestLength)
	: UTFWriter<UTFCodec::encodeUTF16, 4> ((unsigned char*)dest, maxDestLength << 1)
	{}
	
	int getLength () const { return numBytes >> 1; }

	void finish ()
	{
		if(dest == nullptr)
			numBytes += 2;
		else if(numBytes + 2 <= maxBytes)
		{
			*(uchar*)dest = 0;
			numBytes += 2;
		}
	}
};

//************************************************************************************************
// UTFFunctions
//************************************************************************************************

namespace UTFFunctions
{
	/** Decode UTF-8 string to UTF-16. */
	inline int decodeUTF8String (uchar* uString, int uStringSize, const char* cString, int cStringLength)
	{
		ASSERT (cStringLength >= 0)

		UTF8Reader reader (cString, cStringLength);
		UTF16Writer writer (uString, uStringSize);

		uchar32 c = 0;
		while((c = reader.getNext ()))
			if(!writer.writeNext (c))
				break;

		writer.finish ();
		return writer.getLength ();
	}

	/** Encode UTF-16 to UTF-8. */
	inline int encodeUTF8String (char* cString, int cStringSize, const uchar* uString, int uStringLength)
	{
		ASSERT (uStringLength >= 0)

		UTF16Reader reader (uString, uStringLength);
		UTF8Writer writer (cString, cStringSize);

		uchar32 c = 0;
		while((c = reader.getNext ()))
			if(!writer.writeNext (c))
				break;

		writer.finish ();
		return writer.getLength ();
	}
}

} // namespace Text
} // namespace Core

#endif // _coreutfcodec_h
