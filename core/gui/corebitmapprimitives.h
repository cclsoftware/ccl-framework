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
// Filename    : core/gui/corebitmapprimitives.h
// Description : Bitmap primitives
//
//************************************************************************************************

#ifndef _corebitmapprimitives_h
#define _corebitmapprimitives_h

#include "core/public/gui/corecolor.h"
#include "core/public/gui/corebitmapdata.h"
#include "core/public/gui/corerect.h"

namespace Core {

//************************************************************************************************
// BitmapPrimitives
/** Functions for any pixel format. */
//************************************************************************************************

class BitmapPrimitives
{
public:
	static void clear (BitmapData& dstData);
};

//************************************************************************************************
// BitmapPrimitives32
/** Functions for 32 bit RGBA bitmaps. */
//************************************************************************************************

class BitmapPrimitives32: public BitmapPrimitives
{
public:
	typedef void (*BasicModifier) (BitmapData& dstData, const BitmapData& srcData);
	typedef void (*ValueModifier) (BitmapData& dstData, const BitmapData& srcData, float value);
	typedef void (*ColorModifier) (BitmapData& dstData, const BitmapData& srcData, Color color);

	static INLINE Color toColor (RGBA p)
	{
		return Color (p.red, p.green, p.blue, p.alpha); 
	}

	static INLINE RGBA toRGBA (Color c)
	{
		RGBA p;
		p.red = c.red;
		p.green = c.green;
		p.blue = c.blue;
		p.alpha = c.alpha;
		return p;
	}
	
	static INLINE ColorHSL toHSL (RGBA p)
	{
		ColorHSL hsl;
		hsl.fromRGBA (p.red / 255.f, p.green / 255.f, p.blue / 255.f, p.alpha / 255.f);
		return hsl;
	}
	
	static INLINE RGBA toRGBA (ColorHSL& hsl)
	{
		RGBA p;
		float r = 0, g = 0, b = 0, a = 0;
		hsl.toRGBA (r, g, b, a);
		p.red = Color::setC (r * 255.f);
		p.green = Color::setC (g * 255.f);
		p.blue = Color::setC (b * 255.f);
		p.alpha = Color::setC (a * 255.f);
		return p;
	}

	static void copyFrom (BitmapData& dstData, const BitmapData& srcData);
	static void copyPart (BitmapData& dstData, const BitmapData& srcData, int srcOffsetX, int srcOffsetY);
	static void copyPart (BitmapData& dstData, int dstX, int dstY, const BitmapData& srcData, int srcX, int srcY, int width, int height);

	static void scrollRect (BitmapData& dstData, const Rect& rect, const Point& delta);

	static void premultiplyAlpha (BitmapData& dstData, const BitmapData& srcData);
	static void revertPremultipliedAlpha (BitmapData& dstData, const BitmapData& srcData);
	static void byteSwapRGB (BitmapData& dstData, const BitmapData& srcData);

	static void invert (BitmapData& dstData, const BitmapData& srcData);
	static void grayScale (BitmapData& dstData, const BitmapData& srcData);
	
	static void setAlpha (BitmapData& dstData, const BitmapData& srcData, float value);
	static void scaleAlpha (BitmapData& dstData, const BitmapData& srcData, float value);
	static void lighten (BitmapData& dstData, const BitmapData& srcData, float value);
	static void addNoise (BitmapData& dstData, const BitmapData& srcData, float value);
	static void blurX (BitmapData& dstData, const BitmapData& srcData, float value);
	static void blurY (BitmapData& dstData, const BitmapData& srcData, float value);
	static void saturate (BitmapData& dstData, const BitmapData& srcData, float value);

	static void tint (BitmapData& dstData, const BitmapData& srcData, Color color);
	static void colorize (BitmapData& dstData, const BitmapData& srcData, Color color);
	static void lightAdapt (BitmapData& dstData, const BitmapData& srcData, Color color);

	static void fillRect (BitmapData& data, RectRef r, ColorRef color);
};

//************************************************************************************************
// BitmapPrimitives16
/** Functions for 16 bit bitmaps (RGB 565). */
//************************************************************************************************

class BitmapPrimitives16: public BitmapPrimitives
{
public:
	static const uint16 kRedMask = 0xF800;
	static const uint16 kGreenMask = 0x7E0;
	static const uint16 kBlueMask = 0x1F;

	/** Expand RGB 565 to RGB 888. */
	static INLINE void fromRGB565 (uint8 rgb[3], uint16 pixel)
	{
		rgb[0] = (pixel & kRedMask) >> 8;
		rgb[1] = (pixel & kGreenMask) >> 3;
		rgb[2] = (uint8)((pixel & kBlueMask) << 3);
	}

	static INLINE void fromRGB565 (RGBA& dst, uint16 pixel) // respect RGBA channel order!
	{
		dst.red = (pixel & kRedMask) >> 8;
		dst.green = (pixel & kGreenMask) >> 3;
		dst.blue = (uint8)((pixel & kBlueMask) << 3);
		dst.alpha = 0xFF;
	}

	/** Convert RGB 888 to RGB 565 - lossy, of course! */
	static INLINE uint16 toRGB565 (const uint8 rgb[3])
	{
		return	((rgb[0] << 8) & kRedMask) |
				((rgb[1] << 3) & kGreenMask) |
				((rgb[2] >> 3) & kBlueMask);
	}

	static INLINE uint16 toRGB565 (ColorRef color)
	{
		return toRGB565 (reinterpret_cast<const uint8*> (&color));
	}

	static INLINE uint16 toRGB565 (RGBA p) // respect RGBA channel order!
	{
		return	((p.red << 8) & kRedMask) |
				((p.green << 3) & kGreenMask) |
				((p.blue >> 3) & kBlueMask);
	}

	/** Alpha-blending, found at http://stackoverflow.com/questions/18937701/combining-two-16-bits-rgb-colors-with-alpha-blending */
	static INLINE uint16 alphaBlend (uint16 fg, uint16 bg, uint8 alpha)
	{
															//   rrrrrggggggbbbbb
		static const uint16 kMaskRB = kRedMask|kBlueMask;	// 0b1111100000011111
		static const uint16 kMaskG = kGreenMask;			// 0b0000011111100000
		static const uint32 kMaskMulRB = 0x3E07C0;			// 0b1111100000011111000000
		static const uint32 kMaskMulG = 0x1F800;			// 0b0000011111100000000000
		static const uint8 kMaxAlpha = 64;					// 6bits+1 with rounding

		// alpha for foreground multiplication
		// convert from 8bit to (6bit+1) with rounding
		// will be in [0..64] inclusive
		alpha = (alpha + 2) >> 2;

		// "beta" for background multiplication; (6bit+1);
		// will be in [0..64] inclusive
		uint8 beta = kMaxAlpha - alpha;
		// so (0..64)*alpha + (0..64)*beta always in 0..64

		return (uint16)((((alpha * (uint32)(fg & kMaskRB) + beta * (uint32)(bg & kMaskRB)) & kMaskMulRB)|
						((alpha * (fg & kMaskG) + beta * (bg & kMaskG)) & kMaskMulG )) >> 6);

		/*
		  result masks of multiplications
		  uppercase: usable bits of multiplications
		  RRRRRrrrrrrBBBBBbbbbbb // 5-5 bits of red+blue
				1111100000011111 // from MASK_RB * 1
		  1111100000011111000000 //   to MASK_RB * MAX_ALPHA // 22 bits!


		  -----GGGGGGgggggg----- // 6 bits of green
				0000011111100000 // from MASK_G * 1
		  0000011111100000000000 //   to MASK_G * MAX_ALPHA
		*/
	}

	static void copyPart (BitmapData& dstData, int dstX, int dstY, const BitmapData& srcData, int srcX, int srcY, int width, int height);

	static void convertToRGBA (BitmapData& dstData, const BitmapData& srcData);

	static void fillRect (BitmapData& data, RectRef r, ColorRef color);
};

//************************************************************************************************
// BitmapPrimitivesMonochrome
/** Functions for monochrome bitmaps (1 bit per pixel). */
//************************************************************************************************

class BitmapPrimitivesMonochrome: public BitmapPrimitives
{
public:
	static void convertToRGBA (BitmapData& dstData, const BitmapData& srcData);
};

} // namespace Core

#endif // _corebitmapprimitives_h
