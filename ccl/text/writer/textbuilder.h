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
// Filename    : ccl/text/writer/textbuilder.h
// Description : Text Builder
//
//************************************************************************************************

#ifndef _ccl_textbuilder_h
#define _ccl_textbuilder_h

#include "ccl/public/base/unknown.h"

#include "ccl/public/text/itextbuilder.h"

#include "ccl/public/collections/vector.h"

namespace CCL {

class MarkupEncoder;

//************************************************************************************************
// TextBuilder
//************************************************************************************************

class TextBuilder: public Unknown,
				   public ITextBuilder
{
public:
	TextBuilder (TextLineFormat lineFormat, MarkupEncoder* encoder);
	~TextBuilder ();

	PROPERTY_VARIABLE (TextLineFormat, lineFormat, LineFormat)
	String getLineEnd () const;

	String unpack (const Text::Chunk& chunk) const;

	// ITextBuilder
	ITextTable* CCL_API createTable () override;

	CLASS_INTERFACE (ITextBuilder, Unknown)

protected:
	MarkupEncoder* encoder;
};

//************************************************************************************************
// TextTable
//************************************************************************************************

class TextTable: public Unknown,
				 public ITextTable
{
public:
	TextTable (TextBuilder* builder);
	~TextTable ();

	/** Cell implementation. */
	class Cell: public Unknown,
			    public ICell
	{
	public:
		Cell (TextTable* table);
		PROPERTY_POINTER (TextTable, table, Table)
		void CCL_API setContent (const Text::Chunk& chunk) override; ///< [ITextTable::ICell]
		StringRef CCL_API getContent () const override;
		CLASS_INTERFACE (ICell, Unknown)
	protected:
		String content;
	};

	/** Row implementation. */
	class Row: public Unknown,
			   public IRow
	{
	public:
		Row (TextTable* table, int cellCount);
		~Row ();
		PROPERTY_POINTER (TextTable, table, Table)
		int getCellCount () const { return cells.count (); }
		ICell& CCL_API getCell (int column) override; ///< [ITextTable::IRow]
		CLASS_INTERFACE (IRow, Unknown)
	protected:
		Vector<Cell*> cells;
	};

	// ITextTable
	tresult CCL_API construct (int rowCount, int columnCount) override;
	void CCL_API getSize (int& rowCount, int& columnCount) const override;
	void CCL_API setTitle (const Text::Chunk& chunk) override;
	StringRef CCL_API getTitle () const override;
	IRow& CCL_API getRow (int row) override;

	CLASS_INTERFACE (ITextTable, Unknown)

protected:
	friend class Cell;
	SharedPtr<TextBuilder> builder;
	Vector<Row*> rows;
	String title;

	void removeAll ();
};

} // namespace CCL

#endif // _ccl_textbuilder_h
