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
// Filename    : ccl/text/markdown/markdownwriter.cpp
// Description : Markdown Writer
//
//************************************************************************************************

#include "ccl/text/markdown/markdownwriter.h"

#include "ccl/text/writer/markupencoder.h"
#include "ccl/text/xml/xmlentities.h"

using namespace CCL;

//************************************************************************************************
// MarkdownWriter
//************************************************************************************************

MarkdownWriter::MarkdownWriter ()
: MarkupWriter (NEW XmlEntities)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITextBuilder* CCL_API MarkdownWriter::createTextBuilder ()
{
	return NEW MarkdownBuilder (lineFormat, return_shared (encoder));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MarkdownWriter::beginDocument (IStream& stream, TextEncoding encoding)
{
	return TextWriter::beginDocument (stream, encoding);
}

//************************************************************************************************
// MarkdownBuilder
//************************************************************************************************

MarkdownBuilder::MarkdownBuilder (TextLineFormat lineFormat, MarkupEncoder* encoder)
: TextBuilder (lineFormat, encoder)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MarkdownBuilder::printFragment (String& result, const TextFragment& fragment)
{
	result.empty ();
	tresult tr = kResultOk;

	switch(fragment.nodeType)
	{
	case Text::kHeading :
		{
			int level = ccl_bound<int> (fragment.argument.asInt (), Text::kH1, Text::kMaxHeadingLevel);
			result << String ("#", level) << " " << unpack (fragment) << getLineEnd ();
		}
		break;

	case Text::kPlainText :
		result << unpack (fragment);
		break;

	case Text::kLineBreak :
		result << CCLSTR ("\\") << getLineEnd ();
		break;
		
	case Text::kHorizontalLine :
		result << getLineEnd () << "---" << getLineEnd ();
		break;

	case Text::kStrong :
	case Text::kBold :
		result << "**" << unpack (fragment) << "**";
		break;

	case Text::kEmphasis :
	case Text::kItalic :
		result << "*" << unpack (fragment) << "*";
		break;

	case Text::kAnchor :
		result << "[" << fragment.argument.asString () << "]: " << getLineEnd ();
		break;

	case Text::kFragmentLink :
		result << "[" << unpack (fragment) << "](" << fragment.argument.asString () << ")";
		break;

	case Text::kLink :
		result << "[" << unpack (fragment) << "](" << fragment.argument.asString () << ")";
		break;

	case Text::kParagraph :
		result << getLineEnd () << getLineEnd ();
		result << unpack (fragment);
		result << getLineEnd () << getLineEnd ();
		break;

	case Text::kListItem :
		if(fragment.argument.asInt () == Text::kOrderedList)
		{
			int& number = listCounters.last ();
			result << number << ". " << unpack (fragment) << getLineEnd ();
			number++;
		}
		else
			result << "- " << unpack (fragment) << getLineEnd ();
		break;

	case Text::kListBegin :
		if(fragment.argument.asInt () == Text::kOrderedList)
			listCounters.add (1);
		result << getLineEnd ();
		break;

	case Text::kListEnd :
		if(fragment.argument.asInt () == Text::kOrderedList)
			listCounters.removeLast ();
		result << getLineEnd ();
		break;

	case Text::kTable :
		CCL_NOT_IMPL ("Markdown: Text::kTable not implemented\n")
		break;

	default :
		CCL_DEBUGGER ("Unsupported text node type!\n")
		tr = kResultInvalidArgument;
		break;
	}
	return tr;
}
