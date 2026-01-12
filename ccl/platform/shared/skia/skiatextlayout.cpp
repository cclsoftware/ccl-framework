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
// Filename    : ccl/platform/shared/skia/skiatextlayout.cpp
// Description : Skia Text Layout
//
//************************************************************************************************

#define DEBUG_LOG 0
#define DEBUG_TEXT_BOUNDS 0
#define DEBUG_CHARACTER_BOUNDS 0

#include "ccl/platform/shared/skia/skiafontmanager.h"
#include "ccl/platform/shared/skia/skiatextlayout.h"
#include "ccl/platform/shared/skia/skiadevice.h"
#include "ccl/platform/shared/skia/skiaengine.h"

#include "ccl/public/gui/graphics/brush.h"
#include "ccl/public/collections/vector.h"

#include "ccl/base/singleton.h"

#include "core/text/coreutfcodec.h"

#include <limits.h>

using namespace CCL;

//************************************************************************************************
// SkiaFontCache
//************************************************************************************************

DEFINE_SINGLETON (SkiaFontCache)

//////////////////////////////////////////////////////////////////////////////////////////////////

SkiaFontCache::SkiaFontCache ()
: fontManager (SkiaFontManagerFactory::createFontManager ()),
  fontCollection (sk_make_sp<skia::textlayout::FontCollection> ()),
  entries (kMaxChacheEntries)
{
	fontCollection->setDefaultFontManager (fontManager);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkiaFontCache::~SkiaFontCache ()
{
	removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

sk_sp<skia::textlayout::FontCollection> SkiaFontCache::getFontCollection () const
{
	return fontCollection;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int SkiaFontCache::fromSkFontStyle (SkFontStyle style)
{
	int fontStyle = Font::kNormal;
	if(style.slant () >= SkFontStyle::kItalic_Slant)
		fontStyle |= Font::kItalic;
	if(style.weight () >= SkFontStyle::kBold_Weight)
		fontStyle |= Font::kBold;
	return fontStyle;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkFontStyle SkiaFontCache::toSkFontStyle (int style)
{
	bool isBold = (style & Font::kBold) != 0;
	bool isItalic = (style & Font::kItalic) != 0;
	if(isBold && !isItalic)
		return SkFontStyle::Bold ();
	else if(!isBold && isItalic)
		return SkFontStyle::Italic ();
	else if(isBold && isItalic)
		return SkFontStyle::BoldItalic ();
	return SkFontStyle::Normal ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkiaFontCache::FontCacheRecord* SkiaFontCache::add (FontRef font, SkFont& skFont)
{
	FontCacheRecord e;
	e.font = font;
	e.skFont = skFont;
	entries.add (e);
	return &entries.last ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkiaFontCache::removeAll ()
{
	entries.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkiaFontCache::FontCacheRecord* SkiaFontCache::createEntry (FontRef font)
{
	FontCacheRecord* entry = lookup (font);
	if(entry == nullptr)
	{
		// check for max. cache size
		if(entries.count () >= kMaxChacheEntries)
		{
			entries.removeAll ();
			entries.resize (kMaxChacheEntries);
		}

		auto createStyleSet = [&] (const MutableCString& name, int fontStyle, MutableCString& fontStyleName) -> sk_sp<SkFontStyleSet>
		{
			MutableCString fullName (name);
			if(!fontStyleName.isEmpty ())
			{
				fullName.append (" ").append (fontStyleName);
				sk_sp<SkFontStyleSet> styleSet (fontManager->matchFamily (fullName.str ()));
				if(styleSet->count () > 0)
				{
					fontStyleName = "";
					return styleSet;
				}
			}
			
			sk_sp<SkFontStyleSet> styleSet (fontManager->matchFamily (name.str ()));
			if(styleSet->count () > 0)
				return styleSet;

			CCL::ListIterator<StyledFont> iter (styledFontList);
			while(!iter.done ())
			{
				StyledFont& styledFont = iter.next ();
				if(styledFont.fullName == name.str () && styledFont.fontStyle == fontStyle)
				{
					fontStyleName = styledFont.styleName;
					MutableCString familyName (styledFont.familyName, Text::kUTF8);
					styleSet = sk_sp<SkFontStyleSet> (fontManager->matchFamily (familyName.str ()));
					break;
				}
			}

			return styleSet;
		};

		String fullName (font.getFace ());
		MutableCString fontStyleName (font.getStyleName (), Text::kUTF8);
		int style = getUsedStyle (font);
		sk_sp<SkFontStyleSet> styleSet (createStyleSet (fullName, style, fontStyleName));

		sk_sp<SkTypeface> typeFace;
		if(fontStyleName.isEmpty ())
		{
			SkFontStyle skStyle = toSkFontStyle (font.getStyle ());
			typeFace = sk_sp<SkTypeface> (styleSet->matchStyle (skStyle));
		}
		else
		{
			SkString skFonstStyleName (fontStyleName.str ());
			for(int i = 0; i < styleSet->count (); i++)
			{
				SkString matchStyleName;
				styleSet->getStyle (i, nullptr, &matchStyleName);
				if(skFonstStyleName.equals (matchStyleName))
				{
					typeFace = sk_sp<SkTypeface> (styleSet->createTypeface (i));
					break;
				}
			}
		}

		// If there is no typeface which matches the requested style, use the requested family with normal style.
		// Weight and slant are simulated by SkShaper.
		if(!typeFace)
			typeFace = sk_sp<SkTypeface> (styleSet->matchStyle (SkFontStyle ()));

		// If we can't even find a typeface with normal style, use the default font instead.
		ASSERT (typeFace)
		if(!typeFace)
			typeFace = sk_sp<SkTypeface> (fontManager->matchFamilyStyle (nullptr, SkFontStyle::Normal ()));

		SkFont skFont (typeFace, font.getSize ());
		skFont.setEdging (SkFont::Edging::kSubpixelAntiAlias);
		skFont.setSubpixel (true);
		entry = add (font, skFont);
	}

	return entry;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkiaFontCache::FontCacheRecord* SkiaFontCache::lookup (FontRef font) const
{
	for(int i = 0; i < entries.count (); i++)
	{
		const FontCacheRecord& e = entries[i];
		if(e.font.getFace () == font.getFace () &&
		   e.font.getSize () == font.getSize () &&
		   ((e.font.getStyleName ().isEmpty () && font.getStyleName ().isEmpty ())
				? getUsedStyle (e.font) == getUsedStyle (font) // ignore underline, etc.
				: e.font.getStyleName () == font.getStyleName ()))
		{
			return const_cast<FontCacheRecord*> (&e);
		}
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkFont* SkiaFontCache::createFont (FontRef font)
{
	SkFont* skFont = nullptr;
	if(FontCacheRecord* entry = createEntry (font))
		skFont = &(entry->skFont);

	return skFont;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkiaFontCache::addStyledFont (StringRef familyName, int fontStyle, StringRef fullName, StringRef styleName)
{
	StyledFont record (familyName, fontStyle, fullName, styleName);
	styledFontList.append (record);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkiaFontCache::addUserFont (StringRef familyName)
{
	if(!userFontList.contains (familyName))
		userFontList.append (familyName);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkiaFontCache::isUserFont (StringRef familyName) const
{
	return userFontList.contains (familyName);
}

//************************************************************************************************
// SkiaTextLayout
//************************************************************************************************

static const CoordF kPaddingLeft = 2.f;
static const CoordF kPaddingRight = 2.f;
static const CoordF kPaddingTop = 2.f;
static const CoordF kPaddingBottom = 2.f;

#if 0 && DEBUG
const char SkiaTextLayout::kTabReplacementCharacter = '_';
#else
const char SkiaTextLayout::kTabReplacementCharacter = ' ';
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (SkiaTextLayout, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

SkiaTextLayout::SkiaTextLayout ()
: needUpdate (false),
  textBoundsChanged (false),
  imageBoundsChanged (false),
  characterBoundsChanged (false),
  restrictWidth (false),
  defaultColor (Colors::kBlack),
  spaceWidth (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaTextLayout::construct (StringRef text, Coord width, Coord height, FontRef font, LineMode lineMode, TextFormatRef format)
{
	return construct (text, (CoordF)width, (CoordF)height, font, lineMode, format);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaTextLayout::construct (StringRef _text, CoordF width, CoordF height, FontRef font, LineMode _lineMode, TextFormatRef format)
{
	styles.empty ();
	tabPositions.empty ();

	alignment = format.getAlignment ();
	lineMode = _lineMode;
	restrictWidth = (lineMode == kMultiLine) && format.isWordBreak ();

	boundingRect.setWidth (width);
	boundingRect.setHeight (height);

	text = MutableCString (_text, Text::kUTF8);
	originalText = _text;

	paragraphStyle.setHeight (height - kPaddingTop - kPaddingBottom);
	paragraphStyle.setTextHeightBehavior (skia::textlayout::TextHeightBehavior::kAll);
	if(lineMode == kMultiLine)
	{
		switch(alignment.getAlignH ())
		{
			case Alignment::kHCenter :
				paragraphStyle.setTextAlign (skia::textlayout::TextAlign::kCenter);
				restrictWidth = true;
				break;
			case Alignment::kRight :
				paragraphStyle.setTextAlign (skia::textlayout::TextAlign::kRight);
				restrictWidth = true;
				break;
			default : // left aligned
				paragraphStyle.setTextAlign (skia::textlayout::TextAlign::kLeft);
		}
	}
	else
		paragraphStyle.setMaxLines (1);

	SkFont* skFont = SkiaFontCache::instance ().createFont (font);
	SkTypeface* typeface = skFont ? skFont->getTypeface () : nullptr;
	if(typeface)
	{
		typeface->getFamilyName (&familyName);
		SkFontStyle style = typeface->fontStyle ();
		if(!typeface->isItalic () && (font.getStyle () & Font::kItalic) != 0)
			style = SkFontStyle (style.weight (), style.width (), SkFontStyle::kItalic_Slant);
		if(!typeface->isBold () && (font.getStyle () & Font::kBold) != 0)
			style = SkFontStyle (SkFontStyle::kBold_Weight, style.width (), style.slant ());
		textStyle.setFontStyle (style);
	}
	else
	{
		familyName.set (MutableCString (font.getFace (), Text::kUTF8).str ());
		textStyle.setFontStyle (SkiaFontCache::toSkFontStyle (font.getStyle ()));
	}
	textStyle.setFontFamilies ({ familyName });
	textStyle.setFontSize (font.getSize ());
	textStyle.setColor (SK_ColorTRANSPARENT);
	textStyle.setHalfLeading (true);
	textStyle.setLetterSpacing (font.getSpacing ());
	if(font.getLineSpacing () != 1.f)
	{
		textStyle.setHeightOverride (true);
		textStyle.setHeight (font.getLineSpacing ());
	}

	int decoration = 0;
	if(font.getStyle () & Font::kStrikeout)
		decoration |= skia::textlayout::kLineThrough;
	if(font.getStyle () & Font::kUnderline)
		decoration |= skia::textlayout::kUnderline;
	textStyle.setDecoration (static_cast<skia::textlayout::TextDecoration> (decoration));

	spaceWidth = skFont->measureText (" ", 1, SkTextEncoding::kUTF8);
	tabStyle.fWidth = (kTabSize - 1) * spaceWidth;
	for(int i = 0; i < text.length (); i++)
		if(text[i] == '\t')
			tabPositions.add (i);

	needUpdate = true;
	textBoundsChanged = true;
	imageBoundsChanged = true;
	characterBoundsChanged = true;

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaTextLayout::resize (Coord width, Coord height)
{
	return resize (CoordF (width), CoordF (height));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaTextLayout::resize (CoordF width, CoordF height)
{
	boundingRect.setWidth (width);
	boundingRect.setHeight (height);

	textBoundsChanged = true;
	imageBoundsChanged = true;
	characterBoundsChanged = true;

	SkScalar textWidth (SK_ScalarInfinity);
	if(restrictWidth)
		textWidth = boundingRect.getWidth () - kPaddingLeft - kPaddingRight;

	if(paragraph)
		paragraph->layout (textWidth);

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkiaTextLayout::updateParagraph ()
{
	std::unique_ptr<skia::textlayout::ParagraphBuilder> paragraphBuilder = skia::textlayout::ParagraphBuilder::make (paragraphStyle, SkiaFontCache::instance ().getFontCollection ());
	if(paragraphBuilder == nullptr)
		return;

	skia::textlayout::TextStyle style (textStyle);
	setTextColor (style, defaultColor);
	paragraphBuilder->pushStyle (style);

	int processed = 0;
	int currentTabIndex = 0;

	auto insertText = [&] (int end)
	{
		if(end > processed)
		{
			// SkParagraph does not resolve \t characters. Use placeholders instead.
			while(currentTabIndex < tabPositions.count () && tabPositions[currentTabIndex] < end)
			{
				paragraphBuilder->addText (text.str () + processed, tabPositions[currentTabIndex] - processed);
				processed = tabPositions[currentTabIndex] + 1;
				paragraphBuilder->addText (&kTabReplacementCharacter, 1);
				paragraphBuilder->addPlaceholder (tabStyle);
				currentTabIndex++;
			}
			if(end - processed > 0)
				paragraphBuilder->addText (text.str () + processed, end - processed);
			processed = end;
		}
	};

	for(const TextStyle& textStyle : styles)
	{
		insertText (textStyle.position);

		skia::textlayout::TextStyle style (textStyle.style);
		if(style.getColor () == SK_ColorTRANSPARENT)
			setTextColor (style, defaultColor);
		paragraphBuilder->pushStyle (style);
	}
	insertText (text.length ());

	SkScalar textWidth (SK_ScalarInfinity);
	if(restrictWidth)
		textWidth = boundingRect.getWidth () - kPaddingLeft - kPaddingRight;

	paragraph = paragraphBuilder->Build ();
	paragraph->layout (textWidth);

	needUpdate = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkiaTextLayout::updateTextBounds ()
{
	if(utf8Positions.isEmpty ())
		updateUtf8Positions ();

	textRect.setReallyEmpty ();

	const std::vector<skia::textlayout::TextBox>& bounds = paragraph->getRectsForRange (0, utf8Positions[originalText.length ()], skia::textlayout::RectHeightStyle::kTight, skia::textlayout::RectWidthStyle::kTight);
	CoordF lineOffset = 0.f;
	CoordF previousTop = 0.f;
	for(const skia::textlayout::TextBox& bound : bounds)
	{
		RectF boundRect;
		textRect.join (SkiaDevice::fromSkRect (boundRect, bound.rect));
		if(bound.rect.isEmpty ())
			continue;

		lineOffset = bound.rect.top () - previousTop;
		previousTop = bound.rect.top ();
	}

	if(originalText.endsWith ("\n") && !bounds.empty ())
		textRect.bottom += lineOffset;

	if(textRect.left > textRect.right)
		textRect.setEmpty ();

	if(textRect.isEmpty ())
	{
		textRect.top = 0;
		textRect.bottom = paragraph->getHeight ();
	}

	textBoundsChanged = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkiaTextLayout::updateImageBounds ()
{
	SkPaint paint;
	paint.setStyle (SkPaint::kStroke_Style);
	paint.setStrokeWidth (0);

	SkRect imageBounds (SkRect::MakeEmpty ());
	paragraph->visit ([&] (int lineNumber, const skia::textlayout::Paragraph::VisitorInfo* info)
	{
		if(info == nullptr)
			return;

		Vector<SkRect> glyphBounds;
		glyphBounds.setCount (info->count);
		info->font.getBounds (info->glyphs, info->count, glyphBounds, &paint);
		for(int i = 0; i < info->count; i++)
		{
			imageBounds.join (glyphBounds[i].makeOffset (info->positions[i] + info->origin));
		}
	});
	SkiaDevice::fromSkRect (imageRect, imageBounds);

	imageBoundsChanged = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkiaTextLayout::updateCharacterBounds ()
{
	if(utf8Positions.isEmpty ())
		updateUtf8Positions ();

	characterBounds.removeAll ();
	hitTestBounds.removeAll ();

	std::vector<skia::textlayout::LineMetrics> lineMetrics;
	paragraph->getLineMetrics (lineMetrics);
	
	StringChars characters (originalText);

	int textIndex = 0;
	int lastLineNumber = 0;
	uchar currentCharacter = characters[0];
	int textLength = originalText.length ();

	CoordF lineHeight = 0.f;
	const std::vector<skia::textlayout::TextBox>& bounds = paragraph->getRectsForRange (0, utf8Positions[textLength], skia::textlayout::RectHeightStyle::kTight, skia::textlayout::RectWidthStyle::kTight);
	for(const skia::textlayout::TextBox& bound : bounds)
		lineHeight = ccl_max (lineHeight, bound.rect.height ());

	auto adjustLineHeight = [&] (RectF& rect)
	{
		if(lineHeight > 0.f && rect.getHeight () > lineHeight)
		{
			rect.top += (rect.getHeight () - lineHeight) / 2.f;
			rect.setHeight (lineHeight);
		}
	};

	auto addBounds = [&] (RectF rect, int lineNumber)
	{
		adjustLineHeight (rect);

		// if there is a preceding character in the same line, extend the preceding characters rect to remove gaps before the current characters rect
		if(!characterBounds.isEmpty () && characterBounds.last ().bottom > rect.top && characterBounds.last ().right < rect.left)
		{
			characterBounds.last ().right = rect.left;
			hitTestBounds.last ().right = rect.left;
		}

		// Skia places newline characters at the start of the next line. This is ok for hit testing. For character bounds (e.g. caret position), we want newlines to be placed at the end of the line.
		if(currentCharacter == '\n')
		{
			RectF newlineRect (rect);
			newlineRect.top = lineMetrics[ccl_max (0, lineNumber - 1)].fBaseline - lineMetrics[ccl_max (0, lineNumber - 1)].fAscent;
			newlineRect.setHeight (lineMetrics[ccl_max (0, lineNumber - 1)].fAscent + lineMetrics[ccl_max (0, lineNumber - 1)].fDescent);

			if(!characterBounds.isEmpty () && characterBounds.last ().bottom > newlineRect.top)
				newlineRect.right = characterBounds.last ().right;
			else
				newlineRect.right = 0;

			newlineRect.left = newlineRect.right;
			adjustLineHeight (newlineRect);
			characterBounds.add (newlineRect);
		}
		else
			characterBounds.add (rect);

		if(textIndex == textLength - 1)
		{
			RectF lastRect (rect);
			lastRect.left = lastRect.right;
			characterBounds.add (lastRect);
		}

		RectF hitTestRect (rect);
		if(lineNumber > 0)
			hitTestRect.top = ccl_min<CoordF> (hitTestRect.top, lineMetrics[lineNumber].fBaseline - lineMetrics[lineNumber].fAscent - (lineMetrics[lineNumber].fBaseline - lineMetrics[lineNumber].fAscent - (lineMetrics[lineNumber - 1].fBaseline + lineMetrics[lineNumber - 1].fDescent)) / 2.f);
		if(lineNumber + 1 < lineMetrics.size ())
			hitTestRect.bottom = ccl_max<CoordF> (hitTestRect.bottom, lineMetrics[lineNumber].fBaseline + lineMetrics[lineNumber].fDescent + (lineMetrics[lineNumber + 1].fBaseline - lineMetrics[lineNumber + 1].fAscent - (lineMetrics[lineNumber].fBaseline + lineMetrics[lineNumber].fDescent)) / 2.f);

		hitTestBounds.add (hitTestRect);

		CCL_PRINTF ("Character bounds at textIndex %d (%s): (%.1f, %.1f, %.1f, %.1f)\n", textIndex, currentCharacter == '\n' ? "<newline>" : MutableCString ().append (currentCharacter).str (), characterBounds.last ().left, characterBounds.last ().top, characterBounds.last ().right, characterBounds.last ().bottom)
		CCL_PRINTF ("Hit test bounds at textIndex %d (%s): (%.1f, %.1f, %.1f, %.1f)\n", textIndex, currentCharacter == '\n' ? "<newline>" : MutableCString ().append (currentCharacter).str (), hitTestBounds.last ().left, hitTestBounds.last ().top, hitTestBounds.last ().right, hitTestBounds.last ().bottom)

		textIndex++;
		currentCharacter = characters[textIndex];
	};

	auto processWhitespace = [&] ()
	{
		if(currentCharacter == '\n' && lastLineNumber + 1 < lineMetrics.size ())
			lastLineNumber++;
	
		RectF rect;
		rect.left = 0;
		rect.right = rect.left;
		rect.top = lineMetrics[lastLineNumber].fBaseline - lineMetrics[lastLineNumber].fAscent;
		rect.setHeight (lineMetrics[lastLineNumber].fAscent + lineMetrics[lastLineNumber].fDescent);
	
		CCL_PRINTF ("line metrics (%d): %f, %f, %f\n", lastLineNumber, lineMetrics[lastLineNumber].fBaseline, lineMetrics[lastLineNumber].fAscent, lineMetrics[lastLineNumber].fDescent)
		
		if(currentCharacter == ' ')
		{
			if(!characterBounds.isEmpty () && characterBounds.last ().bottom > rect.top)
				rect.left = characterBounds.last ().right;
			rect.setWidth (spaceWidth);
		}
		if(currentCharacter == '\t')
		{
			if(!characterBounds.isEmpty () && characterBounds.last ().bottom > rect.top)
				rect.left = characterBounds.last ().right;
			rect.setWidth (spaceWidth * kTabSize);
		}

		SOFT_ASSERT (currentCharacter == ' ' || currentCharacter == '\t' || currentCharacter == '\n', "Unexpected non-printable character")

		addBounds (rect, lastLineNumber);
	};

	paragraph->visit ([&] (int lineNumber, const skia::textlayout::Paragraph::VisitorInfo* info)
	{
		if(info == nullptr)
			return;

		for(int i = 0; i < info->count; i++)
		{
			while(utf8Positions[textIndex] < info->utf8Starts[i])
				processWhitespace ();

			RectF rect;
			getGlyphPosition (rect.left, rect.right, info->utf8Starts[i], i, info);
			rect.top = info->origin.y () - lineMetrics[lineNumber].fAscent;
			rect.setHeight (lineMetrics[lineNumber].fAscent + lineMetrics[lineNumber].fDescent);
			
			// tab characters are placeholders, we need to specifiy the width explicitly
			if(currentCharacter == '\t')
				rect.right += spaceWidth * kTabSize;

			addBounds (rect, lineNumber);

			lastLineNumber = lineNumber;
		}
	});

	while(characterBounds.count () < originalText.length ())
		processWhitespace ();

	characterBoundsChanged = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkiaTextLayout::updateUtf8Positions ()
{
	int currentLength = 0;
	int textLength = originalText.length ();
	utf8Positions.empty ();
	utf8Positions.resize (textLength + 1);
	StringChars characters (originalText);
	Core::Text::UTF16Reader reader (characters, textLength);
	unsigned char uCharBuffer[kMaxCodePointLength + 1] = {0};

	uchar32 codePoint = 0;
	int bytesUsed = 0;
	while((codePoint = reader.getNext (&bytesUsed)))
	{
		utf8Positions.add (currentLength);
		if(codePoint == u'\t')
		{
			// We're using placeholders for tabs. Skia inserts a replacement character (utf16: 0xFFFC) in this case,
			// which resolves to (utf8: 0xEF 0xBF 0xBC) and we add another space.
			// Insert four utf8 codepoints to match the resulting string length.
			currentLength += 4;
		}
		else
		{
			// Get length of utf8 encoding
			currentLength += Core::Text::UTFCodec::encodeUTF8 (codePoint, uCharBuffer, kMaxCodePointLength);
		}
		for(int i = 2; i < bytesUsed; i += 2)
			utf8Positions.add (currentLength);
	}
	utf8Positions.add (currentLength);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int SkiaTextLayout::countTabs (int position) const
{
	int tabCount = 0;
	for(int i = 0; i < tabPositions.count (); i++)
		if(tabPositions[i] + tabCount * kPlaceholderCodepoints < position)
			tabCount++;
	return tabCount;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename StyleFunction>
void SkiaTextLayout::insertStyle (const Range& range, StyleFunction styleFunction)
{
	if(utf8Positions.isEmpty ())
		updateUtf8Positions ();
	int utf8Start = (utf8Positions.count () > range.start) ? utf8Positions[range.start] : utf8Positions.last ();
	int utf8End = (utf8Positions.count () > range.start + range.length) ? utf8Positions[range.start + range.length] : utf8Positions.last ();

	utf8Start -= countTabs (utf8Start) * kPlaceholderCodepoints;
	utf8End -= countTabs (utf8End) * kPlaceholderCodepoints;

	ASSERT (utf8Start >= 0 && utf8End >= 0)

	int newIndex = -1;
	int resetIndex = -1;
	for(int i = 0; i < styles.count (); i++)
	{
		if(newIndex < 0 && utf8Start <= styles[i].position)
			newIndex = i;
		if(utf8End > styles[i].position)
			resetIndex = i;
	}

	skia::textlayout::TextStyle resetStyle (resetIndex >= 0 ? styles[resetIndex].style : textStyle);

	if(newIndex < 0)
	{
		skia::textlayout::TextStyle newStyle (resetStyle);
		styleFunction (newStyle);
		styles.add ({ utf8Start, newStyle });
		styles.add ({ utf8End, resetStyle });
	}
	else
	{
		if(styles[newIndex].position == utf8Start)
		{
			styleFunction (styles[newIndex].style);
		}
		else
		{
			skia::textlayout::TextStyle newStyle (newIndex > 0 ? styles[newIndex - 1].style : textStyle);
			styleFunction (newStyle);
			styles.insertAt (newIndex, { utf8Start, newStyle });
		}

		for(int j = newIndex; j < styles.count (); j++)
		{
			if(styles[j].position > utf8End)
			{
				styles.insertAt (j, { utf8End, resetStyle });
				break;
			}
			else if(styles[j].position == utf8End)
				break;
			else
				styleFunction (styles[j].style);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkiaTextLayout::setFontStyle (skia::textlayout::TextStyle& style, int mask, tbool state)
{
	int fontStyle = SkiaFontCache::fromSkFontStyle (style.getFontStyle ());
	if(state)
		fontStyle |= mask;
	else
		fontStyle &= ~mask;
	style.setFontStyle (SkiaFontCache::toSkFontStyle (fontStyle));

	int decoration = style.getDecorationType ();
	int decorationMask = (mask & Font::kStrikeout) ? skia::textlayout::kLineThrough : 0 | (mask & Font::kUnderline) ? skia::textlayout::kUnderline : 0;
	if(state)
		decoration |= decorationMask;
	else
		decoration &= ~decorationMask;
	style.setDecoration (static_cast<skia::textlayout::TextDecoration> (decoration));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaTextLayout::setFontStyle (const Range& range, int mask, tbool state)
{
	insertStyle (range, [this, mask, state] (skia::textlayout::TextStyle& style)
	{
		setFontStyle (style, mask, state);
	});
	needUpdate = true;
	imageBoundsChanged = true;
	textBoundsChanged = true;
	characterBoundsChanged = true;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkiaTextLayout::setFontSize (skia::textlayout::TextStyle& style, float size)
{
	style.setFontSize (size);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaTextLayout::setFontSize (const Range& range, float size)
{
	insertStyle (range, [this, size] (skia::textlayout::TextStyle& style)
	{
		setFontSize (style, size);
	});
	needUpdate = true;
	imageBoundsChanged = true;
	textBoundsChanged = true;
	characterBoundsChanged = true;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkiaTextLayout::setSpacing (skia::textlayout::TextStyle& style, float spacing)
{
	style.setLetterSpacing (spacing);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaTextLayout::setSpacing (const Range& range, float spacing)
{
	insertStyle (range, [this, spacing] (skia::textlayout::TextStyle& style)
	{
		setSpacing (style, spacing);
	});
	needUpdate = true;
	imageBoundsChanged = true;
	textBoundsChanged = true;
	characterBoundsChanged = true;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkiaTextLayout::setLineSpacing (skia::textlayout::TextStyle& style, float lineSpacing)
{
	style.setHeightOverride (lineSpacing != 1.f);
	style.setHeight (lineSpacing);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaTextLayout::setLineSpacing (const Range& range, float lineSpacing)
{
	insertStyle (range, [this, lineSpacing] (skia::textlayout::TextStyle& style)
	{
		setLineSpacing (style, lineSpacing);
	});
	needUpdate = true;
	imageBoundsChanged = true;
	textBoundsChanged = true;
	characterBoundsChanged = true;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkiaTextLayout::setTextColor (skia::textlayout::TextStyle& style, Color color)
{
	style.setColor (SkColorSetARGB (color.alpha, color.red, color.green, color.blue));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaTextLayout::setTextColor (const Range& range, Color color)
{
	insertStyle (range, [this, color] (skia::textlayout::TextStyle& style)
	{
		setTextColor (style, color);
	});
	needUpdate = true;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaTextLayout::setBaselineOffset (const Range& range, float offset)
{
	insertStyle (range, [this, offset] (skia::textlayout::TextStyle& style)
	{
		style.setBaselineShift (-offset);
	});
	needUpdate = true;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaTextLayout::setSuperscript (const Range& range)
{
	return setSuperscript (range, kSuperscriptSizeFactor, kSuperscriptBaselineFactor);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaTextLayout::setSubscript (const Range& range)
{
	return setSuperscript (range, kSubscriptSizeFactor, -kSubscriptBaselineFactor);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult SkiaTextLayout::setSuperscript (const Range& _range, float sizeFactor, float baselineFactor)
{
	auto setStyle = [&] (const Range& range, float fontSize, float baselineOffset)
	{
		setFontSize (range, fontSize * sizeFactor);
		setBaselineOffset (range, baselineOffset + baselineFactor * fontSize);
	};

	Range range (_range);
	for(int i = 0; i < styles.count (); i++)
	{
		int currentRangeFrom = styles[i].position;
		int currentRangeTo = i < styles.count () - 1 ? styles[i + 1].position : originalText.length ();
		if(range.start < currentRangeTo && range.start + range.length > currentRangeFrom)
		{
			int overlapStart = ccl_max (range.start, currentRangeFrom);
			int overlapEnd = ccl_min (range.start + range.length, currentRangeTo);
			Range overlapRange (overlapStart, overlapEnd - overlapStart);
			float fontSize = styles[i].style.getFontSize ();
			float baselineOffset = -styles[i].style.getBaselineShift ();
			setStyle (overlapRange, fontSize, baselineOffset);

			if(overlapStart > range.start)
			{
				setSuperscript (Range (range.start, overlapStart - range.start), sizeFactor, baselineFactor);
				range.length -= overlapStart - range.start;
				range.start = overlapStart;
			}

			if(overlapEnd < range.start + range.length)
			{
				range.length = overlapEnd - (range.start + range.length);
				range.start = overlapEnd;
			}

			if(overlapStart == range.start && overlapEnd == range.start + range.length)
				return kResultOk;
		}
	}

	if(range.length > 0)
		setStyle (range, textStyle.getFontSize (), -textStyle.getBaselineShift ());

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkiaTextLayout::setBackgroundColor (skia::textlayout::TextStyle& style, Color color)
{
	SkColor backgroundColor = SkColorSetARGB (color.alpha, color.red, color.green, color.blue);
	style.setBackgroundColor (SkPaint (SkColor4f::FromColor (backgroundColor)));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult SkiaTextLayout::setBackgroundColor (const Range& range, Color color)
{
	insertStyle (range, [this, color] (skia::textlayout::TextStyle& style)
	{
		setBackgroundColor (style, color);
	});
	needUpdate = true;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaTextLayout::getBounds (Rect& _bounds, int flags) const
{
    RectF bounds;
    tresult result = getBounds (bounds, flags);
    _bounds = rectFToInt (bounds);
    return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaTextLayout::getBounds (RectF& bounds, int flags) const
{
	SkiaTextLayout* This = const_cast<SkiaTextLayout*> (this);

	if(needUpdate)
		This->updateParagraph ();
	if(paragraph == nullptr)
		return kResultFailed;

	if(textBoundsChanged)
		This->updateTextBounds ();

	PointF offset;
	getParagraphOffset (offset);
	bounds = textRect;
	bounds.offset (offset);

	if(!(flags & kNoMargin))
	{
		bounds.left -= kPaddingLeft;
		bounds.right += kPaddingRight;
		bounds.top -= kPaddingTop;
		bounds.bottom += kPaddingBottom;
	}

    return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaTextLayout::getImageBounds (RectF& bounds) const
{
	SkiaTextLayout* This = const_cast<SkiaTextLayout*> (this);

	if(needUpdate)
		This->updateParagraph ();
	if(paragraph == nullptr)
		return kResultFailed;

	if(textBoundsChanged)
		This->updateTextBounds ();

	if(imageBoundsChanged)
		This->updateImageBounds ();

	bounds = imageRect;

	PointF offset;
	getParagraphOffset (offset);
	bounds.offset (offset);

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaTextLayout::getBaselineOffset (PointF& baseline) const
{
	SkiaTextLayout* This = const_cast<SkiaTextLayout*> (this);

	if(needUpdate)
		This->updateParagraph ();
	if(paragraph == nullptr)
		return kResultFailed;

	if(textBoundsChanged)
		This->updateTextBounds ();

	getParagraphOffset (baseline);
	baseline.y += SkScalarFloorToScalar (paragraph->getAlphabeticBaseline () + .5f);

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaTextLayout::hitTest (int& textIndex, PointF& position) const
{
	SkiaTextLayout* This = const_cast<SkiaTextLayout*> (this);

	if(needUpdate)
		This->updateParagraph ();
	if(paragraph == nullptr)
		return kResultFailed;

	if(textBoundsChanged)
		This->updateTextBounds ();

	if(characterBoundsChanged)
		This->updateCharacterBounds ();
	
	PointF offset;
	getParagraphOffset (offset);
	position.offset (-offset.x, -offset.y);
	
	textIndex = -1;
	bool endOfLine = false;
	for(int i = 0; i < hitTestBounds.count (); i++)
	{
		if(hitTestBounds[i].pointInside (position))
		{
			// hit a character
			textIndex = i;
			break;
		}
		if(position.x < 0 && position.y >= hitTestBounds[i].top && position.y < hitTestBounds[i].bottom && originalText[i] != '\n')
		{
			// hitpoint before the leftmost character
			textIndex = i;
			break;
		}
		if(position.y < hitTestBounds[i].top)
		{
			// hitpoint after the rightmost character
			textIndex = i;
			endOfLine = true;
			break;
		}
	}
	if(textIndex < 0)
	{
		if(position.y < characterBounds.first ().bottom && position.x < characterBounds.first ().left)
		{
			textIndex = 0;
			position = characterBounds.first ().getLeftTop ();
		}
		else
		{
			textIndex = ccl_max (characterBounds.count () - 1, 0);
			position = characterBounds.last ().getRightTop ();
		}
	}
	else if(endOfLine)
		position = characterBounds[ccl_max (textIndex - 1, 0)].getRightTop ();
	else if(position.x >= characterBounds[textIndex].left + (characterBounds[textIndex].right - characterBounds[textIndex].left) / 2.f)
	{
		position = characterBounds[textIndex].getRightTop ();
		textIndex++;
	}
	else
		position = characterBounds[textIndex].getLeftTop ();

	position.offset (offset.x, offset.y);

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaTextLayout::getCharacterBounds (RectF& rect, int textIndex) const
{
	SkiaTextLayout* This = const_cast<SkiaTextLayout*> (this);

	if(textIndex < 0)
		return kResultInvalidArgument;

	if(needUpdate)
		This->updateParagraph ();
	if(paragraph == nullptr)
		return kResultFailed;

	if(textBoundsChanged)
		This->updateTextBounds ();

	if(characterBoundsChanged)
		This->updateCharacterBounds ();
	
	PointF offset;
	getParagraphOffset (offset);
	
	if(textIndex >= characterBounds.count ())
	{
		rect = characterBounds.last ();

		if(characterBounds.isEmpty () && textIndex == 0)
			rect.setHeight (paragraph->getHeight () / (textStyle.getHeightOverride () ? textStyle.getHeight () : 1));

		rect.left = rect.right;
	}
	else
		rect = characterBounds[textIndex];
	rect.offset (offset.x, offset.y);

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaTextLayout::getTextBounds (IMutableRegion& bounds, const Range& range) const
{
	SkiaTextLayout* This = const_cast<SkiaTextLayout*> (this);

	if(needUpdate)
		This->updateParagraph ();
	if(paragraph == nullptr)
		return kResultFailed;

	if(characterBoundsChanged)
		This->updateCharacterBounds ();

	PointF offset;
	getParagraphOffset (offset);

	Rect rect;
	float lastBottom = 0;
	for(int i = ccl_max (0, range.start); i < range.start + range.length && i < characterBounds.count (); i++)
	{
		if(characterBounds[i].top > lastBottom)
		{
			if(rect.isEmpty ())
				rect.setWidth (1);
			rect.offset (offset.x, offset.y);
			bounds.addRect (rect);
			rect.setEmpty ();
		}
		if(rect.isEmpty ())
			rect = rectFToInt (characterBounds[i]);
		else
			rect.join (rectFToInt (characterBounds[i]));
	}
	if(rect.isEmpty ())
		rect.setWidth (1);

	if(characterBounds.isEmpty () && range.start == 0 && range.length == 0)
		rect.setHeight (paragraph->getHeight () / (textStyle.getHeightOverride () ? textStyle.getHeight () : 1));

	rect.offset (offset.x, offset.y);

	if(!rect.isEmpty ())
		bounds.addRect (rect);

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaTextLayout::getLineRange (Range& range, int textIndex) const
{
	SkiaTextLayout* This = const_cast<SkiaTextLayout*> (this);

	if(needUpdate)
		This->updateParagraph ();
	if(paragraph == nullptr)
		return kResultFailed;

	if(characterBoundsChanged)
		This->updateCharacterBounds ();

	int textLength = originalText.length ();
	int numCharacterBounds = characterBounds.count ();
	if(textIndex == 0 && numCharacterBounds == 0)
	{
		range.start = 0;
		range.length = 0;
		return kResultOk;
	}

	if(textIndex < 0 || textIndex > textLength)
		return kResultInvalidArgument;

	range.start = -1;
	range.length = -1;

	// characterBounds have line height as height, so compare y center positions in case lines are vertically overlapping
	CoordF textIndexY = characterBounds[textIndex].getCenter ().y;
	for(int i = textIndex; i >= 0; i--)
	{
		CoordF currentY = characterBounds[i].getCenter ().y;
		if(currentY < textIndexY - 1.f) // 1.f to ignore slightly different height for newlines
			break;

		range.start = i;
	}

	for(int i = textIndex; i < numCharacterBounds; i++)
	{
		CoordF currentY = characterBounds[i].getCenter ().y;
		if(currentY > textIndexY + 1.f)
		{
			range.length = i - range.start;
			break;
		}
	}

	if(range.length < 0 && range.start >= 0)
		range.length = numCharacterBounds - range.start;

	CCL_PRINTF ("line range (%d, %d): %s\n", range.start, range.length, MutableCString (originalText.subString (range.start, range.length)).replace ('\n', '$').str ());

	return (range.start >= 0 && range.length > 0) ? kResultOk : kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaTextLayout::getWordRange (Range& range, int textIndex) const
{
	SkiaTextLayout* This = const_cast<SkiaTextLayout*> (this);

	if(needUpdate)
		This->updateParagraph ();
	if(paragraph == nullptr)
		return kResultFailed;

	if(characterBoundsChanged)
		This->updateCharacterBounds ();

	int textLength = originalText.length ();
	int numCharacterBounds = characterBounds.count ();
	if(textIndex == 0 && numCharacterBounds == 0)
	{
		range.start = 0;
		range.length = 0;
		return kResultOk;
	}

	if(textIndex < 0 || textIndex > textLength)
		return kResultInvalidArgument;

	// tabs are represented as two characters in skia's count (see updateUtf8Positions)
	int whitespaceOffset = 0;
	for(int i = textIndex; i > 0; i--)
	{
		if(originalText[i] == '\t')
			whitespaceOffset++;
	}

	const skia::textlayout::TextRange& skRange = paragraph->getWordBoundary (textIndex + whitespaceOffset);
	range.start = int(skRange.start - whitespaceOffset);
	range.length = int(skRange.end - skRange.start);

	return (range.start >= 0 && range.length > 0) ? kResultOk : kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int SkiaTextLayout::findIndex (int utf8Position, const skia::textlayout::Paragraph::VisitorInfo* info) const
{
	for(int i = 0; i < info->count + 1; i++)
	{
		if(utf8Position <= info->utf8Starts[i])
			return i;
	}
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float SkiaTextLayout::getEndOfRange (const skia::textlayout::Paragraph::VisitorInfo* info) const
{
	SkPaint paint;
	paint.setStyle (SkPaint::kStroke_Style);
	paint.setStrokeWidth (0);

	SkRect glyphBounds;
	info->font.getBounds (info->glyphs + info->count - 1, 1, &glyphBounds, &paint);
	glyphBounds.offset (info->positions[info->count - 1] + info->origin);

	return glyphBounds.x () + glyphBounds.width ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkiaTextLayout::getGlyphPosition (float& left, float& right, int utf8Position, int index, const skia::textlayout::Paragraph::VisitorInfo* info) const
{
	auto adjustForCluster = [&utf8Position, &info](float& value, int index)
	{
		if(utf8Position < info->utf8Starts[index] && index > 0)
		{
			// Text position is somewhere inside a cluster. Try to find an estimate for the glyph position
			int clusterStartIndex = info->utf8Starts[index - 1];
			int clusterEndIndex = info->utf8Starts[index];
			SkScalar clusterWidth = info->positions[index].x () - info->positions[index - 1].x ();
			value = info->positions[index - 1].x () + info->origin.x ();
			value += clusterWidth * (utf8Position - clusterStartIndex) / (clusterEndIndex - clusterStartIndex);
		}
	};

	left = info->positions[index].x () + info->origin.x ();
	adjustForCluster (left, index);

	if(index + 1 < info->count)
	{
		right = info->positions[index + 1].x () + info->origin.x ();
		adjustForCluster (right, index + 1);
	}
	else
		right = getEndOfRange (info);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkiaTextLayout::draw (SkCanvas& canvas, PointF position, Color textColor)
{
	if(textColor != defaultColor)
	{
		defaultColor = textColor;
		needUpdate = true;
	}

	if(needUpdate)
		updateParagraph ();
	if(paragraph == nullptr)
		return;

	if(textBoundsChanged)
		updateTextBounds ();

	PointF blobPosition (position);
	PointF offset;
	getParagraphOffset (offset);
	blobPosition.offset (offset);
	
	paragraph->paint (&canvas, blobPosition.x, blobPosition.y);

#if DEBUG_TEXT_BOUNDS
	SkPaint paint (SkColors::kRed);
	paint.setStyle (SkPaint::kStroke_Style);
	RectF imageBounds;
	getImageBounds (imageBounds);
	imageBounds.offset (position);
	canvas.drawRect (SkRect::MakeLTRB (imageBounds.left, imageBounds.top, imageBounds.right, imageBounds.bottom), paint);

	paint.setColor (SkColors::kBlue);
	RectF textBounds;
	getBounds (textBounds);
	textBounds.offset (position);
	canvas.drawRect (SkRect::MakeLTRB (textBounds.left, textBounds.top, textBounds.right, textBounds.bottom), paint);

	paint.setColor (SkColors::kGreen);
	PointF baseline;
	getBaselineOffset (baseline);
	baseline.offset (position);
	canvas.drawLine (SkPoint::Make (baseline.x - 2, baseline.y), SkPoint::Make (textBounds.right, baseline.y), paint);
	canvas.drawLine (SkPoint::Make (baseline.x, baseline.y - 2), SkPoint::Make (baseline.x, baseline.y + 2), paint);
#endif
	
#if DEBUG_CHARACTER_BOUNDS
	if(characterBoundsChanged)
		updateCharacterBounds ();
	
	for(int i = 0; i < characterBounds.count (); i++)
	{
		SkPaint paint (SkColors::kCyan);
		paint.setStyle (SkPaint::kStroke_Style);
		RectF bounds = hitTestBounds[i];
		bounds.offset (blobPosition);
		canvas.drawRect (SkRect::MakeLTRB (bounds.left, bounds.top, bounds.right, bounds.bottom), paint);

		paint.setColor (SkColors::kBlue);
		bounds = characterBounds[i];
		bounds.offset (blobPosition);
		canvas.drawRect (SkRect::MakeLTRB (bounds.left, bounds.top, bounds.right, bounds.bottom), paint);
	}
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkiaTextLayout::getParagraphOffset (PointF& offset) const
{
	switch(alignment.getAlignH ())
	{
		case Alignment::kHCenter :
			offset.x = kPaddingLeft + (boundingRect.getWidth () - textRect.getWidth () - kPaddingLeft - kPaddingRight) / 2;
			break;
		case Alignment::kRight :
			offset.x = kPaddingLeft + (boundingRect.getWidth () - textRect.getWidth () - kPaddingLeft -  kPaddingRight);
			break;
		default : // left aligned
			offset.x = kPaddingLeft;
	}

	switch(alignment.getAlignV ())
	{
		case Alignment::kVCenter :
			offset.y = kPaddingTop + (boundingRect.getHeight () - textRect.getHeight () - kPaddingTop - kPaddingBottom) / 2;
			break;
		case Alignment::kTop :
			offset.y = kPaddingTop;
			break;
		default : // bottom aligned
			offset.y = kPaddingTop + (boundingRect.getHeight () - textRect.getHeight ()  - kPaddingTop - kPaddingBottom);
	}
	
	offset.offset (-textRect.left, -textRect.top);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API SkiaTextLayout::getText () const
{
	return originalText;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Vector<int>& SkiaTextLayout::getUtf8Positions () const
{
	return utf8Positions;
}
