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
// Filename    : ccl/gui/graphics/markupsupport.cpp
// Description : CCL Markup Support
//
//************************************************************************************************

#include "ccl/gui/graphics/markupsupport.h"

#include "ccl/gui/graphics/textlayoutbuilder.h"
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

TextNodeType MarkupParser::getType (StringRef tag)
{
	TextNodeType type = Text::kNodeTypeUnknown;
	
	if(tag == MarkupTags::kBold)
		type = Text::kBold;
	else if(tag == MarkupTags::kItalic)
		type = Text::kItalic;
	else if(tag == MarkupTags::kUnderline)
		type = Text::kUnderline;
	else if(tag == MarkupTags::kStyleColor || tag == MarkupTags::kColor)
		type = Text::kFontColor;
	else if(tag == MarkupTags::kStyleSize || tag == MarkupTags::kSize)
		type = Text::kFontSize;
	else if(tag == MarkupTags::kSuperscript)
		type = Text::kSuperscript;
	else if(tag == MarkupTags::kSubscript)
		type = Text::kSubscript;

	return type;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MarkupParser::MarkupParser (StringRef string)
{
	formatInstructions.objectCleanup (true);
	parse (string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MarkupParser::parse (StringRef string)
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

		// read until next occurence of markup tag
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
				if(auto* currentEntry = static_cast<FormattedText::FormatRange*> (openedInstructions.pop ()))
				{
					bool matched = false;
					if(currentEntry->getType () == getType (tagName))
						matched = true;
					else if((currentEntry->getType () == Text::kFontSize || currentEntry->getType () == Text::kFontColor) && tagName.startsWith ("style"))
						matched = true;

					if(matched)
						currentEntry->setLength (plainText.length () - currentEntry->getStart ());
					else
						continue;
				}
			}	
			else
			{
				TextNodeType type = getType (tagName);
				if(type == Text::kNodeTypeUnknown)
					continue;

				auto* formatEntry = NEW FormattedText::FormatRange (type);
				formatInstructions.add (formatEntry);
				openedInstructions.push (formatEntry);
								
				switch(formatEntry->getType ())
				{
				case Text::kBold :
					formatEntry->setArgument (true);
					break;

				case Text::kItalic :
					formatEntry->setArgument (true);
					break;

				case Text::kUnderline :
					formatEntry->setArgument (true);
					break;

				case Text::kSubscript :
				case Text::kSuperscript :
					formatEntry->setArgument (true);
					break;

				case Text::kFontSize :
					{
						double size = 0.;
						if(tagValue.getFloatValue (size))
							formatEntry->setArgument (size);
					}
					break;

				case Text::kFontColor :
					{
						Color color;
						if(Colors::fromString (color, tagValue))
							formatEntry->setArgumentColor (color);
					}
					break;

				default:
					break;
				}
				formatEntry->setStart (plainText.length ());
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

void MarkupParser::applyFormatting (IFormattedTextHandler& handler) const
{
	ArrayForEach (formatInstructions, FormattedText::FormatRange, entry)
		handler.applyFormat (*entry);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int MarkupParser::getPlainTextPosition (int markupPosition) const
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

int MarkupParser::getMarkupPosition (int plainTextPosition, bool positionBeforeMarkup) const
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
	MarkupParser parser (text);
	AutoPtr<ITextLayout> textLayout = NativeGraphicsEngine::instance ().createTextLayout ();
	TextFormat textFormat (alignment);
	textLayout->construct (parser.getPlainText (), rect.getWidth (), rect.getHeight (), font, ITextLayout::kSingleLine, textFormat);
	TextLayoutBuilder builder (*textLayout);
	parser.applyFormatting (builder);

	return graphics.drawTextLayout (rect.getLeftTop (), textLayout, brush);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MarkupPainter::measureMarkupString (Rect& size, StringRef text, FontRef font)
{
	RectF sizeF;
	tresult result = measureMarkupString (sizeF, text, font);
	if(result == kResultOk)
		size = rectFToEnclosingInt (sizeF);

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MarkupPainter::measureMarkupString (RectF& size, StringRef text, FontRef font)
{
	MarkupParser parser (text);
	AutoPtr<ITextLayout> textLayout = NativeGraphicsEngine::instance ().createTextLayout ();
	TextFormat textFormat;
	textLayout->construct (parser.getPlainText (), size.getWidth (), size.getHeight (), font, ITextLayout::kSingleLine, textFormat);
	TextLayoutBuilder builder (*textLayout);
	parser.applyFormatting (builder);

	return textLayout->getBounds (size);
}

//************************************************************************************************
// MarkupBuilder
//************************************************************************************************

DEFINE_CLASS (MarkupBuilder, Object)
DEFINE_CLASS_UID (MarkupBuilder, 0x71caabe0, 0x969f, 0x46d5, 0x87, 0xc4, 0xb7, 0xc6, 0x6f, 0xa3, 0x27, 0x4d) // ClassID::MarkupBuilder

//////////////////////////////////////////////////////////////////////////////////////////////////

String MarkupBuilder::unpack (const TextFragment& fragment) const
{
	String result (fragment.content);
	if(fragment.encode)
		MarkupTags::escapePlainText (result);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MarkupBuilder::printFragment (String& result, const TextFragment& fragment)
{
	result.empty ();
	tresult tr = kResultOk;

	auto printTag = [] (StringRef tag, StringRef paramValue, StringRef content) 
	{
		String markup;
		markup << "[" << tag;
		if(!paramValue.isEmpty ())
			markup << "=" << paramValue;
		markup << "]";
		markup << content;
		markup << "[/" << tag << "]";
		return markup;
	};

	switch(fragment.nodeType)
	{
	case Text::kPlainText :
		result = unpack (fragment);
		break;

	case Text::kBold :
		result = printTag (MarkupTags::kBold, nullptr, unpack (fragment));
		break;

	case Text::kItalic :
		result = printTag (MarkupTags::kItalic, nullptr, unpack (fragment));
		break;

	case Text::kUnderline :
		result = printTag (MarkupTags::kUnderline, nullptr, unpack (fragment));
		break;

	case Text::kSuperscript :
		result = printTag (MarkupTags::kSuperscript, nullptr, unpack (fragment));
		break;

	case Text::kSubscript :
		result = printTag (MarkupTags::kSubscript, nullptr, unpack (fragment));
		break;

	case Text::kFontColor :
		result = printTag (MarkupTags::kColor, VariantString (fragment.argument), unpack (fragment));
		break;

	case Text::kFontSize :
		result = printTag (MarkupTags::kSize, VariantString (fragment.argument), unpack (fragment));
		break;

	default :
		CCL_DEBUGGER ("Unsupported text node type!\n")
		tr = kResultInvalidArgument;
		break;
	}
	return tr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITextTable* CCL_API MarkupBuilder::createTable ()
{
	ASSERT (0) // not suppored!
	return nullptr;
}
