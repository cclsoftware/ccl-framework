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
// Filename    : ccl/text/writer/textbuilder.cpp
// Description : Text Builder
//
//************************************************************************************************

#include "ccl/text/writer/textbuilder.h"
#include "ccl/text/writer/markupencoder.h"

using namespace CCL;

//************************************************************************************************
// TextBuilder
//************************************************************************************************

TextBuilder::TextBuilder (TextLineFormat lineFormat, MarkupEncoder* encoder)
: lineFormat (lineFormat),
  encoder (encoder)
{
	ASSERT (encoder != nullptr)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TextBuilder::~TextBuilder ()
{
	encoder->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String TextBuilder::getLineEnd () const
{
	return String::getLineEnd (lineFormat);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String TextBuilder::unpack (const Text::Chunk& chunk) const
{
	if(chunk.encode)
		return encoder->encode (chunk.content);
	else
		return chunk.content;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITextTable* CCL_API TextBuilder::createTable ()
{
	return NEW TextTable (this);
}

//************************************************************************************************
// TextTable
//************************************************************************************************

TextTable::TextTable (TextBuilder* builder)
: builder (builder)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

TextTable::~TextTable ()
{
	removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextTable::removeAll ()
{
	VectorForEach (rows, Row*, row)
		row->release ();
	EndFor
	rows.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API TextTable::construct (int rowCount, int columnCount)
{
	removeAll ();
	for(int i = 0; i < rowCount; i++)
		rows.add (NEW Row (this, columnCount));
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TextTable::getSize (int& rowCount, int& columnCount) const
{
	rowCount = rows.count ();
	columnCount = rows.count () > 0 ? rows.at (0)->getCellCount () : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TextTable::setTitle (const Text::Chunk& chunk)
{
	title = builder->unpack (chunk);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API TextTable::getTitle () const
{
	return title;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TextTable::IRow& CCL_API TextTable::getRow (int row)
{
	if(Row* r = rows.at (row))
		return *r;

	CCL_DEBUGGER ("Invalid row!")
	static Row errorRow (nullptr, 0);
	return errorRow;
}

//************************************************************************************************
// TextTable::Cell
//************************************************************************************************

TextTable::Cell::Cell (TextTable* table)
: table (table)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TextTable::Cell::setContent (const Text::Chunk& chunk)
{
	ASSERT (table)
	if(!table)
		return;

	table->builder->printChunk (content, chunk);
	//was: content = table->builder->unpack (chunk);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API TextTable::Cell::getContent () const
{
	return content;
}

//************************************************************************************************
// TextTable::Row
//************************************************************************************************

TextTable::Row::Row (TextTable* table, int cellCount)
: table (table)
{
	for(int i = 0; i < cellCount; i++)
		cells.add (NEW Cell (table));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TextTable::Row::~Row ()
{
	VectorForEach (cells, Cell*, cell)
		cell->release ();
	EndFor
	cells.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TextTable::ICell& CCL_API TextTable::Row::getCell (int column)
{
	if(Cell* cell = cells.at (column))
		return *cell;

	CCL_DEBUGGER ("Invalid column!")
	static Cell errorCell (nullptr);
	return errorCell;
}
