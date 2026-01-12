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
// Filename    : ccl/gui/graphics/textlayoutbuilder.h
// Description : Text Markup Parser
//
//************************************************************************************************

#ifndef _ccl_textlayoutbuilder_h
#define _ccl_textlayoutbuilder_h

#include "ccl/base/collections/objectarray.h"
#include "ccl/base/collections/objectstack.h"

#include "ccl/public/base/variant.h"
#include "ccl/public/gui/graphics/itextlayout.h"
#include "ccl/public/gui/graphics/imarkuppainter.h"
#include "ccl/public/gui/framework/ivisualstyle.h"

namespace CCL {

//************************************************************************************************
// IMarkupContentHandler
//************************************************************************************************

interface IMarkupContentHandler: IUnknown
{
	DEFINE_ENUM (FormatType)
	{
		kUnknown,
		kBold,
		kItalic,
		kUnderline,
		kColor,
		kSize,
		kSuperscript,
		kSubscript
	};

	struct FormatEntry
	{
		FormatType type;
		Variant paramValue;
		int start;
		int length;

		FormatEntry (FormatType type)
		: type (type),
		  start (0),
		  length (0)
		{}
	};

	virtual tresult CCL_API applyFormat (const FormatEntry& entry) = 0;
};

//************************************************************************************************
// IMarkupParser
//************************************************************************************************

interface IMarkupParser: IUnknown
{
	virtual void CCL_API parse (StringRef string) = 0;
	
	virtual StringRef CCL_API getPlainText () const = 0;
	
	virtual int CCL_API getPlainTextPosition (int markupPosition) const = 0;
	
	virtual int CCL_API getMarkupPosition (int plainTextPosition, tbool positionBeforeMarkup) const = 0;
	
	virtual tbool CCL_API escapePlainText (String& text) const = 0;
	
	virtual void CCL_API applyFormatting (IMarkupContentHandler& handler, ITextLayout::Range range = {0, -1}, int textOffset = 0) const = 0;
};

//************************************************************************************************
// MarkupParser
/** Interpret format tags in a string. */
//************************************************************************************************
	
class MarkupParser: public Object,
					public IMarkupParser
{
public:
	MarkupParser (StringRef string, const IVisualStyle& style);

	// IMarkupParser
	void CCL_API parse (StringRef string) override;
	StringRef CCL_API getPlainText () const override { return plainText; }
	int CCL_API getPlainTextPosition (int markupPosition) const override;
	int CCL_API getMarkupPosition (int plainTextPosition, tbool positionBeforeMarkup) const override;
	tbool CCL_API escapePlainText (String& text) const override;
	void CCL_API applyFormatting (IMarkupContentHandler& handler, ITextLayout::Range range = { 0, -1 }, int textOffset = 0) const override;

	CLASS_INTERFACE (IMarkupParser, Object)

protected:
	static IMarkupContentHandler::FormatType getType (StringRef tag);

	class FormatObject: public Object,
						public IMarkupContentHandler::FormatEntry
	{
	public:
		FormatObject (IMarkupContentHandler::FormatType type)
		: FormatEntry (type)
		{}
	};

	struct TextRange
	{
		int markupPosition;
		int length;
	};

	Vector<TextRange> ranges;
	String plainText;
	ObjectArray formatInstructions;
	ObjectStack openedInstructions;
	const IVisualStyle& style;
};

//************************************************************************************************
// TextLayoutBuilder
//************************************************************************************************

class TextLayoutBuilder: public Object,
						 public IMarkupContentHandler
{
public:
	DECLARE_CLASS (TextLayoutBuilder, Object)
	
	TextLayoutBuilder (ITextLayout* textLayout = nullptr);
	
	// IMarkupContentHandler
	tresult CCL_API applyFormat (const FormatEntry& entry) override;

	CLASS_INTERFACE (IMarkupContentHandler, Object)
	
protected:
	ITextLayout* textLayout;
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
	tresult CCL_API drawMarkupString (IGraphics& graphics, RectRef rect, StringRef text, FontRef font, BrushRef brush, AlignmentRef alignment = Alignment ()) override;
	tresult CCL_API drawMarkupString (IGraphics& graphics, RectFRef rect, StringRef text, FontRef font, BrushRef brush, AlignmentRef alignment = Alignment ()) override;
	tresult CCL_API measureMarkupString (Rect& size, StringRef text, FontRef font, int flags = 0) override;
	tresult CCL_API measureMarkupString (RectF& size, StringRef text, FontRef font, int flags = 0) override;

	CLASS_INTERFACE (IMarkupPainter, Object)
};

} // namespace CCL

#endif // _ccl_textlayoutbuilder_h
