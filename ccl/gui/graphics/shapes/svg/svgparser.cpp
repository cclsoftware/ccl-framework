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
// Filename    : ccl/gui/graphics/shapes/svg/svgparser.cpp
// Description : SVG parser
//
//************************************************************************************************

#include "ccl/base/storage/textparser.h"

#include "ccl/gui/graphics/shapes/svg/svgparser.h"
#include "ccl/gui/graphics/shapes/svg/svgpath.h"

#include "ccl/gui/graphics/shapes/shapes.h"
#include "ccl/gui/graphics/shapes/shapeimage.h"

#include "ccl/public/storage/iurl.h"
#include "ccl/public/storage/filetype.h"
#include "ccl/public/text/istringdict.h"
#include "ccl/public/text/translation.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/math/mathprimitives.h"

#include "ccl/public/systemservices.h"

namespace CCL {

class GraphicsPath;

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace FileTypes
{
	static FileType svg (nullptr, "svg", "image/svg+xml");
}

namespace SVG {

//////////////////////////////////////////////////////////////////////////////////////////////////
/*
in order to be able to distinguish if a shape's style property was explicity defined by itself
or has been inherited, we use our own private flags (in addition to the Shape::style flags)
*/
enum
{
	kPropertyFill			= (1<<(Shape::kLastStyleFlag+1)),
	kPropertyStroke			= (1<<(Shape::kLastStyleFlag+2)),
	kPropertyStrokeWidth	= (1<<(Shape::kLastStyleFlag+3)),
	kPropertyStrokeOpacity	= (1<<(Shape::kLastStyleFlag+4)),
	kPropertyFillOpacity	= (1<<(Shape::kLastStyleFlag+5)),
	kPropertyOpacity		= (1<<(Shape::kLastStyleFlag+6)),
	kPropertyFontSize		= (1<<(Shape::kLastStyleFlag+7)),
	kPropertyFontStyle		= (1<<(Shape::kLastStyleFlag+8)),
	kPropertyFontFamily		= (1<<(Shape::kLastStyleFlag+9)),
	kPropertyFontWeight		= (1<<(Shape::kLastStyleFlag+10)),
	kPropertyTextDecoration	= (1<<(Shape::kLastStyleFlag+11)),
	kPropertyTextAlignH		= (1<<(Shape::kLastStyleFlag+12)),
	kPropertyTextAlignV		= (1<<(Shape::kLastStyleFlag+13)),
	kPropertyFillMode		= (1<<(Shape::kLastStyleFlag+14)),

	kPropertyMask = kPropertyFill|kPropertyStroke|kPropertyStrokeWidth|kPropertyStrokeOpacity|kPropertyFillOpacity|kPropertyFillMode
		|kPropertyOpacity|kPropertyFontSize|kPropertyFontStyle|kPropertyFontFamily|kPropertyFontWeight|kPropertyTextDecoration|kPropertyTextAlignH|kPropertyTextAlignV,
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// common strings
//////////////////////////////////////////////////////////////////////////////////////////////////

static const String strNone = CCLSTR ("none");
static const String strDisplay = CCLSTR ("display");
static const String strInherit = CCLSTR ("inherit");
static const String strNonZero = CCLSTR ("nonzero");
static const String strEvenOdd = CCLSTR ("evenodd");

//////////////////////////////////////////////////////////////////////////////////////////////////

void linkSvgHandler () {} // force linkage of this file

//************************************************************************************************
// SvgImageHandler
//************************************************************************************************

class SvgImageHandler: public ImageHandler
{
public:
	bool canHandleImage (const FileType& type) const override
	{
		return type == FileTypes::svg;
	}

	Image* loadImage (IStream& stream, const FileType& type) const override
	{
		ShapeImage* image = nullptr;
		AutoPtr<Shape> shape (SvgParser::parseShape (stream));
		if(shape)
		{
			shape->shouldAntiAlias (true);
			image = NEW ShapeImage (shape);
		}
		return image;
	}

	int getNumFileTypes () const override
	{
		return 0; // not a public file type!
	}

	const FileType* getFileType (int index) const override
	{
		return index == 0 ? &FileTypes::svg : nullptr;
	}

	bool saveImage (IStream& stream, Image* image, const FileType& type,
					const IAttributeList* encoderOptions = nullptr) const override
	{
		CCL_NOT_IMPL ("SVG save not implemented!\n")
		return false;
	}
};

static SvgImageHandler svgHandler;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("FileType")
	XSTRING (SVGFile, "Scalable Vector Graphics")
END_XSTRINGS

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_INIT_LEVEL (SvgImageHandler, kFrameworkLevelFirst)
{
	Image::registerHandler (&svgHandler);
	return true;
}

CCL_KERNEL_INIT_LEVEL (SvgFileTypes, kFrameworkLevelLast)
{
	FileTypes::init (FileTypes::svg, XSTR (SVGFile));
	System::GetFileTypeRegistry ().registerFileType (FileTypes::svg);
	return true;
}

//************************************************************************************************
// StyleParser (CSS style format)
//************************************************************************************************

class StyleParser: public TextParser
{
public:
	StyleParser (IStream& stream);

	/** Parse a sequence of style class definitions (no delimiter, e.g. in a <Style> tag) */
	bool parseStylesContent (SvgParser& parser);

	/** Parse a style definition with class name: .className { ... } */
	bool parseStyleClass (SvgParser& parser);
	
	/** Parse a sequence of style attributes: name:value; name:value */
	void parseStyle (SVG::Style& style);

	/** Parse one style attribute: name:value */
	static bool parseStyleAttribute (SVG::Style& style, StringRef name, StringRef value);
};

//////////////////////////////////////////////////////////////////////////////////////////////////

static ObjectList tagHandlers;

//************************************************************************************************
// TagHandler
//************************************************************************************************

class TagHandler: public Object 
{
public:
	StringRef getName () const { return tagName; }
	virtual Shape* createShape (SvgParser& parser, const IStringDictionary& attributes) = 0;
	virtual bool wantsCharacterData () const { return false; }
	virtual void onCharacterData (SvgParser& parser, Shape* shape, const uchar* data, int length) {}
	virtual void onTagClose (SvgParser& parser, Shape* shape) {}

protected:
	TagHandler (StringRef tagName) : tagName (tagName) { tagHandlers.add (this); }

private:
	String tagName;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// register a tag handler class
//////////////////////////////////////////////////////////////////////////////////////////////////

#define REGISTER_TAG(Handler) static Handler UNIQUE_IDENT (svg);

//////////////////////////////////////////////////////////////////////////////////////////////////
// helpers for handling attributes in a loop. other attribs are handled by parser
//////////////////////////////////////////////////////////////////////////////////////////////////

#define BeginAttributes for(int i = 0; i < attributes.countEntries (); ++i) \
{	String name (attributes.getKeyAt (i)); \
	MutableCString asciiName (name); \
	String value (attributes.getValueAt (i));

#define handleAttrib(a, code) \
	if(asciiName == a) { code; continue; }

#define EndAttributes \
	parser.parseAttribute (name, value); }

//************************************************************************************************
// Root <svg> tag handler
//************************************************************************************************

class RootHandler: public TagHandler
{
public:
	RootHandler () : TagHandler (CCLSTR ("svg")) {}

	Shape* createShape (SvgParser& parser, const IStringDictionary& attributes) override
	{
		parser.parseAttributes (attributes);
		return nullptr; // root shape exists in advance
	}
	
	void onTagClose (SvgParser& parser, Shape* shape) override
	{
		parser.applyViewPort ();
	}
};
REGISTER_TAG (RootHandler);

//************************************************************************************************
// Style
//************************************************************************************************

class StyleHandler: public TagHandler
{
public:
	StyleHandler () : TagHandler (CCLSTR ("style")) {}

	Shape* createShape (SvgParser& parser, const IStringDictionary& attributes) override
	{
		return nullptr;
	}
	
	bool wantsCharacterData () const override
	{
		return true;
	}

	void onCharacterData (SvgParser& parser, Shape* shape, const uchar* data, int length) override
	{
		MemoryStream memstream ((void*)data, length * sizeof(uchar));
		StyleParser styleParser (memstream);
		styleParser.parseStylesContent (parser);
	}
};
REGISTER_TAG (StyleHandler);

//************************************************************************************************
// Group
//************************************************************************************************

class GroupHandler: public TagHandler
{
public:
	GroupHandler () : TagHandler (CCLSTR("g")) {}

	Shape* createShape (SvgParser& parser, const IStringDictionary& attributes) override
	{
		BeginAttributes
		EndAttributes
		return NEW ComplexShape;
	}
};
REGISTER_TAG (GroupHandler);

//************************************************************************************************
// Use
//************************************************************************************************

class UseHandler: public TagHandler
{
public:
	UseHandler () : TagHandler (CCLSTR("use")) {}

	Shape* createShape (SvgParser& parser, const IStringDictionary& attributes) override
	{
		static const String strHrefKey = CCLSTR ("xlink:href");
		String href = attributes.lookupValue (strHrefKey);
		if(!href.isEmpty () && href.firstChar () == '#') // local URI
			href.remove (0, 1); 

		Shape* original = parser.findShape (href);
		if(!original)
			return nullptr;
		
		Shape* shape = ccl_cast<Shape>(original->clone ());

		Length x = 0, y = 0, w = -1, h = -1;
		BeginAttributes
			handleAttrib ("x",      x  = parser.parseLength (value); )
			handleAttrib ("y",      y  = parser.parseLength (value); )
			handleAttrib ("width",  w  = parser.parseLength (value); )
			handleAttrib ("height", h  = parser.parseLength (value); )
		EndAttributes

		// the cloned shape must inherit properties from the use element and it's ancestors, not from it's original parents.
		// => recursively apply each style property of the use element to the clone (until a shape overides the property)
		applyPropertiesDeep (*shape, parser.getStyle (), kPropertyMask);
		
		shape->setStyle (shape->getStyle () | (parser.getStyle ().getShapeStyle ()));

		// insert a transformation if necessary
		Transform transform;
		bool isTransformed = false;
		if(x != 0 || y != 0)
		{
			isTransformed = true;
			transform.translate (x, y);
		}

		if(w > 0 || h > 0)
		{
			Rect bounds;
			shape->getBounds (bounds);

			int origW = bounds.getWidth ();
			int origH = bounds.getHeight ();
			float sx = origW ? w / origW : 1;
			float sy = origH ? h / origH : 1;

			if(sx != 1 || sy != 1)
			{
				isTransformed = true;
				transform.scale (sx, sy);
			}
		}

		if(isTransformed)
		{
			AutoPtr<Shape> releaser (shape);
			shape = NEW TransformShape (transform, shape);
		}
		return shape;
	}

	// template method for applying a property, specialized below
	template<int propertyFlag>
	void applyProperty (Shape& shape, const SVG::Style& style);

	template<int propertyFlag>
	void checkProperty (Shape& shape, const SVG::Style& style, int& propertiesToApply)
	{
		if(shape.getStyle () & propertyFlag)
			propertiesToApply &= ~ propertyFlag;// don't apply this property further, shape overrides it
		else if((propertiesToApply & propertyFlag) && (style.hasProperty (propertyFlag)))
			applyProperty<propertyFlag> (shape, style);
	}

	void applyPropertiesDeep (Shape& shape, const SVG::Style& style, int propertiesToApply)
	{
		checkProperty<kPropertyFill>(shape, style, propertiesToApply);
		checkProperty<kPropertyStroke>(shape, style, propertiesToApply);

		// recurse if still something to apply
		if(propertiesToApply)
			for(int i = 0; i < shape.countShapes (); ++i)
				applyPropertiesDeep (*shape.getShape (i), style, propertiesToApply);
	}
};
REGISTER_TAG (UseHandler);

template<>
void UseHandler::applyProperty<kPropertyFill> (Shape& shape, const SVG::Style& style)
{
	shape.setFillBrush (style.getFillBrush ());
	shape.isFill (style.isFill ());
}

template<>
void UseHandler::applyProperty<kPropertyStroke> (Shape& shape, const SVG::Style& style)
{
	shape.setStrokePen (style.getStrokePen ());
	shape.isStroke (style.isStroke ());
}

template<>
void UseHandler::applyProperty<kPropertyStrokeWidth> (Shape& shape, const SVG::Style& style)
{
	Pen pen (shape.getStrokePen ());
	pen.setWidth (style.getStrokeWidth ());
	shape.setStrokePen (pen);
}

template<>
void UseHandler::applyProperty<kPropertyStrokeOpacity> (Shape& shape, const SVG::Style& style)
{
	Pen pen (shape.getStrokePen ());
	Color color (pen.getColor ());
	color.setAlphaF (style.getStrokeOpacity ());
	pen.setColor (color);
	shape.setStrokePen (pen);
}

template<>
void UseHandler::applyProperty<kPropertyFillOpacity> (Shape& shape, const SVG::Style& style)
{
	SolidBrush brush (shape.getFillBrush ());
	Color color (brush.getColor ());
	color.setAlphaF (style.getFillOpacity ());
	brush.setColor (color);
	shape.setFillBrush (brush);
}

template<>
void UseHandler::applyProperty<kPropertyOpacity> (Shape& shape, const SVG::Style& style)
{
	applyProperty<kPropertyStrokeOpacity> (shape, style);
	applyProperty<kPropertyFillOpacity> (shape, style);
}

template<>
void UseHandler::applyProperty<kPropertyFontSize> (Shape& shape, const SVG::Style& style)
{
	TextShape* textShape = ccl_cast<TextShape> (&shape);
	if(textShape)
	{
		Font font (textShape->getFont ());
		font.setSize (style.getFontSize ());
		textShape->setFont (font);
	}
}

template<>
void UseHandler::applyProperty<kPropertyFontStyle> (Shape& shape, const SVG::Style& style)
{
	TextShape* textShape = ccl_cast<TextShape> (&shape);
	if(textShape)
	{
		Font font (textShape->getFont ());
		font.isItalic (style.hasFontStyle (Font::kItalic));
		textShape->setFont (font);
	}
}

template<>
void UseHandler::applyProperty<kPropertyFontFamily> (Shape& shape, const SVG::Style& style)
{
	TextShape* textShape = ccl_cast<TextShape> (&shape);
	if(textShape)
	{
		Font font (textShape->getFont ());
		font.setFace (style.getFontFamily ());
		textShape->setFont (font);
	}
}

template<>
void UseHandler::applyProperty<kPropertyFontWeight> (Shape& shape, const SVG::Style& style)
{
	TextShape* textShape = ccl_cast<TextShape> (&shape);
	if(textShape)
	{
		Font font (textShape->getFont ());
		font.isBold (style.hasFontStyle (Font::kBold));
		textShape->setFont (font);
	}
}

template<>
void UseHandler::applyProperty<kPropertyTextDecoration> (Shape& shape, const SVG::Style& style)
{
	TextShape* textShape = ccl_cast<TextShape> (&shape);
	if(textShape)
	{
		Font font (textShape->getFont ());
		font.isUnderline (style.hasFontStyle (Font::kUnderline));
		textShape->setFont (font);
	}
}

//************************************************************************************************
// Defs
//************************************************************************************************

class DefsHandler: public TagHandler
{
public:
	DefsHandler () : TagHandler (CCLSTR("defs")) {}

	Shape* createShape (SvgParser& parser, const IStringDictionary& attributes) override
	{
		parser.parseAttributes (attributes);
		parser.parseAttribute (strDisplay, strNone); // childs not rendered
		return nullptr;
	}
};
REGISTER_TAG (DefsHandler);

//************************************************************************************************
// Symbol
//************************************************************************************************

class SymbolHandler: public TagHandler
{
public:
	SymbolHandler () : TagHandler (CCLSTR("symbol")) {}

	Shape* createShape (SvgParser& parser, const IStringDictionary& attributes) override
	{
		parser.parseAttributes (attributes);
		parser.parseAttribute (strDisplay, strNone); // this is not rendered
		return NEW ComplexShape;
	}
};
REGISTER_TAG (SymbolHandler);

//************************************************************************************************
// Basic shapes
//************************************************************************************************

class RectHandler: public TagHandler
{
public:
	RectHandler () : TagHandler (CCLSTR("rect")) {}

	Shape* createShape (SvgParser& parser, const IStringDictionary& attributes) override
	{
		Length x = 0, y = 0, w = 0, h = 0, rx = 0, ry = 0;
		bool round = false;

		BeginAttributes
			handleAttrib ("x",      x  = parser.parseLength (value); )
			handleAttrib ("y",      y  = parser.parseLength (value); )
			handleAttrib ("width",  w  = parser.parseLength (value); )
			handleAttrib ("height", h  = parser.parseLength (value); )
			handleAttrib ("rx",     rx = parser.parseLength (value); round = true; )
			handleAttrib ("ry",     ry = parser.parseLength (value); round = true; )
		EndAttributes

		RectShapeF* shape = NEW RectShapeF;
		shape->setRect (RectF (makeCoordF (x), makeCoordF (y), makePointF (w, h)));
		if(round)
		{
			if(rx == 0)
				rx = ry;
			else if(ry == 0)
				ry = rx;

			if(rx > w / 2)
				rx = w / 2;
			if(ry > h / 2)
				ry = h / 2;
			shape->setRadiusX (makeCoordF (rx));
			shape->setRadiusY (makeCoordF (ry));
		}
		return shape;
	}
};
REGISTER_TAG (RectHandler);

//////////////////////////////////////////////////////////////////////////////////////////////////

class CircleHandler: public TagHandler
{
public:
	CircleHandler () : TagHandler (CCLSTR("circle")) {}

	Shape* createShape (SvgParser& parser, const IStringDictionary& attributes) override
	{
		Length cx = 0, cy = 0, r = 0;

		BeginAttributes
			handleAttrib ("cx", cx = parser.parseLength (value); )
			handleAttrib ("cy", cy = parser.parseLength (value); )
			handleAttrib ("r",  r  = parser.parseLength (value); )
		EndAttributes

		EllipseShapeF* shape = NEW EllipseShapeF;
		shape->setRect (makeRectF (cx - r , cy - r, cx + r, cy + r));
		return shape;
	}
};
REGISTER_TAG (CircleHandler);

//////////////////////////////////////////////////////////////////////////////////////////////////

class EllipseHandler: public TagHandler
{
public:
	EllipseHandler () : TagHandler (CCLSTR("ellipse")) {}

	Shape* createShape (SvgParser& parser, const IStringDictionary& attributes) override
	{
		Length cx = 0, cy = 0, rx = 0, ry = 0;

		BeginAttributes
			handleAttrib ("cx", cx = parser.parseLength (value); )
			handleAttrib ("cy", cy = parser.parseLength (value); )
			handleAttrib ("rx", rx = parser.parseLength (value); )
			handleAttrib ("ry", ry = parser.parseLength (value); )
		EndAttributes

		EllipseShapeF* shape = NEW EllipseShapeF;
		shape->setRect (makeRectF (cx - rx , cy - ry, cx + rx, cy + ry));
		return shape;
	}
};
REGISTER_TAG (EllipseHandler);

//////////////////////////////////////////////////////////////////////////////////////////////////

class LineHandler: public TagHandler
{
public:
	LineHandler () : TagHandler (CCLSTR("line")) {}

	Shape* createShape (SvgParser& parser, const IStringDictionary& attributes) override
	{
		Length x1 = 0, y1 = 0, x2 = 0, y2 = 0;

		BeginAttributes
			handleAttrib ("x1", x1 = parser.parseLength (value); )
			handleAttrib ("y1", y1 = parser.parseLength (value); )
			handleAttrib ("x2", x2 = parser.parseLength (value); )
			handleAttrib ("y2", y2 = parser.parseLength (value); )
		EndAttributes

		LineShapeF* shape = NEW LineShapeF;
		shape->setStart (makePointF (x1, y1));
		shape->setEnd (makePointF (x2, y2));
		return shape;
	}
};
REGISTER_TAG (LineHandler);

//////////////////////////////////////////////////////////////////////////////////////////////////

class PolylineHandler: public TagHandler
{
public:
	PolylineHandler () : TagHandler (CCLSTR("polyline")) {}

	Shape* createShape (SvgParser& parser, const IStringDictionary& attributes) override
	{
		String points;
		IGraphicsPath::FillMode fillMode = parser.getStyle ().getFillMode ();

		BeginAttributes
			handleAttrib ("points", points = value; )
			handleAttrib ("fill-rule", fillMode = parser.parseFillRule (value); )
		EndAttributes

		AutoPtr<GraphicsPath> path = PathParser::parsePolyLine (points, fillMode);
		return path ? NEW PathShape (path) : nullptr;
	}
};
REGISTER_TAG (PolylineHandler);

//////////////////////////////////////////////////////////////////////////////////////////////////

class PolygonHandler: public TagHandler
{
public:
	PolygonHandler () : TagHandler (CCLSTR("polygon")) {}

	Shape* createShape (SvgParser& parser, const IStringDictionary& attributes) override
	{
		String points;
		IGraphicsPath::FillMode fillMode = parser.getStyle ().getFillMode ();

		BeginAttributes
			handleAttrib ("points", points = value; )
			handleAttrib ("fill-rule", fillMode = parser.parseFillRule (value); )
		EndAttributes

		AutoPtr<GraphicsPath> path = PathParser::parsePolygon (points, fillMode);
		return path ? NEW PathShape (path) : nullptr;
	}
};
REGISTER_TAG (PolygonHandler);

//************************************************************************************************
// Path
//************************************************************************************************

class PathHandler: public TagHandler
{
public:
	PathHandler () : TagHandler (CCLSTR("path")) {}

	Shape* createShape (SvgParser& parser, const IStringDictionary& attributes) override
	{
		String data;
		IGraphicsPath::FillMode fillMode = parser.getStyle ().getFillMode ();

		BeginAttributes
			handleAttrib ("d", data = value; )
			handleAttrib ("fill-rule", fillMode = parser.parseFillRule (value); )
		EndAttributes

		AutoPtr<GraphicsPath> path = PathParser::parsePath (data, fillMode);
		return path ? NEW PathShape (path) : nullptr;
	}
};
REGISTER_TAG (PathHandler);

//////////////////////////////////////////////////////////////////////////////////////////////////

class TextHandler: public TagHandler
{
public:
	TextHandler () : TagHandler (CCLSTR("text")) {}

	Shape* createShape (SvgParser& parser, const IStringDictionary& attributes) override
	{
		Length x = 0, y = 0;
		BeginAttributes
			handleAttrib ("x", x = parser.parseLength (value); )
			handleAttrib ("y", y = parser.parseLength (value); )
		EndAttributes

		TextShapeF* shape = NEW TextShapeF ();
		shape->setFont (parser.getStyle ().getFont ());
		shape->setAlignment (parser.getStyle ().getTextAlignment ());
		shape->setPosition (makePointF (x, y));
		return shape;
	}

	bool wantsCharacterData () const override
	{
		return true;
	}

	void onCharacterData (SvgParser& parser, Shape* shape, const uchar* data, int length) override
	{
		TextShapeF* textShape = ccl_cast<TextShapeF>(shape);
		if(textShape)
		{
			String text (textShape->getText ());
			String newText;
			newText.assign (data, length);
			text.append (newText);
			textShape->setText (text);
		}
	}

	void onTagClose (SvgParser& parser, Shape* shape) override
	{
		TextShapeF* textShape = ccl_cast<TextShapeF>(shape);
		if(textShape)
		{
			String text (textShape->getText ());
			text.trimWhitespace ();
			textShape->setText (text);
		}
	}
};
REGISTER_TAG (TextHandler);

//************************************************************************************************
// SVG::Style
//************************************************************************************************

void Style::applyStyle (const Style& other)
{
	if(other.properties & kPropertyFill)
		setFillColor (other.fillColor);
	if(other.properties & kPropertyStroke)
		setStrokeColor (other.strokeColor);

	if(other.properties & kPropertyStrokeWidth)
		setStrokeWidth (other.strokeWidth);
	if(other.properties & kPropertyStrokeOpacity)
		setStrokeOpacity (other.strokeOpacity);

	if(other.properties & kPropertyFillOpacity)
		setFillOpacity (other.fillOpacity);

	if(other.properties & kPropertyFillMode)
		setFillMode (other.fillMode);

	if(other.properties & kPropertyFontFamily)
		setFontFamily (other.fontFamily);
	if(other.properties & kPropertyFontSize)
		setFontSize (other.fontSize);
	if(other.properties & kPropertyFontStyle)
		setFontStyle (other.fontStyle & kFontStyleMask);
	if(other.properties & kPropertyFontWeight)
		setFontWeight (other.fontStyle & kFontWeightMask);
	if(other.properties & kPropertyTextDecoration)
		setTextDecoration (other.fontStyle & kTextDecorationMask);

	if(other.properties & kPropertyTextAlignH)
		setTextAlignmentH (other.textAlignment.getAlignH ());
	if(other.properties & kPropertyTextAlignV)
		setTextAlignmentV (other.textAlignment.getAlignV ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Style::setStrokeColor (ColorRef value)
{
	strokeColor = value;
	isStroke (strokeColor != 0);
	properties |= kPropertyStroke;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Style::setStrokeWidth (float value)
{
	strokeWidth = value;
	properties |= kPropertyStrokeWidth;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Style::setStrokeOpacity (float value)
{
	strokeOpacity = value;
	properties |= kPropertyStrokeOpacity;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Style::setFillColor (ColorRef value)
{
	fillColor = value;
	isFill (fillColor != 0);
	properties |= kPropertyFill;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Style::setFillOpacity (float value)
{
	fillOpacity = value;
	properties |= kPropertyFillOpacity;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Style::setOpacity (float value)
{
	fillOpacity *= value;
	strokeOpacity *= value;
	properties |= (kPropertyOpacity|kPropertyStrokeOpacity|kPropertyFillOpacity);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Style::setFillMode (IGraphicsPath::FillMode value)
{
	fillMode = value;
	properties |= kPropertyFillMode;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Style::setFontFamily (StringRef value)
{
	fontFamily = value;
	properties |= kPropertyFontFamily;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Style::setFontSize (float value)
{
	fontSize = value;
	properties |= kPropertyFontSize;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Style::setFontStyle (int flags)
{
	setFontStyleInternal<kFontStyleMask, kPropertyFontStyle> (flags);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Style::setFontWeight (int flags)
{
	setFontStyleInternal<kFontWeightMask, kPropertyFontWeight> (flags);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Style::setTextDecoration (int flags)
{
	setFontStyleInternal<kTextDecorationMask, kPropertyTextDecoration> (flags);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Style::setTextAlignmentH (int value)
{
	textAlignment.setAlignH (value);
	properties |= kPropertyTextAlignH;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Style::setTextAlignmentV (int value)
{
	textAlignment.setAlignV (value);
	properties |= kPropertyTextAlignV;
}

} //namespace SVG

//************************************************************************************************
// SvgParser::StyleItem
//************************************************************************************************

struct SvgParser::StyleItem: public Object
{
	String name;
	SVG::Style style;

	StyleItem (StringRef name, const SVG::Style& style)
	: name (name),
	  style (style)
	{}
};

//************************************************************************************************
// SvgParser
//************************************************************************************************

SvgParser::SvgParser ()
: rootShape (nullptr),
  invisibleShapes (NEW ComplexShape),
  transform (nullptr)
{
	state.style.isFill (true); // (black)
	rootShape = state.shape = state.container = NEW ComplexShape;

	styleItems.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SvgParser::~SvgParser ()
{
	safe_release (rootShape);
	safe_release (invisibleShapes);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SvgParser::addStyle (StringRef className, const SVG::Style& style)
{
	styleItems.add (NEW StyleItem (className, style));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SVG::Style* SvgParser::lookupStyle (StringRef className)
{
	StyleItem* item = lookupStyleItem (className);
	return item ? &item->style : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SvgParser::StyleItem* SvgParser::lookupStyleItem (StringRef className)
{
	for(auto item : iterate_as<StyleItem> (styleItems))
		if(item->name == className)
			return item;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SvgParser::StyleItem* SvgParser::getStyleItem (StringRef className)
{
	StyleItem* item = lookupStyleItem (className);
	if(!item)
		styleItems.add (item = NEW StyleItem (className, SVG::Style ()));
	return item;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Shape* SvgParser::parseShape (UrlRef url)
{
	AutoPtr<IStream> stream = System::GetFileSystem ().openStream (url, IStream::kOpenMode);
	return stream ? parseShape (*stream) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Shape* SvgParser::parseShape (IStream& stream)
{
	SvgParser parser;
	if(parser.parse (stream) && parser.getShape ())
	{
		parser.getShape ()->retain ();
		return parser.getShape ();
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Shape* SvgParser::getShape ()
{
	return rootShape;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Shape* SvgParser::findShape (StringRef name)
{
	Shape* found = invisibleShapes->findShape (name);
	return found ? found : rootShape->findShape (name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SvgParser::startElement (StringRef name, const IStringDictionary& attributes)
{
	stateStack.push (state);

	// mask out all property flags, we want to know which explicit properties this shapes has
	state.style.resetProperties ();
	state.viewPort.setEmpty ();

	ForEach (SVG::tagHandlers, SVG::TagHandler, handler)
		if(name == handler->getName ())
		{
			state.tagHandler = handler;
			currentTagId.empty ();

			Shape* shape = handler->createShape (*this, attributes);
			if(shape)
			{
				shape->setName (currentTagId);
				static const String strUse = CCLSTR("use");
				if(name != strUse)
				{
					shape->isStroke (state.style.isStroke ());
					shape->isFill (state.style.isFill ());
					if(state.style.isStroke ())
						shape->setStrokePen (state.style.getStrokePen ());
					if(state.style.isFill ())
						shape->setFillBrush (state.style.getFillBrush ());
				}

				ComplexShape* container = state.doDisplay ? state.container : invisibleShapes;
				state.doDisplay = true;

				if(transform)
				{
					TransformShape* transShape = NEW TransformShape (*transform, shape);
					container->addShape (transShape);
					shape->release ();
					delete transform;
					transform = nullptr;
				}
				else
					container->addShape (shape);

				state.shape = shape;
				ComplexShape* complex = ccl_cast<ComplexShape> (shape);
				if(complex)
					state.container = complex;
			}
			break;
		}
	EndFor
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SvgParser::endElement (StringRef name)
{
	if(state.tagHandler)
	{
		if(currentCharacterData.getPosition () > 0)
		{
			ASSERT (state.tagHandler->wantsCharacterData ())
			state.tagHandler->onCharacterData (*this, state.shape, currentCharacterData.getBuffer ().as<uchar> (), (int)currentCharacterData.getPosition () / sizeof(uchar));

			currentCharacterData.setPosition (0, IStream::kSeekSet);
		}

		state.tagHandler->onTagClose (*this, state.shape);
	}
	state = stateStack.pop ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SvgParser::characterData (const uchar* data, int length, tbool isCDATA)
{
	// can be multiple small chunks: collect data first, to feed handler in one call
	if(state.tagHandler && state.tagHandler->wantsCharacterData ())
		currentCharacterData.write ((void*)data, length * sizeof(uchar));

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SvgParser::applyViewPort ()
{
	if(!state.viewPort.isEmpty () && state.shape)
	{
		ViewPortShape* viewPortShape = NEW ViewPortShape (state.viewPort, state.shape);

		if(state.shape == rootShape)
		{
			rootShape->release ();
			rootShape = viewPortShape;
		}
		state.shape = viewPortShape;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SvgParser::parseAttributes (const IStringDictionary& attributes)
{
	for(int i = 0; i < attributes.countEntries (); ++i)
		parseAttribute (attributes.getKeyAt (i), attributes.getValueAt (i));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SvgParser::parseAttribute (StringRef _name, StringRef value)
{
	if(SVG::StyleParser::parseStyleAttribute (state.style, _name, value))
		return true;

	MutableCString name (_name);

	if(name == "id")
	{
		currentTagId = value;
	}
	else if(name == "class")
	{
		SVG::Style* style = lookupStyle (value);
		if(style)
			state.style.applyStyle (*style);
	}
	else if(name == "style")
	{
		StringChars chars (value);
		MemoryStream memstream ((void*)(const uchar*)chars, (value.length () + 1) * sizeof(uchar));

		SVG::StyleParser styleParser (memstream);
		styleParser.parseStyle (state.style);
	}
	else if(name == "transform") // todo: should also be handled as part of a style
	{
		parseTransform (value);
	}
	else if(name == "display") // todo: should also be handled as part of a style
	{
		// possible values: none | inline | inherit | ... 
		if(value == SVG::strNone)
			state.doDisplay = false;
		else if(value != SVG::strInherit)
			state.doDisplay = true;
	}
	else if(name == "viewBox")
	{
		parseViewPort (value, state.viewPort);
	}
	else
		return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SvgParser::parseTransform (StringRef value)
{
	StringChars chars (value);
	MemoryStream memstream ((void*)(const uchar*)chars, (value.length () + 1) * sizeof(uchar));
	TextParser transParser (memstream);
	transParser.addWhitespace (','); // hmm, actually only allowed inside the ()

	String identifier;
	transParser.skipWhite ();
	while(!transParser.readIdentifier (identifier).isEmpty ())
	{
		transParser.skipWhite ();
		if(transParser.read ('('))
		{
			transParser.skipWhite ();

			SVG::Length args[6];
			int numArgs = 0;
			while((numArgs < 6) && transParser.readFloat (args[numArgs]))
			{
				transParser.skipWhite ();
				++numArgs;
			}

			if(transParser.read (')'))
			{
				transParser.skipWhite ();

				if(!transform)
					transform = NEW Transform;
				
				MutableCString operation (identifier);

				if(operation == "translate" && numArgs > 0)
				{
					if(numArgs == 1)
						args[1] = 0;
					transform->translate (args[0], args[1]);
				}
				else if(operation == "scale" && numArgs > 0)
				{
					if(numArgs == 1)
						args[1] = args[0];
					transform->scale (args[0], args[1]);
				}
				else if(operation == "rotate" && numArgs > 0)
				{
					if (numArgs == 3)
					{
						SVG::Length cx = args[1];
						SVG::Length cy = args[2];
						transform->translate (cx, cy);
						transform->rotate (Math::degreesToRad (args[0]));
						transform->translate (-cx, -cy);
					}
					else
						transform->rotate (Math::degreesToRad (args[0]));
				}
				else if(operation == "skewX" && numArgs == 1)
					transform->skewX (Math::degreesToRad (args[0]));
				else if(operation == "skewY" && numArgs == 1)
					transform->skewY (Math::degreesToRad (args[0]));
				else if(operation == "matrix" && numArgs == 6)
				{
					Transform t(args[0], args[1], args[2], args[3], args[4], args[5]);
					transform->multiply (t);			
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SvgParser::parseViewPort (StringRef string, Rect& viewPort)
{
	viewPort.setEmpty ();
	int i = 0;
	ForEachStringToken (string, CCLSTR (" ,\t\r\n"), token)
		switch(i++)
		{
		case 0: viewPort.left = Coord (ccl_round<0> (token.scanFloat ()));
			break;
		case 1: viewPort.top = Coord (ccl_round<0> (token.scanFloat ()));
			break;
		case 2: viewPort.setWidth (Coord (ccl_round<0> (token.scanFloat ())));
			break;
		case 3: viewPort.setHeight (Coord (ccl_round<0> (token.scanFloat ())));
			break;
		default:
			break;
		}
	EndFor
	return !viewPort.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SVG::Length SvgParser::parseLength (StringRef string)
{
	float value = 0;
	parseLength (string, value);
	return value;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SvgParser::parseLength (StringRef string, SVG::Length& value)
{
	double d = 0.;
	if(string.getFloatValue (d)) // (note: this includes scientific notation with exponent) todo: handle units px,mm,%,...
	{
		value = (SVG::Length)d;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Color SvgParser::parseColor (StringRef value)
{
	Color c;
	Colors::fromString (c, value);
	return c;
	// todo:
	// format: rgb(100%, 0%, 0%)
	// clip values out of range
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphicsPath::FillMode SvgParser::parseFillRule (StringRef value)
{
	// possible values: nonzero | evenodd
	if(value == SVG::strEvenOdd)
		return IGraphicsPath::kFillEvenOdd;

	return IGraphicsPath::kFillNonZero; // default
}

//************************************************************************************************
// SVG::StyleParser 
//************************************************************************************************

SVG::StyleParser::StyleParser (IStream& stream)
: TextParser (stream)
{
	addIdentifierChar ('-');
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SVG::StyleParser::parseStylesContent (SvgParser& parser)
{
	// parse a sequence of style class definitions
	bool result = false;
	while(parseStyleClass (parser))
		result = true;
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SVG::StyleParser::parseStyleClass (SvgParser& parser)
{
	ObjectList targetStyles;

	// multiple class names, separated by ','
	do
	{
		skipWhite ();
		read ('.');
		
		String className;
		if(!readIdentifier (className).isEmpty ())
		{
			CCL_PRINTF ("parseStyleClass: %s\n", MutableCString (className).str ())
			SvgParser::StyleItem* styleItem = parser.getStyleItem (className);
			ASSERT (styleItem)
			targetStyles.add (styleItem);
		}
		skipWhite ();
	} while(read (','));

	// style definitions in braces
	skipWhite ();
	if(read ('{'))
	{
		Style parsedStyle;
		parseStyle (parsedStyle);

		// apply parsed style to all specified classes
		for(auto styleItem : iterate_as<SvgParser::StyleItem> (targetStyles))
			styleItem->style.applyStyle (parsedStyle);

		return read ('}');
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SVG::StyleParser::parseStyle (SVG::Style& style)
{
	bool readMore = true;
	while(readMore)
	{
		readMore = false;
		skipWhite ();
		String name;
		if(!readIdentifier (name).isEmpty ())
		{
			skipWhite ();
			if(read (':'))
			{
				skipWhite ();

				String value;
				readMore = readUntil (";}", value);
				if(readMore && peek () == ';')
					advance ();

				parseStyleAttribute (style, name, value);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SVG::StyleParser::parseStyleAttribute (SVG::Style& style, StringRef name, StringRef value)
{
	CCL_PRINTF ("  parseStyleAttribute: %s : %s\n", MutableCString (name).str (), MutableCString (value).str ())

	if(name == "stroke")
	{
		if(value == SVG::strNone)
			style.setStrokeColor (Color (0, 0, 0, 0));
		else
			style.setStrokeColor (SvgParser::parseColor (value));
	}
	else if(name == "fill")
	{
		if(value == SVG::strNone)
			style.setFillColor (Color (0, 0, 0, 0));
		else
			style.setFillColor (SvgParser::parseColor (value));
	}
	else if(name == "stroke-width")
	{
		SVG::Length width = 1.f;
		if(SvgParser::parseLength (value, width))
			style.setStrokeWidth (width);
	}
	else if(name == "stroke-opacity")
	{
		double alpha = 1.;
		if(value.getFloatValue (alpha))
			style.setStrokeOpacity (float(alpha));
	}
	else if(name == "fill-opacity")
	{
		double alpha = 1.;
		if(value.getFloatValue (alpha))
			style.setFillOpacity (float(alpha));
	}
	else if(name == "opacity")
	{
		double alpha = 1.;
		if(value.getFloatValue (alpha))
			style.setOpacity (float(alpha));
	}
	else if(name == "font-size")
	{
		SVG::Length size;
		if(SvgParser::parseLength (value, size))
			style.setFontSize (size);
	}
	else if(name == "font-style")
	{
		static const String strItalic = CCLSTR ("italic");

		// possible values: normal, italic, oblique (not supported)
		style.setFontStyle (value == strItalic ? Font::kItalic : 0);
	}
	else if(name == "font-family")
	{
		if(!value.isEmpty ())
			style.setFontFamily (value);
	}
	else if(name == "font-weight")
	{
		static const String strBold = CCLSTR ("bold");
		static const String strBolder = CCLSTR ("bolder");
		
		bool isBold = false;
		if(value == SVG::strInherit)
			return true;
		else if(value == strBold || value == strBolder)
			isBold = true;
		else
		{
			double weight;
			if(value.getFloatValue (weight) && weight >= 400)
				isBold = true;
		}
		style.setFontWeight (isBold ? Font::kBold : 0);
	}
	else if(name == "text-anchor")
	{
		static const String strStart = CCLSTR ("start");
		static const String strMiddle = CCLSTR ("middle");
		static const String strEnd = CCLSTR ("end");

		// possible values: start | middle | end | inherit
		if(value == strStart)
			style.setTextAlignmentH (Alignment::kLeft);
		else if(value == strMiddle)
			style.setTextAlignmentH (Alignment::kHCenter); 
		else if(value == strEnd)
			style.setTextAlignmentH (Alignment::kRight); 
	}
	else if(name == "alignment-baseline")
	{
		static const String strBeforeEdge = CCLSTR ("before-edge");
		static const String strTextBeforeEdge = CCLSTR ("text-before-edge");
		static const String strCentral = CCLSTR ("central");
		static const String strAfterEdge = CCLSTR ("after-edge");
		static const String strTextAfterEdge = CCLSTR ("text-after-edge");

		if(value == strBeforeEdge || value == strTextBeforeEdge)
			style.setTextAlignmentV (Alignment::kTop); 
		else if(value == strCentral)
			style.setTextAlignmentV (Alignment::kVCenter); 
		else if(value == strAfterEdge || value == strTextAfterEdge)
			style.setTextAlignmentV (Alignment::kBottom); 
	}
	else if(name == "text-decoration")
	{
		static const String strUnderline = CCLSTR ("underline");

		// possible values: none | [ underline || overline || line-through || blink ] | inherit
		if(value != SVG::strInherit)
			style.setTextDecoration (value.contains (strUnderline) ? Font::kUnderline : 0);
	}
	else if(name == "fill-rule")
	{
		style.setFillMode (SvgParser::parseFillRule (value));
	}
	else
		return false;

	return true;
}

} // namespace CCL
