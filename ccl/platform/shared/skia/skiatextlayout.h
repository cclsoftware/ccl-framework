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
// Filename    : ccl/platform/shared/skia/skiatextlayout.h
// Description : Skia Text Layout
//
//************************************************************************************************

#ifndef _ccl_skia_textlayout_h
#define _ccl_skia_textlayout_h

#include "ccl/platform/shared/skia/skiaglue.h"

#include "ccl/platform/shared/skia/skiarendertarget.h"

#include "ccl/gui/graphics/nativegraphics.h"

namespace CCL {

//************************************************************************************************
// SkiaFontCache
//************************************************************************************************

class SkiaFontCache: public Object,
					 public Singleton<SkiaFontCache>
{
public:
	SkiaFontCache ();
	~SkiaFontCache ();

	static int fromSkFontStyle (SkFontStyle style);
	static SkFontStyle toSkFontStyle (int style);

	SkFont* createFont (FontRef font);
	void removeAll ();
	void addStyledFont (StringRef familyName, int fontStyle, StringRef fullName, StringRef styleName);
	void addUserFont (StringRef familyName);
	bool isUserFont (StringRef familyName) const;

	sk_sp<skia::textlayout::FontCollection> getFontCollection () const;

protected:
	enum Constants { kStylesUsed = Font::kBold|Font::kItalic };

	struct FontCacheRecord
	{
		Font font;
		SkFont skFont;
	};

	struct StyledFont
	{
		StyledFont (StringRef _familyName, int _fontStyle, StringRef _fullName, StringRef _styleName)
		: familyName (_familyName),
		fontStyle (_fontStyle),
		styleName (_styleName),
		fullName (_fullName)
		{}

		StyledFont ()
		: familyName (""),
		fontStyle (0),
		styleName (""),
		fullName ("")
		{}

		String familyName;
		int fontStyle;
		String fullName;
		String styleName;
	};

	sk_sp<skia::textlayout::FontCollection> fontCollection;
	sk_sp<SkFontMgr> fontManager;
	static const int kMaxChacheEntries = 128;
	Vector<FontCacheRecord> entries;

	LinkedList<StyledFont> styledFontList;
	LinkedList<String> userFontList;

	static int getUsedStyle (FontRef font) {return font.getStyle () & kStylesUsed;}

	FontCacheRecord* lookup (FontRef font) const;
	FontCacheRecord* createEntry (FontRef font);
	FontCacheRecord* add (FontRef font, SkFont& skFont);
};

//************************************************************************************************
// SkiaTextLayout
//************************************************************************************************

class SkiaTextLayout: public NativeTextLayout
{
public:
	SkiaTextLayout ();

	DECLARE_CLASS_ABSTRACT (SkiaTextLayout, NativeTextLayout)

	void draw (SkCanvas& canvas, PointF position, Color textColor);
	tresult setBackgroundColor (const Range& range, Color color);
	const Vector<int>& getUtf8Positions () const;

	// ITextLayout
	tresult CCL_API construct (StringRef text, Coord width, Coord height, FontRef font, LineMode lineMode, TextFormatRef format) override;
	tresult CCL_API construct (StringRef text, CoordF width, CoordF height, FontRef font, LineMode lineMode, TextFormatRef format) override;
	tresult CCL_API resize (Coord width, Coord height) override;
	tresult CCL_API resize (CoordF width, CoordF height) override;
	tresult CCL_API setFontStyle (const Range& range, int style, tbool state) override;
	tresult CCL_API setFontSize (const Range& range, float size) override;
	tresult CCL_API setSpacing (const Range& range, float spacing) override;
	tresult CCL_API setLineSpacing (const Range& range, float lineSpacing) override;
	tresult CCL_API setTextColor (const Range& range, Color color) override;
	tresult CCL_API setBaselineOffset (const Range& range, float offset) override;
	tresult CCL_API setSuperscript (const Range& range) override;
	tresult CCL_API setSubscript (const Range& range) override;
	tresult CCL_API getBounds (Rect& bounds, int flags = 0) const override;
	tresult CCL_API getBounds (RectF& bounds, int flags = 0) const override;
	tresult CCL_API getImageBounds (RectF& bounds) const override;
	tresult CCL_API getBaselineOffset (PointF& offset) const override;
	tresult CCL_API hitTest (int& textIndex, PointF& position) const override;
	tresult CCL_API getCharacterBounds (RectF& offset, int textIndex) const override;
	tresult CCL_API getTextBounds (IMutableRegion& bounds, const Range& range) const override;
	tresult CCL_API getLineRange (Range& range, int textIndex) const override;
	tresult CCL_API getWordRange (Range& range, int textIndex) const override;
	StringRef CCL_API getText () const override;

protected:
	struct TextStyle
	{
		int position;
		skia::textlayout::TextStyle style;

		TextStyle (int position = 0, const skia::textlayout::TextStyle& style = skia::textlayout::TextStyle ())
		: position (position),
		  style (style)
		{}
	};

	static const int kTabSize = 8;
	static const char kTabReplacementCharacter;
	static const int kPlaceholderCodepoints = 3;
	static const int kMaxCodePointLength = 4;

	std::unique_ptr<skia::textlayout::Paragraph> paragraph;
	skia::textlayout::ParagraphStyle paragraphStyle;
	skia::textlayout::PlaceholderStyle tabStyle;

	String originalText;
	MutableCString text;
	Vector<int> tabPositions;
	float spaceWidth;

	Vector<int> utf8Positions;
	Vector<RectF> characterBounds;
	Vector<RectF> hitTestBounds;

	RectF boundingRect;
	RectF textRect;
	RectF imageRect;

	Alignment alignment;
	bool restrictWidth;
	LineMode lineMode;

	skia::textlayout::TextStyle textStyle;
	Vector<TextStyle> styles;
	Color defaultColor;
	SkString familyName;

	bool needUpdate;
	bool imageBoundsChanged;
	bool textBoundsChanged;
	bool characterBoundsChanged;

	void updateParagraph ();
	void updateTextBounds ();
	void updateImageBounds ();
	void updateCharacterBounds ();
	void updateUtf8Positions ();
	void getParagraphOffset (PointF& offset) const;

	template<typename StyleFunction>
	void insertStyle (const Range& range, StyleFunction styleFunction);

	void setFontStyle (skia::textlayout::TextStyle& style, int mask, tbool state);
	void setFontSize (skia::textlayout::TextStyle& style, float size);
	void setSpacing (skia::textlayout::TextStyle& style, float spacing);
	void setLineSpacing (skia::textlayout::TextStyle& style, float lineSpacing);
	void setTextColor (skia::textlayout::TextStyle& style, Color color);
	void setBackgroundColor (skia::textlayout::TextStyle& style, Color color);
	tresult setSuperscript (const Range& range, float sizeFactor, float baselineFactor);

	int countTabs (int position) const;
	int findIndex (int utfPosition, const skia::textlayout::Paragraph::VisitorInfo* info) const;
	float getEndOfRange (const skia::textlayout::Paragraph::VisitorInfo* info) const;
	void getGlyphPosition (float& left, float& right, int utf8Position, int index, const skia::textlayout::Paragraph::VisitorInfo* info) const;
};

} // namespace CCL

#endif // _ccl_skia_textlayout_h
