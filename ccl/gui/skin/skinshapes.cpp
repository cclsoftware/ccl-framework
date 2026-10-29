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

DEFINE_CLASS_ABSTRACT_HIDDEN (ShapeElement, Element)
DEFINE_TYPELIB_CLASS_WITH_PARENT (ShapeElement, Element, TAG_BASESHAPE, SKIN_TYPELIB)
DEFINE_TYPELIB_ABSTRACT (ShapeElement)
DEFINE_TYPELIB_DOCGROUP (ShapeElement, DOC_GROUP_SHAPES)
DEFINE_TYPELIB_RELATED_CLASS (ShapeElement, Shape)
BEGIN_TYPELIB_MEMBERS (ShapeElement)
	ADD_TYPELIB_MEMBER_COLOR (ATTR_BRUSHCOLOR)
	ADD_TYPELIB_MEMBER_STRING (ATTR_BRUSHGRADIENT)
	ADD_TYPELIB_MEMBER_COLOR (ATTR_PENCOLOR)
	ADD_TYPELIB_MEMBER (ATTR_PENWIDTH, TypeNames::kFloat)
	ADD_TYPELIB_MEMBER_STRING (ATTR_SHAPEREF)
	ADD_TYPELIB_MEMBER (ATTR_SIZE, TypeNames::kSize)
	ADD_TYPELIB_MEMBER (ATTR_STYLE, TypeNames::kEnum)
END_TYPELIB_MEMBERS (ShapeElement)
DEFINE_TYPELIB_ENUMERATION (TAG_BASESHAPE, ATTR_STYLE, ShapeElement::shapeStyles, SKIN_TYPELIB)

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

DEFINE_CLASS_HIDDEN (ComplexShapeElement, ShapeElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (ComplexShapeElement, ShapeElement, TAG_SHAPE, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (ComplexShapeElement, DOC_GROUP_SHAPES)
DEFINE_TYPELIB_RELATED_CLASS (ComplexShapeElement, ComplexShape)
BEGIN_TYPELIB_ATTRIBUTES (ComplexShapeElement)
	ADD_TYPELIB_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_SHAPES)
	ADD_TYPELIB_CHILDGROUP_ATTRIBUTE (SCHEMA_GROUP_SHAPES)
END_TYPELIB_ATTRIBUTES (ComplexShapeElement)

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

				// resolve embedded image
				if(auto* imageElement = ccl_cast<EmbeddedImageShapeElement> (shapeElement))
					if(auto* imageShape = ccl_cast<ImageShape> (&subShape))
						resolveEmbeddedImage (*imageShape, *imageElement);
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

//////////////////////////////////////////////////////////////////////////////////////////////////

void ComplexShapeElement::resolveEmbeddedImage (ImageShape& imageShape, EmbeddedImageShapeElement& imageElement) const
{
	SharedPtr<Image> image;
	
	SkinModel* model = SkinModel::getModel (this);
	if(model)
		image = model->getImage (imageElement.getImageName (), &imageElement);

	if(!image && SkinModel::loadingModel && SkinModel::loadingModel != model) // access outer loading skin model (not current include)
		image = SkinModel::loadingModel->getImage (imageElement.getImageName (), &imageElement);

	// deep-copy shapes for potential color mapping
	if(auto* shapeImage = ccl_cast<ShapeImage> (image))
	{
		AutoPtr<ShapeImage> imageCopy = shapeImage->cloneDeep ();
		image = imageCopy;
	}

	if(image)
	{
		if(imageShape.getSrcRect ().isEmpty ())
			imageShape.setSrcRect (Rect (0, 0, image->getWidth (), image->getHeight ()));

		if(imageShape.getDstRect ().isEmpty ())
		{
			Coord width = imageShape.getWidth ();
			Coord height = imageShape.getHeight ();
			imageShape.setDstRect (Rect ((width - image->getWidth ()) / 2, (height - image->getHeight ()) / 2, width, height));
		}
	}

	imageShape.setImage (image);
}

//************************************************************************************************
// LineShapeElement
//************************************************************************************************
	
BEGIN_STYLEDEF (LineShapeElement::scaleAlignment)
	{"right",	LineShape::kRightAligned},
	{"bottom", 	LineShape::kBottomAligned},
END_STYLEDEF

DEFINE_CLASS_HIDDEN (LineShapeElement, ShapeElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (LineShapeElement, ShapeElement, TAG_LINE, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (LineShapeElement, DOC_GROUP_SHAPES)
DEFINE_TYPELIB_RELATED_CLASS (LineShapeElement, LineShape)
BEGIN_TYPELIB_MEMBERS (LineShapeElement)
	ADD_TYPELIB_MEMBER (ATTR_START, TypeNames::kPoint)
	ADD_TYPELIB_MEMBER (ATTR_END, TypeNames::kPoint)
	ADD_TYPELIB_MEMBER (ATTR_LINESCALEALIGN, TypeNames::kEnum)
END_TYPELIB_MEMBERS (LineShapeElement)
BEGIN_TYPELIB_ATTRIBUTES (LineShapeElement)
	ADD_TYPELIB_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_SHAPES)
END_TYPELIB_ATTRIBUTES (LineShapeElement)
DEFINE_TYPELIB_ENUMERATION (TAG_LINE, ATTR_LINESCALEALIGN, LineShapeElement::scaleAlignment, SKIN_TYPELIB)

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

DEFINE_CLASS_HIDDEN (RectShapeElement, ShapeElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (RectShapeElement, ShapeElement, TAG_RECTSHAPE, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (RectShapeElement, DOC_GROUP_SHAPES)
DEFINE_TYPELIB_RELATED_CLASS (RectShapeElement, RectShape)
BEGIN_TYPELIB_MEMBERS (RectShapeElement)
	ADD_TYPELIB_MEMBER (ATTR_RADIUS, TypeNames::kInt)
	ADD_TYPELIB_MEMBER (ATTR_RECT, TypeNames::kRect)
END_TYPELIB_MEMBERS (RectShapeElement)
BEGIN_TYPELIB_ATTRIBUTES (RectShapeElement)
	ADD_TYPELIB_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_SHAPES)
END_TYPELIB_ATTRIBUTES (RectShapeElement)

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

DEFINE_CLASS_HIDDEN (EllipseShapeElement, RectShapeElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (EllipseShapeElement, RectShapeElement, TAG_ELLIPSE, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (EllipseShapeElement, DOC_GROUP_SHAPES)
DEFINE_TYPELIB_RELATED_CLASS (EllipseShapeElement, EllipseShape)

//////////////////////////////////////////////////////////////////////////////////////////////////

Shape* EllipseShapeElement::newShape () const
{
	return NEW EllipseShape;
}

//************************************************************************************************
// TriangleShapeElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (TriangleShapeElement, ShapeElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (TriangleShapeElement, ShapeElement, TAG_TRIANGLE, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (TriangleShapeElement, DOC_GROUP_SHAPES)
DEFINE_TYPELIB_RELATED_CLASS (TriangleShapeElement, TriangleShape)
BEGIN_TYPELIB_MEMBERS (TriangleShapeElement)
	ADD_TYPELIB_MEMBER (ATTR_POINT1, TypeNames::kPoint)
	ADD_TYPELIB_MEMBER (ATTR_POINT2, TypeNames::kPoint)
	ADD_TYPELIB_MEMBER (ATTR_POINT3, TypeNames::kPoint)
END_TYPELIB_MEMBERS (TriangleShapeElement)
BEGIN_TYPELIB_ATTRIBUTES (TriangleShapeElement)
	ADD_TYPELIB_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_SHAPES)
END_TYPELIB_ATTRIBUTES (TriangleShapeElement)

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
// EmbeddedImageShapeElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (EmbeddedImageShapeElement, ShapeElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (EmbeddedImageShapeElement, ShapeElement, TAG_GRAPHIC, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (EmbeddedImageShapeElement, DOC_GROUP_SHAPES)
DEFINE_TYPELIB_RELATED_CLASS (EmbeddedImageShapeElement, ImageShape)
BEGIN_TYPELIB_MEMBERS (EmbeddedImageShapeElement)
	ADD_TYPELIB_MEMBER_STRING (ATTR_IMAGE) ///< name of image to embed (from resources section)
	ADD_TYPELIB_MEMBER (ATTR_SOURCE, TypeNames::kRect) ///< image source rectangle
END_TYPELIB_MEMBERS (EmbeddedImageShapeElement)
BEGIN_TYPELIB_ATTRIBUTES (EmbeddedImageShapeElement)
	ADD_TYPELIB_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_SHAPES)
END_TYPELIB_ATTRIBUTES (EmbeddedImageShapeElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

Shape* EmbeddedImageShapeElement::newShape () const
{
	return NEW ImageShape;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EmbeddedImageShapeElement::setAttributes (const SkinAttributes& a)
{
	ImageShape& imageShape = (ImageShape&)getShape ();

	imageName = a.getString (ATTR_IMAGE);

	Rect srcRect;
	a.getSize (srcRect, ATTR_SOURCE);
	imageShape.setSrcRect (srcRect);

	Rect dstRect;
	a.getSize (dstRect, ATTR_SIZE);
	imageShape.setDstRect (dstRect);

	return ShapeElement::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EmbeddedImageShapeElement::getAttributes (SkinAttributes& a) const
{
	ImageShape& imageShape = (ImageShape&)getShape ();

	a.setString (ATTR_IMAGE, imageName);
	a.setSize (ATTR_SOURCE, imageShape.getSrcRect ());
	a.setSize (ATTR_SIZE, imageShape.getDstRect ());

	return ShapeElement::getAttributes (a);
}

//************************************************************************************************
// ShapeImageElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ShapeImageElement, ImageElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (ShapeImageElement, ImageElement, TAG_SHAPEIMAGE, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (ShapeImageElement, DOC_GROUP_SHAPES)
DEFINE_TYPELIB_RELATED_CLASS (ShapeImageElement, ShapeImage)
BEGIN_TYPELIB_ATTRIBUTES (ShapeImageElement)
	ADD_TYPELIB_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_IMAGECHILDREN)
	ADD_TYPELIB_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_RESOURCES)
	ADD_TYPELIB_CHILDGROUP_ATTRIBUTE (TAG_SHAPECOLORMAPPING)
END_TYPELIB_ATTRIBUTES (ShapeImageElement)

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
		// check for embedded shape image
		if(auto* imageShape = ccl_cast<ImageShape> (shape))
			if(auto* embeddedImage = unknown_cast<ShapeImage> (imageShape->getImage ()))
				if(Shape* embeddedShape = embeddedImage->getShape ())
				{
					applyShapeModificationDeep (mapping, embeddedShape);
				}

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
