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
// Filename    : ccl/public/gui/graphics/igraphics.h
// Description : 2D Graphics Interface
//
//************************************************************************************************

#ifndef _ccl_igraphics_h
#define _ccl_igraphics_h

#include "ccl/public/gui/graphics/types.h"

#include "ccl/meta/generated/cpp/graphics-constants-generated.h"

namespace CCL {

//************************************************************************************************
// IGraphics
/** Graphics interface for drawing 2D shapes, images, and text.
	\ingroup gui_graphics */
//************************************************************************************************

interface IGraphics: IUnknown
{
	//////////////////////////////////////////////////////////////////////////////////////////////
	// Graphics State
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Save current state to internal stack (clipping and transformation). */
	virtual tresult CCL_API saveState () = 0;

	/** Restore previous state from internal stack. */
	virtual tresult CCL_API restoreState () = 0;

	/** Append rectangle to current clipping region. */
	virtual tresult CCL_API addClip (RectRef rect) = 0;
	virtual tresult CCL_API addClip (RectFRef rect) = 0;

	/** Append path to current clipping region. */
	virtual tresult CCL_API addClip (IGraphicsPath* path) = 0;

	/** Append transformation matrix. */
	virtual tresult CCL_API addTransform (TransformRef matrix) = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Graphics Mode
	//////////////////////////////////////////////////////////////////////////////////////////////

	enum Modes
	{
		kAntiAlias = 1<<0	///< enable anti-aliasing
	};

private:
	friend struct AntiAliasSetter;
	/** Set graphics mode. Should not be called directly, use AntiAliasSetter instead. */
	virtual tresult CCL_API setMode (int mode) = 0;

public:
	/** Get graphics mode. Use AntiAliasSetter for setting it in the current scope. */
	virtual int CCL_API getMode () = 0;

	/** Get points to pixels scaling factor. */
	virtual float CCL_API getContentScaleFactor () const = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Primitives
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Clear rectangle with transparent black. */
	virtual tresult CCL_API clearRect (RectRef rect) = 0;
	virtual tresult CCL_API clearRect (RectFRef rect) = 0;

	/** Fill rectangle with given brush. */
	virtual tresult CCL_API fillRect (RectRef rect, BrushRef brush) = 0;
	virtual tresult CCL_API fillRect (RectFRef rect, BrushRef brush) = 0;

	/** Stroke rectangle with given pen. */
	virtual tresult CCL_API drawRect (RectRef rect, PenRef pen) = 0;
	virtual tresult CCL_API drawRect (RectFRef rect, PenRef pen) = 0;
	
	/** Stroke line with given pen. */
	virtual tresult CCL_API drawLine (PointRef p1, PointRef p2, PenRef pen) = 0;
	virtual tresult CCL_API drawLine (PointFRef p1, PointFRef p2, PenRef pen) = 0;
	
	/** Stroke ellipse with given pen. */
	virtual tresult CCL_API drawEllipse (RectRef rect, PenRef pen) = 0;
	virtual tresult CCL_API drawEllipse (RectFRef rect, PenRef pen) = 0;

	/** Fill ellipse with given brush. */
	virtual tresult CCL_API fillEllipse (RectRef rect, BrushRef brush) = 0;
	virtual tresult CCL_API fillEllipse (RectFRef rect, BrushRef brush) = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Paths
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Stroke path with given pen. */
	virtual tresult CCL_API drawPath (IGraphicsPath* path, PenRef pen) = 0;

	/** Fill path with given brush. */
	virtual tresult CCL_API fillPath (IGraphicsPath* path, BrushRef brush) = 0;

	/** Stroke rounded rectangle with given pen. */
	virtual tresult CCL_API drawRoundRect (RectRef rect, Coord rx, Coord ry, PenRef pen) = 0;
	virtual tresult CCL_API drawRoundRect (RectFRef rect, CoordF rx, CoordF ry, PenRef pen) = 0;

	/** Fill rounded rectangle with given brush. */
	virtual tresult CCL_API fillRoundRect (RectRef rect, Coord rx, Coord ry, BrushRef brush) = 0;
	virtual tresult CCL_API fillRoundRect (RectFRef rect, CoordF rx, CoordF ry, BrushRef brush) = 0;

	/** Stroke triangle with given pen. */
	virtual tresult CCL_API drawTriangle (const Point points[3], PenRef pen) = 0;
	virtual tresult CCL_API drawTriangle (const PointF points[3], PenRef pen) = 0;

	/** Fill triangle with given brush. */
	virtual tresult CCL_API fillTriangle (const Point points[3], BrushRef brush) = 0;
	virtual tresult CCL_API fillTriangle (const PointF points[3], BrushRef brush) = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Text
	//////////////////////////////////////////////////////////////////////////////////////////////

	enum DrawTextOptions
	{
		kDrawAtBaseline = kDrawTextOptionsDrawAtBaseline
	};

	/** Draw Unicode string with given brush and alignment in bounding rectangle (clips to bounding rect). */
	virtual tresult CCL_API drawString (RectRef rect, StringRef text,
										FontRef font, BrushRef brush,
										AlignmentRef alignment = Alignment ()) = 0;

	/** Draw Unicode string with given brush and alignment in bounding rectangle (clips to bounding rect, float coordinates). */
	virtual tresult CCL_API drawString (RectFRef rect, StringRef text,
										FontRef font, BrushRef brush,
										AlignmentRef alignment = Alignment ()) = 0;

	/** Draw Unicode string with given brush at given point (left/top or optionally anchored to baseline). */
	virtual tresult CCL_API drawString (PointRef point, StringRef text,
										FontRef font, BrushRef brush, int options = 0) = 0;

	/** Draw Unicode string with given brush at given point, (left/top or optionally anchored to baseline, float coordinates). */
	virtual tresult CCL_API drawString (PointFRef point, StringRef text,
										FontRef font, BrushRef brush, int options = 0) = 0;

	/** Get width of Unicode string with given font. */
	virtual int CCL_API getStringWidth (StringRef text, FontRef font) = 0;

	/** Get width of Unicode string with given font (float coordinate). */
	virtual CoordF CCL_API getStringWidthF (StringRef text, FontRef font) = 0;

	/** Measure extent of Unicode string with given font. */
	virtual tresult CCL_API measureString (Rect& size, StringRef text, FontRef font) = 0;

	/** Measure extent of Unicode string with given font (float coordinates). */
	virtual tresult CCL_API measureString (RectF& size, StringRef text, FontRef font) = 0;

	/** Measure extent of multiline Unicode text based on lineWidth. */
	virtual tresult CCL_API measureText (Rect& size, Coord lineWidth, StringRef text, FontRef font) = 0;

	/** Measure extent of multiline Unicode text based on lineWidth (float coordinates). */
	virtual tresult CCL_API measureText (RectF& size, CoordF lineWidth, StringRef text, FontRef font) = 0;

	/** Draw multiline Unicode text in bounding rectangle (clips to bounding rect). */
	virtual tresult CCL_API drawText (RectRef rect, StringRef text,
									  FontRef font, BrushRef brush,
									  TextFormatRef format = TextFormat ()) = 0;

	/** Draw multiline Unicode text in bounding rectangle (clips to bounding rect, float coordinates). */
	virtual tresult CCL_API drawText (RectFRef rect, StringRef text,
									  FontRef font, BrushRef brush,
									  TextFormatRef format = TextFormat ()) = 0;

	/** Draw formatted text described by ITextLayout object (no clipping). */
	virtual tresult CCL_API drawTextLayout (PointRef pos, ITextLayout* textLayout,
											BrushRef brush, int options = 0) = 0;

	/** Draw formatted text described by ITextLayout object (no clipping, float coordinates). */
	virtual tresult CCL_API drawTextLayout (PointFRef pos, ITextLayout* textLayout,
											BrushRef brush, int options = 0) = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Images
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Draw image at given position. */
	virtual tresult CCL_API drawImage (IImage* image, PointRef pos, const ImageMode* mode = nullptr) = 0;
	virtual tresult CCL_API drawImage (IImage* image, PointFRef pos, const ImageMode* mode = nullptr) = 0;

	/** Draw portion of image. If size of source and destination rectangle differ, scaling is applied. */
	virtual tresult CCL_API drawImage (IImage* image, RectRef src, RectRef dst, const ImageMode* mode = nullptr) = 0;
	virtual tresult CCL_API drawImage (IImage* image, RectFRef src, RectFRef dst, const ImageMode* mode = nullptr) = 0;

	DECLARE_IID (IGraphics)
};

DEFINE_IID (IGraphics, 0xbf0934ba, 0xb439, 0x4ba7, 0x95, 0x8b, 0xca, 0x35, 0x96, 0xfa, 0x69, 0x7c)

//************************************************************************************************
// ClipSetter
/** Helper class to add/restore clipping region. 
	\ingroup gui_graphics */
//************************************************************************************************

struct ClipSetter
{
	ClipSetter (IGraphics& graphics, RectRef clip)
	: graphics (graphics)
	{ graphics.saveState (); graphics.addClip (clip); }

	ClipSetter (IGraphics& graphics, RectFRef clip)
	: graphics (graphics)
	{ graphics.saveState (); graphics.addClip (clip); }

	~ClipSetter ()
	{ graphics.restoreState (); }

	IGraphics& graphics;
};

//************************************************************************************************
// TransformSetter
/** Helper class to add/restore transformation. 
	\ingroup gui_graphics */
//************************************************************************************************

struct TransformSetter
{
	TransformSetter (IGraphics& graphics, TransformRef t)
	: graphics (graphics),
	  transformed (!t.isIdentity ())
	{ if(transformed) { graphics.saveState (); graphics.addTransform (t); } }
	
	~TransformSetter ()
	{ if(transformed) graphics.restoreState (); }
	
	IGraphics& graphics;
	bool transformed;
};

//************************************************************************************************
// ContextSaver
/** Helper class to save/restore graphics state. 
	\ingroup gui_graphics */
//************************************************************************************************

struct ContextSaver
{
	ContextSaver (IGraphics& graphics)
	: graphics (graphics)
	{ graphics.saveState (); }

	~ContextSaver ()
	{ graphics.restoreState (); }

	IGraphics& graphics;
};

//************************************************************************************************
// AntiAliasSetter
/** Helper class to enable/disable anti-aliasing. 
	\ingroup gui_graphics */
//************************************************************************************************

struct AntiAliasSetter
{
	AntiAliasSetter (IGraphics& graphics, bool state = true)
	: graphics (graphics),
	  oldMode (graphics.getMode ())
	{ 
		newMode = state ? (oldMode | IGraphics::kAntiAlias) : (oldMode & ~IGraphics::kAntiAlias);
		if(newMode != oldMode)
			graphics.setMode (newMode); 
	}

	~AntiAliasSetter ()
	{ 
		if(newMode != oldMode)
			graphics.setMode (oldMode); 
	}

	static void setAntialias (IGraphics& graphics, bool state)
	{
		int oldMode = graphics.getMode ();
		graphics.setMode (state ? (oldMode | IGraphics::kAntiAlias) : (oldMode & ~IGraphics::kAntiAlias));
	}

	IGraphics& graphics;
	int oldMode;
	int newMode;
};

} // namespace CCL

#endif // _ccl_igraphics_h
