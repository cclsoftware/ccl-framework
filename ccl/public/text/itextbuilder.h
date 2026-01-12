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

#include "ccl/public/text/cclstring.h"

namespace CCL {

interface ITextTable;

/**	\addtogroup ccl_text 
@{ */

//////////////////////////////////////////////////////////////////////////////////////////////////
// Text Builder Definitions
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Text 
{

	/** Heading level. */
	enum HeadingLevels
	{
		kH1 = 1,
		kH2,
		kH3
	};

	/** Text decoration. */
	enum Decorations
	{
		kBold = 1<<0,
		kItalic = 1<<1,
		kUnderline = 1<<2
	};

	/** List type. */
	enum ListTypes
	{
		kOrdered,
		kUnordered
	};

	/** Text chunk type. */
	enum ChunkTypes
	{
		kPlainText,
		kLineBreak,
		kHeading,
		kDecoration,
		kAnchor,
		kLink,
		kURL,
		kParagraph,
		kListItem,
		kListBegin,
		kListEnd,
		kTable,
		kHorizontalLine,
		
		kLastChunk = 1000
	};

	/** Basic text chunk. */
	struct Chunk
	{
		int chunkType;
		String content;
		tbool encode;

		Chunk (int chunkType, StringRef content, bool encode)
		: chunkType (chunkType),
		  content (content),
		  encode (encode)
		{}
	};

	/** Plain text. */
	struct Plain: Chunk
	{
		Plain (StringRef content, bool encode = true)
		: Chunk (kPlainText, content, encode)
		{}
	};

	/** Line break. */
	struct Break: Chunk
	{
		Break ()
		: Chunk (kLineBreak, nullptr, false)
		{}
	};

	/** Horizontal Line. */
	struct HorizontalLine: Chunk
	{
		HorizontalLine ()
		: Chunk (kHorizontalLine, nullptr, false)
		{}
	};

	/** Heading chunk. */
	struct Heading: Chunk
	{
		int level;

		Heading (int level, StringRef content, bool encode = true)
		: Chunk (kHeading, content, encode),
		  level (level)
		{}
	};

	/** Decoration chunk. */
	struct Decoration: Chunk
	{
		int decoration;

		Decoration (int decoration, StringRef content, bool encode = true)
		: Chunk (kDecoration, content, encode),
		  decoration (decoration)
		{}
	};

	/** Anchor definition. */
	struct Anchor: Chunk
	{
		String name;

		Anchor (StringRef name)
		: Chunk (kAnchor, nullptr, false),
		  name (name)
		{}
	};

	/** Link to local anchor. */
	struct Link: Chunk
	{
		String anchorName;

		Link (StringRef anchorName, StringRef content, bool encode = true)
		: Chunk (kLink, content, encode),
		  anchorName (anchorName)
		{}
	};

	/** Link to URL. */
	struct URL: Chunk
	{
		String url;

		URL (StringRef url, StringRef content, bool encode = true)
		: Chunk (kURL, content, encode),
		  url (url)
		{}
	};

	/** Paragraph. */
	struct Paragraph: Chunk
	{
		Paragraph (StringRef content, bool encode = true)
		: Chunk (kParagraph, content, encode)
		{}
	};

	/** List item. */
	struct ListItem: Chunk
	{
		int listType;

		ListItem (int listType, StringRef content, bool encode = true)
		: Chunk (kListItem, content, encode),
		  listType (listType) 
		{}
	};

	/** List begin. */
	struct ListBegin: ListItem
	{
		ListBegin (int listType)
		: ListItem (listType, nullptr, false)
		{ chunkType = kListBegin; }
	};

	/** List end. */
	struct ListEnd: ListItem
	{
		ListEnd (int listType)
		: ListItem (listType, nullptr, false)
		{ chunkType = kListEnd; }
	};

	/** Table. */
	struct Table: Chunk
	{
		ITextTable* table;

		Table (ITextTable* table)
		: Chunk (kTable, nullptr, false),
		  table (table)
		{}
	};
}

/**	@} */

//************************************************************************************************
// ITextBuilder
/**	\ingroup ccl_text */
//************************************************************************************************

interface ITextBuilder: IUnknown
{
	/** Create empty table. */
	virtual ITextTable* CCL_API createTable () = 0;

	/** Print given chunk to string. */
	virtual tresult CCL_API printChunk (String& result, const Text::Chunk& chunk) = 0;

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
		virtual void CCL_API setContent (const Text::Chunk& chunk) = 0;

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
	virtual void CCL_API setTitle (const Text::Chunk& chunk) = 0;

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
	TextBlock (ITextBuilder* builder)
	: builder (builder)
	{}

	TextBlock& operator << (const Text::Chunk& chunk)
	{
		String result;
		builder->printChunk (result, chunk);
		text.append (result);
		return *this;
	}

	TextBlock& operator << (const TextBlock& block)
	{
		text.append (block.text);
		return *this;
	}

	operator StringRef () const { return text; }
	
	ITextBuilder* getBuilder ()  { return builder; }
	ITextBuilder* operator -> () { return builder; }

protected:
	String text;
	SharedPtr<ITextBuilder> builder;
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
