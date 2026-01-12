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
// Filename    : core/gui/corebmphandler.h
// Description : BMP handler
//
//************************************************************************************************

#ifndef _corebmphandler_h
#define _corebmphandler_h

#include "core/gui/corebitmapprimitives.h"

#include "core/public/corestream.h"
#include "core/public/corebuffer.h"
#include "core/public/coreprimitives.h"

namespace Core {

//************************************************************************************************
// BMPHandler
/** \ingroup core_gui */
//************************************************************************************************

// Bitmap File Format
// https://msdn.microsoft.com/en-us/library/dd183391%28v=vs.85%29.aspx

class BMPHandler
{
public:
	#pragma pack(push,1) // push current alignment, force 1-byte alignment

	struct BitmapFileHeader
	{
		uint16 type;
		uint32 size;
		uint16 reserved1;
		uint16 reserved2;
		uint32 offsetToBits;
	};

	struct BitmapInfoHeader
	{
		uint32 size;
		int32 width;
		int32 height;
		uint16 planes;
		uint16 bitCount;
		uint32 compression;
		uint32 sizeImage;
		int32 xPixelsPerMeter;
		int32 yPixelsPerMeter;
		uint32 colorsUsed;
		uint32 colorsImportant;

		uint32 calcSize () const
		{
			int rowBytes = BitmapData::getRowBytes (width, bitCount, true);
			return rowBytes * abs (height);
		}

		uint32 getSizeSafe () const
		{
			uint32 expectedSize = calcSize ();
			if(sizeImage == 0) // may be set to zero for RGB bitmaps
				return expectedSize;
			else // tolerate errors in bitmap file headers
				return sizeImage <= expectedSize ? sizeImage : expectedSize;
		}
	};

	static const int kBitmapFileHeaderSize = sizeof(BitmapFileHeader);
	static const int kBitmapInfoHeaderSize = sizeof(BitmapInfoHeader);

	static const uint16 kRegularBitmapType = 0x4d42; // 'BM'
	static const uint16 kCustomBitmapType = 0x4243; // 'CB' (obfuscation only)
	static const uint16 kUncompressed = 0;
	static const uint16 kBitfields = 3;

	#pragma pack(pop) // restore alignment

	static bool isKnownType (uint16 type)
	{
		return type == kRegularBitmapType || type == kCustomBitmapType;
	}

	static BitmapPixelFormat getKnownFormat (const BitmapInfoHeader& info)
	{
		// Monochrome
		if(info.bitCount == 1 && info.compression == kUncompressed)
			return kBitmapMonochrome;

		// RGB565
		if(info.bitCount == 16)
			return kBitmapRGB565;

		// 24 Bit RGB
		if(info.bitCount == 24)
			return kBitmapRGB;

		// 32 Bit RGBA
		if(info.bitCount == 32)
			return kBitmapRGBAlpha;

		return kBitmapAny; // other formats not implemented!
	}

	BMPHandler (IO::Stream& stream)
	: stream (stream)
	{
		::memset (&header, 0, sizeof(header));
		::memset (&info, 0, sizeof(info));
	}

	bool readInfo ()
	{
		if(stream.readBytes (&header, kBitmapFileHeaderSize) != kBitmapFileHeaderSize)
			return false;
		if(!isKnownType (header.type))
			return false;
		if(stream.readBytes (&info, kBitmapInfoHeaderSize) != kBitmapInfoHeaderSize)
			return false;
		return true;
	}

	const BitmapInfoHeader& getInfo () const
	{
		return info;
	}

	bool readData (void* bufferAddress, uint32 bufferSize)
	{
		// seek to data
		if(stream.setPosition (header.offsetToBits, IO::kSeekSet) != header.offsetToBits)
			return false;

		int toRead = static_cast<int> (get_min (bufferSize, info.getSizeSafe ()));
		int numRead = stream.readBytes (bufferAddress, toRead);
		if(numRead != toRead)
			return false;

		return true;
	}

	bool readBitmapData (BitmapData& bitmapData)
	{
		ASSERT (info.width == bitmapData.width && abs (info.height) == bitmapData.height)

		BitmapPixelFormat srcFormat = getKnownFormat (info);
		BitmapPixelFormat dstFormat = static_cast<BitmapPixelFormat> (bitmapData.format);

		IO::Buffer srcBuffer;
		srcBuffer.resize (info.getSizeSafe ());
		if(!readData (srcBuffer.getAddress (), srcBuffer.getSize ()))
			return false;

		BitmapData srcData;
		srcData.init (info.width, abs (info.height), srcFormat, true);
		srcData.initScan0 (srcBuffer.as<uint8> (), info.height >= 0);

		bool result = true;
		if(dstFormat == kBitmapRGBAlpha && srcFormat == kBitmapRGB565)
		{
			BitmapPrimitives16::convertToRGBA (bitmapData, srcData);
		}
		else if(dstFormat == kBitmapRGBAlpha && srcFormat == kBitmapMonochrome)
		{
			BitmapPrimitivesMonochrome::convertToRGBA (bitmapData, srcData);
		}
		else
		{
			ASSERT (0)
			result = false;
		}

		return result;
	}

protected:
	IO::Stream& stream;
	BitmapFileHeader header;
	BitmapInfoHeader info;
};

} // namespace Core

#endif // _corebmphandler_h
