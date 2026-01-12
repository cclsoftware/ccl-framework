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
// Filename    : core/gui/corepnghandler.h
// Description : PNG handler using libpng
//
//************************************************************************************************

#ifndef _corepnghandler_h
#define _corepnghandler_h

#include "core/gui/corebitmapprimitives.h"

#include "core/public/corestream.h"
#include "core/public/corebuffer.h"

extern "C"
{
#include "png.h"
}

namespace Core {

//************************************************************************************************
// PNGHandler
/** \ingroup core_gui */
//************************************************************************************************

class PNGHandler
{
public:
	PNGHandler (IO::Stream& stream)
	: stream (stream),
	  reader (nullptr),
	  info (nullptr)
	{}

	bool construct ()
	{
		reader = ::png_create_read_struct (PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
		ASSERT (reader != nullptr)
		if(reader == nullptr)
			return false;

		info = ::png_create_info_struct (reader);
		ASSERT (info != nullptr)
		if(info == nullptr)
			return false;

		if(::setjmp (png_jmpbuf (reader)))
			return false;

		::png_set_read_fn (reader, &stream, readData);
		return true;
	}

	~PNGHandler ()
	{
		if(info)
			::png_destroy_info_struct (reader, &info);
		if(reader)
			::png_destroy_read_struct (&reader, nullptr, nullptr);
	}

	bool readInfo (int& width, int& height, bool& hasAlpha)
	{
		// first check PNG signature
		unsigned char signature[8];
		if(stream.readBytes (signature, 8) == 8
			&& ::png_sig_cmp (signature, 0, 8) == 0)
		{
			::png_set_sig_bytes (reader, 8); // tell libpng that we already consumed the 8 byte signature
			::png_read_info (reader, info);

			width = ::png_get_image_width (reader, info);
			height = ::png_get_image_height (reader, info);
			hasAlpha = (::png_get_color_type (reader, info) & PNG_COLOR_MASK_ALPHA) || ::png_get_valid (reader, info, PNG_INFO_tRNS);
			return true;
		}
		return false;
	}

	bool readBitmapData (BitmapData& bitmapData)
	{
		uint32 width = 0, height = 0;
		int bitDepth = 0, colorType = 0;
		::png_get_IHDR (reader, info, &width, &height, &bitDepth, &colorType, nullptr, nullptr, nullptr);
		ASSERT (static_cast<int> (width) == bitmapData.width && static_cast<int> (height) == bitmapData.height)

		::png_set_strip_16 (reader);
		if(colorType == PNG_COLOR_TYPE_PALETTE || bitDepth < 8)
			::png_set_expand (reader);
		else if(colorType == PNG_COLOR_TYPE_GRAY || colorType == PNG_COLOR_TYPE_GRAY_ALPHA)
			::png_set_gray_to_rgb (reader);

		::png_set_add_alpha (reader, 0xFF, PNG_FILLER_AFTER);
		bool hasAlpha = (colorType & PNG_COLOR_MASK_ALPHA) || ::png_get_valid (reader, info, PNG_INFO_tRNS);

		int bytesPerRow = width << 2;
		IO::Array<png_byte> tempBuffer (height * bytesPerRow);
		IO::Array<png_bytep> rows (height);
		png_bytep rowPtr = tempBuffer.getAddress ();
		for(uint32 i = 0; i < height; i++)
		{
			rows[i] = rowPtr;
			rowPtr += bytesPerRow;
		}

		::png_read_image (reader, rows);
        ::png_read_end (reader, info);

		bool result = true;
		if(bitmapData.format == kBitmapMonochrome)
		{
			for(uint32 y = 0; y < height; y++)
			{
				png_bytep src = rows[y];
				for(uint32 x = 0; x < width; x++, src += 4)
				{
					bool state = src[0] != 0;
					bitmapData.setBit (x, y, state);
				}
			}
		}
		else if(bitmapData.format == kBitmapRGB565)
		{
			for(uint32 y = 0; y < height; y++)
			{
				uint16* dst = (uint16*)bitmapData.getScanline (y);
				png_bytep src = rows[y];
				for(uint32 x = 0; x < width; x++, src += 4, dst++)
					*dst = BitmapPrimitives16::toRGB565 (src);
			}
		}
		else if(bitmapData.format == kBitmapRGBAlpha)
		{
			for(uint32 y = 0; y < height; y++)
			{
				RGBA* dst = (RGBA*)bitmapData.getScanline (y);
				png_bytep src = rows[y];
				for(uint32 x = 0; x < width; x++, src += 4, dst++)
				{
					dst->red = src[0];
					dst->green = src[1];
					dst->blue = src[2];
					dst->alpha = src[3];
				}
			}

			if(hasAlpha == true)
				BitmapPrimitives32::premultiplyAlpha (bitmapData, bitmapData);
		}
		else
			result = false;

		return result;
	}

protected:
	IO::Stream& stream;
	png_structp reader;
	png_infop info;

	static void readData (png_structp png, png_bytep data, png_size_t length)
	{
		reinterpret_cast<IO::Stream*> (::png_get_io_ptr (png))->readBytes (data, static_cast<int> (length));
	}
};

} // namespace Core

#endif // _corepnghandler_h
