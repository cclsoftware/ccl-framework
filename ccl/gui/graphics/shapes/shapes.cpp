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
// Filename    : ccl/gui/graphics/shapes/shapes.cpp
// Description : Shapes
//
//************************************************************************************************

#include "ccl/gui/graphics/shapes/shapes.h"
#include "ccl/gui/graphics/graphicsdevice.h"
#include "ccl/gui/graphics/colorgradient.h"

#include "ccl/gui/theme/colorscheme.h"

#include "ccl/public/math/mathprimitives.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
// Point/Rect helper
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace CCL {

inline Point scale (PointRef p, float sx, float sy)
{
	PointF point (p.x * sx, p.y * sy);
	return pointFToInt (point);
}

inline PointF scale (PointFRef p, float sx, float sy)
{
	PointF point (p.x * sx, p.y * sy);
	return point;
}

inline Rect scale (RectRef r, float sx, float sy)
{
	RectF rect (r.left * sx, r.top * sy, r.right * sx, r.bottom * sy);
	return rectFToInt (rect);
}

inline RectF scale (RectFRef r, float sx, float sy)
{
	RectF rect (r.left * sx, r.top * sy, r.right * sx, r.bottom * sy);
	return rect;
}

inline Rect& assignRect (Rect& dst, RectRef src)
{
	dst = src;
	return dst;
}

inline Rect& assignRect (Rect& dst, RectFRef src)
{
	dst = rectFToInt (src);
	return dst;
}

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// Shape::GradientCache
//************************************************************************************************

Shape::GradientCache::GradientCache ()
: modifiedGradient (nullptr),
  sourceGradient (nullptr),
  cachedSx (0),
  cachedSy (0),
  cachedAlpha (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Shape::GradientCache::~GradientCache ()
{
	safe_release (modifiedGradient);
}
		
//////////////////////////////////////////////////////////////////////////////////////////////////

IGradient* Shape::GradientCache::getModifiedGradient (ColorGradient* gradient, float sx, float sy, float alpha)
{
	if(gradient != sourceGradient || sx != cachedSx || sy != cachedSy || alpha != cachedAlpha)
		safe_release (modifiedGradient);
	
	if(!modifiedGradient)
	{
		modifiedGradient = static_cast<ColorGradient*> (gradient->clone ());
		modifiedGradient->scale (sx, sy);
		modifiedGradient->setOpacity (alpha);
		sourceGradient = gradient;
		cachedSx = sx;
		cachedSy = sy; 
		cachedAlpha = alpha;
	}
	return modifiedGradient;
}

//************************************************************************************************
// Shape
//************************************************************************************************

DEFINE_CLASS_HIDDEN (Shape, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

Shape::Shape (int style)
: style (style),
  strokePen (Colors::kBlack),
  fillBrush (SolidBrush (Colors::kWhite)),
  currentSx (1),
  currentSy (1)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Shape::Shape (const Shape& shape)
: style (shape.style),
  strokePen (shape.strokePen),
  fillBrush (shape.fillBrush),
  currentSx (1),
  currentSy (1)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Shape::~Shape ()
{
	setStrokeColorReference (nullptr, nullptr);
	setFillColorReference (nullptr, nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Shape::countShapes () const
{ 
	return 0; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Shape* Shape::getShape (int index) const
{ 
	return nullptr; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Shape* Shape::findShape (StringRef name)
{
	return (name == this->name) ? this : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Pen Shape::getStrokePen (const ImageMode* mode) const
{
	Pen pen (getStrokePen ());
	if(mode && mode->getAlphaF () != 1.f)
	{
		Color c (pen.getColor ());
		c.scaleAlpha (mode->getAlphaF ());
		pen.setColor (c);
	}
	return pen;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Brush Shape::getFillBrush (const ImageMode* mode) const
{
	bool needsScaling = (currentSx != 1 || currentSy != 1);
	bool modifyAlpha = (mode && mode->getAlphaF () != 1.f);
	
	if(needsScaling || modifyAlpha)
	{
		float alpha = modifyAlpha ? mode->getAlphaF () : 1.f;
		
		auto gradient = fillBrush.getGradient ();
		auto colorGradient = unknown_cast<ColorGradient> (gradient);
			
		ASSERT (!gradient || colorGradient) // scaling or modifying the opacity requires a colorGradient
		
		if(colorGradient)
			return GradientBrush (gradientCache.getModifiedGradient (colorGradient, currentSx, currentSy, alpha));
		else if(modifyAlpha)
			return SolidBrush (Color (fillBrush.getColor ()).scaleAlpha (alpha));
	}
	
	return fillBrush;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Shape::setPenAndBrush (PenRef pen, BrushRef brush)
{
	setStrokePen (pen);
	setFillBrush (brush);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Shape::setStrokeColorReference (ColorScheme* scheme, StringID nameInScheme)
{
	share_and_observe (this, strokeColorReference.scheme, scheme);
	strokeColorReference.nameInScheme = nameInScheme;

	if(scheme != nullptr)
		strokePen.setColor (scheme->getColor (nameInScheme));		
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Shape::setFillColorReference (ColorScheme* scheme, StringID nameInScheme)
{
	share_and_observe (this, fillColorReference.scheme, scheme);
	fillColorReference.nameInScheme = nameInScheme;

	if(scheme != nullptr)
		fillBrush.setColor (scheme->getColor (nameInScheme));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Shape::hasReferences (IColorScheme& scheme) const
{
	if(strokeColorReference.scheme == &scheme || fillColorReference.scheme == &scheme)
		return true;

	if(auto gradient = unknown_cast<ColorGradient> (fillBrush.getGradient ()))
		if(gradient->hasReferences (&scheme))
			return true;

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Shape::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kChanged)
		if(ColorScheme* scheme = unknown_cast<ColorScheme> (subject))
		{
			if(scheme == fillColorReference.scheme)
				fillBrush.setColor (scheme->getColor (fillColorReference.nameInScheme));

			if(scheme == strokeColorReference.scheme)
				strokePen.setColor (scheme->getColor (strokeColorReference.nameInScheme));
		}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect& Shape::getBounds (Rect& bounds) const
{
	bounds.setEmpty ();
	return bounds;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Shape::getWidth () const
{
	Rect r;
	return getBounds (r).getWidth ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Shape::getHeight () const
{
	Rect r;
	return getBounds (r).getHeight ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Shape::drawShape (GraphicsDevice& graphics, const ImageMode* mode) const
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Shape::drawShape (GraphicsDevice& graphics, float sx, float sy, const ImageMode* mode) const
{
	CCL_WARN ("Shape: scaled drawing not implemented for %s\n", myClass ().getPersistentName ());
	drawShape (graphics);
}

//************************************************************************************************
// ComplexShape
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ComplexShape, Shape)

//////////////////////////////////////////////////////////////////////////////////////////////////

ComplexShape::ComplexShape ()
: shapes (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ComplexShape::ComplexShape (const ComplexShape& shape)
: Shape (shape),
  size (shape.size),
  shapes (nullptr)
{
	for(int i = 0; i < shape.countShapes (); i++)
		addShape ((Shape*)shape.getShape (i)->clone ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ComplexShape::~ComplexShape ()
{
	if(shapes)
		shapes->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ComplexShape::addShape (Shape* shape)
{
	if(!shapes)
	{
		shapes = NEW ObjectArray;
		shapes->objectCleanup (true);
	}
	shapes->add (shape);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ComplexShape::countShapes () const
{
	return shapes ? shapes->count () : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Shape* ComplexShape::getShape (int index) const
{
	return shapes ? (Shape*)shapes->at (index) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Shape* ComplexShape::findShape (StringRef name)
{
	if(name == this->name)
		return this;

	if(shapes)
		ArrayForEach (*shapes, Shape, shape)
			Shape* found = shape->findShape (name);
			if(found)
				return found;
		EndFor

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect& ComplexShape::getBounds (Rect& bounds) const
{
	if(!size.isEmpty ()) // size is known ;-)
	{
		bounds = size;
		return bounds;
	}

	return getJoinedBounds (bounds);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect& ComplexShape::getJoinedBounds (Rect& bounds) const
{
	bounds.setReallyEmpty ();
	
	if(shapes)
		ArrayForEach (*shapes, Shape, shape)
			Rect r;
			if(ComplexShape* complex = ccl_cast<ComplexShape> (shape))
				complex->getJoinedBounds (r);
			else
				shape->getBounds (r);
			bounds.join (r);
		EndFor
	
	return bounds;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ComplexShape::drawShape (GraphicsDevice& graphics, const ImageMode* mode) const
{
	if(shapes)
	{
		TransformSetter t (graphics, Transform ().translate ((float)size.left, (float)size.top));

		ArrayForEach (*shapes, Shape, shape)
			shape->drawShape (graphics, mode);
		EndFor
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ComplexShape::drawShape (GraphicsDevice& graphics, float sx, float sy, const ImageMode* mode) const
{
	if(shapes)
		ArrayForEach (*shapes, Shape, shape)
			shape->drawShape (graphics, sx, sy, mode);
		EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ComplexShape::hasReferences (IColorScheme& scheme) const
{
	if(shapes)
		ArrayForEach (*shapes, Shape, shape)
			if(shape->hasReferences (scheme))
				return true;
		EndFor
	return false;
}

//************************************************************************************************
// TLineShape: LineShape, LineShapeF
//************************************************************************************************

DEFINE_SHAPE_CLASS_INT_FLOAT (LineShape, Shape)

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TCoord>
Rect& TLineShape<TCoord>::getBounds (Rect& bounds) const
{
	assignRect (bounds, TRect<TCoord> (start.x, start.y, end.x, end.y));

	// force at least 1 pixel for horizontal / vertcial lines; todo: use pen width?
	if(bounds.top == bounds.bottom)
		bounds.bottom++;
	else if(bounds.left == bounds.right)
		bounds.right++;
	return bounds;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TCoord>
void TLineShape<TCoord>::drawShape (GraphicsDevice& graphics, const ImageMode* mode) const
{
	if(!isStroke ())
		return;
	
	graphics.drawLine (start, end, getStrokePen (mode));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TCoord>
void TLineShape<TCoord>::drawShape (GraphicsDevice& graphics, float sx, float sy, const ImageMode* mode) const
{
	if(!isStroke ())
		return;
	
	TPoint<TCoord> scaledStart = start;
	TPoint<TCoord> scaledEnd = end;
	
	int strokeOffset = ccl_to_int ((strokePen.getWidth () / 2.f) + 0.5);
	
	if(getScaleAlignment () & kRightAligned)
	{
		scaledStart.x += (TCoord)strokeOffset;
		scaledEnd.x += (TCoord)strokeOffset;
	}
	if(getScaleAlignment () & kBottomAligned)
	{
		scaledStart.y += (TCoord)strokeOffset;
		scaledEnd.y += (TCoord)strokeOffset;
	}
	
	scaledStart = scale (scaledStart, sx, sy);
	scaledEnd = scale (scaledEnd, sx, sy);
	
	if(getScaleAlignment () & kRightAligned)
	{
		scaledStart.x -= (TCoord)strokeOffset;
		scaledEnd.x -= (TCoord)strokeOffset;
	}
	if(getScaleAlignment () & kBottomAligned)
	{
		scaledStart.y -= (TCoord)strokeOffset;
		scaledEnd.y -= (TCoord)strokeOffset;
	}
	
	graphics.drawLine (scaledStart, scaledEnd, getStrokePen (mode));
}

//************************************************************************************************
// TRectShape: RectShape, RectShapeF
//************************************************************************************************

DEFINE_SHAPE_CLASS_INT_FLOAT (RectShape, Shape)

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TCoord>
Rect& TRectShape<TCoord>::getBounds (Rect& bounds) const
{
	return assignRect (bounds, rect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TCoord>
void TRectShape<TCoord>::drawShape (GraphicsDevice& graphics, const ImageMode* mode) const
{
	if(rx != 0 || ry != 0)
	{
		GraphicsPath& path = pathCache.getPath (rect, rx, ry);
		drawRectShape (graphics, path, mode);
	}
	else
	{
		drawRectShape (graphics, rect, mode);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TCoord>
void TRectShape<TCoord>::drawShape (GraphicsDevice& graphics, float sx, float sy, const ImageMode* mode) const
{
	currentSx = sx;
	currentSy = sy;
		
	if(rx != 0 || ry != 0)
	{
		TCoord hMargin = rect.left;
		TCoord vMargin = rect.top;
		TRect<TCoord> scaledRect (rect);
		TCoord leftClippingOffset = 0;
		TCoord topClippingOffset = 0;
		
		auto isPenMargin = [&](TCoord margin)
		{
			return (getStrokePen ().getWidth () / 2) == ccl_sign (margin) * margin;
		};
		
		if(keepMargin ())
		{
			// negative margin clips left/top side...

			if(hMargin < 0)
			{
				// ...but not if it is half the pen-width
				if(!isPenMargin (hMargin))
				{
					hMargin += rx;
					if(hMargin < 0)
						hMargin *= -1;
						
					leftClippingOffset = (scaledRect.right + hMargin) / 2;
				}
			}
			if(vMargin < 0)
			{	
				if(!isPenMargin (vMargin))
				{
					vMargin += ry;				
					if(vMargin < 0)
						vMargin *= -1;
						
					topClippingOffset = (scaledRect.bottom + vMargin) / 2;
				}
			}
			
			scaledRect.left = leftClippingOffset;
			scaledRect.top = topClippingOffset;
			scaledRect.right += hMargin;
			scaledRect.bottom += vMargin;
		}
		
		scaledRect = scale (scaledRect, sx, sy);

		if(keepMargin ())
		{
			scaledRect.left = hMargin - leftClippingOffset;
			scaledRect.top = vMargin - topClippingOffset;
			scaledRect.right -= hMargin;
			scaledRect.bottom -= vMargin;
		}
		
		GraphicsPath& path = pathCache.getPath (scaledRect, isTiled () ? rx : ccl_to_int (rx * sx), isTiled () ? ry : ccl_to_int (ry * sy));
		drawRectShape (graphics, path, mode);
	}
	else
	{
		TRect<TCoord> r (scale (rect, sx, sy));
		drawRectShape (graphics, r, mode);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TCoord>
void TRectShape<TCoord>::drawRectShape (GraphicsDevice& graphics, GraphicsPath& path, const ImageMode* mode) const
{
	if(isFill ())
		graphics.fillPath (path, getFillBrush (mode));
	
	if(isStroke ())
		graphics.drawPath (path, getStrokePen (mode));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TCoord>
void TRectShape<TCoord>::drawRectShape (GraphicsDevice& graphics, const TRect<TCoord>& rect, const ImageMode* mode) const
{
	if(isFill ())
		graphics.fillRect (rect, getFillBrush (mode));
		
	if(isStroke ())
		graphics.drawRect (rect, getStrokePen (mode));
}

//************************************************************************************************
// TRectShape::PathCache
//************************************************************************************************

template<class TCoord>
GraphicsPath& TRectShape<TCoord>::PathCache::getPath (const TRect<TCoord>& _rect, TCoord _rx, TCoord _ry)
{
	if(_rect != rect || _rx != rx || _ry != ry)
		path.release ();

	if(!path)
	{
		path = NEW GraphicsPath;
		path->addRoundRect (_rect, _rx, _ry);

		rect = _rect;
		rx = _rx;
		ry = _ry;
	}
	return *path;
}

//************************************************************************************************
// TTriangleShape: TriangleShape, TriangleShapeF
//************************************************************************************************

DEFINE_SHAPE_CLASS_INT_FLOAT (TriangleShape, Shape)

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TCoord>
Rect& TTriangleShape<TCoord>::getBounds (Rect& bounds) const
{
	TRect<TCoord> b;
	b.left   = ccl_min<TCoord> (ccl_min<TCoord> (p1.x, p2.x), p3.x);
	b.top    = ccl_min<TCoord> (ccl_min<TCoord> (p1.y, p2.y), p3.y);
	b.right  = ccl_max<TCoord> (ccl_max<TCoord> (p1.x, p2.x), p3.x);
	b.bottom = ccl_max<TCoord> (ccl_max<TCoord> (p1.y, p2.y), p3.y);
	return assignRect (bounds, b);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TCoord>
void TTriangleShape<TCoord>::drawShape (GraphicsDevice& graphics, const ImageMode* mode) const
{
	GraphicsPath& path = pathCache.getPath (p1, p2, p3);

	if(isFill ())
		graphics.fillPath (path, getFillBrush (mode));

	if(isStroke ())
		graphics.drawPath (path, getStrokePen (mode));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TCoord>
void TTriangleShape<TCoord>::drawShape (GraphicsDevice& graphics, float sx, float sy, const ImageMode* mode) const
{
	currentSx = sx;
	currentSy = sy;
		
	GraphicsPath& path = pathCache.getPath (scale (p1, sx, sy), scale (p2, sx, sy), scale (p3, sx, sy));

	if(isFill ())
		graphics.fillPath (path, getFillBrush (mode));

	if(isStroke ())
		graphics.drawPath (path, getStrokePen (mode));
}

//************************************************************************************************
// TTriangleShape::PathCache
//************************************************************************************************

template<class TCoord>
GraphicsPath& TTriangleShape<TCoord>::PathCache::getPath (const TPoint<TCoord>& _p1, const TPoint<TCoord>& _p2, const TPoint<TCoord>& _p3)
{
	if(_p1 != p1 || _p2 != p2 || _p3 != p3)
		path.release ();

	if(!path)
	{
		path = NEW GraphicsPath;
		path->addTriangle (_p1, _p2, _p3);

		p1 = _p1;
		p2 = _p2;
		p3 = _p3;
	}
	return *path;
}

//************************************************************************************************
// TEllipseShape: EllipseShape, EllipseShapeF
//************************************************************************************************

DEFINE_SHAPE_CLASS_INT_FLOAT2 (EllipseShape, RectShape, RectShapeF)

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TCoord>
void TEllipseShape<TCoord>::drawShape (GraphicsDevice& graphics, const ImageMode* mode) const
{
	if(this->isFill ())
		graphics.fillEllipse (this->rect, this->getFillBrush (mode));

	if(this->isStroke ())
		graphics.drawEllipse (this->rect, this->getStrokePen (mode));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TCoord>
void TEllipseShape<TCoord>::drawShape (GraphicsDevice& graphics, float sx, float sy, const ImageMode* mode) const
{
	this->currentSx = sx;
	this->currentSy = sy;
		
	TRect<TCoord> r (scale (this->rect, sx, sy));

	if(this->isFill ())
		graphics.fillEllipse (r, this->getFillBrush (mode));

	if(this->isStroke ())
		graphics.drawEllipse (r, this->getStrokePen (mode));
}

//************************************************************************************************
// PathShape
//************************************************************************************************

DEFINE_CLASS_HIDDEN (PathShape, Shape)

//////////////////////////////////////////////////////////////////////////////////////////////////

PathShape::PathShape (GraphicsPath* path, int style)
: Shape (style),
  path (path)
{
	if(path)
		path->retain ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PathShape::PathShape (const PathShape& shape)
: Shape (shape),
  path (nullptr)
{
	setPath (shape.path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PathShape::~PathShape ()
{
	if(path)
		path->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PathShape::drawShape (GraphicsDevice& graphics, const ImageMode* mode) const
{
	if(path)
	{
		if(isFill ())
			graphics.fillPath (*path, getFillBrush (mode));

		if(isStroke ())
			graphics.drawPath (*path, getStrokePen (mode));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PathShape::drawShape (GraphicsDevice& graphics, float sx, float sy, const ImageMode* mode) const
{
	currentSx = sx;
	currentSy = sy;
		
	if(path)
	{
		GraphicsPath& p = pathCache.getPath (path, sx, sy);

		if(isFill ())
			graphics.fillPath (p, getFillBrush (mode));

		if(isStroke ())
			graphics.drawPath (p, getStrokePen (mode));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect& PathShape::getBounds (Rect& bounds) const
{
	if(path)
		path->getBounds (bounds);
	else
		bounds.setEmpty ();
	return bounds;
}

//************************************************************************************************
// PathShape::PathCache
//************************************************************************************************

GraphicsPath& PathShape::PathCache::getPath (const GraphicsPath* _p, float _sx, float _sy)
{
	if(_p != p || _sx != sx || _sy != sy)
		path.release ();

	if(!path)
	{
		path = NEW GraphicsPath (*_p);
		path->transform (Transform ().scale (_sx, _sy));

		p = _p;
		sx = _sx;
		sy = _sy;
	}
	return *path;
}

//************************************************************************************************
// TTextShape: TextShape, TextShapeF
//************************************************************************************************

DEFINE_SHAPE_CLASS_INT_FLOAT (TextShape, Shape)

template<class TCoord>
void TTextShape<TCoord>::drawShape (GraphicsDevice& graphics, const ImageMode* mode) const
{
	if(isFill ())
	{
		Rect rect;
		getBounds (rect);

		graphics.drawString (rect, text, font, getFillBrush (mode), alignment);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TCoord>
void TTextShape<TCoord>::drawShape (GraphicsDevice& graphics, float sx, float sy, const ImageMode* mode) const
{
	if(isFill ())
	{
		Rect rect;
		getBounds (rect);

		// (not scaling font size)
		graphics.drawString (scale (rect, sx, sy), text, font, getFillBrush (mode), alignment);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TCoord>
Rect& TTextShape<TCoord>::getBounds (Rect& bounds) const
{
	if(!size.isNull ()) // size of text rectangle is fixed
		bounds ((Coord)position.x, (Coord)position.y, Coord(position.x + size.x), Coord(position.y + size.y));
	else
	{
		Font::measureString (bounds, text, getFont ());

		Point pos ((Coord)position.x, (Coord)position.y); // hmm

		if(alignment.getAlignH () == Alignment::kHCenter)
			pos.x -= bounds.getWidth () / 2;
		else if(alignment.getAlignH () == Alignment::kRight)
			pos.x -= bounds.getWidth ();

		if(alignment.getAlignV () == Alignment::kVCenter)
			pos.y -= (int)font.getSize () / 2;
		else if(alignment.getAlignV () == Alignment::kBottom)
			pos.y -= (int)font.getSize ();

		bounds.offset (pos);
	}
	return bounds;
}

//************************************************************************************************
// TransformShape
//************************************************************************************************

DEFINE_CLASS_HIDDEN (TransformShape, Shape)

//////////////////////////////////////////////////////////////////////////////////////////////////

TransformShape::TransformShape (TransformRef transform, Shape* shape)
: transform (transform),
  shape (shape)
{
	if(shape)
		shape->retain ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TransformShape::TransformShape (const TransformShape& ts)
: Shape (ts),
  transform (ts.transform),
  shape ((Shape*)ts.shape->clone ())
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TransformShape::~TransformShape ()
{
	if(shape)
		shape->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int TransformShape::countShapes () const
{
	return shape ? 1 : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Shape* TransformShape::getShape (int index) const
{
	return ((index == 0) && shape) ? shape : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect& TransformShape::getTransformedBounds (const Shape& subShape, Rect& bounds) const
{
	subShape.getBounds (bounds);
	transform.transform (bounds);
	return bounds;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect& TransformShape::getBounds (Rect& bounds) const
{
	ComplexShape* complex = ccl_cast<ComplexShape>(shape);
	if(complex)
	{
		// for a complex shape, transform bounds of subshapes separately (can be smaller!)
		bounds.setReallyEmpty ();

		for(int i = 0; i < shape->countShapes (); ++i)
		{
			Shape* subShape = shape->getShape (i);
			Rect r;
			bounds.join (getTransformedBounds (*subShape, r));
		}
	}
	else
		getTransformedBounds (*shape, bounds);
	return bounds;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TransformShape::drawShape (GraphicsDevice& graphics, const ImageMode* mode) const
{
	TransformSetter t (graphics, transform);

	shape->drawShape (graphics, mode);

#if 0 && DEBUG // draw bounds of target shape
	Rect bounds;
	shape->getBounds (bounds);
	graphics.drawRect (bounds, Pen (Colors::kRed));
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TransformShape::drawShape (GraphicsDevice& graphics, float sx, float sy, const ImageMode* mode) const
{
	Transform tr (transform);
	tr.scale (sx, sy);
	TransformSetter t (graphics, tr);

	shape->drawShape (graphics, mode);
}

//************************************************************************************************
// ViewPortShape
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ViewPortShape, TransformShape)

//////////////////////////////////////////////////////////////////////////////////////////////////

ViewPortShape::ViewPortShape (RectRef viewPort, Shape* content)
: TransformShape (Transform (), content),
  viewPort (viewPort)
{
	// note: no scaling required here: we report the specified viewport size in getBounds, and so a ShapeImage will use the appropriate scale factors
	transform.translate (-float(viewPort.left), -float(viewPort.top));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ViewPortShape::ViewPortShape (const ViewPortShape& vps)
: TransformShape (Transform (), AutoPtr<Shape> (static_cast<Shape*> (vps.shape->clone ()))),
  viewPort (vps.viewPort)
{
	// note: no scaling required here: we report the specified viewport size in getBounds, and so a ShapeImage will use the appropriate scale factors
	transform.translate (-float(viewPort.left), -float(viewPort.top));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect& ViewPortShape::getBounds (Rect& bounds) const
{
	bounds = Rect (0, 0, viewPort.getSize ());
	return bounds;
}

//************************************************************************************************
// TImageShape: ImageShape, ImageShapeF
//************************************************************************************************

DEFINE_SHAPE_CLASS_INT_FLOAT (ImageShape, Shape)

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TCoord>
Rect& TImageShape<TCoord>::getBounds (Rect& bounds) const
{
	assignRect (bounds, dstRect);
	return bounds;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TCoord>
void TImageShape<TCoord>::drawShape (GraphicsDevice& graphics, const ImageMode* mode) const
{
	ASSERT (image != nullptr)
	if(image)
	{
		if(mode && mode->getAlphaF () != 1.f)
		{
			ImageMode combinedMode (imageMode.getAlphaF () * mode->getAlphaF (), mode->getInterpolationMode ());
			graphics.drawImage (image, srcRect, dstRect, &combinedMode);
		}
		else
			graphics.drawImage (image, srcRect, dstRect, &imageMode);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TCoord>
void TImageShape<TCoord>::drawShape (GraphicsDevice& graphics, float sx, float sy, const ImageMode* mode) const
{
	ASSERT (image != nullptr)
	if(image)
	{
		if(mode && mode->getAlphaF () != 1.f)
		{
			ImageMode combinedMode (imageMode.getAlphaF () * mode->getAlphaF (), mode->getInterpolationMode ());
			graphics.drawImage (image, scale (srcRect, sx, sy), scale (dstRect, sx, sy), &combinedMode);
		}
		else
			graphics.drawImage (image, scale (srcRect, sx, sy), scale (dstRect, sx, sy), &imageMode);
	}
}
