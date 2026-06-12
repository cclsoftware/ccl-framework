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
// Filename    : ccl/text/writer/plaintextwriter.cpp
// Description : Plain Text Writer
//
//************************************************************************************************

#include "ccl/text/writer/plaintextwriter.h"
#include "ccl/text/writer/markupencoder.h"

using namespace CCL;

//************************************************************************************************
// PlainTextWriter
//************************************************************************************************

PlainTextWriter::PlainTextWriter ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITextBuilder* CCL_API PlainTextWriter::createTextBuilder ()
{
	return NEW PlainTextBuilder (lineFormat);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PlainTextWriter::setDocumentLineFormat (TextLineFormat lineFormat)
{
	TextWriter::setDocumentLineFormat (lineFormat);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PlainTextWriter::beginDocument (IStream& stream, TextEncoding encoding)
{
	return TextWriter::beginDocument (stream, encoding);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PlainTextWriter::endDocument ()
{
	return TextWriter::endDocument ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PlainTextWriter::writeLine (StringRef text)
{
	return TextWriter::writeLine (text);
}

//************************************************************************************************
// PlainTextBuilder
//************************************************************************************************

const String PlainTextBuilder::kListBulletString (Text::kUTF8, u8"\u2022");

//////////////////////////////////////////////////////////////////////////////////////////////////

PlainTextBuilder::PlainTextBuilder (TextLineFormat lineFormat)
: TextBuilder (lineFormat, NEW PlainMarkupEncoder),
  listLevel (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PlainTextBuilder::printFragment (String& result, const TextFragment& fragment)
{
	result.empty ();
	tresult tr = kResultOk;

	switch(fragment.nodeType)
	{
	case Text::kHeading :
		result << unpack (fragment) << getLineEnd () << getLineEnd ();
		break;

	case Text::kPlainText :
		result << unpack (fragment);
		break;

	case Text::kLineBreak :
		result << getLineEnd ();
		break;

	case Text::kAnchor :
		break;

	case Text::kFragmentLink :
	case Text::kLink :
		result << unpack (fragment);
		break;

	case Text::kParagraph :
		result << unpack (fragment) << getLineEnd () << getLineEnd ();
		break;

	case Text::kListItem :
		if(listLevel > 0)
			result.append ("\t", listLevel);

		result << kListBulletString << " " << unpack (fragment) << getLineEnd ();
		break;

	case Text::kListBegin :
		listLevel++;
		break;

	case Text::kListEnd :
		listLevel--;
		result << getLineEnd ();
		break;

	case Text::kTable :
		if(UnknownPtr<ITextTable> table = fragment.argument.asUnknown ())
		{
			int rowCount = 0, columnCount = 0;
			table->getSize (rowCount, columnCount);

			for(int row = 0; row < rowCount; row++)
			{
				ITextTable::IRow& tableRow = table->getRow (row);
				for(int column = 0; column < columnCount; column++)
				{
					if(column > 0)
						result << '\t';

					ITextTable::ICell& cell = tableRow.getCell (column);
					result << cell.getContent ();
				}
				result << getLineEnd ();
			}
		}
		break;

	default :
		CCL_DEBUGGER ("Unsupported text node type!\n")
		tr = kResultInvalidArgument;
		break;
	}
	return tr;
}
