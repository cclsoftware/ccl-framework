//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/gui/graphics/textlayoutbuilder.cpp
// Description : Text Layout Builder
//
//************************************************************************************************

#include "ccl/gui/graphics/textlayoutbuilder.h"

#include "ccl/gui/theme/visualstyle.h"

using namespace CCL;

//************************************************************************************************
// TextLayoutBuilder
//************************************************************************************************

TextLayoutBuilder::TextLayoutBuilder (ITextLayout& _textLayout, const IVisualStyle* style)
: textLayout (_textLayout),
  style (style)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextLayoutBuilder::applyTextStyle (const IVisualStyle* style, StringID styleName, const ITextLayout::Range& range)
{
	if(style)
	{
		StyleConditionContext conditionContext;
		conditionContext.setCondition (styleName, true);

		Font textFont;
		style->getFont (textFont, StyleID::kTextFont, &conditionContext);

		FontRef baseFont (textLayout.getFont ());

		if(textFont.getFace () != baseFont.getFace ())
			textLayout.setFontFace (range, textFont.getFace ());

		if(textFont.getSize () != baseFont.getSize ())
			textLayout.setFontSize (range, textFont.getSize ());

		if(textFont.getSpacing () != baseFont.getSpacing ())
			textLayout.setSpacing (range, textFont.getSpacing ());

		if(textFont.getLineSpacing () != baseFont.getLineSpacing ())
			textLayout.setLineSpacing (range, textFont.getLineSpacing ());

		if(textFont.getStyle () != baseFont.getStyle ())
		{
			for(int style : {Font::kBold, Font::kItalic, Font::kUnderline, Font::kStrikeout})
			{
				bool hasStyle = get_flag (textFont.getStyle (), style);
				if(hasStyle != get_flag (baseFont.getStyle (), style))
					textLayout.setFontStyle (range, style, hasStyle);
			}
		}

		// always apply text color, we don't know the color/brush that will be used for drawing
		Color textColor;
		if(style->getColor (textColor, StyleID::kTextColor, &conditionContext))
			textLayout.setTextColor (range, textColor);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextLayoutBuilder::applyFormat (const FormattedText::FormatRange& entry)
{
	ITextLayout::Range range (entry.getStart (), entry.getLength ());

	switch(entry.getType ())
	{
	case Text::kHeading :
		{
			int level = ccl_bound<int> (entry.getArgument ().asInt (), Text::kH1, Text::kMaxHeadingLevel);
			MutableCString styleName ("H");
			styleName.appendInteger (level);
			applyTextStyle (style, styleName, range);
		}
		break;

	case Text::kStyleSpan :
		{
			MutableCString styleName (entry.getArgument ());
			applyTextStyle (style, styleName, range);
		}
		break;

	case Text::kLink :
		applyTextStyle (style, TextStyles::kLink, range);
		break;

	case Text::kBold :
		textLayout.setFontStyle (range, Font::kBold, entry.getArgument ().asBool ());
		break;

	case Text::kItalic :
		textLayout.setFontStyle (range, Font::kItalic, entry.getArgument ().asBool ());
		break;

	case Text::kUnderline :
		textLayout.setFontStyle (range, Font::kUnderline, entry.getArgument ().asBool ());
		break;

	case Text::kFontSize :
		textLayout.setFontSize (range, entry.getArgument ().asFloat ());
		break;

	case Text::kFontColor :
		textLayout.setTextColor (range, entry.getArgumentColor ());
		break;

	case Text::kSuperscript :
		textLayout.setSuperscript (range);
		break;

	case Text::kSubscript :
		textLayout.setSubscript (range);
		break;

	default:
		ASSERT (0) // not implemented
		break;
	}
}
