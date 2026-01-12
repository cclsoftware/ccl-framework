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
// Filename    : core/gui/corebitmapprimitives.impl.h
// Description : Bitmap primitives
//
//************************************************************************************************

#include "corebitmapprimitives.h"
#include <math.h>

namespace Core {

//////////////////////////////////////////////////////////////////////////////////////////////////

#define ASSERT_COMPATIBLE_RGBA(d1, d2) \
ASSERT (d1.format == kBitmapRGBAlpha && d1.format == d2.format) \

#define ASSERT_COMPATIBLE_RGB565(d1, d2) \
ASSERT (d1.format == kBitmapRGB565 && d1.format == d2.format) \

#define ASSERT_COMPATIBLE_SIZE(d1, d2) \
ASSERT (d1.width == d2.width && d1.height == d2.height)

// ATTENTION: src RGBA must be copied (not referenced) to work with inplace processing!
#define ForEachRGBA(dstData, dst, srcData, src) \
{ for(int y = 0; y < srcData.height; y++) \
	for(int x = 0; x < srcData.width; x++) \
	{ const RGBA src = srcData.rgbaAt (x, y); \
      RGBA& dst = dstData.rgbaAt (x, y);

//************************************************************************************************
// BitmapPrimitives
//************************************************************************************************

void BitmapPrimitives::clear (BitmapData& dstData)
{
	int bytesPerRow = dstData.rowBytes < 0 ? -dstData.rowBytes : dstData.rowBytes;

	for(int y = 0; y < dstData.height; y++)
		::memset (dstData.getScanline (y), 0, bytesPerRow);
}

//************************************************************************************************
// BitmapPrimitives32
//************************************************************************************************

void BitmapPrimitives32::copyFrom (BitmapData& dstData, const BitmapData& srcData)
{
	ASSERT_COMPATIBLE_RGBA (dstData, srcData)

	int height = dstData.height < srcData.height ? dstData.height : srcData.height;
	int width = dstData.width < srcData.width ? dstData.width : srcData.width;
	int bytesPerRow = width << 2;

	for(int y = 0; y < height; y++)
	{
		const void* src = srcData.getScanline (y);
		void* dst = dstData.getScanline (y);
		::memcpy (dst, src, bytesPerRow);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BitmapPrimitives32::copyPart (BitmapData& dstData, const BitmapData& srcData, int srcOffsetX, int srcOffsetY)
{
	ASSERT_COMPATIBLE_RGBA (dstData, srcData)

	int bytesPerPixel = 4;
	for(int y = 0; y < dstData.height; y++)
	{
		int ySource = y + srcOffsetY;
		if(ySource < 0)
			continue;
		if(ySource >= srcData.height)
			break;

		for(int x = 0; x < dstData.width; x++)
		{
			int xSource = x + srcOffsetX;
			if(xSource < 0)
				continue;
			if(xSource >= srcData.width)
				break;

			char* src = (char*)srcData.getScanline (ySource);
			src += xSource * bytesPerPixel;

			char* dst = (char*)dstData.getScanline (y);
			dst += x * bytesPerPixel;

			::memcpy (dst, src, bytesPerPixel);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BitmapPrimitives32::copyPart (BitmapData& dstData, int dstX, int dstY, const BitmapData& srcData, int srcX, int srcY, int width, int height)
{
	ASSERT_COMPATIBLE_RGBA (dstData, srcData)

	int dstByteOffset = dstX << 2;
	int srcByteOffset = srcX << 2;
	int bytesToCopy = width << 2;

	for(int y = 0; y < height; y++)
	{
		uint8* dst = (uint8*)dstData.getScanline (dstY + y) + dstByteOffset;
		const uint8* src = (const uint8*)srcData.getScanline (srcY + y) + srcByteOffset;
		::memcpy (dst, src, bytesToCopy);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BitmapPrimitives32::scrollRect (BitmapData& dstData, const Rect& rect, const Point& delta)
{
	ASSERT (dstData.format == kBitmapRGBAlpha)

	Rect imageSize (0, 0, dstData.width, dstData.height);
	Rect r (rect);
	r.offset (delta);
	if(r.bound (imageSize) == false)
		return; // nothing to do
	r.offset (-delta.x, -delta.y);

	int bytesPerPixel = 4;
	int numPixelsX = r.getWidth ();
	int numPixelsY = r.getHeight ();
	int dirX = delta.x < 0 ? -1 : 1;
	int dirY = delta.y < 0 ? -1 : 1;
	int rowPixels = dstData.width;

	int startX = delta.x < 0 ? r.left : r.right - 1;
	int startY = delta.y < 0 ? r.top : r.bottom - 1;
	char* bits = (char*)dstData.scan0;

	for(int y = 0; y < numPixelsY; y++)
	{
		int cs = (rowPixels * (startY - (y * dirY)          )) + startX;
		int cd = (rowPixels * (startY - (y * dirY) + delta.y)) + startX + delta.x;

		uint32* src = (uint32*)(bits + (cs * bytesPerPixel));
		uint32* dst = (uint32*)(bits + (cd * bytesPerPixel));

		for(int x = 0; x < numPixelsX; x++)
		{
			*dst = *src;
			dst -= dirX;
			src -= dirX;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BitmapPrimitives32::premultiplyAlpha (BitmapData& dstData, const BitmapData& srcData)
{
	ASSERT_COMPATIBLE_RGBA (dstData, srcData)
	ASSERT_COMPATIBLE_SIZE (dstData, srcData)

	#define MULTIPLY(c,a) (uint8)((c * a) / 0xFF)

	ForEachRGBA (dstData, dst, srcData, src)
		dst.red   = MULTIPLY (src.red,   src.alpha);
		dst.green = MULTIPLY (src.green, src.alpha);
		dst.blue  = MULTIPLY (src.blue,  src.alpha);
		dst.alpha = src.alpha; // copy alpha
	EndFor

	#undef MULTIPLY
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BitmapPrimitives32::revertPremultipliedAlpha (BitmapData& dstData, const BitmapData& srcData)
{
	ASSERT_COMPATIBLE_RGBA (dstData, srcData)
	ASSERT_COMPATIBLE_SIZE (dstData, srcData)

	#define REVERT(c,a) (uint8)(a == 0 ? 0 : (c * 0xFF) / a)

	ForEachRGBA (dstData, dst, srcData, src)
		dst.red   = REVERT (src.red,   src.alpha);
		dst.green = REVERT (src.green, src.alpha);
		dst.blue  = REVERT (src.blue,  src.alpha);
		dst.alpha = src.alpha; // copy alpha
	EndFor

	#undef REVERT
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BitmapPrimitives32::byteSwapRGB (BitmapData& dstData, const BitmapData& srcData)
{
	ASSERT_COMPATIBLE_RGBA (dstData, srcData)
	ASSERT_COMPATIBLE_SIZE (dstData, srcData)

	ForEachRGBA (dstData, dst, srcData, src)
		dst.red = src.blue;
		dst.green = src.green;
		dst.blue = src.red;
		dst.alpha = src.alpha;
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BitmapPrimitives32::invert (BitmapData& dstData, const BitmapData& srcData)
{
	ASSERT_COMPATIBLE_RGBA (dstData, srcData)
	ASSERT_COMPATIBLE_SIZE (dstData, srcData)

	ForEachRGBA (dstData, dst, srcData, src)
		dst.red   = ~src.red;
		dst.green = ~src.green;
		dst.blue  = ~src.blue;
		dst.alpha = src.alpha; // copy alpha
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BitmapPrimitives32::grayScale (BitmapData& dstData, const BitmapData& srcData)
{
	ASSERT_COMPATIBLE_RGBA (dstData, srcData)
	ASSERT_COMPATIBLE_SIZE (dstData, srcData)

	ForEachRGBA (dstData, dst, srcData, src)
		Color c = toColor (src);
		c.grayScale ();
		dst = toRGBA (c);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BitmapPrimitives32::setAlpha (BitmapData& dstData, const BitmapData& srcData, float value)
{
	ASSERT_COMPATIBLE_RGBA (dstData, srcData)
	ASSERT_COMPATIBLE_SIZE (dstData, srcData)

	Pixel alphaValue = (Pixel)(value * 255.f);

	ForEachRGBA (dstData, dst, srcData, src)
	    (void)(src);
		dst.alpha = alphaValue;
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BitmapPrimitives32::scaleAlpha (BitmapData& dstData, const BitmapData& srcData, float value)
{
	ASSERT_COMPATIBLE_RGBA (dstData, srcData)
	ASSERT_COMPATIBLE_SIZE (dstData, srcData)

	ForEachRGBA (dstData, dst, srcData, src)
		Color c = toColor (src);
		c.setAlphaF (c.getAlphaF () * value);
		dst = toRGBA (c);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BitmapPrimitives32::lighten (BitmapData& dstData, const BitmapData& srcData, float value)
{
	ASSERT_COMPATIBLE_RGBA (dstData, srcData)
	ASSERT_COMPATIBLE_SIZE (dstData, srcData)

	ForEachRGBA (dstData, dst, srcData, src)
		Color c = toColor (src);
		c.addBrightness (value);
		dst = toRGBA (c);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BitmapPrimitives32::saturate (BitmapData& dstData, const BitmapData& srcData, float value)
{
	ASSERT_COMPATIBLE_RGBA (dstData, srcData)
	ASSERT_COMPATIBLE_SIZE (dstData, srcData)

	ForEachRGBA (dstData, dst, srcData, src)
		Color c = toColor (src);
		ColorHSL hsl (c);
		hsl.s = bound (hsl.s + (2 * value * (1 - hsl.l) * ((- (2 * hsl.s - 1)*(2 * hsl.s - 1)) + 1)));
		hsl.l = ((hsl.l * hsl.s) + ((1 - hsl.s) * ((0.7f * hsl.l) - (0.2f * (hsl.l * hsl.l)))));
		hsl.toColor (c);
		dst = toRGBA (c);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BitmapPrimitives32::blurX (BitmapData& dstData, const BitmapData& srcData, float value)
{
	ASSERT_COMPATIBLE_RGBA (dstData, srcData)
	ASSERT_COMPATIBLE_SIZE (dstData, srcData)

	int window = bound (int(srcData.width * value), 1, srcData.width);
	float sigma2Inv = 1 / (window * window * 0.2f);

	struct AccuPixel {float red, green, blue, alpha;} pixel;
	float sum;
	
	Vector<float> factorTable;
	factorTable.setCount (2 * window + 1);
	for(int i = 0; i < window; i++)
		factorTable[window - 1 - i] = factorTable[window + 1 + i] = expf (-i * i * sigma2Inv);
	factorTable[window] = 1.f;

	Vector<RGBA> runningPixels;
	runningPixels.setCount (2 * window + 1);

	//blur x components
	for(int y = 0; y < srcData.height; y++)
	{
		for(int i = 0; i < window; i++)
		{
			runningPixels[i].red = 0;
			runningPixels[i].blue = 0;
			runningPixels[i].green = 0;
			runningPixels[i].alpha = 0xFF;
		}
		for(int i = window; i <= 2 * window; i++)
			runningPixels[i] = srcData.rgbaAt (i - window, y);

		for(int x = 0; x < srcData.width; x++)
		{
			//process a pixel
			sum = 0;
			pixel.red = 0;
			pixel.blue = 0;
			pixel.green = 0;
			pixel.alpha = 0;

			//accumulate colors
			for(int i = 0; i < runningPixels.count (); i++)
			{
				float factor = factorTable[i];
				sum += factor;

				const RGBA& current = runningPixels[i];
				runningPixels[i] = runningPixels[i + 1];
				pixel.red += current.red * factor;
				pixel.green += current.green * factor;
				pixel.blue += current.blue * factor;
				pixel.alpha += current.alpha * factor;
			};
			runningPixels.last () = srcData.rgbaAt (get_min (x + window, srcData.width - 1), y);

			RGBA& dst = dstData.rgbaAt (x, y);

			//copy a pixel
			dst.red = uint8(pixel.red / sum);
			dst.green = uint8(pixel.green / sum);
			dst.blue = uint8(pixel.blue / sum);
			dst.alpha = uint8(pixel.alpha / sum);
		};
	};
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BitmapPrimitives32::blurY (BitmapData& dstData, const BitmapData& srcData, float value)
{
	ASSERT_COMPATIBLE_RGBA (dstData, srcData)
	ASSERT_COMPATIBLE_SIZE (dstData, srcData)

	int window = bound (int(srcData.height * value), 1, srcData.height);
	float sigma2Inv = 1 / (window * window * 0.2f);

	struct AccuPixel {float red, green, blue, alpha;} pixel;
	float sum;
	
	Vector<float> factorTable;
	factorTable.setCount (2 * window + 1);
	for(int i = 0; i < window; i++)
		factorTable[window - 1 - i] = factorTable[window + 1 + i] = expf (-i * i * sigma2Inv);
	factorTable[window] = 1.f;

	Vector<RGBA> runningPixels;
	runningPixels.setCount (2 * window + 1);

	//blur y components
	for(int x = 0; x < srcData.width; x++)
	{
		for(int i = 0; i < window; i++)
		{
			runningPixels[i].red = 0;
			runningPixels[i].blue = 0;
			runningPixels[i].green = 0;
			runningPixels[i].alpha = 0xFF;
		}
		for(int i = window; i <= 2 * window; i++)
			runningPixels[i] = srcData.rgbaAt (x, i - window);

		for(int y = 0; y < srcData.height; y++)
		{
			//process a pixel
			sum = 0;
			pixel.red = 0;
			pixel.blue = 0;
			pixel.green = 0;
			pixel.alpha = 0;

			//accumulate colors
			for(int i = 0; i < runningPixels.count (); i++)
			{
				float factor = factorTable[i];
				sum += factor;

				const RGBA& current = runningPixels[i];
				runningPixels[i] = runningPixels[i + 1];
				pixel.red += current.red * factor;
				pixel.green += current.green * factor;
				pixel.blue += current.blue * factor;
				pixel.alpha += current.alpha * factor;
			};
			runningPixels.last () = srcData.rgbaAt (x, get_min (y + window, srcData.height - 1));

			RGBA& dst = dstData.rgbaAt (x, y);

			//copy a pixel
			dst.red = uint8(pixel.red / sum);
			dst.green = uint8(pixel.green / sum);
			dst.blue = uint8(pixel.blue / sum);
			dst.alpha = uint8(pixel.alpha / sum);
		};
	};
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BitmapPrimitives32::addNoise (BitmapData& dstData, const BitmapData& srcData, float value)
{
	ASSERT_COMPATIBLE_RGBA (dstData, srcData)
	ASSERT_COMPATIBLE_SIZE (dstData, srcData)

	// add simple noise (see-through)
	ForEachRGBA (dstData, dst, srcData, src)
		Color c = toColor (src);
		float intensity = c.getIntensity ();
		float prop = (float) rand () / (float) RAND_MAX;

		if(prop < value)
		{
			c.alpha = 0;
		}
		Color n = c;
		n.setIntensity (intensity);
		dst = toRGBA (n);

	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BitmapPrimitives32::lightAdapt (BitmapData& dstData, const BitmapData& srcData, Color color)
{
	ASSERT_COMPATIBLE_RGBA (dstData, srcData)
	ASSERT_COMPATIBLE_SIZE (dstData, srcData)

	ColorHSL hslColor (color);
	bool isLightColor = (hslColor.l > 0.5f) ? true : false;
	int lightPixels = 0;

	for(int y = 0; y < srcData.height; y++)
	{
		for(int x = 0; x < srcData.width; x++)
		{
			const RGBA& src = srcData.rgbaAt (x, y);
			if(src.alpha > 127)	// at least 0.5f
			{
				float pixelLuminance = ((.3f / 255.f) * (float)src.red + (.59f / 255.f) * src.green + (.11f / 255.f) * (float)src.blue);
				lightPixels += (pixelLuminance > 0.5f) ? 2 : -1; // favour light pixels
			}
		}
	}

	if(lightPixels > 0)
	{
		if(isLightColor) // adapt light pixels
		{
			float lumSub = (1.f - hslColor.l);
			ForEachRGBA (dstData, dst, srcData, src)
				ColorHSL srcPixel = toHSL (src);

				if(srcPixel.l > 0.5f)
					srcPixel.l -= (lumSub * ((srcPixel.l - 0.5f) * 2));

				dst = toRGBA (srcPixel);
			EndFor
		}
		else // adapt light pixels and invert
		{
			float lumSub = hslColor.l;
			ForEachRGBA (dstData, dst, srcData, src)
				ColorHSL srcPixel = toHSL (src);

				if(srcPixel.l > 0.5f)
					srcPixel.l -= (lumSub * ((srcPixel.l - 0.5f) * 2));

				if(src.alpha > 0)
					srcPixel.l = 1.f - (srcPixel.l);

				dst = toRGBA (srcPixel);
			EndFor
		}
	}
	else
	{
		if(isLightColor) // adapt dark pixels and invert
		{
			float lumAdd = (1.f - hslColor.l);
			ForEachRGBA (dstData, dst, srcData, src)
				ColorHSL srcPixel = toHSL (src);

				if(srcPixel.l < 0.5f)
					srcPixel.l += (lumAdd * (1 - (srcPixel.l * 2)));

				if(src.alpha > 0)
					srcPixel.l = 1.f - (srcPixel.l);

				dst = toRGBA (srcPixel);
			EndFor
		}
		else // adapt dark pixels
		{
			float lumAdd = hslColor.l;
			ForEachRGBA (dstData, dst, srcData, src)
				ColorHSL srcPixel = toHSL (src);

				if(srcPixel.l < 0.5f)
					srcPixel.l += (lumAdd * (1 - (srcPixel.l * 2)));

				dst = toRGBA (srcPixel);
			EndFor
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BitmapPrimitives32::tint (BitmapData& dstData, const BitmapData& srcData, Color color)
{
	ASSERT_COMPATIBLE_RGBA (dstData, srcData)
	ASSERT_COMPATIBLE_SIZE (dstData, srcData)

	if(color.isOpaque ())
	{
		ForEachRGBA (dstData, dst, srcData, src)
			Color c = toColor (src);
			float intensity = c.getIntensity ();
			Color n (color);
			n.setIntensity (intensity);
			dst = toRGBA (n);
			dst.alpha = src.alpha;
		EndFor
	}
	else // scale alpha
	{
		ForEachRGBA (dstData, dst, srcData, src)
			Color c = toColor (src);
			float intensity = c.getIntensity ();
			Color n (color);
			float nAlpha = n.getAlphaF ();
			n.setIntensity (intensity * nAlpha);
			c.setIntensity (1 - nAlpha);

			dst = toRGBA (n);
			dst.red += c.red;
			dst.green += c.green;
			dst.blue += c.blue;

			dst.alpha = src.alpha;
		EndFor
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BitmapPrimitives32::colorize (BitmapData& dstData, const BitmapData& srcData, Color color)
{
	ASSERT_COMPATIBLE_RGBA (dstData, srcData)
	ASSERT_COMPATIBLE_SIZE (dstData, srcData)

	RGBA c = toRGBA (color);
	if(color.isOpaque ())
	{
		ForEachRGBA (dstData, dst, srcData, src)
			dst = c;
			dst.alpha = src.alpha;
		EndFor
	}
	else // scale alpha
	{
		float alphaScaler = color.getAlphaF ();
		ForEachRGBA (dstData, dst, srcData, src)
			dst = c;
			float alphaF = (float)src.alpha / 0xFF;
			alphaF *= alphaScaler;
			dst.alpha = Color::setC (alphaF * 255.f);
		EndFor
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BitmapPrimitives32::fillRect (BitmapData& data, RectRef r, ColorRef color)
{
	RGBA value = BitmapPrimitives32::toRGBA (color);

	for(int y = r.top; y < r.bottom; y++)
		for(int x = r.left; x < r.right; x++)
			data.rgbaAt (x, y) = value;
}

//************************************************************************************************
// BitmapPrimitives16
//************************************************************************************************

void BitmapPrimitives16::copyPart (BitmapData& dstData, int dstX, int dstY, const BitmapData& srcData, int srcX, int srcY, int width, int height)
{
	ASSERT_COMPATIBLE_RGB565 (dstData, srcData)

	int dstByteOffset = dstX << 1;
	int srcByteOffset = srcX << 1;
	int bytesToCopy = width << 1;

	for(int y = 0; y < height; y++)
	{
		uint8* dst = (uint8*)dstData.getScanline (dstY + y) + dstByteOffset;
		const uint8* src = (const uint8*)srcData.getScanline (srcY + y) + srcByteOffset;
		::memcpy (dst, src, bytesToCopy);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BitmapPrimitives16::convertToRGBA (BitmapData& dstData, const BitmapData& srcData)
{
	ASSERT (dstData.format == kBitmapRGBAlpha)
	ASSERT (srcData.format == kBitmapRGB565)
	ASSERT_COMPATIBLE_SIZE (dstData, srcData)

	for(int y = 0; y < srcData.height; y++)
		for(int x = 0; x < srcData.width; x++)
			fromRGB565 (dstData.rgbaAt (x, y), srcData.rgb16At (x, y));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

#if CTL_RTOS
CORE_HOT_FUNCTION 
#endif			
void BitmapPrimitives16::fillRect (BitmapData& data, RectRef r, ColorRef color)
{
	uint16 value = toRGB565 (color);

	int32 rowQuadPixelCount = (r.right - r.left) / 4;
	int64 quadPixelVal = ((uint64)value << 48) |
						 ((uint64)value << 32) |
						 ((uint64)value << 16) |
						 ((uint64)value <<  0);
	int32 remainder = (r.right - r.left) % 4;

	for(int y = r.top; y < r.bottom; y++)
	{
		int32 rowQuadCounter = rowQuadPixelCount;
		uint16* pixel = &data.rgb16At (r.left, y);  // first pixel on row
		int32 rowRemainder = remainder;
		if((UIntPtr)pixel % 4 != 0)
		{
			// Handle halfword alignment of row
			*pixel++ = value;
			rowQuadCounter--;  // may decrement to -1 for short row
			rowRemainder = (rowQuadCounter >= 0) ? (remainder + 3) :  // -1 + 4 = 3
												   (remainder - 1);	  // -1
		}

		// Process majority of assignments in compact loop with double word ops
		uint64* quadPixel = (uint64*)pixel;
		while(rowQuadCounter > 8)
		{
			*quadPixel++ = quadPixelVal;	// 1 instruction per C line, 4 pixels
			*quadPixel++ = quadPixelVal;
			*quadPixel++ = quadPixelVal;
			*quadPixel++ = quadPixelVal;
			*quadPixel++ = quadPixelVal;
			*quadPixel++ = quadPixelVal;
			*quadPixel++ = quadPixelVal;
			*quadPixel++ = quadPixelVal;

			rowQuadCounter = rowQuadCounter - 8;
		}
		// Catch last quad pixels in smaller loop
		while(rowQuadCounter-- > 0)
		{
			*quadPixel++ = quadPixelVal;
		}
		// Catch non mod-4 remainder of pixels
		pixel = (uint16*)quadPixel;
		while(rowRemainder-- > 0)
		{
			*pixel++ = value;
		}
	}
}

//************************************************************************************************
// BitmapPrimitivesMonochrome
//************************************************************************************************

void BitmapPrimitivesMonochrome::convertToRGBA (BitmapData& dstData, const BitmapData& srcData)
{
	ASSERT (dstData.format == kBitmapRGBAlpha)
	ASSERT (srcData.format == kBitmapMonochrome)
	ASSERT_COMPATIBLE_SIZE (dstData, srcData)

	RGBA onPixel = BitmapPrimitives32::toRGBA (Color (0xFF, 0xFF, 0xFF)); // white
	RGBA offPixel = BitmapPrimitives32::toRGBA (Color ()); // black

	for(int y = 0; y < srcData.height; y++)
		for(int x = 0; x < srcData.width; x++)
			dstData.rgbaAt (x, y) = srcData.getBit (x, y) ? onPixel : offPixel;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace Core
