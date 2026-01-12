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
// Filename    : ccl/public/gui/graphics/igraphicspath.h
// Description : Graphics Path Interface
//
//************************************************************************************************

#ifndef _ccl_igraphicspath_h
#define _ccl_igraphicspath_h

#include "ccl/public/gui/graphics/types.h"

namespace CCL {

//************************************************************************************************
// IGraphicsPath
/** A path stores a sequence of graphical shapes. 
	\ingroup gui_graphics */
//************************************************************************************************

interface IGraphicsPath: IUnknown
{
	/** Path type hint. */
	DEFINE_ENUM (TypeHint)
	{
		kPaintPath,		///< optimized for quality
		kClipPath		///< optimized for clipping
	};

	/** Fill mode: rule that determines whether a point in the path is "inside". */
	DEFINE_ENUM (FillMode)
	{
		kFillNonZero,	///< nonzero winding number rule: "inside" is assumed on an odd number of edge crossings of an arbitrary ray (default)
		kFillEvenOdd	///< even-odd rule: "inside" is assumed on a non-zero sum of signed edge crossings of an arbitrary ray
	};

	/** Set fill mode. Must be called before adding elements. */
	virtual void CCL_API setFillMode (FillMode fillMode) = 0;

	/** Get bounding rectangle of path. */
	virtual void CCL_API getBounds (Rect& bounds) const = 0;

	/** Get bounding rectangle of path. */
	virtual void CCL_API getBounds (RectF& bounds) const = 0;

	/** Transform path. */
	virtual void CCL_API transform (TransformRef matrix) = 0;

	/** Start new figure. */
	virtual void CCL_API startFigure (PointRef p) = 0;

	/** Start new figure. */
	virtual void CCL_API startFigure (PointFRef p) = 0;
	
	/** Close current figure (adds line to first point). */
	virtual void CCL_API closeFigure () = 0;

	/** Draw line from current position to p */
	virtual void CCL_API lineTo (PointRef p) = 0;

	/** Draw line from current position to p */
	virtual void CCL_API lineTo (PointFRef p) = 0;

	/** Add rectangle. */
	virtual void CCL_API addRect (RectRef rect) = 0;
	
	/** Add rectangle. */
	virtual void CCL_API addRect (RectFRef rect) = 0;

	/** Add rounded rectangle. */
	virtual void CCL_API addRoundRect (RectRef rect, Coord rx, Coord ry) = 0;

	/** Add rounded rectangle. */
	virtual void CCL_API addRoundRect (RectFRef rect, CoordF rx, CoordF ry) = 0;

	/** Add triangle. */
	virtual void CCL_API addTriangle (PointRef p1, PointRef p2, PointRef p3) = 0;

	/** Add triangle. */
	virtual void CCL_API addTriangle (PointFRef p1, PointFRef p2, PointFRef p3) = 0;

	/** Add Bezier curve. */
	virtual void CCL_API addBezier (PointRef p1, PointRef c1, PointRef c2, PointRef p2) = 0;

	/** Add Bezier curve. */
	virtual void CCL_API addBezier (PointFRef p1, PointFRef c1, PointFRef c2, PointFRef p2) = 0;

	/** Add arc. 
		\param startAngle: Specifies the clockwise angle, in degrees, between the horizontal axis of the ellipse and the starting point of the arc.
		\param sweepAngle: Specifies the clockwise angle, in degrees, between the starting point (startAngle) and ending point of the arc. */
	virtual void CCL_API addArc (RectRef rect, float startAngle, float sweepAngle) = 0;

	/** Add arc. */
	virtual void CCL_API addArc (RectFRef rect, float startAngle, float sweepAngle) = 0;

	DECLARE_IID (IGraphicsPath)
};

DEFINE_IID (IGraphicsPath, 0x2d5c5551, 0x141d, 0x4bfe, 0xba, 0x54, 0x54, 0xcc, 0x89, 0x3, 0x8f, 0x8e)

} // namespace CCL

#endif // _ccl_igraphicspath_h
