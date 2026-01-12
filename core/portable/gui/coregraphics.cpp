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
// Filename    : core/portable/gui/coregraphics.cpp
// Description : Embedded Graphics Engine
//
//************************************************************************************************

#include "coregraphics.h"
#include "corefont.h"

#include "core/system/coredebug.h"

#include <math.h>

namespace Core {
namespace Portable {

//////////////////////////////////////////////////////////////////////////////////////////////////

#if DEBUG
void dumpRect (const Rect& rect, const char* string)
{
	DebugPrintf ("%s (%d, %d)-(%d, %d) %d x %d\n",
				string ? string : "Rect",
				rect.left, rect.top, rect.right, rect.bottom,
				rect.getWidth (), rect.getHeight ());
}
#endif

//************************************************************************************************
// GraphicsAlgorithm
//************************************************************************************************

class GraphicsAlgorithm
{
public:
	template <typename Target, typename ColorType>
	static void drawLine (Target& target, BitmapData& data, int x0, int y0, int x1, int y1, ColorType color)
	{
		int dx = x1 - x0;
		int dy = y1 - y0;

		int xs, ys;
		if(dx < 0) { dx = -dx; xs = -1; } else xs = 1;
		if(dy < 0) { dy = -dy; ys = -1; } else ys = 1;

		dx <<= 1;
		dy <<= 1;

		target.setPixel (data, x0, y0, color);
		if(dx > dy)
		{
			int fract = dy - (dx >> 1);
			while(x0 != x1)
			{
				if(fract >= 0)
				{
					y0 += ys;
					fract -= dx;
				}
				x0 += xs;
				fract += dy;
				target.setPixel (data, x0, y0, color);
			}
		}
		else
		{
			int fract = dx - (dy >> 1);
			while(y0 != y1)
			{
				if(fract >= 0)
				{
					x0 += xs;
					fract -= dy;
				}
				y0 += ys;
				fract += dx;
				target.setPixel (data, x0, y0, color);
			}
		}
	}

	// Anti-Aliased Line Drawing 
	// http://en.wikipedia.org/wiki/Xiaolin_Wu%27s_line_algorithm

	template <typename T> static void swap (T& a, T&b) { T tmp = a; a = b; b = tmp; }
	static int round (double x) { return (int)(x + 0.5); }
	static int ipart (double x) { return (int)x; }
	static double fpart (double x) { return x - (double)ipart (x); }
	static double rfpart (double x) { return 1.0 - fpart (x); }

	static void drawLineAntialias (ColorBitmapRenderer& target, BitmapData& data, int x0, int y0, int x1, int y1, Color color)
	{
		struct Plotter
		{
			ColorBitmapRenderer& target;
			BitmapData& data;
			Color color;

			Plotter (ColorBitmapRenderer& target,
					 BitmapData& data,
					 Color color)
			: target (target),
			  data (data),
			  color (color)
			{}

			INLINE void plot (int x, int y, double brightness)
			{
				Color dstColor = BitmapPrimitives32::toColor (data.rgbaAt (x, y));
				dstColor.alphaBlend (color, (float)brightness);
				target.setPixel (data, x, y, BitmapPrimitives32::toRGBA (dstColor));
			}
		};

		Plotter plotter (target, data, color);
		#define plot(x,y,b) plotter.plot (x, y, b)

		bool steep = abs (y1 - y0) > abs (x1 - x0);
		if(steep)
		{
			swap (x0, y0);
			swap (x1, y1);
		}

		if(x0 > x1)
		{
		   swap (x0, x1);
		   swap (y0, y1);
		}

		int dx = x1 - x0;
		int dy = y1 - y0;
		double gradient = (double)dy / (double)dx;
 
		 // handle first endpoint
		 int xend = round(x0);
		 double yend = y0 + gradient * (xend - x0);
		 double xgap = rfpart (x0 + 0.5);
		 int xpxl1 = xend; // this will be used in the main loop
		 int ypxl1 = ipart(yend);
		 if(steep)
		 {
			 plot (ypxl1,   xpxl1, rfpart(yend) * xgap);
			 plot (ypxl1+1, xpxl1,  fpart(yend) * xgap);
		 }
		 else
		 {
			 plot (xpxl1, ypxl1  , rfpart(yend) * xgap);
			 plot (xpxl1, ypxl1+1,  fpart(yend) * xgap);
		 }

		double intery = yend + gradient; // first y-intersection for the main loop
 
		// handle second endpoint
		xend = round(x1);
		yend = y1 + gradient * (xend - x1);
		xgap = fpart(x1 + 0.5);
		int xpxl2 = xend; // this will be used in the main loop
		int ypxl2 = ipart (yend);
		if(steep)
		{
			plot (ypxl2  , xpxl2, rfpart(yend) * xgap);
			plot (ypxl2+1, xpxl2,  fpart(yend) * xgap);
		}
		else
		{
			plot (xpxl2, ypxl2,  rfpart(yend) * xgap);
			plot (xpxl2, ypxl2+1, fpart(yend) * xgap);
		}
 
		// main loop
		for(int x = xpxl1 + 1; x <= xpxl2 - 1; x++)
		{
			if(steep)
			{
				plot (ipart (intery)  , x, rfpart (intery));
				plot (ipart (intery)+1, x,  fpart (intery));
			}
			else
			{
				plot (x, ipart (intery),  rfpart (intery));
				plot (x, ipart (intery)+1, fpart (intery));
			}

			intery = intery + gradient;
		}

		#undef plot
	}

	template <typename Target, typename ColorType>
	static void drawLinearGradient (Target target, BitmapData& data, PointRef startPoint, PointRef endPoint, ColorType startColor, ColorType endColor, Graphics::Direction direction)
	{
		float t = 0.0f;

		if(direction == Graphics::kHorizontal)
		{
			float step = 1.0f / static_cast<float>(endPoint.x - startPoint.x);

			for(int x = startPoint.x; x < endPoint.x; x++)
			{
				ColorType color = lerp (t, startColor, endColor);

				for(int y = startPoint.y; y < endPoint.y; y++)
					target.setPixel (data, x, y, color);
		
				t += step;
			}
		}
		else
		{
			float step = 1.0f / static_cast<float>(endPoint.y - startPoint.y);

			for(int y = startPoint.y; y < endPoint.y; y++)
			{
				ColorType color = lerp (t, startColor, endColor);

				for(int x = startPoint.x; x < endPoint.x; x++)
					target.setPixel (data, x, y, color);
		
				t += step;
			}
		}
	}

private:
	static uint16 lerp (float t, uint16 colorA, uint16 colorB)
	{
		uint8 rgbA[3] = { 0 };
		BitmapPrimitives16::fromRGB565 (rgbA, colorA);

		uint8 rgbB[3] = { 0 };
		BitmapPrimitives16::fromRGB565 (rgbB, colorB);

		uint8 result[3] = { 0 };
		result[0] = static_cast<uint8> ((1.0f - t) * rgbA[0] + t * rgbB[0]);
		result[1] = static_cast<uint8> ((1.0f - t) * rgbA[1] + t * rgbB[1]);
		result[2] = static_cast<uint8> ((1.0f - t) * rgbA[2] + t * rgbB[2]);

		return BitmapPrimitives16::toRGB565 (result);
	}

	static RGBA lerp (float t, RGBA colorA, RGBA colorB)
	{
		RGBA result = { 0 };
		result.red = static_cast<uint8> ((1.0f - t) * colorA.red + t * colorB.red);
		result.green = static_cast<uint8> ((1.0f - t) * colorA.green + t * colorB.green);
		result.blue = static_cast<uint8> ((1.0f - t) * colorA.blue + t * colorB.blue);
		result.alpha = static_cast<uint8> ((1.0f - t) * colorA.alpha + t * colorB.alpha);

		return result;
	}
};

} // namespace Portable
} // namespace Core

using namespace Core;
using namespace Portable;

//************************************************************************************************
// Colors
//************************************************************************************************

Color Colors::kBlack (0x00, 0x00, 0x00);
Color Colors::kTransparentBlack (0x00, 0x00, 0x00, 0x00);
Color Colors::kWhite (0xFF, 0xFF, 0xFF);
Color Colors::kRed (0xFF, 0x00, 0x00);
Color Colors::kGreen (0x00, 0xFF, 0x00);
Color Colors::kBlue (0x00, 0x00, 0xFF);
Color Colors::kGray   (0x86, 0x86, 0x86);
Color Colors::kLtGray (0xD3, 0xD3, 0xD3);

//************************************************************************************************
// GraphicsRenderer
//************************************************************************************************

GraphicsRenderer::GraphicsRenderer (RectRef maxClipRect)
: maxClipRect (maxClipRect),
  renderMode (0),
  defaultFont (nullptr)
{
	absClipRect = maxClipRect;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GraphicsRenderer::setOrigin (PointRef point)
{
	origin = point;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GraphicsRenderer::setClip (RectRef rect)
{
	absClipRect = rect;
	absClipRect.offset (origin);
	absClipRect.bound (maxClipRect);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int GraphicsRenderer::setMode (int _mode)
{
	int oldMode = renderMode;
	renderMode = _mode;
	return oldMode;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GraphicsRenderer::fillRect (RectRef _rect, ColorRef color)
{
	Rect r (_rect);
	r.offset (origin);
	r.bound (absClipRect);

	if(!r.isEmpty ())
		fillRectAbsolute (r, color);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GraphicsRenderer::drawRect (RectRef _rect, ColorRef color)
{
	Rect r (_rect);
	r.offset (origin);

	drawHorizontalLineAbsolute (r.top, r.left, r.right, color);
	drawVerticalLineAbsolute (r.left, r.top + 1, r.bottom - 1, color);
	drawHorizontalLineAbsolute (r.bottom - 1, r.left, r.right, color);
	drawVerticalLineAbsolute (r.right - 1, r.top + 1, r.bottom - 1, color);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GraphicsRenderer::drawLinearGradient (PointRef startPoint, PointRef endPoint, ColorRef startColor, ColorRef endColor, Direction direction)
{
	ASSERT (false); // not implemented
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GraphicsRenderer::drawLine (PointRef p1, PointRef p2, ColorRef color)
{
	if(p1.y == p2.y)
		drawHorizontalLineAbsolute (p1.y + origin.y, p1.x + origin.x, p2.x + origin.x, color);
	else if(p1.x == p2.x)
		drawVerticalLineAbsolute (p1.x + origin.x, p1.y + origin.y, p2.y + origin.y, color);
	else
		drawLineAbsolute (p1.x + origin.x, p1.y + origin.y, p2.x + origin.x, p2.y + origin.y, color);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const BitmapFont* GraphicsRenderer::getFont (CStringPtr fontName) const
{
	const BitmapFont* font = FontManager::instance ().getFont (fontName);
	if(font == nullptr)
		font = defaultFont;
	return font;	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GraphicsRenderer::drawString (RectRef rect, CStringPtr text, ColorRef color, CStringPtr fontName, int alignment)
{
	const BitmapFont* font = getFont (fontName);
	ASSERT (font != nullptr)
	if(font == nullptr)
		return false;

	Point pos (rect.left, rect.top);
	int length = ConstString (text).length ();

	int alignH = alignment & Alignment::kHMask;
	if(alignH != Alignment::kLeft)
	{
		int textWidth = font->getStringWidth (text, length);
		switch(alignH)
		{
		case Alignment::kRight : pos.x = rect.right - textWidth; break;
		case Alignment::kHCenter : pos.x = rect.left + (rect.getWidth () - textWidth)/2; break;
		}
	}

	int alignV = alignment & Alignment::kVMask;
	if(alignV != Alignment::kTop)
	{
		int textHeight = font->getLineHeight ();
		switch(alignV)
		{
		case Alignment::kBottom : pos.y = rect.bottom - textHeight;  break;
		case Alignment::kVCenter : pos.y = rect.top + (rect.getHeight () - textHeight)/2; break;
		}
	}

	drawBitmapFont (*font, pos, text, length, color);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GraphicsRenderer::renderMultiLineChunk (CStringPtr text, const BitmapFont& font, Rect& lineRect, RectRef bounds, ColorRef color, int textAlignment)
{
	if(lineRect.top > bounds.bottom || lineRect.left > bounds.right)
	{
		#if DEBUG
		DebugPrintf ("Warning: GraphicsRenderer attempting to draw text \"%s\" offscreen", text);
		#endif
		return false;
	}
	
	Coord lineHeight = font.getLineHeight ();
	Coord lineWidth = bounds.getWidth ();
		
	CStringTokenizer tokenizer (text, " ");
	LineBuffer lineText;
	LineBuffer testText;
	int wordsInLine = 0;
	while(true)
	{
		CStringPtr token = tokenizer.next ();
		LineBuffer word (token);
		if(token)
		{
			int tabIndex = word.index ('\t');
			if(tabIndex >= 0)
			{
				static const CStringPtr kTabString = "        "; // tab width = 8
				word = word.replace (tabIndex, 1, kTabString);
			}
			
			if(wordsInLine > 0)
				testText.append (" ");
			testText.append (word);
		}
		
		if(token == nullptr || font.getStringWidth (testText, testText.length ()) > lineWidth)
		{
			CStringPtr textPtr = wordsInLine == 0 ? testText : lineText;
			if(textPtr)
				drawString (lineRect, textPtr, color, font.getName (), textAlignment);
			
			if(token)
			{
				lineRect.offset (0, lineHeight);
				testText.empty ();
				lineText.empty ();
				testText.append (token);
				lineText = testText;
				wordsInLine = 1;
				
				if(lineRect.top >= bounds.bottom)
					return true;
			}
			else
				return true;
		}
		else
		{
			if(wordsInLine > 0)
				lineText.append (" ");
			lineText.append (word);
			wordsInLine++;
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/*static*/void GraphicsRenderer::splitIntoLines (Vector<LineBuffer>& lines, CStringPtr text)
{
	LineBuffer remainder (text);
	while(true)
	{
		int linefeedIndex = remainder.index ('\n');
		if(linefeedIndex < 0)
		{
			lines.add (remainder);
			return;
		}
		
		LineBuffer line;
		remainder.subString (line, 0, linefeedIndex);
		lines.add (line);
		remainder.subString (remainder, linefeedIndex+1);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GraphicsRenderer::drawMultiLineString (RectRef rect, CStringPtr text, ColorRef color, CStringPtr fontName, int textAlignment)
{
	const BitmapFont* font = getFont (fontName);
	ASSERT (font);
	if(!font)
		return false;
		
	Coord lineHeight = font->getLineHeight ();
	Rect lineRect (rect);
	lineRect.setHeight (lineHeight);

	Vector<LineBuffer> lines;
	splitIntoLines (lines, text);
	VectorForEach (lines, CStringPtr, line)
		if(!renderMultiLineChunk (line, *font, lineRect, rect, color, textAlignment))
			break;
		
		lineRect.offset (0, lineHeight);
	EndFor
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int GraphicsRenderer::getStringWidth (CStringPtr text, CStringPtr fontName) const
{
	const BitmapFont* font = FontManager::instance ().getFont (fontName);
	if(font == nullptr)
		font = defaultFont;
	ASSERT (font != nullptr)
	if(font == nullptr)
		return 0;

	int length = ConstString (text).length ();
	return font->getStringWidth (text, length);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GraphicsRenderer::drawBitmapFont (const BitmapFont& font, Point pos, CStringPtr text, int length, ColorRef color)
{
	font.render (*this, pos, text, length, color);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GraphicsRenderer::drawBitmap (PointRef pos, Bitmap& srcBitmap, RectRef _srcRect, const BitmapMode* mode)
{
	// make sure source size is valid
	Rect srcRect (_srcRect);
	Rect maxSrcRect;
	srcBitmap.getSize (maxSrcRect);
	srcRect.bound (maxSrcRect);

	// calculate destination size
	Rect dstRect (0, 0, srcRect.getWidth (), srcRect.getHeight ());
	dstRect.offset (origin);
	dstRect.offset (pos);
	
	// bound dstRect to clip
	Rect dstRectUnbound (dstRect);
	dstRect.bound (absClipRect);

	bool result = true;
	if(!dstRect.isEmpty ())
	{
		// apply clip bounds to srcRect
		srcRect.top += dstRect.top - dstRectUnbound.top;
		srcRect.bottom -= dstRectUnbound.bottom - dstRect.bottom;
		srcRect.left += dstRect.left - dstRectUnbound.left;
		srcRect.right -= dstRectUnbound.right - dstRect.right;
	
		result = drawBitmapAbsolute (dstRect.left, dstRect.top, srcBitmap, srcRect.left, srcRect.top, srcRect.getWidth (), srcRect.getHeight (), mode);
	}

	#if DEBUG
	if(result == false)
		DebugPrintf ("Failed to draw bitmap %p!\n", &srcBitmap);
	#endif

	return result;
}

//************************************************************************************************
// BitmapGraphicsRenderer
//************************************************************************************************

BitmapGraphicsRenderer::BitmapGraphicsRenderer (Bitmap& bitmap)
: GraphicsRenderer (bitmap.getSize ()),
  bitmap (bitmap)
{}

//************************************************************************************************
// ColorBitmapRenderer
//************************************************************************************************

ColorBitmapRenderer::ColorBitmapRenderer (Bitmap& bitmap)
: BitmapGraphicsRenderer (bitmap)
{
	ASSERT (bitmap.getFormat () == kBitmapRGBAlpha)
	ASSERT (bitmap.isAlphaChannelUsed () == false)

	defaultFont = FontManager::instance ().getDefaultColorFont ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorBitmapRenderer::fillRectAbsolute (RectRef r, ColorRef color)
{
	BitmapData& data = bitmap.accessForWrite ();

	BitmapPrimitives32::fillRect (data, r, color);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ColorBitmapRenderer::drawLinearGradient (PointRef startPoint, PointRef endPoint, ColorRef startColor, ColorRef endColor, Direction direction)
{
	RGBA startColor32 = BitmapPrimitives32::toRGBA (startColor);
	RGBA endColor32 = BitmapPrimitives32::toRGBA (endColor);
	BitmapData& data = bitmap.accessForWrite ();

	GraphicsAlgorithm::drawLinearGradient<ColorBitmapRenderer, RGBA> (*this, data, startPoint, endPoint, startColor32, endColor32, direction);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorBitmapRenderer::drawHorizontalLineAbsolute (int y, int x0, int x1, ColorRef _color)
{
	BitmapData& data = bitmap.accessForWrite ();
	RGBA color = BitmapPrimitives32::toRGBA (_color);

	for(int x = x0; x < x1; x++)
		setPixel (data, x, y, color);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorBitmapRenderer::drawVerticalLineAbsolute (int x, int y0, int y1, ColorRef _color)
{
	BitmapData& data = bitmap.accessForWrite ();
	RGBA color = BitmapPrimitives32::toRGBA (_color);

	for(int y = y0; y < y1; y++)
		setPixel (data, x, y, color);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorBitmapRenderer::drawLineAbsolute (int x0, int y0, int x1, int y1, ColorRef color)
{
	BitmapData& data = bitmap.accessForWrite ();

	if(renderMode & kAntiAlias)
		GraphicsAlgorithm::drawLineAntialias (*this, data, x0, y0, x1, y1, color);
	else
		GraphicsAlgorithm::drawLine<ColorBitmapRenderer, RGBA> (*this, data, x0, y0, x1, y1, BitmapPrimitives32::toRGBA (color));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ColorBitmapRenderer::drawBitmapAbsolute (int dstX, int dstY, const Bitmap& srcBitmap,
											  int srcX, int srcY, int width, int height, 
											  const BitmapMode* mode)
{
	BitmapData& dstData = bitmap.accessForWrite ();
	const BitmapData& srcData = srcBitmap.accessForRead ();
	if(dstData.format != srcData.format)
		return false;

	ASSERT (!mode || mode->paintMode == BitmapMode::kNormal)

	if(srcBitmap.isAlphaChannelUsed ())
	{
		for(int y = 0; y < height; y++)
			for(int x = 0; x < width; x++)
			{
				RGBA& dst = dstData.rgbaAt (dstX + x, dstY + y);
				const RGBA& src = srcData.rgbaAt (srcX + x, srcY + y);

				float factor = 1.f - (float)src.alpha / 255.f;

				dst.red = Color::setC ((float)src.red + factor * dst.red); 
				dst.green = Color::setC ((float)src.green + factor * dst.green); 
				dst.blue = Color::setC ((float)src.blue + factor * dst.blue); 

				dst.alpha = 0xFF;
			}
	}
	else
	{
		#if 0 // explicitly maintain alpha channel
		for(int y = 0; y < height; y++)
			for(int x = 0; x < width; x++)
			{
				RGBA& dst = dstData.rgbaAt (dstX + x, dstY + y);
				const RGBA& src = srcData.rgbaAt (srcX + x, srcY + y);
				dst = src;

				dst.alpha = 0xFF;
			}
		#else // source bitmap must have alpha channel set to FF
		BitmapPrimitives32::copyPart (dstData, dstX, dstY, srcData, srcX, srcY, width, height);
		#endif
	}
	return true;
}

//************************************************************************************************
// RGB565BitmapRenderer
//************************************************************************************************

RGB565BitmapRenderer::RGB565BitmapRenderer (Bitmap& bitmap)
: BitmapGraphicsRenderer (bitmap)
{
	ASSERT (bitmap.getFormat () == kBitmapRGB565)

	defaultFont = FontManager::instance ().getDefaultColorFont ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RGB565BitmapRenderer::fillRectAbsolute (RectRef r, ColorRef color) 
{
	BitmapData& data = bitmap.accessForWrite ();

	BitmapPrimitives16::fillRect (data, r, color);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RGB565BitmapRenderer::drawLinearGradient (PointRef startPoint, PointRef endPoint, ColorRef startColor, ColorRef endColor, Direction direction)
{
	uint16 startColor16 = BitmapPrimitives16::toRGB565 (startColor);
	uint16 endColor16 = BitmapPrimitives16::toRGB565 (endColor);
	BitmapData& data = bitmap.accessForWrite ();

	GraphicsAlgorithm::drawLinearGradient<RGB565BitmapRenderer, uint16> (*this, data, startPoint, endPoint, startColor16, endColor16, direction);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RGB565BitmapRenderer::drawHorizontalLineAbsolute (int y, int x0, int x1, ColorRef _color)
{
	BitmapData& data = bitmap.accessForWrite ();
	uint16 color = BitmapPrimitives16::toRGB565 (_color);

	for(int x = x0; x < x1; x++)
		setPixel (data, x, y, color);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RGB565BitmapRenderer::drawVerticalLineAbsolute (int x, int y0, int y1, ColorRef _color)
{
	BitmapData& data = bitmap.accessForWrite ();
	uint16 color = BitmapPrimitives16::toRGB565 (_color);

	for(int y = y0; y < y1; y++)
		setPixel (data, x, y, color);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RGB565BitmapRenderer::drawLineAbsolute (int x0, int y0, int x1, int y1, ColorRef _color)
{
	BitmapData& data = bitmap.accessForWrite ();
	uint16 color = BitmapPrimitives16::toRGB565 (_color);

	GraphicsAlgorithm::drawLine<RGB565BitmapRenderer, uint16> (*this, data, x0, y0, x1, y1, color);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RGB565BitmapRenderer::drawBitmapAbsolute (int dstX, int dstY, const Bitmap& srcBitmap, 
											   int srcX, int srcY, int width, int height, 
											   const BitmapMode* mode)
{
	BitmapData& dstData = bitmap.accessForWrite ();
	const BitmapData& srcData = srcBitmap.accessForRead ();

	if(srcBitmap.isAlphaChannelUsed ()) // blend RGBA on RGB565
	{
		if(srcData.format != kBitmapRGBAlpha)
			return false;

		if(mode && mode->paintMode == BitmapMode::kColored)
		{
			uint16 colorPixel = BitmapPrimitives16::toRGB565 (mode->color);
			
			for(int y = 0; y < height; y++)
				for(int x = 0; x < width; x++)
				{
					uint16& dst = dstData.rgb16At (dstX + x, dstY + y);
					const RGBA& src = srcData.rgbaAt (srcX + x, srcY + y);

					dst = BitmapPrimitives16::alphaBlend (colorPixel, dst, src.alpha);
				}			
		}
		else if(mode && mode->paintMode == BitmapMode::kBlend)
		{
			for(int y = 0; y < height; y++)
				for(int x = 0; x < width; x++)
				{
					uint16& dst = dstData.rgb16At (dstX + x, dstY + y);
					const RGBA& src = srcData.rgbaAt (srcX + x, srcY + y);

					uint8 newAlpha = Color::setC ((float(src.alpha) / 255.f) * mode->alphaF);

					uint16 fg = BitmapPrimitives16::toRGB565 (src);
					dst = BitmapPrimitives16::alphaBlend (fg, dst, newAlpha);
				}
		}
		else
		{
			ASSERT (!mode || mode->paintMode == BitmapMode::kNormal)

			for(int y = 0; y < height; y++)
				for(int x = 0; x < width; x++)
				{
					uint16& dst = dstData.rgb16At (dstX + x, dstY + y);
					const RGBA& src = srcData.rgbaAt (srcX + x, srcY + y);

					uint16 fg = BitmapPrimitives16::toRGB565 (src);
					dst = BitmapPrimitives16::alphaBlend (fg, dst, src.alpha);
				}
		}
	}
	else
	{
		if(srcData.format != kBitmapRGB565)
			return false;

		if(mode && mode->paintMode == BitmapMode::kBlend)
		{
			uint8 alpha = Color::setC (mode->alphaF * 255.f);

			for(int y = 0; y < height; y++)
				for(int x = 0; x < width; x++)
				{
					uint16& dst = dstData.rgb16At (dstX + x, dstY + y);
					uint16 src = srcData.rgb16At (srcX + x, srcY + y);

					dst = BitmapPrimitives16::alphaBlend (src, dst, alpha);
				}
		}
		else
		{
			ASSERT (!mode || mode->paintMode == BitmapMode::kNormal)

			BitmapPrimitives16::copyPart (dstData, dstX, dstY, srcData, srcX, srcY, width, height);
		}
	}
	return true;
}

//************************************************************************************************
// MonoBitmapRenderer
//************************************************************************************************

MonoBitmapRenderer::MonoBitmapRenderer (Bitmap& bitmap)
: BitmapGraphicsRenderer (bitmap)
{
	ASSERT (bitmap.getFormat () == kBitmapMonochrome)

	defaultFont = FontManager::instance ().getDefaultMonoFont ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MonoBitmapRenderer::fillRectAbsolute (RectRef r, ColorRef color)
{
	BitmapData& data = bitmap.accessForWrite ();
	bool state = color == Colors::kWhite && !isInvertMode ();

	for(int y = r.top; y < r.bottom; y++)
		for(int x = r.left; x < r.right; x++)
			data.setBit (x, y, state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MonoBitmapRenderer::drawHorizontalLineAbsolute (int y, int x0, int x1, ColorRef color)
{
	BitmapData& data = bitmap.accessForWrite ();
	bool state = color == Colors::kWhite && !isInvertMode ();

	for(int x = x0; x < x1; x++)
		setPixel (data, x, y, state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MonoBitmapRenderer::drawVerticalLineAbsolute (int x, int y0, int y1, ColorRef color)
{
	BitmapData& data = bitmap.accessForWrite ();
	bool state = color == Colors::kWhite && !isInvertMode ();

	for(int y = y0; y < y1; y++)
		setPixel (data, x, y, state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MonoBitmapRenderer::drawLineAbsolute (int x0, int y0, int x1, int y1, ColorRef color)
{
	BitmapData& data = bitmap.accessForWrite ();
	bool state = color == Colors::kWhite && !isInvertMode ();

	GraphicsAlgorithm::drawLine<MonoBitmapRenderer, bool> (*this, data, x0, y0, x1, y1, state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MonoBitmapRenderer::drawBitmapAbsolute (int dstX, int dstY, const Bitmap& srcBitmap, 
											 int srcX, int srcY, int width, int height,
											 const BitmapMode* mode)
{
	BitmapData& dstData = bitmap.accessForWrite ();
	const BitmapData& srcData = srcBitmap.accessForRead ();
	if(dstData.format != srcData.format)
		return false;

	ASSERT (!mode || mode->paintMode == BitmapMode::kNormal)

	if(isInvertMode ())
	{
		for(int y = 0; y < height; y++)
			for(int x = 0; x < width; x++)
			{
				bool state = srcData.getBit (srcX + x, srcY + y);
				dstData.setBit (dstX + x, dstY + y, !state); // <-- inverted here
			}

	}
	else
	{
		for(int y = 0; y < height; y++)
			for(int x = 0; x < width; x++)
			{
				bool state = srcData.getBit (srcX + x, srcY + y);
				dstData.setBit (dstX + x, dstY + y, state);
			}
	}

	return true;
}

//************************************************************************************************
// GraphicsCommandRenderer
//************************************************************************************************

GraphicsCommandRenderer::GraphicsCommandRenderer (IGraphicsCommandSink& commandSink, RectRef maxClipRect, bool monochrome)
: GraphicsRenderer (maxClipRect),
  commandSink (commandSink),
  monochrome (monochrome),
  clipChanged (true) // make sure to notify initial state
{
	if(monochrome)
		defaultFont = FontManager::instance ().getDefaultMonoFont ();
	else
		defaultFont = FontManager::instance ().getDefaultColorFont ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GraphicsCommandRenderer::setClip (RectRef rect)
{
	Rect lastClipRect = absClipRect;
	GraphicsRenderer::setClip (rect);
	if(absClipRect != lastClipRect)
		clipChanged = true;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GraphicsCommandRenderer::flushClip ()
{
	if(clipChanged)
	{
		clipChanged = false;
		commandSink.setClip (absClipRect);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GraphicsCommandRenderer::fillRectAbsolute (RectRef r, ColorRef color)
{
	flushClip ();
	commandSink.fillRect (r, color);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GraphicsCommandRenderer::drawHorizontalLineAbsolute (int y, int x0, int x1, ColorRef color)
{
	flushClip ();
	commandSink.drawHorizontalLine (y, x0, x1, color);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GraphicsCommandRenderer::drawVerticalLineAbsolute (int x, int y0, int y1, ColorRef color)
{
	flushClip ();
	commandSink.drawVerticalLine (x, y0, y1, color);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GraphicsCommandRenderer::drawLineAbsolute (int x0, int y0, int x1, int y1, ColorRef color)
{
	flushClip ();
	commandSink.drawLine (x0, y0, x1, y1, color);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GraphicsCommandRenderer::drawBitmapFont (const BitmapFont& font, Point _pos, CStringPtr text, int length, ColorRef _color)
{
	flushClip ();
	Point pos (_pos);
	pos.offset (origin);

	Color color (_color);
	if(monochrome == true)
	{
		// workaround for inconsistency with monochrome fonts:
		// any color represents a lit pixel, except the special value for inversion
		if(color == BitmapFont::kInvertColor)
			color = Colors::kBlack;
		else
			color = Colors::kWhite;
	}

	commandSink.drawString (font, pos, text, length, color);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GraphicsCommandRenderer::drawBitmapAbsolute (int dstX, int dstY, const Bitmap& srcBitmap, 
												  int srcX, int srcY, int width, int height,
												  const BitmapMode* mode)
{
	if(width == 0 || height == 0)
		return true;
	if(monochrome && srcBitmap.getFormat () != kBitmapMonochrome)
		return false;

	ASSERT (!mode || mode->paintMode == BitmapMode::kNormal)

	flushClip ();
	commandSink.drawBitmap (dstX, dstY, srcBitmap, srcX, srcY, width, height);
	return true;
}
