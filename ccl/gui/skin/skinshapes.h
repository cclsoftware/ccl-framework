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
// Filename    : ccl/gui/skin/skinshapes.h
// Description : Skin Shape Elements
//
//************************************************************************************************

#ifndef _ccl_skinshapes_h
#define _ccl_skinshapes_h

#include "ccl/gui/skin/skinmodel.h"

#include "ccl/gui/graphics/shapes/shapes.h"

namespace CCL {
namespace SkinElements {

//************************************************************************************************
// ShapeElement
/** A Shape is a vector graphics object, a combination of geometric figures that can be scaled without quality loss.
This is the base class of all shapes, not to be used directly. */
//************************************************************************************************

class ShapeElement: public Element
{
public:
	DECLARE_SKIN_ELEMENT (ShapeElement, Element)

	DECLARE_STYLEDEF (shapeStyles)

	Shape& getShape () const;

	PROPERTY_MUTABLE_CSTRING (shapeRef, ShapeRef)

	// Element
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
	void loadFinished () override;

protected:
	mutable AutoPtr<Shape> shape;

	virtual Shape* newShape () const;
	virtual bool resolveShapeReference ();
};

//************************************************************************************************
// ComplexShapeElement
/** A shape that does not draw anything itself, but contains child shapes. */
//************************************************************************************************

class ComplexShapeElement: public ShapeElement
{
public:
	DECLARE_SKIN_ELEMENT (ComplexShapeElement, ShapeElement)

	// Element
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
	void loadFinished () override;

protected:
	Shape* newShape () const override;
	bool resolveShapeReference () override;
};

//************************************************************************************************
// LineShapeElement
/** A shape that draws a line. */
//************************************************************************************************

class LineShapeElement: public ShapeElement
{
public:
	DECLARE_SKIN_ELEMENT (LineShapeElement, ShapeElement)
	
	DECLARE_STYLEDEF (scaleAlignment)

	// Element
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;

protected:
	Shape* newShape () const override;
};

//************************************************************************************************
// RectShapeElement
/** A shape that draws a rectangle. */
//************************************************************************************************

class RectShapeElement: public ShapeElement
{
public:
	DECLARE_SKIN_ELEMENT (RectShapeElement, ShapeElement)

	// Element
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;

protected:
	Shape* newShape () const override;
};

//************************************************************************************************
// EllipseShapeElement
/** A shape that draws an ellipse. */
//************************************************************************************************

class EllipseShapeElement: public RectShapeElement
{
public:
	DECLARE_SKIN_ELEMENT (EllipseShapeElement, RectShapeElement)

protected:
	Shape* newShape () const override;
};

//************************************************************************************************
// TriangleShapeElement
/** A shape that draws a triangle. */
//************************************************************************************************

class TriangleShapeElement: public ShapeElement
{
public:
	DECLARE_SKIN_ELEMENT (TriangleShapeElement, ShapeElement)

	// Element
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;

protected:
	Shape* newShape () const override;
};

//************************************************************************************************
// ShapeImageElement
/** Defines an image resource using a vector graphics shape. */
//************************************************************************************************

class ShapeImageElement: public ImageElement
{
public:
	DECLARE_SKIN_ELEMENT (ShapeImageElement, ImageElement)

	// ImageElement
	bool loadImage (SkinModel& model) override;

private:
	void applyShapeModification ();
	void applyShapeModificationDeep (ShapeColorMappingElement* mapping, Shape* shape);
};

} // namespace CCL
} // namespace SkinElements

#endif // _ccl_skinshapes_h
