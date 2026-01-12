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
// Filename    : ccl/gui/graphics/shapes/svg/svgparser.h
// Description : SVG parser
//
//************************************************************************************************

#ifndef _ccl_svgparser_h
#define _ccl_svgparser_h

#include "ccl/gui/graphics/shapes/svg/svgtypes.h"
#include "ccl/gui/graphics/shapes/shapes.h"

#include "ccl/base/collections/objectlist.h"
#include "ccl/public/collections/stack.h"
#include "ccl/public/text/xmlcontentparser.h"
#include "ccl/public/base/memorystream.h"

namespace CCL {
namespace SVG {

//************************************************************************************************
// SVG::Style
//************************************************************************************************

class Style
{
public:
	Style ();

	void applyStyle (const Style& other);

	PROPERTY_FLAG (shapeStyle, Shape::kStroke, isStroke)
	PROPERTY_FLAG (shapeStyle, Shape::kFill, isFill)

	Pen getStrokePen () const;
	SolidBrush getFillBrush () const;
	Font getFont () const;

	ColorRef getStrokeColor () const;
	float getStrokeWidth () const;
	float getStrokeOpacity () const;
	ColorRef getFillColor () const;
	float getFillOpacity () const;
	IGraphicsPath::FillMode getFillMode () const;
	float getFontSize () const;
	StringRef getFontFamily () const;
	Alignment getTextAlignment () const;
	bool hasFontStyle (int flags) const;
	bool hasProperty (int flags) const;
	int getProperties () const;
	int getShapeStyle () const;

	void setStrokeColor (ColorRef value);
	void setStrokeWidth (float value);
	void setStrokeOpacity (float value);
	void setFillColor (ColorRef value);
	void setFillOpacity (float value);
	void setOpacity (float value);
	void setFillMode (IGraphicsPath::FillMode value);
	void setFontFamily (StringRef value);
	void setFontSize (float value);
	void setFontStyle (int flags);
	void setFontWeight (int flags);
	void setTextDecoration (int flags);
	void setTextAlignmentH (int value);
	void setTextAlignmentV (int value);
	void resetProperties ();

private:
	int properties;
	int shapeStyle; // kStroke, kFill;

	Color strokeColor;
	float strokeWidth;
	float strokeOpacity;

	Color fillColor;
	float fillOpacity;
	IGraphicsPath::FillMode fillMode;

	String fontFamily;
	float fontSize;
	int fontStyle; ///< as in Font class, combines svg font-style, wont-weight, text-decoration
	Alignment textAlignment;

	enum FontStyleMasks
	{
		kFontStyleMask = Font::kItalic,
		kFontWeightMask = Font::kBold,
		kTextDecorationMask = Font::kUnderline
	};

	template<int mask, int propertyFlag> void setFontStyleInternal (int flags);
};

class TagHandler;
class StyleParser;

} // namespace SVG

//************************************************************************************************
// SvgParser
//************************************************************************************************

class SvgParser: public XmlContentParser
{
public:
	SvgParser ();
	~SvgParser ();

	static Shape* parseShape (UrlRef url);
	static Shape* parseShape (IStream& stream);

	Shape* getShape ();
	Shape* findShape (StringRef name);

	// XmlContentParser
	tresult CCL_API startElement (StringRef name, const IStringDictionary& attributes) override;
	tresult CCL_API endElement (StringRef name) override;
	tresult CCL_API characterData (const uchar* data, int length, tbool isCDATA) override;

	static SVG::Length parseLength (StringRef value);
	static bool parseLength (StringRef string, SVG::Length& value);
	static bool parseViewPort (StringRef string, Rect& viewPort);
	static Color parseColor (StringRef value);
	static IGraphicsPath::FillMode parseFillRule (StringRef value);
	const SVG::Style& getStyle () const { return state.style; }

	bool parseAttribute (StringRef name, StringRef value);
	void parseAttributes (const IStringDictionary& attributes);

	void addStyle (StringRef className, const SVG::Style& style);
	SVG::Style* lookupStyle (StringRef className);

	bool applyViewPort ();

private:
	struct StyleItem;
	struct State
	{
		SVG::Style style;
		ComplexShape* container;
		Shape* shape;
		SVG::TagHandler* tagHandler;
		Rect viewPort;
		bool doDisplay;

		State (int dummy = 0)
		: container (nullptr), shape (nullptr), tagHandler (nullptr), doDisplay (true)
		{}
	};

	State state;
	Stack<State> stateStack;
	String currentTagId;
	ObjectList styleItems;

	Transform* transform;
	Shape* rootShape;
	ComplexShape* invisibleShapes;
	MemoryStream currentCharacterData;

	void parseTransform (StringRef value);
	StyleItem* lookupStyleItem (StringRef className);
	StyleItem* getStyleItem (StringRef className); // lookup existing or create new

	friend class SVG::StyleParser;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline SVG::Style::Style ()
: shapeStyle (0),
  properties (0),
  strokeOpacity (1.f),
  fillOpacity (1.f),
  fillMode (IGraphicsPath::kFillNonZero),
  strokeWidth (1.f),
  fontFamily (Font::getDefaultFont ().getFace ()),
  fontSize (Font::kDefaultSize),
  fontStyle (Font::kNormal),
  textAlignment (Alignment::kLeft|Alignment::kBottom)
{}

inline Pen SVG::Style::getStrokePen () const { return Pen (Color (strokeColor).setAlphaF (strokeOpacity), strokeWidth); }
inline SolidBrush SVG::Style::getFillBrush () const { return SolidBrush (Color (fillColor).setAlphaF (fillOpacity)); }
inline Font SVG::Style::getFont () const { return Font (fontFamily, fontSize, fontStyle); }
inline ColorRef SVG::Style::getStrokeColor () const { return strokeColor; }
inline float SVG::Style::getStrokeWidth () const { return strokeWidth; };
inline float SVG::Style::getStrokeOpacity () const { return strokeOpacity; }
inline ColorRef SVG::Style::getFillColor () const { return fillColor; }
inline float SVG::Style::getFillOpacity () const { return fillOpacity; }
inline IGraphicsPath::FillMode SVG::Style::getFillMode () const { return fillMode; }
inline float SVG::Style::getFontSize () const { return fontSize; }
inline StringRef SVG::Style::getFontFamily () const { return fontFamily; }
inline Alignment SVG::Style::getTextAlignment () const { return textAlignment; }
inline bool SVG::Style::hasFontStyle (int flag) const { return fontStyle & flag; }
inline bool SVG::Style::hasProperty (int flag) const { return properties & flag; }
inline int SVG::Style::getProperties () const { return properties; }
inline int SVG::Style::getShapeStyle () const { return shapeStyle; }
inline void SVG::Style::resetProperties () { properties = 0; }
template<int mask, int propertyFlag>
inline void SVG::Style::setFontStyleInternal (int flags)
{
	fontStyle = (fontStyle & ~mask) | flags;
	properties |= propertyFlag;
}

} // namespace CCL

#endif // _ccl_svgparser_h
