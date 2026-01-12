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
// Filename    : core/portable/gui/coregraphics.h
// Description : Embedded Graphics Engine
//
//************************************************************************************************

#ifndef _coregraphics_h
#define _coregraphics_h

#include "core/portable/coretypeinfo.h"
#include "core/portable/gui/corebitmap.h"

namespace Core {
namespace Portable {

class BitmapFont;

#if DEBUG
/** Dump coordinates to debug output. */
void dumpRect (const Rect& rect, const char* string = nullptr);
#endif

//************************************************************************************************
// Colors
//************************************************************************************************

namespace Colors
{
	extern Color kBlack;
	extern Color kWhite;
	extern Color kRed;
	extern Color kGreen;
	extern Color kBlue;
	extern Color kGray;
	extern Color kLtGray;

	extern Color kTransparentBlack; ///< TODO: add Graphics::clearRect() instead!
}

//************************************************************************************************
// BitmapMode
/** \ingroup core_gui */
//************************************************************************************************

struct BitmapMode
{
	enum PaintMode
	{
		kNormal,	///< no bitmap modification
		kColored,	///< reuse source alpha channel with new color
		kBlend		///< blend source with given alpha
	};
	
	PaintMode paintMode;
	Color color;
	float alphaF;

	BitmapMode (PaintMode paintMode = kNormal)
	: paintMode (paintMode),
	  alphaF (1.f)
	{}

	BitmapMode (ColorRef color)
	: paintMode (kColored),
	  color (color),
	  alphaF (1.f)
	{}

	BitmapMode (float alphaF)
	: paintMode (kBlend),
	  alphaF (alphaF)
	{}
};

//************************************************************************************************
// Graphics
/** \ingroup core_gui */
//************************************************************************************************

class Graphics: public TypedObject
{
public:
	DECLARE_CORE_CLASS ('Grph', Graphics, TypedObject)

	virtual bool setOrigin (PointRef point) = 0;
	
	virtual bool setClip (RectRef rect) = 0;

	enum Modes
	{
		kAntiAlias = 1<<0,	///< anti-alias lines (color renderer)
		kInvert = 1<<1		///< invert (monochrome renderer)
	};

	virtual int setMode (int mode) = 0;
	
	virtual bool fillRect (RectRef rect, ColorRef color) = 0;
	
	virtual bool drawRect (RectRef rect, ColorRef color) = 0;

	enum Direction
	{
		kHorizontal,
		kVertical
	};

	virtual bool drawLinearGradient (PointRef startPoint, PointRef endPoint, ColorRef startColor, ColorRef endColor, Direction direction) = 0;
	
	virtual bool drawLine (PointRef p1, PointRef p2, ColorRef color) = 0;
	
	virtual bool drawString (RectRef rect, CStringPtr text, ColorRef color, 
							 CStringPtr fontName = nullptr, int alignment = Alignment::kLeftCenter) = 0;

	virtual bool drawMultiLineString (RectRef rect, CStringPtr text, ColorRef color,
									  CStringPtr fontName = nullptr, int alignment = Alignment::kLeftCenter) = 0;

	virtual int getStringWidth (CStringPtr text, CStringPtr fontName = nullptr) const = 0;
	
	virtual bool drawBitmap (PointRef pos, Bitmap& bitmap, RectRef srcRect, const BitmapMode* mode = nullptr) = 0;

	virtual const BitmapFont* getFont (CStringPtr fontName) const = 0;

	bool drawBitmap (PointRef pos, Bitmap& bitmap, const BitmapMode* mode = nullptr)
	{
		Rect srcRect;
		bitmap.getSize (srcRect);
		return drawBitmap (pos, bitmap, srcRect, mode);
	}
};

//************************************************************************************************
// BitmapPainter
/** \ingroup core_gui */
//************************************************************************************************

class BitmapPainter
{
public:
	static void draw (Graphics& graphics, PointRef pos, Bitmap& bitmap, int frameIndex = 0, const BitmapMode* mode = nullptr)
	{
		Rect srcRect;
		bitmap.getFrame (srcRect, frameIndex);
		graphics.drawBitmap (pos, bitmap, srcRect, mode);
	}

	static void drawCentered (Graphics& graphics, RectRef layoutRect, Bitmap& bitmap, int frameIndex = 0, const BitmapMode* mode = nullptr)
	{
		Rect srcRect;
		bitmap.getFrame (srcRect, frameIndex);
		Rect dstRect (0, 0, srcRect.getWidth (), srcRect.getHeight ());
		dstRect.center (layoutRect);
		graphics.drawBitmap (dstRect.getLeftTop (), bitmap, srcRect, mode);
	}
};

//************************************************************************************************
// GraphicsRenderer
/* Base class for all types of built-in rendering.
	\ingroup core_gui */
//************************************************************************************************

class GraphicsRenderer: public Graphics
{
public:
	DECLARE_CORE_CLASS ('GRnd', GraphicsRenderer, Graphics)

	GraphicsRenderer (RectRef maxClipRect);

	static const int kMaxMultilineStringLength = STRING_STACK_SPACE_MAX;

	// Graphics
	bool setOrigin (PointRef point) override;
	bool setClip (RectRef rect) override;
	int setMode (int mode) override;
	bool fillRect (RectRef rect, ColorRef color) override;
	bool drawRect (RectRef rect, ColorRef color) override;
	bool drawLinearGradient (PointRef startPoint, PointRef endPoint, ColorRef startColor, ColorRef endColor, Direction direction) override;
	bool drawLine (PointRef p1, PointRef p2, ColorRef color) override;
	bool drawString (RectRef rect, CStringPtr text, ColorRef color, 
					 CStringPtr fontName = nullptr, int alignment = Alignment::kLeftCenter) override;
	bool drawMultiLineString (RectRef rect, CStringPtr text, ColorRef color,
							  CStringPtr fontName = nullptr, int alignment = Alignment::kLeftCenter) override;
	int getStringWidth (CStringPtr text, CStringPtr fontName = nullptr) const override;
	const BitmapFont* getFont (CStringPtr fontName) const override;
	bool drawBitmap (PointRef pos, Bitmap& bitmap, RectRef srcRect, const BitmapMode* mode = nullptr) override;

protected:
	Point origin;
	Rect absClipRect;
	Rect maxClipRect;
	int renderMode;
	const BitmapFont* defaultFont;

	typedef CStringBuffer<kMaxMultilineStringLength> LineBuffer;
	bool renderMultiLineChunk (CStringPtr text, const BitmapFont& font, Rect& lineRect, RectRef bounds, ColorRef color, int textAlignment);
	static void splitIntoLines (Vector<LineBuffer>& lines, CStringPtr text);
	
	virtual void fillRectAbsolute (RectRef rect, ColorRef color) = 0;
	virtual void drawHorizontalLineAbsolute (int y, int x0, int x1, ColorRef color) = 0;
	virtual void drawVerticalLineAbsolute (int x, int y0, int y1, ColorRef color) = 0;
	virtual void drawLineAbsolute (int x0, int y0, int x1, int y1, ColorRef color) = 0;
	virtual void drawBitmapFont (const BitmapFont& font, Point pos, CStringPtr text, int length, ColorRef color);
	virtual bool drawBitmapAbsolute (int dstX, int dstY, const Bitmap& bitmap, int srcX, int srcY, int width, int height, const BitmapMode* mode) = 0;
};

//************************************************************************************************
// BitmapGraphicsRenderer
/* Base class for rendering into bitmap.
	\ingroup core_gui */
//************************************************************************************************

class BitmapGraphicsRenderer: public GraphicsRenderer
{
public:
	DECLARE_CORE_CLASS ('BGRd', BitmapGraphicsRenderer, GraphicsRenderer)

	BitmapGraphicsRenderer (Bitmap& bitmap);

	INLINE Bitmap& getBitmap ()
	{
		return bitmap;
	}

	INLINE bool isVisible (int x, int y) const
	{
		return	x >= absClipRect.left && x < absClipRect.right && 
				y >= absClipRect.top && y < absClipRect.bottom;
	}

protected:
	Bitmap& bitmap;
};

//************************************************************************************************
// ColorBitmapRenderer
/** Render into 32 bit RGBA bitmap.
	\ingroup core_gui */
//************************************************************************************************

class ColorBitmapRenderer: public BitmapGraphicsRenderer
{
public:
	ColorBitmapRenderer (Bitmap& bitmap);

	INLINE void setPixel (BitmapData& data, int x, int y, RGBA color)
	{
		if(isVisible (x, y))
			data.rgbaAt (x, y) = color;
	}

protected:
	// BitmapGraphicsRenderer
	void fillRectAbsolute (RectRef rect, ColorRef color) override;
	bool drawLinearGradient (PointRef startPoint, PointRef endPoint, ColorRef startColor, ColorRef endColor, Direction direction) override;
	void drawHorizontalLineAbsolute (int y, int x0, int x1, ColorRef color) override;
	void drawVerticalLineAbsolute (int x, int y0, int y1, ColorRef color) override;
	void drawLineAbsolute (int x0, int y0, int x1, int y1, ColorRef color) override;
	bool drawBitmapAbsolute (int dstX, int dstY, const Bitmap& bitmap, int srcX, int srcY, int width, int height, const BitmapMode* mode) override;
};

//************************************************************************************************
// RGB565BitmapRenderer
/** Render into 16 bit RGB 565 bitmap.
	\ingroup core_gui */
//************************************************************************************************

class RGB565BitmapRenderer: public BitmapGraphicsRenderer
{
public:
	RGB565BitmapRenderer (Bitmap& bitmap);

	INLINE void setPixel (BitmapData& data, int x, int y, uint16 color)
	{
		if(isVisible (x, y))
			data.rgb16At (x, y) = color;
	}

protected:
	// BitmapGraphicsRenderer
	void fillRectAbsolute (RectRef rect, ColorRef color) override;
	bool drawLinearGradient (PointRef startPoint, PointRef endPoint, ColorRef startColor, ColorRef endColor, Direction direction) override;
	void drawHorizontalLineAbsolute (int y, int x0, int x1, ColorRef color) override;
	void drawVerticalLineAbsolute (int x, int y0, int y1, ColorRef color) override;
	void drawLineAbsolute (int x0, int y0, int x1, int y1, ColorRef color) override;
	bool drawBitmapAbsolute (int dstX, int dstY, const Bitmap& bitmap, int srcX, int srcY, int width, int height, const BitmapMode* mode) override;
};

//************************************************************************************************
// MonoBitmapRenderer
/** Render into monochrome bitmap, white (Colors::kWhite) represents a lit pixel. 
	Exception: text rendering ignores color, use BitmapFont::kInvertColor to invert it.
	\ingroup core_gui */
//************************************************************************************************

class MonoBitmapRenderer: public BitmapGraphicsRenderer
{
public:
	MonoBitmapRenderer (Bitmap& bitmap);

	INLINE void setPixel (BitmapData& data, int x, int y, bool state)
	{
		if(isVisible (x, y))
			data.setBit (x, y, state);
	}

protected:
	bool isInvertMode () const { return (renderMode & kInvert) != 0; }

	// BitmapGraphicsRenderer
	void fillRectAbsolute (RectRef rect, ColorRef color) override;
	void drawHorizontalLineAbsolute (int y, int x0, int x1, ColorRef color) override;
	void drawVerticalLineAbsolute (int x, int y0, int y1, ColorRef color) override;
	void drawLineAbsolute (int x0, int y0, int x1, int y1, ColorRef color) override;
	bool drawBitmapAbsolute (int dstX, int dstY, const Bitmap& bitmap, int srcX, int srcY, int width, int height, const BitmapMode* mode) override;
};

//************************************************************************************************
// IGraphicsCommandSink
/** Sink for graphics commands.
	\ingroup core_gui */
//************************************************************************************************

struct IGraphicsCommandSink
{
	virtual void setClip (RectRef rect) = 0;
	virtual void fillRect (RectRef rect, ColorRef color) = 0;
	virtual void drawHorizontalLine (int y, int x0, int x1, ColorRef color) = 0;
	virtual void drawVerticalLine (int x, int y0, int y1, ColorRef color) = 0;
	virtual void drawLine (int x0, int y0, int x1, int y1, ColorRef color) = 0;
	virtual void drawString (const BitmapFont& font, Point pos, CStringPtr text, int length, ColorRef color) = 0;
	virtual void drawBitmap (int dstX, int dstY, const Bitmap& bitmap, int srcX, int srcY, int width, int height) = 0;
};

//************************************************************************************************
// GraphicsCommandRenderer
/** Render graphics commands into given sink.
	\ingroup core_gui */
//************************************************************************************************

class GraphicsCommandRenderer: public GraphicsRenderer
{
public:
	GraphicsCommandRenderer (IGraphicsCommandSink& commandSink, RectRef maxClipRect, bool monochrome);

	// GraphicsRenderer
	bool setClip (RectRef rect) override;

protected:
	IGraphicsCommandSink& commandSink;
	bool monochrome;
	bool clipChanged;

	void flushClip ();

	// GraphicsRenderer
	void fillRectAbsolute (RectRef rect, ColorRef color) override;
	void drawHorizontalLineAbsolute (int y, int x0, int x1, ColorRef color) override;
	void drawVerticalLineAbsolute (int x, int y0, int y1, ColorRef color) override;
	void drawLineAbsolute (int x0, int y0, int x1, int y1, ColorRef color) override;
	void drawBitmapFont (const BitmapFont& font, Point pos, CStringPtr text, int length, ColorRef color) override;
	bool drawBitmapAbsolute (int dstX, int dstY, const Bitmap& bitmap, int srcX, int srcY, int width, int height, const BitmapMode* mode) override;
};

} // namespace Portable
} // namespace Core

#endif // _coregraphics_h
