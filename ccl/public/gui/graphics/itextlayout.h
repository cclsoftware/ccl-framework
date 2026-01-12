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
// Filename    : ccl/public/gui/graphics/itextlayout.h
// Description : Text Layout Interface
//
//************************************************************************************************

#ifndef _ccl_itextlayout_h
#define _ccl_itextlayout_h

#include "ccl/public/gui/graphics/types.h"

#include "ccl/meta/generated/cpp/graphics-constants-generated.h"

namespace CCL {

interface IMutableRegion;

//************************************************************************************************
// ITextLayout
/** Text layout interface. 
	\ingroup gui_graphics */
//************************************************************************************************

interface ITextLayout: IUnknown
{
	/** Range of text positions. */
	struct Range
	{
		int start;
		int length;

		Range (int start, int length)
		: start (start),
		  length (length)
		{}
	};

	/** Line mode. */
	DEFINE_ENUM (LineMode)
	{
		kSingleLine = kTextLayoutLineModeSingleLine,
		kMultiLine = kTextLayoutLineModeMultiLine
	};

	/** Measure flags. */
	DEFINE_ENUM (MeasureFlags)
	{
		kNoMargin = 1<<0 ///< do not add layout margin
	};

	/** Initialize text layout. */
	virtual tresult CCL_API construct (StringRef text, Coord width, Coord height, FontRef font,
									   LineMode lineMode = kMultiLine, TextFormatRef format = TextFormat ()) = 0;

	/** Initialize text layout (float coordinates). */
	virtual tresult CCL_API construct (StringRef text, CoordF width, CoordF height, FontRef font,
									   LineMode lineMode = kMultiLine, TextFormatRef format = TextFormat ()) = 0;

	/** Get plain unformatted text (that was used to construct the layout). */
	virtual StringRef CCL_API getText () const = 0;

	/** Resize the text layout */
	virtual tresult CCL_API resize (Coord width, Coord height) = 0;

	/** Resize the text layout (float coordinates). */
	virtual tresult CCL_API resize (CoordF width, CoordF height) = 0;

	/** Set font style for given text range (@see Font::Styles enumeration). */
	virtual tresult CCL_API setFontStyle (const Range& range, int style, tbool state) = 0;

	/** Set font size for given text range. */
	virtual tresult CCL_API setFontSize (const Range& range, float size) = 0;

	/** Set character spacing for given text range. */
	virtual tresult CCL_API setSpacing (const Range& range, float spacing) = 0;

	/** Set line spacing for given text range. */
	virtual tresult CCL_API setLineSpacing (const Range& range, float lineSpacing) = 0;

	/** Set color for given text range. */
	virtual tresult CCL_API setTextColor (const Range& range, Color color) = 0;

	/** Get bounding rectangle of formatted text. */
	virtual tresult CCL_API getBounds (Rect& bounds, int flags = 0) const = 0;

	/** Get bounding rectangle of formatted text. */
	virtual tresult CCL_API getBounds (RectF& bounds, int flags = 0) const = 0;

	/** Get tightly enclosing rectangle of the text's glyphs. */
	virtual tresult CCL_API getImageBounds (RectF& bounds) const = 0;

	/** Get the offset of the text's baseline (in addition to the text alignment). */
	virtual tresult CCL_API getBaselineOffset (PointF& offset) const = 0;

	/** Get the text position and coordinates of the cluster at the given position. */
	virtual tresult CCL_API hitTest (int& textIndex, PointF& position) const = 0;

	/** Get bounds of the character at textIndex. The bounds height is the line height. */
	virtual tresult CCL_API getCharacterBounds (RectF& bounds, int textIndex) const = 0;

	/** Get a set of rectangles which fully enclose the text in the given range. */
	virtual tresult CCL_API getTextBounds (IMutableRegion& bounds, const Range& range) const = 0;

	/** Get the text range of the line at the given text position, possibly ending with a newline character. */
	virtual tresult CCL_API getLineRange (Range& range, int textIndex) const = 0;

	/** Get the text range of the word at the given text position. If textIndex is between words,
		the range refers to the characters inbetween. The range does not include newlines. */
	virtual tresult CCL_API getWordRange (Range& range, int textIndex) const = 0;

	/** Get the text range of the line at the given text position not considering layout line breaks but only
		explicit linebreaks by newline characters. The range does not include the newline character. */
	virtual tresult CCL_API getExplicitLineRange (Range& range, int textIndex) const = 0;

	/** Set baseline up (positive offset) or down (negative offset) to shift text vertically. */
	virtual tresult CCL_API setBaselineOffset (const Range& range, float offset) = 0;

	/** Set superscript for text range. */
	virtual tresult CCL_API setSuperscript (const Range& range) = 0;

	/** Set subscript for text range. */
	virtual tresult CCL_API setSubscript (const Range& range) = 0;

	DECLARE_IID (ITextLayout)
};

DEFINE_IID (ITextLayout, 0x86432219, 0x65b4, 0x44cf, 0x87, 0x16, 0x1e, 0xaf, 0x39, 0xe, 0xc0, 0x2a)

} // namespace CCL

#endif // _ccl_itextlayout_h
