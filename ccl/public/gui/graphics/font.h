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
// Filename    : ccl/public/gui/graphics/font.h
// Description : Font definition
//
//************************************************************************************************

#ifndef _ccl_font_h
#define _ccl_font_h

#include "ccl/public/gui/graphics/color.h"
#include "ccl/public/gui/graphics/rect.h"
#include "ccl/public/gui/graphics/textformat.h"

#include "ccl/public/base/cclmacros.h"
#include "ccl/public/text/cclstring.h"

#include "ccl/meta/generated/cpp/graphics-constants-generated.h"

namespace CCL {

class Font;
interface IFontTable;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Font reference
//////////////////////////////////////////////////////////////////////////////////////////////////

/** Font reference type. 
	\ingroup gui_graphics */
typedef const Font& FontRef;

//************************************************************************************************
// PlainFont
/** The font class below is binary equivalent to this C structure. 
	\ingroup gui_graphics */
//************************************************************************************************

struct PlainFont
{
	String face;
	float size = 0.f;
	int style = 0;
	String styleName;
	int mode = 0;
	float spacing = 0.f;
	float lineSpacing = 1.f;
};

//************************************************************************************************
// Font
/** Font definition. 
	\ingroup gui_graphics */
//************************************************************************************************

class Font: protected PlainFont
{
public:
	/** Font styles. */
	enum Styles
	{
		kNormal		= kFontStyleNormal,
		kBold		= kFontStyleBold,
		kItalic		= kFontStyleItalic,
		kUnderline	= kFontStyleUnderline,
		kStrikeout	= kFontStyleStrikeout
	};

	/** Font smoothing mode. */
	enum SmoothingMode
	{
		kDefault = 0,			///< default
		kNone,					///< no anti-aliasing
		kAntiAlias				///< anti-aliasing
	};

	/** Font defaults. */
	enum Defaults 
	{ 
		kDefaultSize = 11 		///< default font size
	};

	Font ()
	{
		assign (getDefaultFont ());
	}

	Font (StringRef _face, float _size = kDefaultSize, int _style = kNormal, int _mode = kDefault)
	{
		face = _face;
		size = _size;
		style = _style;
		mode = _mode;
	}

	Font (FontRef other)
	{
		assign (other);
	}

	PROPERTY_STRING_METHODS (face, Face)
	PROPERTY_BY_VALUE (float, size, Size)	
	PROPERTY_BY_VALUE (int, style, Style)

	PROPERTY_FLAG (style, kBold, isBold)
	PROPERTY_FLAG (style, kItalic, isItalic)
	PROPERTY_FLAG (style, kUnderline, isUnderline)
	PROPERTY_FLAG (style, kStrikeout, isStrikeout)
	PROPERTY_STRING_METHODS (styleName, StyleName)     	///< style name overwrites style flags

	PROPERTY_BY_VALUE (int, mode, Mode)
	PROPERTY_BY_VALUE (float, spacing, Spacing)			///< additional character spacing in points
	PROPERTY_BY_VALUE (float, lineSpacing, LineSpacing)	///< line spacing factor

	Font& assign (FontRef font)
	{
		face = font.getFace ();
		size = font.getSize ();
		style = font.getStyle ();
		styleName = font.getStyleName ();
		mode = font.getMode ();
		spacing = font.getSpacing ();
		lineSpacing = font.getLineSpacing ();
		return *this;
	}

	bool isEqual (FontRef font) const
	{
		return	font.getFace () == face &&
				font.getSize () == size &&
				(styleName.isEmpty () && font.getStyleName ().isEmpty () ? font.getStyle () == style : font.getStyleName () == styleName) &&
				font.getMode () == mode &&
				font.getSpacing () == spacing &&
				font.getLineSpacing () == lineSpacing;
	}

	Font& zoom (float zoomFactor)
	{
		size *= zoomFactor;
		return *this;
	}

	/** Get default font. */
	static const Font& getDefaultFont ();
	
	/** Get width of Unicode string. */
	static int getStringWidth (StringRef text, FontRef font);
	static CoordF getStringWidthF (StringRef text, FontRef font);

	/** Get extent of Unicode string. */
	static void measureString (Rect& size, StringRef text, FontRef font, int flags = 0);
	static void measureString (RectF& size, StringRef text, FontRef font, int flags = 0);
	
	/** Get extent of a Unicode string's glyphs without typographical spacing. */
	static void measureStringImage (RectF& size, StringRef text, FontRef font, bool shiftToBaseline);

	/** Get extent of a multiline text area. */
	static void measureText (Rect& size, Coord lineWidth, StringRef text, FontRef font, TextFormatRef format = TextFormat (Alignment::kLeftTop, TextFormat::kWordBreak));
	static void measureText (RectF& size, CoordF lineWidth, StringRef text, FontRef font, TextFormatRef format = TextFormat (Alignment::kLeftTop, TextFormat::kWordBreak));

	/** Trim modes for collapseString(). */
	enum TrimMode
	{
		kTrimModeKeepEnd = 0,	///< put ".." to the right but leave last four characters
		kTrimModeLeft,			///< put ".." to the left
		kTrimModeMiddle,		///< put ".." in the middle
		kTrimModeRight,			///< put ".." to the right
		kTrimModeNumeric,		///< like kTrimModeKeepEnd, but removes spaces first (e\.g\. between a number and a unit)

		kTrimModeDefault = kTrimModeKeepEnd
	};

	/** Collapse string to fit into given width (replacing characters with ".."). */
	static void collapseString (String& string, Coord maxWidth, FontRef font, int trimMode = kTrimModeDefault, bool exact = false);
	static void collapseString (String& string, CoordF maxWidth, FontRef font, int trimMode = kTrimModeDefault, bool exact = false);

	/** Font collection flags. */
	enum CollectFontFlags
	{
		kCollectSymbolicFonts = 1<<0,  ///< collect fonts that contain symbols instead of characters (like 'Webdings' and such)
		kCollectAppFonts = 1<<1,       ///< collect fonts that have been registered by the running app
		kCollectSimulatedFonts = 1<<2, ///< collect font styles which are simulated by the text system (like bold for a font family without own defined bold style)
		kCollectAllFonts = kCollectSymbolicFonts|kCollectAppFonts|kCollectSimulatedFonts
	};

	/** Create list of fonts. */
	static IFontTable* collectFonts (int flags = kCollectAppFonts);
};

//************************************************************************************************
// IFont
//************************************************************************************************

interface IFont: IUnknown
{	
	virtual void CCL_API assign (FontRef font) = 0;

	virtual void CCL_API copyTo (Font& font) const = 0;

	DECLARE_IID (IFont)
};

DEFINE_IID (IFont, 0xaccb12a, 0xef6d, 0x484b, 0xa5, 0xda, 0x35, 0x7a, 0xf, 0x55, 0x42, 0xb)

//************************************************************************************************
// IFontTable
//************************************************************************************************

interface IFontTable: IUnknown
{	
	virtual int CCL_API countFonts () = 0; 

	virtual tresult CCL_API getFontName (String& name, int index) = 0;

	virtual int CCL_API countFontStyles (int fontIndex) = 0;

	virtual tresult CCL_API getFontStyleName (String& name, int fontIndex, int styleIndex) = 0;

	virtual tresult CCL_API getExampleText (String& text, int fontIndex, int styleIndex) = 0;

	DECLARE_IID (IFontTable)
};

DEFINE_IID (IFontTable, 0xb5076e3, 0x9ab5, 0x4292, 0xa4, 0x13, 0x70, 0x47, 0xc2, 0xae, 0x5, 0x94)


} // namespace CCL

#endif // _ccl_font_h
