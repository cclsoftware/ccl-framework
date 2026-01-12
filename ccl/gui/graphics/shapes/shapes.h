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
// Filename    : ccl/gui/graphics/shapes/shapes.h
// Description : Shapes
//
//************************************************************************************************

#ifndef _ccl_shapes_h
#define _ccl_shapes_h

#include "ccl/gui/graphics/graphicspath.h"
#include "ccl/gui/theme/colorreference.h"

#include "ccl/public/gui/graphics/iimage.h"

#include "ccl/public/text/cstring.h"

namespace CCL {

class GraphicsDevice;
class ObjectArray;
class ColorScheme;
class ColorGradient;
interface IColorScheme;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Shape macros
//////////////////////////////////////////////////////////////////////////////////////////////////

#define DECLARE_SHAPE_TYPE(type) \
	int getType () const override { return type; }

#define DECLARE_SHAPE_CLASS_INT_FLOAT2(ShapeClass,BaseClassI,BaseClassF)\
	class ShapeClass:    public T##ShapeClass<Coord>  { public: DECLARE_CLASS (ShapeClass, BaseClassI) }; \
	class ShapeClass##F: public T##ShapeClass<CoordF> { public: DECLARE_CLASS (ShapeClass##F, BaseClassF) };

#define DEFINE_SHAPE_CLASS_INT_FLOAT2(ShapeClass,BaseClassI,BaseClassF)\
	DEFINE_CLASS_HIDDEN (ShapeClass, BaseClassI) \
	DEFINE_CLASS_HIDDEN (ShapeClass##F, BaseClassF)

#define DECLARE_SHAPE_CLASS_INT_FLOAT(ShapeClass,BaseClass) DECLARE_SHAPE_CLASS_INT_FLOAT2 (ShapeClass,BaseClass,BaseClass)
#define DEFINE_SHAPE_CLASS_INT_FLOAT(ShapeClass,BaseClass) DEFINE_SHAPE_CLASS_INT_FLOAT2 (ShapeClass,BaseClass,BaseClass)

//************************************************************************************************
// Shape
//************************************************************************************************

class Shape: public Object
{
public:
	DECLARE_CLASS (Shape, Object)

	Shape (int style = 0);
	Shape (const Shape&);
	~Shape ();

	enum BasicShapes
	{
		kUnknown = -1,
		kComplex,				///< @see ComplexShape
		kLine,					///< @see LineShape
		kRectangle,				///< @see RectShape
		kEllipse,				///< @see EllipseShape
		kTriangle,				///< @see TriangleShape
		kPath,					///< @see PathShape
		kText,					///< @see TextShape
		kTransform,				///< @see TransformShape
		kImage,					///< @see ImageShape
		kNumBasicShapes
	};

	enum Styles
	{
		kStroke	   = 1<<0,		///< draw shape's outline with "Pen.color" only
		kFill      = 1<<1,		///< fill entire shape with "Brush.color" or "Brush.gradient"
		kAntiAlias = 1<<2,		///< antialiasing should be enabled
		kScale	   = 1<<3,		///< shape cordinates should be scaled when drawing in other size (instead of raster graphic transformation); specify for top level shape
		kTiled	   = 1<<4,		///< used by RectShape: draw shape tiled (don't stretch the pen border - keep rect radius)
		kMargin	   = 1<<5,		///< used by RectShape: left and top margins are used prevent scaling. It is expected that left equals right margin - and top equals bottom margin.
		
		kStrokeAndFill = kStroke|kFill,
		kLastStyleFlag = 5
	};

	PROPERTY_VARIABLE (int, style, Style)
	PROPERTY_FLAG (style, kStroke, isStroke)
	PROPERTY_FLAG (style, kFill, isFill)
	PROPERTY_FLAG (style, kScale, shouldScale)
	PROPERTY_FLAG (style, kAntiAlias, shouldAntiAlias)
	PROPERTY_FLAG (style, kTiled, isTiled)
	PROPERTY_FLAG (style, kMargin, keepMargin)
	
	PROPERTY_OBJECT (Pen, strokePen, StrokePen)
	PROPERTY_OBJECT (Brush, fillBrush, FillBrush)
	
	Pen getStrokePen (const ImageMode* mode) const;
	Brush getFillBrush (const ImageMode* mode) const;
	void setPenAndBrush (PenRef pen, BrushRef brush);
	void setStrokeWidth (Pen::Size width) { strokePen.setWidth (width); }
	void setStrokeColorReference (ColorScheme* scheme, StringID nameInScheme);
	void setFillColorReference (ColorScheme* scheme, StringID nameInScheme);
	virtual bool hasReferences (IColorScheme& scheme) const;
	
	PROPERTY_STRING (name, Name)

	virtual int getType () const { return kUnknown; }

	int getWidth () const;
	int getHeight () const;
	virtual Rect& getBounds (Rect& bounds) const;

	// Sub-shapes
	virtual int countShapes () const;
	virtual Shape* getShape (int index) const;
	virtual Shape* findShape (StringRef name); ///< recursively

	virtual void drawShape (GraphicsDevice& graphics, const ImageMode* mode = nullptr) const;
	virtual void drawShape (GraphicsDevice& graphics, float sx, float sy, const ImageMode* mode = nullptr) const;

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

protected:
	ColorSchemeReference strokeColorReference;
	ColorSchemeReference fillColorReference;
	mutable float currentSx;
	mutable float currentSy;
	
	mutable struct GradientCache
	{
		ColorGradient* modifiedGradient;
		ColorGradient* sourceGradient;
		float cachedSx;
		float cachedSy;
		float cachedAlpha;
		
		GradientCache ();
		~GradientCache ();

		IGradient* getModifiedGradient (ColorGradient* gradient, float sx, float sy, float alpha);
	} gradientCache;
};

//************************************************************************************************
// ComplexShape
//************************************************************************************************

class ComplexShape: public Shape
{
public:
	DECLARE_CLASS (ComplexShape, Shape)

	ComplexShape ();
	ComplexShape (const ComplexShape&);
	~ComplexShape ();

	PROPERTY_OBJECT (Rect, size, Size)	///< size of whole shape (optional)

	void addShape (Shape* shape);
	Rect& getJoinedBounds (Rect& bounds) const;

	// Shape
	DECLARE_SHAPE_TYPE (kComplex)
	Rect& getBounds (Rect& bounds) const override;
	int countShapes () const override;
	Shape* getShape (int index) const override;
	Shape* findShape (StringRef name) override;
	void drawShape (GraphicsDevice& graphics, const ImageMode* mode = nullptr) const override;
	void drawShape (GraphicsDevice& graphics, float sx, float sy, const ImageMode* mode = nullptr) const override;
	bool hasReferences (IColorScheme& scheme) const override;

protected:
	ObjectArray* shapes;
};

//************************************************************************************************
// LineShape, LineShapeF
//************************************************************************************************

template<class TCoord>
class TLineShape: public Shape
{
public:
	TLineShape (const TPoint<TCoord>& start = TPoint<TCoord> (),
			   const TPoint<TCoord>& end = TPoint<TCoord> (),
			   int style = 0);

	enum ScaleAlignment
	{
		kRightAligned	= 1<<0,		///< draw line right aligned when shape is scaled
		kBottomAligned	= 1<<1,		///< draw line bottom aligned when shape is scaled
	};

	PROPERTY_OBJECT (TPoint<TCoord>, start, Start)
	PROPERTY_OBJECT (TPoint<TCoord>, end, End)
	PROPERTY_VARIABLE (int, scaleAlignment, ScaleAlignment)
	
	// Shape
	DECLARE_SHAPE_TYPE (kLine)
	Rect& getBounds (Rect& bounds) const override;
	void drawShape (GraphicsDevice& graphics, const ImageMode* mode = nullptr) const override;
	void drawShape (GraphicsDevice& graphics, float sx, float sy, const ImageMode* mode = nullptr) const override;
};

DECLARE_SHAPE_CLASS_INT_FLOAT (LineShape, Shape)

//************************************************************************************************
// RectShape, RectShapeF
//************************************************************************************************

template<class TCoord>
class TRectShape: public Shape
{
public:
	TRectShape (const TRect<TCoord>& rect = TRect<TCoord> (), int style = 0);

	PROPERTY_OBJECT (TRect<TCoord>, rect, Rect)
	PROPERTY_VARIABLE (TCoord, rx, RadiusX)
	PROPERTY_VARIABLE (TCoord, ry, RadiusY)

	// Shape
	DECLARE_SHAPE_TYPE (kRectangle)
	Rect& getBounds (Rect& bounds) const override;
	void drawShape (GraphicsDevice& graphics, const ImageMode* mode = nullptr) const override;
	void drawShape (GraphicsDevice& graphics, float sx, float sy, const ImageMode* mode = nullptr) const override;

protected:
	void drawRectShape (GraphicsDevice& graphics, const TRect<TCoord>& rect, const ImageMode* mode) const;
	void drawRectShape (GraphicsDevice& graphics, GraphicsPath& path, const ImageMode* mode) const;

	typedef Shape SuperClass;

	mutable struct PathCache
	{
		AutoPtr<GraphicsPath> path;
		TRect<TCoord> rect;
		TCoord rx = 0;
		TCoord ry = 0;

		GraphicsPath& getPath (const TRect<TCoord>& rect, TCoord rx, TCoord ry);
	} pathCache;
};

DECLARE_SHAPE_CLASS_INT_FLOAT (RectShape, Shape)

//************************************************************************************************
// TriangleShape, TriangleShapeF
//************************************************************************************************

template<class TCoord>
class TTriangleShape: public Shape
{
public:
	TTriangleShape (const TPoint<TCoord>& p1 = TPoint<TCoord> (),
				   const TPoint<TCoord>& p2 = TPoint<TCoord> (),
				   const TPoint<TCoord>& p3 = TPoint<TCoord> (),
				   int style = 0);

	PROPERTY_OBJECT (TPoint<TCoord>, p1, P1)
	PROPERTY_OBJECT (TPoint<TCoord>, p2, P2)
	PROPERTY_OBJECT (TPoint<TCoord>, p3, P3)

	// Shape
	DECLARE_SHAPE_TYPE (kTriangle)
	Rect& getBounds (Rect& bounds) const override;
	void drawShape (GraphicsDevice& graphics, const ImageMode* mode = nullptr) const override;
	void drawShape (GraphicsDevice& graphics, float sx, float sy, const ImageMode* mode = nullptr) const override;

protected:
	mutable struct PathCache
	{
		AutoPtr<GraphicsPath> path;
		TPoint<TCoord> p1, p2, p3;

		GraphicsPath& getPath (const TPoint<TCoord>& p1, const TPoint<TCoord>& p2, const TPoint<TCoord>& p3);
	} pathCache;
};

DECLARE_SHAPE_CLASS_INT_FLOAT (TriangleShape, Shape)

//************************************************************************************************
// EllipseShape, EllipseShapeF
//************************************************************************************************

template<class TCoord>
class TEllipseShape: public TRectShape<TCoord>
{
public:
	TEllipseShape (const TRect<TCoord>& rect = TRect<TCoord> (), int style = 0);

	// Shape
	DECLARE_SHAPE_TYPE (Shape::kEllipse)
	void drawShape (GraphicsDevice& graphics, const ImageMode* mode = nullptr) const override;
	void drawShape (GraphicsDevice& graphics, float sx, float sy, const ImageMode* mode = nullptr) const override;
};

DECLARE_SHAPE_CLASS_INT_FLOAT2 (EllipseShape, RectShape, RectShapeF)

//************************************************************************************************
// PathShape
//************************************************************************************************

class PathShape: public Shape
{
public:
	DECLARE_CLASS (PathShape, Shape)

	PathShape (GraphicsPath* path = nullptr, int style = 0);
	PathShape (const PathShape&);
	~PathShape ();

	PROPERTY_SHARED (GraphicsPath, path, Path)

	// Shape
	DECLARE_SHAPE_TYPE (kPath)
	Rect& getBounds (Rect& bounds) const override;
	void drawShape (GraphicsDevice& graphics, const ImageMode* mode = nullptr) const override;
	void drawShape (GraphicsDevice& graphics, float sx, float sy, const ImageMode* mode = nullptr) const override;

protected:
	mutable struct PathCache
	{
		AutoPtr<GraphicsPath> path;
		const GraphicsPath* p;
		float sx = 0;
		float sy = 0;

		GraphicsPath& getPath (const GraphicsPath* path, float sx, float sy);
	} pathCache;
};

//************************************************************************************************
// TTextShape
//************************************************************************************************

template<class TCoord>
class TTextShape: public Shape
{
public:
	TTextShape (StringRef text = nullptr, int style = 0);

	PROPERTY_OBJECT  (Font, font, Font)
	PROPERTY_VARIABLE (Alignment, alignment, Alignment)
	PROPERTY_OBJECT (TPoint<TCoord>, position, Position)
	PROPERTY_OBJECT (TPoint<TCoord>, size, Size)
	PROPERTY_STRING (text, Text)

	// Shape
	DECLARE_SHAPE_TYPE (kText)
	Rect& getBounds (Rect& bounds) const override;
	void drawShape (GraphicsDevice& graphics, const ImageMode* mode = nullptr) const override;
	void drawShape (GraphicsDevice& graphics, float sx, float sy, const ImageMode* mode = nullptr) const override;
};

DECLARE_SHAPE_CLASS_INT_FLOAT (TextShape, Shape)

//************************************************************************************************
// TransformShape
//************************************************************************************************

class TransformShape: public Shape
{
public:
	DECLARE_CLASS (TransformShape, Shape)

	TransformShape (TransformRef transform = Transform (), Shape* shape = nullptr);
	TransformShape (const TransformShape&);
	~TransformShape ();

	PROPERTY_OBJECT (Transform, transform, Transform)

	// Shape
	DECLARE_SHAPE_TYPE (kTransform)
	int countShapes () const override;
	Shape* getShape (int index) const override;
	Rect& getBounds (Rect& bounds) const override;
	void drawShape (GraphicsDevice& graphics, const ImageMode* mode = nullptr) const override;
	void drawShape (GraphicsDevice& graphics, float sx, float sy, const ImageMode* mode = nullptr) const override;

protected:
	Rect& getTransformedBounds (const Shape& subShape, Rect& bounds) const;

	Shape* shape;
};

//************************************************************************************************
// ViewPortShape
//************************************************************************************************

class ViewPortShape: public TransformShape
{
public:
	DECLARE_CLASS (ViewPortShape, TransformShape)
	
	ViewPortShape (RectRef viewPort = Rect (), Shape* content = nullptr);
	ViewPortShape (const ViewPortShape&);
	
	// Shape
	Rect& getBounds (Rect& bounds) const override;

private:
	Rect viewPort;
};

//************************************************************************************************
// ImageShape, ImageShapeF
//************************************************************************************************

template<class TCoord>
class TImageShape: public Shape
{
public:
	TImageShape (IImage* image = nullptr);

	PROPERTY_SHARED_AUTO (IImage, image, Image)
	PROPERTY_OBJECT (TRect<TCoord>, srcRect, SrcRect)
	PROPERTY_OBJECT (TRect<TCoord>, dstRect, DstRect)
	PROPERTY_OBJECT (ImageMode, imageMode, ImageMode)

	// Shape
	DECLARE_SHAPE_TYPE (kImage)
	Rect& getBounds (Rect& bounds) const override;
	void drawShape (GraphicsDevice& graphics, const ImageMode* mode = nullptr) const override;
	void drawShape (GraphicsDevice& graphics, float sx, float sy, const ImageMode* mode = nullptr) const override;
};

DECLARE_SHAPE_CLASS_INT_FLOAT (ImageShape, Shape)

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TCoord>
inline TLineShape<TCoord>::TLineShape (const TPoint<TCoord>& start, const TPoint<TCoord>& end, int style)
: Shape (style), start (start), end (end) {}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TCoord>
inline TRectShape<TCoord>::TRectShape (const TRect<TCoord>& rect, int style)
: Shape (style), rect (rect), rx (0), ry (0) {}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TCoord>
inline TTriangleShape<TCoord>::TTriangleShape (const TPoint<TCoord>& p1, const TPoint<TCoord>& p2, const TPoint<TCoord>& p3, int style)
: Shape (style), p1 (p1), p2 (p2), p3 (p3) {}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TCoord>
inline TEllipseShape<TCoord>::TEllipseShape (const TRect<TCoord>& rect, int style)
: TRectShape<TCoord> (rect, style) {}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TCoord>
inline TTextShape<TCoord>::TTextShape (StringRef text, int style)
: Shape (style), text (text) {}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TCoord>
inline TImageShape<TCoord>::TImageShape (IImage* image)
: image (image) {}

} // namespace CCL

#endif // _ccl_shapes_h
