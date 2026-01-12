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
// Filename    : ccl/gui/skin/skinshapes.cpp
// Description : Skin Shape Elements
//
//************************************************************************************************

#include "ccl/gui/skin/skinshapes.h"
#include "ccl/gui/skin/skinattributes.h"

#include "ccl/base/storage/url.h"
#include "ccl/base/collections/objectarray.h"
#include "ccl/gui/graphics/shapes/shapeimage.h"

#include "ccl/public/gui/framework/skinxmldefs.h"

#include "ccl/public/text/cstring.h"

namespace CCL {
namespace SkinElements {

//////////////////////////////////////////////////////////////////////////////////////////////////

void linkSkinShapes () {} // force linkage of this file

//************************************************************************************************
// ShapeElement
//************************************************************************************************

BEGIN_STYLEDEF (ShapeElement::shapeStyles)
	{"stroke",	Shape::kStroke},
	{"fill",	Shape::kFill},
	{"scale",	Shape::kScale},
	{"tiled", 	Shape::kTiled},
	{"margin", 	Shape::kMargin},
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_SKIN_ELEMENT_ABSTRACT_WITH_MEMBERS (ShapeElement, Element, TAG_BASESHAPE, DOC_GROUP_SHAPES, Shape)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_BRUSHCOLOR, TYPE_COLOR)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_BRUSHGRADIENT, TYPE_STRING)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_PENCOLOR, TYPE_COLOR)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_PENWIDTH, TYPE_FLOAT)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_SHAPEREF, TYPE_STRING)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_SIZE, TYPE_SIZE)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_STYLE, TYPE_ENUM)
END_SKIN_ELEMENT_WITH_MEMBERS (ShapeElement)
DEFINE_SKIN_ENUMERATION (TAG_BASESHAPE, ATTR_STYLE, ShapeElement::shapeStyles)

//////////////////////////////////////////////////////////////////////////////////////////////////

Shape* ShapeElement::newShape () const
{
	SKIN_WARNING (this, "Abstract shape element instanciated!", 0)
	return NEW Shape;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Shape& ShapeElement::getShape () const
{
	if(!shape)
		shape = newShape ();
	return *shape;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ShapeElement::setAttributes (const SkinAttributes& a)
{
	Shape& shape = getShape ();

	shape.setStyle (a.getOptions (ATTR_STYLE, shapeStyles));
	shapeRef = a.getString (ATTR_SHAPEREF);

	// check if Pen or Brush are defined "inline"...
	ColorValueReference penColor;
	if(SkinModel::getColorFromAttributes (penColor, a, ATTR_PENCOLOR, this))
	{
		Pen::Size penWidth = a.getFloat (ATTR_PENWIDTH, 1.f);

		if(penColor.scheme != nullptr)
		{
			shape.setStrokeColorReference (penColor.scheme, penColor.nameInScheme);
			shape.setStrokeWidth (penWidth);
		}
		else
		{
			Pen pen (penColor.colorValue);
			pen.setWidth (penWidth);
			shape.setStrokePen (pen);
		}
	}

	// Brush
	ColorValueReference brushColor;
	if(SkinModel::getColorFromAttributes (brushColor, a, ATTR_BRUSHCOLOR, this))
	{
		if(brushColor.scheme != nullptr)
			shape.setFillColorReference (brushColor.scheme, brushColor.nameInScheme);
		else
			shape.setFillBrush (SolidBrush (brushColor.colorValue));
	}
	else
	{
		StringID gradientName = a.getCString (ATTR_BRUSHGRADIENT);
		if(!gradientName.isEmpty ())
		{
			auto model = SkinModel::getModel (this);
			ASSERT (model)
			auto gradient = model ? model->getGradient (gradientName, this) : nullptr;
			GradientBrush brush;
			brush.setGradient (gradient);
			shape.setFillBrush (brush);
		}
	}

	return Element::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ShapeElement::getAttributes (SkinAttributes& a) const
{
	if(a.isVerbose ())
	{
		a.setColor (ATTR_PENCOLOR, Colors::kWhite);
		a.setFloat (ATTR_PENWIDTH, 1.f);
		a.setColor (ATTR_BRUSHCOLOR, Colors::kWhite);
		a.setSize (ATTR_SIZE, Rect ());
	}

	Shape& shape = getShape ();
	a.setOptions (ATTR_STYLE, shape.getStyle (), shapeStyles);
	a.setString (ATTR_SHAPEREF, shapeRef);
	return Element::getAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ShapeElement::loadFinished ()
{
	// if this shape is a reference only,
	// try to find "real" shape and copy its content...
	if(!shapeRef.isEmpty ())
		resolveShapeReference ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ShapeElement::resolveShapeReference ()
{
	Element* parent = getParent ();
	ASSERT (parent != nullptr)

	// shape must be of same class!!
	ShapeElement* shapeElement = parent ? (ShapeElement*)parent->findElement (shapeRef, myClass ()) : nullptr;
	if(shapeElement)
	{
		shape.release ();
		shape = (Shape*)shapeElement->getShape ().clone ();
	}
	// else: Skin Parser Warning: Referenced element not found!

	return shapeElement != nullptr;
}

//************************************************************************************************
// ComplexShapeElement
//************************************************************************************************

DEFINE_SKIN_ELEMENT (ComplexShapeElement, ShapeElement, TAG_SHAPE, DOC_GROUP_SHAPES, ComplexShape)
BEGIN_SKIN_ELEMENT_ATTRIBUTES (ComplexShapeElement)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_SHAPES)
	ADD_SKIN_CHILDGROUP_ATTRIBUTE (SCHEMA_GROUP_SHAPES)
END_SKIN_ELEMENT_ATTRIBUTES (ComplexShapeElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

Shape* ComplexShapeElement::newShape () const
{
	return NEW ComplexShape;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ComplexShapeElement::setAttributes (const SkinAttributes& a)
{
	ComplexShape& shape = (ComplexShape&)getShape ();

	// check if a size is given for this shape
	shape.setSize (ElementSizeParser ().trySizeAttributes (a));

	return ShapeElement::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ComplexShapeElement::getAttributes (SkinAttributes& a) const
{
	ComplexShape& shape = (ComplexShape&)getShape ();
	a.setSize (ATTR_SIZE, shape.getSize ());
	return ShapeElement::getAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ComplexShapeElement::loadFinished ()
{
	ShapeElement::loadFinished ();

	ComplexShape& shape = (ComplexShape&)getShape ();
	shape.setName (String (getName ()));

	// add sub-shapes...
	if(count () > 0)
	{
		ArrayForEach (*this, Element, e)
			ShapeElement* shapeElement = ccl_cast<ShapeElement> (e);
			if(shapeElement)
			{
				Shape& subShape= shapeElement->getShape ();
				subShape.retain ();
				shape.addShape (&subShape);
			}
		EndFor
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ComplexShapeElement::resolveShapeReference ()
{
	// remember & restore size after resolving...
	Rect r (((ComplexShape&)getShape ()).getSize ());

	bool result = ShapeElement::resolveShapeReference ();
	if(result)
		((ComplexShape&)getShape ()).setSize (r);

	return result;
}

//************************************************************************************************
// LineShapeElement
//************************************************************************************************
	
BEGIN_STYLEDEF (LineShapeElement::scaleAlignment)
	{"right",	LineShape::kRightAligned},
	{"bottom", 	LineShape::kBottomAligned},
END_STYLEDEF

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (LineShapeElement, ShapeElement, TAG_LINE, DOC_GROUP_SHAPES, LineShape)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_START, TYPE_POINT)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_END, TYPE_POINT)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_LINESCALEALIGN, TYPE_ENUM)
END_SKIN_ELEMENT_WITH_MEMBERS (LineShapeElement)
BEGIN_SKIN_ELEMENT_ATTRIBUTES (LineShapeElement)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_SHAPES)
END_SKIN_ELEMENT_ATTRIBUTES (LineShapeElement)
DEFINE_SKIN_ENUMERATION (TAG_LINE, ATTR_LINESCALEALIGN, LineShapeElement::scaleAlignment)

//////////////////////////////////////////////////////////////////////////////////////////////////

Shape* LineShapeElement::newShape () const
{
	return NEW LineShape;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LineShapeElement::setAttributes (const SkinAttributes& a)
{
	LineShape& shape = (LineShape&)getShape ();

	Point p;
	a.getPoint (p, ATTR_START);
	shape.setStart (p);
	a.getPoint (p, ATTR_END);
	shape.setEnd (p);
	shape.setScaleAlignment (a.getOptions (ATTR_LINESCALEALIGN, LineShapeElement::scaleAlignment, false, 0));

	bool result = ShapeElement::setAttributes (a);
	shape.isStroke (true); // lines can only be drawn!
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LineShapeElement::getAttributes (SkinAttributes& a) const
{
	LineShape& shape = (LineShape&)getShape ();
	a.setPoint (ATTR_START, shape.getStart ());
	a.setPoint (ATTR_END, shape.getEnd ());
	a.setOptions (ATTR_LINESCALEALIGN, shape.getScaleAlignment (), LineShapeElement::scaleAlignment);

	return ShapeElement::getAttributes (a);
}

//************************************************************************************************
// RectShapeElement
//************************************************************************************************

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (RectShapeElement, ShapeElement, TAG_RECTSHAPE, DOC_GROUP_SHAPES, RectShape)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_RADIUS, TYPE_INT)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_RECT, TYPE_RECT)
END_SKIN_ELEMENT_WITH_MEMBERS (RectShapeElement)
BEGIN_SKIN_ELEMENT_ATTRIBUTES (RectShapeElement)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_SHAPES)
END_SKIN_ELEMENT_ATTRIBUTES (RectShapeElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

Shape* RectShapeElement::newShape () const
{
	return NEW RectShape;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RectShapeElement::setAttributes (const SkinAttributes& a)
{
	RectShape& rectShape = (RectShape&)getShape ();
	rectShape.setRect (ElementSizeParser ().trySizeAttributes (a));

	int radius = a.getInt (ATTR_RADIUS);
	rectShape.setRadiusX (radius);
	rectShape.setRadiusY (radius);
	return ShapeElement::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RectShapeElement::getAttributes (SkinAttributes& a) const
{
	RectShape& rectShape = (RectShape&)getShape ();

	a.setRect (ATTR_RECT, rectShape.getRect ());
	a.setInt (ATTR_RADIUS, rectShape.getRadiusX ());
	return ShapeElement::getAttributes (a);
}

//************************************************************************************************
// EllipseShapeElement
//************************************************************************************************

DEFINE_SKIN_ELEMENT (EllipseShapeElement, RectShapeElement, TAG_ELLIPSE, DOC_GROUP_SHAPES, EllipseShape)

//////////////////////////////////////////////////////////////////////////////////////////////////

Shape* EllipseShapeElement::newShape () const
{
	return NEW EllipseShape;
}

//************************************************************************************************
// TriangleShapeElement
//************************************************************************************************

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (TriangleShapeElement, ShapeElement, TAG_TRIANGLE, DOC_GROUP_SHAPES, TriangleShape)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_POINT1, TYPE_POINT)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_POINT2, TYPE_POINT)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_POINT3, TYPE_POINT)
END_SKIN_ELEMENT_WITH_MEMBERS (TriangleShapeElement)
BEGIN_SKIN_ELEMENT_ATTRIBUTES (TriangleShapeElement)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_SHAPES)
END_SKIN_ELEMENT_ATTRIBUTES (TriangleShapeElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

Shape* TriangleShapeElement::newShape () const
{
	return NEW TriangleShape;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TriangleShapeElement::setAttributes (const SkinAttributes& a)
{
	TriangleShape& shape = (TriangleShape&)getShape ();

	Point p;
	a.getPoint (p, ATTR_POINT1);
	shape.setP1 (p);
	a.getPoint (p, ATTR_POINT2);
	shape.setP2 (p);
	a.getPoint (p, ATTR_POINT3);
	shape.setP3 (p);

	return ShapeElement::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TriangleShapeElement::getAttributes (SkinAttributes& a) const
{
	TriangleShape& shape = (TriangleShape&)getShape ();
	a.setPoint (ATTR_POINT1, shape.getP1 ());
	a.setPoint (ATTR_POINT2, shape.getP2 ());
	a.setPoint (ATTR_POINT3, shape.getP3 ());
	return ShapeElement::getAttributes (a);
}

//************************************************************************************************
// ShapeImageElement
//************************************************************************************************

DEFINE_SKIN_ELEMENT (ShapeImageElement, ImageElement, TAG_SHAPEIMAGE, DOC_GROUP_SHAPES, ShapeImage)
BEGIN_SKIN_ELEMENT_ATTRIBUTES (ShapeImageElement)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_IMAGECHILDREN)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_RESOURCES)
	ADD_SKIN_CHILDGROUP_ATTRIBUTE (TAG_SHAPECOLORMAPPING)
END_SKIN_ELEMENT_ATTRIBUTES (ShapeImageElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ShapeImageElement::loadImage (SkinModel& model)
{
	if(!image)
	{
		ShapeElement* shapeElement = model.getShapes ().findElement<ShapeElement> (MutableCString (url));
		if(shapeElement)
		{
			Shape* shape = &shapeElement->getShape ();
			image = NEW ShapeImage (shape);

			if(!frames.isEmpty ())
				((ShapeImage*)image)->setFilmstrip (true);
		}
		else if(!url.isEmpty ())
		{
			Url imageUrl;
			makeSkinUrl (imageUrl, url);
			image = Image::loadImage (imageUrl);
		}
	}
	
	applyShapeModification ();
	
	return image != nullptr;
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////
	
void ShapeImageElement::applyShapeModification ()
{
	ShapeImage* shapeImage = ccl_cast<ShapeImage> (image);
	Shape* shape = shapeImage ? shapeImage->getShape () : nullptr;
	
	if(!shape)
		return;
	
	shapeImage->setIsTemplate (isTemplate);
	shapeImage->setIsAdaptive (isAdaptive);
	
	// check for shape color mappings
	Vector<ShapeColorMappingElement*> mappings;
	ArrayForEachFast (*this, Element, e)
	if(ShapeColorMappingElement* mapping = ccl_cast<ShapeColorMappingElement> (e))
		mappings.add (mapping);
	EndFor
	
	if(mappings.isEmpty ())
		return;
	
	// apply mappings deep
	VectorForEachFast (mappings, ShapeColorMappingElement*, mapping)
		applyShapeModificationDeep (mapping, shape);
	EndFor
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////
	
void ShapeImageElement::applyShapeModificationDeep (ShapeColorMappingElement* mapping, Shape* shape)
{
	if(shape->countShapes () == 0)
	{
		if(mapping->getScheme () != nullptr)
		{
			if(shape->getStrokePen ().getColor () == mapping->getColor ())
				shape->setStrokeColorReference (mapping->getScheme (), mapping->getNameInScheme ());
			if(shape->getFillBrush ().getColor () == mapping->getColor ())
				shape->setFillColorReference (mapping->getScheme (), mapping->getNameInScheme ());
		}
		else
		{
			if(shape->getStrokePen ().getColor () == mapping->getColor ())
			{
				Pen strokePen (shape->getStrokePen ());
				strokePen.setColor (mapping->getReferenceColor ());
				shape->setStrokePen (strokePen);
			}
			if(shape->getFillBrush ().getColor () == mapping->getColor ())
			{
				SolidBrush fillBrush (shape->getFillBrush ());
				fillBrush.setColor (mapping->getReferenceColor ());
				shape->setFillBrush (fillBrush);
			}
		}
	}
	else
	{
		int subShapesCount = shape->countShapes ();
		for(int i = 0; i < subShapesCount; i++)
			applyShapeModificationDeep (mapping, shape->getShape (i));
	}
}
	
} // namespace SkinElements
} // CCL
