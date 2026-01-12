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
// Filename    : ccl/gui/graphics/textlayoutbuilder.cpp
// Description : Text Markup Parser
//
//************************************************************************************************

#include "ccl/gui/graphics/textlayoutbuilder.h"

#include "ccl/public/gui/graphics/markuptags.h"

#include "ccl/gui/theme/visualstyle.h"
#include "ccl/gui/graphics/nativegraphics.h"

namespace CCL {

//************************************************************************************************
// MarkupTextParser
//************************************************************************************************

class MarkupTextParser
{
public:
	MarkupTextParser (StringRef string);

	int getIndex () const { return index; }
	bool done () const { return index >= length; }
	void readToken (StringRef separatorList, String& token, uchar& separator);
	
protected:
	StringRef text;
	int index;
	int length;
	uchar current;
	
	uchar next ();
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// MarkupTextParser
//************************************************************************************************

MarkupTextParser::MarkupTextParser (StringRef string)
: text (string),
  index (0),
  length (string.length ()),
  current (0)
{}	

//////////////////////////////////////////////////////////////////////////////////////////////////

void MarkupTextParser::readToken (StringRef separatorList, String& token, uchar& separator)
{
	token.empty ();
	separator = 0;
	while(!done ())
	{
		uchar c = next ();
		bool isSeparator = false;
		for(int i = 0; i < separatorList.length (); i++)
			if(separatorList[i] == c)
			{
				isSeparator = true;
				break;
			}
		
		if(isSeparator)
		{
			separator = c;
			break;
		}
		else
			token.append (&c, 1);
	}	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uchar MarkupTextParser::next ()
{
	if(done ())
		current = 0;
	else
		current = text[index++];
	return current;
}

//************************************************************************************************
// MarkupParser
//************************************************************************************************

IMarkupContentHandler::FormatType MarkupParser::getType (StringRef tag)
{
	IMarkupContentHandler::FormatType type = IMarkupContentHandler::kUnknown;
	
	if(tag == MarkupTags::kBold)
		type = IMarkupContentHandler::kBold;
	else if(tag == MarkupTags::kItalic)
		type = IMarkupContentHandler::kItalic;
	else if(tag == MarkupTags::kUnderline)
		type = IMarkupContentHandler::kUnderline;
	else if(tag == MarkupTags::kStyleColor || tag == MarkupTags::kColor)
		type = IMarkupContentHandler::kColor;
	else if(tag == MarkupTags::kStyleSize || tag == MarkupTags::kSize)
		type = IMarkupContentHandler::kSize;
	else if(tag == MarkupTags::kSuperscript)
		type = IMarkupContentHandler::kSuperscript;
	else if(tag == MarkupTags::kSubscript)
		type = IMarkupContentHandler::kSubscript;

	return type;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MarkupParser::MarkupParser (StringRef string, const IVisualStyle& style)
: style (style)
{
	formatInstructions.objectCleanup (true);
	parse (string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API MarkupParser::parse (StringRef string)
{
	ranges.removeAll ();
	plainText.empty ();
	openedInstructions.removeAll ();
	formatInstructions.removeAll ();

	MarkupTextParser parser (string);
	String token;
	String tagName;
	String tagValue;
	uchar separator = 0;
	
	while(!parser.done ())
	{
		int position = parser.getIndex ();

		// read until next occurence of BB Code
		parser.readToken ("[", token, separator);

		// flush plain text
		if(!token.isEmpty ())
		{
			ranges.add ({ position, token.length ()});
			plainText.append (token);
		}
		
		// read tag start [tag] or [tag=value]
		parser.readToken ("]=", token, separator);
		tagName = token;
		if(separator == '=')
		{
			parser.readToken ("]", token, separator);
			if(token.firstChar () == '"' && token.lastChar () == '"')
				tagValue = token.subString (1, token.length () - 2);
			else
				tagValue = token;
			tagValue.trimWhitespace ();
		}
		else
			tagValue.empty ();

		if(!tagName.isEmpty ())
		{
			if(tagName[0] == '/')
			{
				tagName.remove (0, 1);
				if(auto* currentEntry = static_cast<FormatObject*> (openedInstructions.pop ()))
				{
					bool matched = false;
					if(currentEntry->type == getType (tagName))
						matched = true;
					else if((currentEntry->type == IMarkupContentHandler::kSize || currentEntry->type == IMarkupContentHandler::kColor) && tagName.startsWith ("style"))
						matched = true;

					if(matched)
						currentEntry->length = plainText.length () - currentEntry->start;
					else
						continue;
				}
			}	
			else
			{
				auto* formatEntry = NEW FormatObject (getType (tagName));
				formatInstructions.add (formatEntry);
				openedInstructions.push (formatEntry);
				
				bool useStyle = false;
				if(tagValue.startsWith ("$"))
				{
					tagValue.remove (0, 1);
					useStyle = true;
				}
				
				switch(formatEntry->type)
				{
				case IMarkupContentHandler::kBold :
					if(useStyle)
						formatEntry->paramValue = (style.getFont (MutableCString (tagValue).str ()).getStyle () & Font::kBold);
					else
						formatEntry->paramValue = true;
					break;

				case IMarkupContentHandler::kItalic :
					if(useStyle)
						formatEntry->paramValue = (style.getFont (MutableCString (tagValue).str ()).getStyle () & Font::kItalic);
					else
						formatEntry->paramValue = true;
					break;

				case IMarkupContentHandler::kUnderline :
					if(useStyle)
						formatEntry->paramValue = (style.getFont (MutableCString (tagValue).str ()).getStyle () & Font::kUnderline);
					else
						formatEntry->paramValue = true;
					break;

				case IMarkupContentHandler::kSubscript :
				case IMarkupContentHandler::kSuperscript :
					formatEntry->paramValue = true;
					break;

				case IMarkupContentHandler::kSize :
					if(useStyle)
						formatEntry->paramValue = style.getFont (MutableCString (tagValue).str ()).getSize ();
					else
					{
						double size;
						if(tagValue.getFloatValue (size))
							formatEntry->paramValue = size;
					}
					break;

				case IMarkupContentHandler::kColor :
					if(useStyle)
						formatEntry->paramValue = (int64)style.getColor (MutableCString (tagValue).str (), style.getColor (StyleID::kTextColor));
					else 
					{
						Color color;
						if(Colors::fromString (color, tagValue))
							formatEntry->paramValue = (int64)color;
					}
					break;

				default:
					break;
				}
				formatEntry->start = plainText.length ();
			}
		}
		else if(separator == ']')
		{
			ranges.add ({position, 1});
			plainText.append ("[");
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API MarkupParser::escapePlainText (String& text) const
{
	text.replace ("[", "[]");
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API MarkupParser::applyFormatting (IMarkupContentHandler& handler, ITextLayout::Range range, int textOffset) const
{
	ArrayForEach (formatInstructions, FormatObject, entry)
		if(range.length < 0 || (entry->start < range.start + range.length && entry->start + entry->length > range.start))
		{
			IMarkupContentHandler::FormatEntry e (*entry);
			e.start -= textOffset;
			if(e.start < 0)
			{
				e.length += e.start;
				e.start = 0;
			}
			if(e.length <= 0)
				continue;
			handler.applyFormat (*entry);
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API MarkupParser::getPlainTextPosition (int markupPosition) const
{
	int plainTextPosition = 0;
	for(const TextRange& range : ranges)
	{
		if(range.markupPosition + range.length > markupPosition)
		{
			plainTextPosition += markupPosition - range.markupPosition;
			return plainTextPosition;
		}
		plainTextPosition += range.length;
	}
	return plainTextPosition;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API MarkupParser::getMarkupPosition (int plainTextPosition, tbool positionBeforeMarkup) const
{
	int totalLength = 0;
	for(const TextRange& range : ranges)
	{
		if(range.length > plainTextPosition || (positionBeforeMarkup && range.length == plainTextPosition))
			return range.markupPosition + plainTextPosition;

		plainTextPosition -= range.length;
		totalLength = range.markupPosition + range.length;
	}

	return totalLength;
}

//************************************************************************************************
// TextLayoutBuilder
//************************************************************************************************

DEFINE_CLASS (TextLayoutBuilder, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

TextLayoutBuilder::TextLayoutBuilder (ITextLayout* _textLayout)
: textLayout (_textLayout)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API TextLayoutBuilder::applyFormat (const FormatEntry& entry)
{
	if(!textLayout)
		return kResultFailed;

	ITextLayout::Range range (entry.start, entry.length);

	switch(entry.type)
	{
	case kBold:
		textLayout->setFontStyle (range, Font::kBold, entry.paramValue.asBool ());
		break;

	case kItalic:
		textLayout->setFontStyle (range, Font::kItalic, entry.paramValue.asBool ());
		break;

	case kUnderline:
		textLayout->setFontStyle (range, Font::kUnderline, entry.paramValue.asBool ());
		break;

	case kSize:
		textLayout->setFontSize (range, entry.paramValue.asFloat ());
		break;

	case kColor:
		{
			int64 colorCode = entry.paramValue.asInt ();
			textLayout->setTextColor (range, Color::fromInt ((uint32)colorCode));
		}
		break;

	case kSuperscript:
		textLayout->setSuperscript (range);
		break;

	case kSubscript:
		textLayout->setSubscript (range);
		break;

	default:
		return kResultNotImplemented;
	}

	return kResultOk;
}

//************************************************************************************************
// MarkupPainter
//************************************************************************************************

DEFINE_CLASS (MarkupPainter, Object)
DEFINE_CLASS_UID (MarkupPainter, 0x9253c60e, 0xfd30, 0x4706, 0x91, 0x03, 0x1d, 0xce, 0x48, 0xfd, 0xd7, 0x49) // ClassID::MarkupPainter

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MarkupPainter::drawMarkupString (IGraphics& graphics, RectRef rect, StringRef text, FontRef font, BrushRef brush, AlignmentRef alignment)
{
	return drawMarkupString (graphics, rectIntToF (rect), text, font, brush, alignment);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MarkupPainter::drawMarkupString (IGraphics& graphics, RectFRef rect, StringRef text, FontRef font, BrushRef brush, AlignmentRef alignment)
{
	VisualStyle vs;
	MarkupParser parser (text, vs);
	AutoPtr<ITextLayout> textLayout = NativeGraphicsEngine::instance ().createTextLayout ();
	TextFormat textFormat (vs.getTextFormat ());
	textFormat.setAlignment (alignment);
	textLayout->construct (parser.getPlainText (), rect.getWidth (), rect.getHeight (), font, ITextLayout::kSingleLine, textFormat);
	TextLayoutBuilder builder (textLayout);
	parser.applyFormatting (builder);

	return graphics.drawTextLayout (rect.getLeftTop (), textLayout, brush);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MarkupPainter::measureMarkupString (Rect& size, StringRef text, FontRef font, int flags)
{
	RectF sizeF;
	tresult result = measureMarkupString (sizeF, text, font, flags);
	if(result == kResultOk)
		size = rectFToInt (sizeF);

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MarkupPainter::measureMarkupString (RectF& size, StringRef text, FontRef font, int flags)
{
	VisualStyle vs;
	MarkupParser parser (text, vs);
	AutoPtr<ITextLayout> textLayout = NativeGraphicsEngine::instance ().createTextLayout ();
	TextFormat textFormat (vs.getTextFormat ());
	textLayout->construct (parser.getPlainText (), size.getWidth (), size.getHeight (), font, ITextLayout::kSingleLine, textFormat);
	TextLayoutBuilder builder (textLayout);
	parser.applyFormatting (builder);

	return textLayout->getBounds (size, flags);
}
