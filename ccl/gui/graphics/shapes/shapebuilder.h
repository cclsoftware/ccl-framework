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
// Filename    : ccl/gui/graphics/shapes/shapebuilder.h
// Description : Shape Builder
//
//************************************************************************************************

#ifndef _ccl_shapebuilder_h
#define _ccl_shapebuilder_h

#include "ccl/gui/graphics/graphicsdevice.h"
#include "ccl/gui/graphics/shapes/shapes.h"

namespace CCL {

class Shape;
class ComplexShape;
class ShapeImage;

//************************************************************************************************
// ShapeBuilder
//************************************************************************************************

class ShapeBuilder: public GraphicsDeviceBase
{
public:
	DECLARE_CLASS (ShapeBuilder, GraphicsDeviceBase)

	ShapeBuilder (ShapeImage* shapeImage = nullptr);
	~ShapeBuilder ();

	// IGraphics
	tresult CCL_API saveState () override;
	tresult CCL_API restoreState () override;
	tresult CCL_API addClip (RectRef rect) override;
	tresult CCL_API addClip (RectFRef rect) override;
	tresult CCL_API addClip (IGraphicsPath* path) override;
	tresult CCL_API addTransform (TransformRef matrix) override;
	tresult CCL_API setMode (int mode) override;
	int CCL_API getMode () override;
	float CCL_API getContentScaleFactor () const override;
	tresult CCL_API clearRect (RectRef rect) override;
	tresult CCL_API clearRect (RectFRef rect) override;
	tresult CCL_API fillRect (RectRef rect, BrushRef brush) override;
	tresult CCL_API fillRect (RectFRef rect, BrushRef brush) override;
	tresult CCL_API drawRect (RectRef rect, PenRef pen) override;
	tresult CCL_API drawRect (RectFRef rect, PenRef pen) override;
	tresult CCL_API drawLine (PointRef p1, PointRef p2, PenRef pen) override;
	tresult CCL_API drawLine (PointFRef p1, PointFRef p2, PenRef pen) override;
	tresult CCL_API drawEllipse (RectRef rect, PenRef pen) override;
	tresult CCL_API drawEllipse (RectFRef rect, PenRef pen) override;
	tresult CCL_API fillEllipse (RectRef rect, BrushRef brush) override;
	tresult CCL_API fillEllipse (RectFRef rect, BrushRef brush) override;
	tresult CCL_API drawPath (IGraphicsPath* path, PenRef pen) override;
	tresult CCL_API fillPath (IGraphicsPath* path, BrushRef brush) override;
	tresult CCL_API drawRoundRect (RectRef rect, Coord rx, Coord ry, PenRef pen) override;
	tresult CCL_API drawRoundRect (RectFRef rect, CoordF rx, CoordF ry, PenRef pen) override;
	tresult CCL_API fillRoundRect (RectRef rect, Coord rx, Coord ry, BrushRef brush) override;
	tresult CCL_API fillRoundRect (RectFRef rect, CoordF rx, CoordF ry, BrushRef brush) override;
	tresult CCL_API drawTriangle (const Point points[3], PenRef pen) override;
	tresult CCL_API drawTriangle (const PointF points[3], PenRef pen) override;
	tresult CCL_API fillTriangle (const Point points[3], BrushRef brush) override;
	tresult CCL_API fillTriangle (const PointF points[3], BrushRef brush) override;
	tresult CCL_API drawString (RectRef rect, StringRef text, FontRef font, BrushRef brush, AlignmentRef alignment = Alignment ()) override;
	tresult CCL_API drawString (RectFRef rect, StringRef text, FontRef font, BrushRef brush, AlignmentRef alignment = Alignment ()) override;
	tresult CCL_API drawString (PointRef point, StringRef text, FontRef font, BrushRef brush, int options = 0) override;
	tresult CCL_API drawString (PointFRef point, StringRef text, FontRef font, BrushRef brush, int options = 0) override;
	int CCL_API getStringWidth (StringRef text, FontRef font) override;
	CoordF CCL_API getStringWidthF (StringRef text, FontRef font) override;
	tresult CCL_API measureString (Rect& size, StringRef text, FontRef font) override;
	tresult CCL_API measureString (RectF& size, StringRef text, FontRef font) override;
	tresult CCL_API measureText (Rect& size, Coord lineWidth, StringRef text, FontRef font) override;
	tresult CCL_API measureText (RectF& size, CoordF lineWidth, StringRef text, FontRef font) override;
	tresult CCL_API drawText (RectRef rect, StringRef text, FontRef font, BrushRef brush, TextFormatRef format = TextFormat ()) override;
	tresult CCL_API drawText (RectFRef rect, StringRef text, FontRef font, BrushRef brush, TextFormatRef format = TextFormat ()) override;
	tresult CCL_API drawTextLayout (PointRef pos, ITextLayout* textLayout, BrushRef brush, int options = 0) override;
	tresult CCL_API drawTextLayout (PointFRef pos, ITextLayout* textLayout, BrushRef brush, int options = 0) override;
	tresult CCL_API drawImage (IImage* image, PointRef pos, const ImageMode* mode = nullptr) override;
	tresult CCL_API drawImage (IImage* image, PointFRef pos, const ImageMode* mode = nullptr) override;
	tresult CCL_API drawImage (IImage* image, RectRef src, RectRef dst, const ImageMode* mode = nullptr) override;
	tresult CCL_API drawImage (IImage* image, RectFRef src, RectFRef dst, const ImageMode* mode = nullptr) override;

protected:
	ComplexShape* rootShape;
	ShapeImage* shapeImage;
	int deviceMode;

	PROPERTY_READONLY_FLAG (deviceMode, kAntiAlias, shouldAntiAlias)

	template <class ShapeType> ShapeType* allocate ();

	tresult add (Shape* shape);
	tresult addStroke (Shape* shape, PenRef pen);
	tresult addFill (Shape* shape, BrushRef brush);

	template<class TCoord> tresult fillRectInternal (TRectShape<TCoord>* shape, const Core::TRect<TCoord>& rect, BrushRef brush);
	template<class TCoord> tresult drawRectInternal (TRectShape<TCoord>* shape, const Core::TRect<TCoord>& rect, PenRef pen);
	template<class TCoord> tresult drawStringInternal (TTextShape<TCoord>* shape, const Core::TRect<TCoord>& rect, StringRef text, FontRef font, BrushRef brush, AlignmentRef alignment);
	template<class TCoord> Shape* makeRoundRect (TRectShape<TCoord>* shape, const Core::TRect<TCoord>& rect, TCoord rx, TCoord ry);
	template<class TCoord> Shape* makeLine (TLineShape<TCoord>* shape, const TPoint<TCoord>& p1, const TPoint<TCoord>& p2);
	template<class TCoord> Shape* makeTriangle (TTriangleShape<TCoord>* shape, const TPoint<TCoord> points[3]);
	template<class TCoord> Shape* makeImage (TImageShape<TCoord>* shape, IImage* image, const TRect<TCoord>& src, const TRect<TCoord>& dst, const ImageMode* mode);
};

} // namespace CCL

#endif // _ccl_shapebuilder_h
