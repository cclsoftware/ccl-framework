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
// Filename    : ccl/gui/graphics/shapes/shapebuilder.cpp
// Description : Shape Builder
//
//************************************************************************************************

#include "ccl/gui/graphics/shapes/shapebuilder.h"
#include "ccl/gui/graphics/shapes/shapes.h"
#include "ccl/gui/graphics/shapes/shapeimage.h"

using namespace CCL;

//************************************************************************************************
// ShapeBuilder
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ShapeBuilder, GraphicsDeviceBase)

//////////////////////////////////////////////////////////////////////////////////////////////////

ShapeBuilder::ShapeBuilder (ShapeImage* _shapeImage)
: rootShape (NEW ComplexShape),
  shapeImage (nullptr),
  deviceMode (0)
{
	take_shared (shapeImage, _shapeImage);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ShapeBuilder::~ShapeBuilder ()
{
	if(shapeImage)
	{
		// transfer new shape to image
		// (We have to add an additional root shape here,
		// otherwise ShapeImage would interpret the content as separate frames)
		AutoPtr<ComplexShape> imageRoot = NEW ComplexShape;
		imageRoot->addShape (return_shared (rootShape));
		shapeImage->setShape (imageRoot);

		shapeImage->release ();
	}

	rootShape->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class ShapeType>
ShapeType* ShapeBuilder::allocate ()
{
	// Some kind of memory pool could be implemented here...
	return NEW ShapeType;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult ShapeBuilder::add (Shape* shape)
{
	rootShape->addShape (shape);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult ShapeBuilder::addStroke (Shape* shape, PenRef pen)
{
	shape->setStrokePen (pen);
	shape->isStroke (true);
	return add (shape);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult ShapeBuilder::addFill (Shape* shape, BrushRef brush)
{
	shape->setFillBrush (brush);
	shape->isFill (true);
	return add (shape);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TCoord>
tresult ShapeBuilder::fillRectInternal (TRectShape<TCoord>* shape, const Core::TRect<TCoord>& rect, BrushRef brush)
{
	shape->setRect (rect);
	return addFill (shape, brush);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TCoord>
tresult ShapeBuilder::drawRectInternal (TRectShape<TCoord>* shape, const Core::TRect<TCoord>& rect, PenRef pen)
{
	shape->setRect (rect);
	return addStroke (shape, pen);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TCoord>
tresult ShapeBuilder::drawStringInternal (TTextShape<TCoord>* shape, const Core::TRect<TCoord>& rect, StringRef text, FontRef font, BrushRef brush, AlignmentRef alignment)
{
	shape->setFont (font);
	shape->setAlignment (alignment);
	shape->setPosition (rect.getLeftTop ());
	shape->setSize (TPoint<TCoord> (rect.getWidth (), rect.getHeight ()));
	shape->setText (text);
	return addFill (shape, brush);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TCoord>
Shape* ShapeBuilder::makeRoundRect (TRectShape<TCoord>* shape, const TRect<TCoord>& rect, TCoord rx, TCoord ry)
{
	shape->setRect (rect);
	shape->setRadiusX (rx);
	shape->setRadiusY (ry);
	shape->setStyle (Shape::kTiled);
	return shape;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TCoord>
Shape* ShapeBuilder::makeTriangle (TTriangleShape<TCoord>* shape, const TPoint<TCoord> points[3])
{
	shape->setP1 (points[0]);
	shape->setP2 (points[1]);
	shape->setP3 (points[2]);
	return shape;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TCoord>
Shape* ShapeBuilder::makeLine (TLineShape<TCoord>* shape, const TPoint<TCoord>& p1, const TPoint<TCoord>& p2)
{
	shape->setStart (p1);
	shape->setEnd (p2);
	return shape;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TCoord>
Shape* ShapeBuilder::makeImage (TImageShape<TCoord>* shape, IImage* image, const TRect<TCoord>& src, const TRect<TCoord>& dst, const ImageMode* mode)
{
	shape->setImage (image);
	shape->setSrcRect (src);
	shape->setDstRect (dst);
	if(mode)
		shape->setImageMode (*mode);
	return shape;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ShapeBuilder::saveState ()
{
	CCL_NOT_IMPL ("ShapeBuilder method missing!")
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ShapeBuilder::restoreState ()
{
	CCL_NOT_IMPL ("ShapeBuilder method missing!")
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ShapeBuilder::addClip (RectRef rect)
{
	CCL_NOT_IMPL ("ShapeBuilder method missing!")
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ShapeBuilder::addClip (RectFRef rect)
{
	CCL_NOT_IMPL ("ShapeBuilder method missing!")
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ShapeBuilder::addClip (IGraphicsPath* path)
{
	CCL_NOT_IMPL ("ShapeBuilder method missing!")
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ShapeBuilder::addTransform (TransformRef matrix)
{
	CCL_NOT_IMPL ("ShapeBuilder method missing!")
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ShapeBuilder::setMode (int mode)
{
	deviceMode = mode;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API ShapeBuilder::getMode ()
{
	return deviceMode;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float CCL_API ShapeBuilder::getContentScaleFactor () const
{
	return 1.f;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ShapeBuilder::clearRect (RectRef rect)
{
	CCL_NOT_IMPL ("ShapeBuilder method missing!")
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ShapeBuilder::clearRect (RectFRef rect)
{
	CCL_NOT_IMPL ("ShapeBuilder method missing!")
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ShapeBuilder::fillRect (RectRef rect, BrushRef brush)
{
	return fillRectInternal (allocate<RectShape> (), rect, brush);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ShapeBuilder::fillRect (RectFRef rect, BrushRef brush)
{
	return fillRectInternal (allocate<RectShapeF> (), rect, brush);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ShapeBuilder::drawRect (RectRef rect, PenRef pen)
{
	return drawRectInternal (allocate<RectShape> (), rect, pen);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ShapeBuilder::drawRect (RectFRef rect, PenRef pen)
{
	return drawRectInternal (allocate<RectShapeF> (), rect, pen);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ShapeBuilder::drawLine (PointRef p1, PointRef p2, PenRef pen)
{
	return addStroke (makeLine (allocate<LineShape> (), p1, p2), pen);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ShapeBuilder::drawLine (PointFRef p1, PointFRef p2, PenRef pen)
{
	return addStroke (makeLine (allocate<LineShapeF> (), p1, p2), pen);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ShapeBuilder::drawEllipse (RectRef rect, PenRef pen)
{
	return drawRectInternal (allocate<EllipseShape> (), rect, pen);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ShapeBuilder::drawEllipse (RectFRef rect, PenRef pen)
{
	return drawRectInternal (allocate<EllipseShapeF> (), rect, pen);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ShapeBuilder::fillEllipse (RectRef rect, BrushRef brush)
{
	return fillRectInternal (allocate<EllipseShape> (), rect, brush);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ShapeBuilder::fillEllipse (RectFRef rect, BrushRef brush)
{
	return fillRectInternal (allocate<EllipseShapeF> (), rect, brush);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ShapeBuilder::drawPath (IGraphicsPath* _path, PenRef pen)
{
	GraphicsPath* path = unknown_cast<GraphicsPath> (_path);
	ASSERT (path != nullptr)
	if(path == nullptr)
		return kResultInvalidArgument;

	PathShape* shape = allocate<PathShape> ();
	shape->setPath (path);
	return addStroke (shape, pen);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ShapeBuilder::fillPath (IGraphicsPath* _path, BrushRef brush)
{
	GraphicsPath* path = unknown_cast<GraphicsPath> (_path);
	ASSERT (path != nullptr)
	if(path == nullptr)
		return kResultInvalidArgument;

	PathShape* shape = allocate<PathShape> ();
	shape->setPath (path);
	return addFill (shape, brush);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ShapeBuilder::drawRoundRect (RectRef rect, Coord rx, Coord ry, PenRef pen)
{
	return addStroke (makeRoundRect (allocate<RectShape> (), rect, rx, ry), pen);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ShapeBuilder::drawRoundRect (RectFRef rect, CoordF rx, CoordF ry, PenRef pen)
{
	return addStroke (makeRoundRect (allocate<RectShapeF> (), rect, rx, ry), pen);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ShapeBuilder::fillRoundRect (RectRef rect, Coord rx, Coord ry, BrushRef brush)
{
	return addFill (makeRoundRect (allocate<RectShape> (), rect, rx, ry), brush);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ShapeBuilder::fillRoundRect (RectFRef rect, CoordF rx, CoordF ry, BrushRef brush)
{
	return addFill (makeRoundRect (allocate<RectShapeF> (), rect, rx, ry), brush);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ShapeBuilder::drawTriangle (const Point points[3], PenRef pen)
{
	return addStroke (makeTriangle (allocate<TriangleShape> (), points), pen);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ShapeBuilder::drawTriangle (const PointF points[3], PenRef pen)
{
	return addStroke (makeTriangle (allocate<TriangleShapeF> (), points), pen);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ShapeBuilder::fillTriangle (const Point points[3], BrushRef brush)
{
	return addFill (makeTriangle (allocate<TriangleShape> (), points), brush);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ShapeBuilder::fillTriangle (const PointF points[3], BrushRef brush)
{
	return addFill (makeTriangle (allocate<TriangleShapeF> (), points), brush);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ShapeBuilder::drawString (RectRef rect, StringRef text, FontRef font, BrushRef brush, AlignmentRef alignment)
{
	return drawStringInternal (allocate<TextShape> (), rect, text, font, brush, alignment);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ShapeBuilder::drawString (RectFRef rect, StringRef text, FontRef font, BrushRef brush, AlignmentRef alignment)
{
	return drawStringInternal (allocate<TextShapeF> (), rect, text, font, brush, alignment);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ShapeBuilder::drawString (PointRef point, StringRef text, FontRef font, BrushRef brush, int options)
{
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ShapeBuilder::drawString (PointFRef point, StringRef text, FontRef font, BrushRef brush, int options)
{
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API ShapeBuilder::getStringWidth (StringRef text, FontRef font)
{
	return Font::getStringWidth (text, font);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CoordF CCL_API ShapeBuilder::getStringWidthF (StringRef text, FontRef font)
{
	return Font::getStringWidthF (text, font);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ShapeBuilder::measureString (Rect& size, StringRef text, FontRef font)
{
	Font::measureString (size, text, font);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ShapeBuilder::measureString (RectF& size, StringRef text, FontRef font)
{
	Font::measureString (size, text, font);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ShapeBuilder::measureText (Rect& size, Coord lineWidth, StringRef text, FontRef font)
{
	Font::measureText (size, lineWidth, text, font);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ShapeBuilder::measureText (RectF& size, CoordF lineWidth, StringRef text, FontRef font)
{
	Font::measureText (size, lineWidth, text, font);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ShapeBuilder::drawText (RectRef rect, StringRef text, FontRef font, BrushRef brush, TextFormatRef format)
{
	CCL_NOT_IMPL ("ShapeBuilder method missing!")
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ShapeBuilder::drawText (RectFRef rect, StringRef text, FontRef font, BrushRef brush, TextFormatRef format)
{
	CCL_NOT_IMPL ("ShapeBuilder method missing!")
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ShapeBuilder::drawTextLayout (PointRef pos, ITextLayout* textLayout, BrushRef brush, int options)
{
	CCL_NOT_IMPL ("ShapeBuilder method missing!")
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ShapeBuilder::drawTextLayout (PointFRef pos, ITextLayout* textLayout, BrushRef brush, int options)
{
	CCL_NOT_IMPL ("ShapeBuilder method missing!")
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ShapeBuilder::drawImage (IImage* image, PointRef pos, const ImageMode* mode)
{
	ASSERT (image != nullptr)
	if(image == nullptr)
		return kResultInvalidArgument;

	Rect src (0, 0, image->getWidth (), image->getHeight ());
	Rect dst (src);
	dst.offset (pos);
	return drawImage (image, src, dst, mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ShapeBuilder::drawImage (IImage* image, PointFRef pos, const ImageMode* mode)
{
	ASSERT (image != nullptr)
	if(image == nullptr)
		return kResultInvalidArgument;

	RectF src (0, 0, (CoordF)image->getWidth (), (CoordF)image->getHeight ());
	RectF dst (src);
	dst.offset (pos);
	return drawImage (image, src, dst, mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ShapeBuilder::drawImage (IImage* image, RectRef src, RectRef dst, const ImageMode* mode)
{
	ASSERT (image != nullptr)
	if(image == nullptr)
		return kResultInvalidArgument;

	return add (makeImage (allocate<ImageShape> (), image, src, dst, mode));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ShapeBuilder::drawImage (IImage* image, RectFRef src, RectFRef dst, const ImageMode* mode)
{
	ASSERT (image != nullptr)
	if(image == nullptr)
		return kResultInvalidArgument;

	return add (makeImage (allocate<ImageShapeF> (), image, src, dst, mode));
}
