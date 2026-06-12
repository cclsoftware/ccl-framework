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
// Filename    : ccl/gui/graphics/markupsupport.h
// Description : CCL Markup Support
//
//************************************************************************************************

#ifndef _ccl_markupsupport_h
#define _ccl_markupsupport_h

#include "ccl/gui/graphics/formattedtext.h"

#include "ccl/base/collections/objectstack.h"

#include "ccl/public/gui/graphics/imarkupsupport.h"
#include "ccl/public/text/itextbuilder.h"

namespace CCL {

//************************************************************************************************
// MarkupTags
/** Supported CCL Markup tags, inspired by BBCode. */
//************************************************************************************************

namespace MarkupTags
{
	const String kBold ("b");
	const String kItalic ("i");
	const String kUnderline ("u");
	const String kSuperscript ("sup");
	const String kSubscript ("sub");
	const String kStyleColor ("style color");
	const String kColor ("color");
	const String kStyleSize ("style size");
	const String kSize ("size");

	inline void escapePlainText (String& text)
	{
		text.replace ("[", "[]");
	}
}

//************************************************************************************************
// MarkupParser
//************************************************************************************************
	
class MarkupParser: public Object
{
public:
	MarkupParser (StringRef string);

	void parse (StringRef string);

	StringRef getPlainText () const { return plainText; }
	int getPlainTextPosition (int markupPosition) const;
	int getMarkupPosition (int plainTextPosition, bool positionBeforeMarkup) const;	

	const ObjectArray& getFormatInstructions () const { return formatInstructions; }
	void applyFormatting (IFormattedTextHandler& handler) const;

protected:
	static TextNodeType getType (StringRef tag);

	struct TextRange
	{
		int markupPosition;
		int length;
	};

	Vector<TextRange> ranges;
	String plainText;
	ObjectArray formatInstructions;
	ObjectStack openedInstructions;
};

//************************************************************************************************
// MarkupPainter
//************************************************************************************************

class MarkupPainter: public Object,
					 public IMarkupPainter
{
public:
	DECLARE_CLASS (MarkupPainter, Object)

	// IMarkupPainter
	tresult CCL_API drawMarkupString (IGraphics& graphics, RectRef rect, StringRef text, FontRef font, BrushRef brush,
									  AlignmentRef alignment = Alignment ()) override;
	tresult CCL_API drawMarkupString (IGraphics& graphics, RectFRef rect, StringRef text, FontRef font, BrushRef brush,
									  AlignmentRef alignment = Alignment ()) override;
	tresult CCL_API measureMarkupString (Rect& size, StringRef text, FontRef font) override;
	tresult CCL_API measureMarkupString (RectF& size, StringRef text, FontRef font) override;

	CLASS_INTERFACE (IMarkupPainter, Object)
};

//************************************************************************************************
// MarkupBuilder
//************************************************************************************************

class MarkupBuilder: public Object,
					 public ITextBuilder
{
public:
	DECLARE_CLASS (MarkupBuilder, Object)

	// ITextBuilder
	tresult CCL_API printFragment (String& result, const TextFragment& fragment) override;
	ITextTable* CCL_API createTable () override;

	CLASS_INTERFACE (ITextBuilder, Object)

protected:
	String unpack (const TextFragment& fragment) const;
};

} // namespace CCL

#endif // _ccl_markupsupport_h
