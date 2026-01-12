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
// Filename    : core/public/gui/corebitmapdata.h
// Description : Bitmap Data
//
//************************************************************************************************

#ifndef _corebitmapdata_h
#define _corebitmapdata_h

#include "core/public/coretypes.h"

namespace Core {

//************************************************************************************************
// Platform-specific bitmap format
//************************************************************************************************

#define CORE_BITMAP_FORMAT_RGBA 0
#define CORE_BITMAP_FORMAT_BGRA 1

#if CORE_PLATFORM_ANDROID
	#define CORE_BITMAP_PLATFORM_FORMAT CORE_BITMAP_FORMAT_RGBA
#else
	#define CORE_BITMAP_PLATFORM_FORMAT CORE_BITMAP_FORMAT_BGRA
#endif

//************************************************************************************************
// BitmapPixelFormat
/**	Bitmap pixel format.
	\ingroup core_gui */
//************************************************************************************************

enum BitmapPixelFormat
{
	kBitmapAny,			///< Undefined, compatible to main screen
	kBitmapRGB,			///< 24 Bit RGB
	kBitmapRGBAlpha,	///< 32 Bit RGB with alpha channel
	kBitmapMonochrome,	///< Monochrome bitmap (1 bit per pixel)
	kBitmapRGB565		///< 16 Bit RGB (5 bits red and blue, 6 bits green)
};

//************************************************************************************************
// RGBA
/**	RGBA struct following platform format, i.e. channel order can be RGBA or BGRA.
	\ingroup core_gui */
//************************************************************************************************

struct RGBA
{
	union
	{
		uint32 color;
		struct
		{
			#if (CORE_BITMAP_PLATFORM_FORMAT == CORE_BITMAP_FORMAT_RGBA)
			uint8 red;
			uint8 green;
			uint8 blue;
			#else
			uint8 blue;
			uint8 green;
			uint8 red;
			#endif
			uint8 alpha;
		};
	};
};

/** Part of 24 Bit RGB pixel. */
typedef uint8 Pixel;

//************************************************************************************************
// BitmapData
/**	Struct for bitmap data mapped into main memory.
	\ingroup core_gui */
//************************************************************************************************

struct BitmapData
{
	int width;			///< bitmap width
	int height;			///< bitmap height
	int format;			///< @see BitmapPixelFormat
	void* scan0;		///< address of first scanline
	int rowBytes;		///< offset between scanlines in bytes (can be negative if image is bottom-up!)
	int bitsPerPixel;	///< number of bits per pixel

	BitmapData ()
	: width (0),
	  height (0),
	  format (kBitmapAny),
	  scan0 (nullptr),
	  rowBytes (0),
	  bitsPerPixel (0)
	{}

	/** Calculate bytes per row based on given bitmap specification. */
	static INLINE int getRowBytes (int width, int bitsPerPixel, bool doubleWordAligned);
	
	/** Initialize bitmap data structure. */
	INLINE void init (int width, int height, BitmapPixelFormat format, bool doubleWordAligned);
	
	/** Initialize pointer to first scanline. */
	INLINE void initScan0 (void* bufferStart, bool topDown);

	/** Get scanline address (writable). */
	INLINE void* getScanline (int y);
	
	/** Get scanline address (read-only). */
	INLINE const void* getScanline (int y) const;
	
	/** Get number of bytes used to store a pixel. */
	INLINE int getBytesPerPixel () const;

	/** Get RGBA pixel (writable). */
	INLINE RGBA& rgbaAt (int x, int y);

	/** Get RGBA pixel (read-only). */
	INLINE const RGBA& rgbaAt (int x, int y) const;

	/** Get RGB pixel address (writable). */
	INLINE Pixel* getPixel (int x, int y);
	
	/** Get RGB pixel address (read-only). */
	INLINE const Pixel* getPixel (int x, int y) const;

	/** Set RGB values at pixel address. */
	static INLINE void setRgb (Pixel* p, Pixel r, Pixel g, Pixel b);
	
	/** Set RGB values at pixel address. */
	static INLINE void getRgb (Pixel& r, Pixel& g, Pixel& b, const Pixel* p);

	/** Get 16 Bit (RGB 565) pixel (writable). */
	INLINE uint16& rgb16At (int x, int y);

	/** Get 16 Bit (RGB 565) pixel (read-only). */
	INLINE const uint16& rgb16At (int x, int y) const;

	/** Get monochrome pixel value. */
	INLINE bool getBit (int x, int y) const;
	
	/** Set monochrome pixel value. */
	INLINE void setBit (int x, int y, bool state);
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// BitmapData inline
//////////////////////////////////////////////////////////////////////////////////////////////////

INLINE int BitmapData::getRowBytes (int width, int bitsPerPixel, bool doubleWordAligned)
{
	int rowBytes = 0;
	if(bitsPerPixel == 1)
	{
		rowBytes = width / 8;
		if(width % 8)
			rowBytes++;
	}
	else
		rowBytes = width * (bitsPerPixel >> 3);

	if(doubleWordAligned == true)
		while(rowBytes & 0x3)
			rowBytes++;

	return rowBytes;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

INLINE void BitmapData::init (int _width, int _height, BitmapPixelFormat _format, bool doubleWordAligned)
{
	ASSERT (_width > 0 && _height > 0)
	ASSERT (_format != kBitmapAny)

	width = _width;
	height = _height;
	format = _format;

	bitsPerPixel = format == kBitmapMonochrome ? 1 :
				   format == kBitmapRGB565 ? 16 :
				   format == kBitmapRGB ? 24 :
				   32;

	rowBytes = getRowBytes (width, bitsPerPixel, doubleWordAligned);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

INLINE void BitmapData::initScan0 (void* bufferStart, bool topDown)
{
	if(topDown == true)
	{
		scan0 = (char*)bufferStart + ((height-1) * rowBytes);
		rowBytes = -rowBytes;
	}
	else
		scan0 = bufferStart;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

INLINE void* BitmapData::getScanline (int y)
{
	return (char*)scan0 + (y * rowBytes);
}

INLINE const void* BitmapData::getScanline (int y) const
{
	return (const char*)scan0 + (y * rowBytes);
}

INLINE int BitmapData::getBytesPerPixel () const
{
	return bitsPerPixel >> 3;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

INLINE RGBA& BitmapData::rgbaAt (int x, int y)
{
	ASSERT (x < width && y < height)
	return ((RGBA*)scan0)[y * (rowBytes >> 2) + x];
}

INLINE const RGBA& BitmapData::rgbaAt (int x, int y) const
{
	ASSERT (x < width && y < height)
	return ((const RGBA*)scan0)[y * (rowBytes >> 2) + x];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

INLINE Pixel* BitmapData::getPixel (int x, int y)
{
	ASSERT (x < width && y < height)
	return (Pixel*)scan0 + y * rowBytes + x * getBytesPerPixel ();
}

INLINE const Pixel* BitmapData::getPixel (int x, int y) const
{
	ASSERT (x < width && y < height)
	return (Pixel*)scan0 + y * rowBytes + x * getBytesPerPixel ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

INLINE void BitmapData::setRgb (Pixel* p, Pixel r, Pixel g, Pixel b)
{
	p[2] = r; p[1] = g; p[0] = b;
}

INLINE void BitmapData::getRgb (Pixel& r, Pixel& g, Pixel& b, const Pixel* p)
{
	r = p[2]; g = p[1]; b = p[0];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

INLINE uint16& BitmapData::rgb16At (int x, int y)
{
	ASSERT (x < width && y < height)
	return ((uint16*)scan0)[y * (rowBytes >> 1) + x];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

INLINE const uint16& BitmapData::rgb16At (int x, int y) const
{
	ASSERT (x < width && y < height)
	return ((const uint16*)scan0)[y * (rowBytes >> 1) + x];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

INLINE bool BitmapData::getBit (int x, int y) const
{
	const char* row = (const char*)getScanline (y);
	uint32 byteIndex = x / 8;
	uint32 bitIndex = x % 8;
	return (row[byteIndex] & (0x80>>bitIndex)) != 0;
}

INLINE void BitmapData::setBit (int x, int y, bool state)
{
	char* row = (char*)getScanline (y);
	uint32 byteIndex = x / 8;
	uint32 bitIndex = x % 8;
	if(state)
		row[byteIndex] |= (0x80>>bitIndex);
	else
		row[byteIndex] &= ~(0x80>>bitIndex);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace Core

#endif // _corebitmapdata_h
