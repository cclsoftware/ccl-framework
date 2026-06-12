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
// Filename    : ccl/public/text/itextbuilder.h
// Description : Text Builder Interface
//
//************************************************************************************************

#ifndef _ccl_itextbuilder_h
#define _ccl_itextbuilder_h

#include "ccl/public/text/textformatting.h"

#include "ccl/public/base/variant.h"

namespace CCL {

interface ITextTable;

/**	\addtogroup ccl_text 
@{ */

//////////////////////////////////////////////////////////////////////////////////////////////////
// Text Fragments used with ITextBuilder and TextBlock
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Text 
{
	/** Basic text fragment. */
	struct Fragment
	{
		TextNodeType nodeType;
		String content;
		tbool encode;
		Variant argument;

		Fragment (TextNodeType nodeType, StringRef content, bool encode = true)
		: nodeType (nodeType),
		  content (content),
		  encode (encode)
		{}
	};

	/** Heading. */
	struct Heading: Fragment
	{
		Heading (int level, StringRef content, bool encode = true)
		: Fragment (kHeading, content, encode)
		{
			argument = level;
		}
	};

	/** Paragraph. */
	struct Paragraph: Fragment
	{
		Paragraph (StringRef content, bool encode = true)
		: Fragment (kParagraph, content, encode)
		{}
	};

	/** Block quote. */
	struct BlockQuote: Fragment
	{
		BlockQuote (StringRef content, bool encode = true)
		: Fragment (kBlockQuote, content, encode)
		{}
	};

	/** List item. */
	struct ListItem: Fragment
	{
		ListItem (TextListType listType, StringRef content, bool encode = true)
		: Fragment (kListItem, content, encode)
		{
			argument = listType;
		}
	};

	/** List begin. */
	struct ListBegin: ListItem
	{
		ListBegin (TextListType listType)
		: ListItem (listType, nullptr, false)
		{
			nodeType = kListBegin; 
		}
	};

	/** List end. */
	struct ListEnd: ListItem
	{
		ListEnd (TextListType listType)
		: ListItem (listType, nullptr, false)
		{
			nodeType = kListEnd; 
		}
	};

	/** Table. */
	struct Table: Fragment
	{
		Table (ITextTable* table)
		: Fragment (kTable, nullptr, false)
		{
			argument.takeShared (reinterpret_cast<IUnknown*> (table));
		}
	};

	/** Code block. */
	struct CodeBlock: Fragment
	{
		CodeBlock (StringRef content, bool encode = true)
		: Fragment (kCodeBlock, content, encode)
		{}
	};

	/** Horizontal line. */
	struct HorizontalLine: Fragment
	{
		HorizontalLine ()
		: Fragment (kHorizontalLine, nullptr, false)
		{}
	};

	/** Plain text. */
	struct Plain: Fragment
	{
		Plain (StringRef content, bool encode = true)
		: Fragment (kPlainText, content, encode)
		{}
	};

	/** Soft break. */
	struct SoftBreak: Fragment
	{
		SoftBreak ()
		: Fragment (kSoftBreak, nullptr, false)
		{}
	};

	/** Line break. */
	struct LineBreak: Fragment
	{
		LineBreak ()
		: Fragment (kLineBreak, nullptr, false)
		{}
	};

	/** Anchor definition. */
	struct Anchor: Fragment
	{
		Anchor (StringRef name)
		: Fragment (kAnchor, nullptr, false)
		{
			argument = name;
			argument.share ();
		}
	};

	/** Link to local anchor. */
	struct FragmentLink: Fragment
	{
		FragmentLink (StringRef anchorName, StringRef content, bool encode = true)
		: Fragment (kFragmentLink, content, encode)
		{
			argument = anchorName;
			argument.share ();
		}
	};

	/** External link. */
	struct Link: Fragment
	{
		Link (StringRef url, StringRef content, bool encode = true)
		: Fragment (kLink, content, encode)
		{
			argument = url;
			argument.share ();
		}
	};

	/** Emphasis. */
	struct Emphasis: Fragment
	{
		Emphasis (StringRef content, bool encode = true)
		: Fragment (kEmphasis, content, encode)
		{}
	};

	/** Strong. */
	struct Strong: Fragment
	{
		Strong (StringRef content, bool encode = true)
		: Fragment (kStrong, content, encode)
		{}
	};

	/** Bold. */
	struct Bold: Fragment
	{
		Bold (StringRef content, bool encode = true)
		: Fragment (kBold, content, encode)
		{}
	};

	/** Italic. */
	struct Italic: Fragment
	{
		Italic (StringRef content, bool encode = true)
		: Fragment (kItalic, content, encode)
		{}
	};

	/** Underline. */
	struct Underline: Fragment
	{
		Underline (StringRef content, bool encode = true)
		: Fragment (kUnderline, content, encode)
		{}
	};

	/** Superscript. */
	struct Superscript: Fragment
	{
		Superscript (StringRef content, bool encode = true)
		: Fragment (kSuperscript, content, encode)
		{}
	};

	/** Subscript. */
	struct Subscript: Fragment
	{
		Subscript (StringRef content, bool encode = true)
		: Fragment (kSubscript, content, encode)
		{}
	};

	/** Font color. */
	struct FontColor: Fragment
	{
		FontColor (StringRef content, StringRef color, bool encode = true)
		: Fragment (kFontColor, content, encode)
		{
			argument = color;
			argument.share ();
		}
	};

	/** Font size. */
	struct FontSize: Fragment
	{
		FontSize (StringRef content, StringRef size, bool encode = true)
		: Fragment (kFontSize, content, encode)
		{
			argument = size;
			argument.share ();
		}
	};

	/** Style span. */
	struct StyleSpan: Fragment
	{
		StyleSpan (StringRef content, StringRef styleName, bool encode = true)
		: Fragment (kStyleSpan, content, encode)
		{
			argument = styleName;
			argument.share ();
		}
	};
}

/** Text Fragment Definition. */
typedef Text::Fragment TextFragment;

/**	@} */

//************************************************************************************************
// ITextBuilder
/**	\ingroup ccl_text */
//************************************************************************************************

interface ITextBuilder: IUnknown
{
	/** Print given text fragment to string. */
	virtual tresult CCL_API printFragment (String& result, const TextFragment& fragment) = 0;

	/** Create table (optional, if supported by underlying format). */
	virtual ITextTable* CCL_API createTable () = 0;

	DECLARE_IID (ITextBuilder)
};

DEFINE_IID (ITextBuilder, 0xac9b00b4, 0x866c, 0x4001, 0x8c, 0xb8, 0x74, 0xc2, 0x39, 0xd8, 0x26, 0xa7)

//************************************************************************************************
// ITextTable
/**	\ingroup ccl_text */
//************************************************************************************************

interface ITextTable: IUnknown
{
	/** Table cell. */
	interface ICell: IUnknown
	{
		/** Set cell content. */
		virtual void CCL_API setContent (const TextFragment& fragment) = 0;

		/** Get cell content. */
		virtual StringRef CCL_API getContent () const = 0;
		
		DECLARE_IID (ICell)
	};

	/** Table row. */
	interface IRow: IUnknown
	{
		/** Access cell by index. */
		virtual ICell& CCL_API getCell (int column) = 0;

		ICell& operator [] (int column) { return getCell (column); }

		DECLARE_IID (IRow)
	};
	
	/** Construct table with given number of rows/columns. */
	virtual tresult CCL_API construct (int rowCount, int columnCount) = 0;

	/** Get size of table. */
	virtual void CCL_API getSize (int& rowCount, int& columnCount) const = 0;

	/** Set title of table. */
	virtual void CCL_API setTitle (const TextFragment& fragment) = 0;

	/** Get title of table. */
	virtual StringRef CCL_API getTitle () const = 0;

	/** Access row by index. */
	virtual IRow& CCL_API getRow (int row) = 0;

	IRow& operator [] (int row) { return getRow (row); }

	DECLARE_IID (ITextTable)
};

DEFINE_IID (ITextTable, 0xb9d7d6ab, 0x7a70, 0x48b4, 0x8e, 0x47, 0xfd, 0x9f, 0x12, 0x71, 0x46, 0x94)
DEFINE_IID (ITextTable::ICell, 0x82db20bb, 0x3d39, 0x4b82, 0x91, 0xd3, 0x7f, 0x1d, 0xf, 0xbe, 0x95, 0xdc)
DEFINE_IID (ITextTable::IRow, 0x70ef9eed, 0xb02, 0x4943, 0x93, 0x7e, 0x9e, 0xc, 0xa5, 0x5f, 0xa1, 0x6e)

//************************************************************************************************
// TextBlock
/**	\ingroup ccl_text */
//************************************************************************************************

class TextBlock
{
public:
	TextBlock (ITextBuilder* builder) ///< takes ownership!
	: builder (builder)
	{}

	TextBlock& operator << (const TextFragment& fragment)
	{
		String result;
		builder->printFragment (result, fragment);
		text.append (result);
		return *this;
	}

	TextBlock& operator << (const TextBlock& block)
	{
		text.append (block.text);
		return *this;
	}

	StringRef asString () const { return text; }
	operator StringRef () const { return text; }
	
	ITextBuilder* getBuilder ()  { return builder; }
	ITextBuilder* operator -> () { return builder; }

protected:
	String text;
	AutoPtr<ITextBuilder> builder;
};

namespace Text
{
	/** Sub block. */
	struct SubBlock: Plain
	{
		SubBlock (const TextBlock& block)
		: Plain (block, false) // text block is already markup-encoded
		{}
	};
}

} // namespace CCL

#endif // _ccl_itextbuilder_h
