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
// Filename    : ccl/text/writer/htmlwriter.cpp
// Description : HTML Writer
//
//************************************************************************************************

#include "ccl/text/writer/htmlwriter.h"

#include "ccl/text/xml/xmlentities.h"
#include "ccl/text/xml/xmlstringdict.h"

namespace CCL {

//************************************************************************************************
// HtmlEntities
//************************************************************************************************

// TODO: separate list of HTML entities!
typedef XmlEntities HtmlEntities;
typedef XmlEncodings HtmlEncodings;

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// HtmlWriter
//************************************************************************************************

HtmlWriter::HtmlWriter ()
: SuperClass (NEW HtmlEntities) 
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API HtmlWriter::beginDocument (IStream& stream, TextEncoding encoding)
{
	if(encoding == Text::kUnknownEncoding)
		encoding = Text::kUTF16;

	const char* htmlEncoding = HtmlEncodings::getEncoding (encoding);
	ASSERT (htmlEncoding != nullptr)
	if(htmlEncoding == nullptr)
		return kResultInvalidArgument; // encoding not supported!

	// let superclass create the streamer
	tresult tr = SuperClass::beginDocument (stream, encoding);
	if(tr != kResultOk)
		return tr;

	tr = writeDocType ("html", "-//W3C//DTD HTML 4.01 Transitional//EN", nullptr, nullptr);
	if(tr != kResultOk)
		return tr;

	// <meta http-equiv="content-type" content="text/html; charset=ISO-8859-1">
	String contentType;
	contentType << "text/html; charset=" << htmlEncoding;
	pushMetaElement (String ("content-type"), contentType, true);

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITextBuilder* CCL_API HtmlWriter::createTextBuilder ()
{
	return NEW HtmlBuilder (lineFormat, NEW HtmlEntities);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API HtmlWriter::pushMetaElement (StringRef name, StringRef content, tbool isHttpEquiv)
{
	metaElements.add (MetaElement (name, content, isHttpEquiv != 0));
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API HtmlWriter::pushStyleElement (StringRef content)
{
	styleElement = content;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API HtmlWriter::writeHead (StringRef title)
{
	startElement (String (HtmlTags::kHead));

	String titleMarkup = getIndent ();
	titleMarkup << "<" << HtmlTags::kTitle << ">" << encodeEntities (title) << "</" << HtmlTags::kTitle << ">";
	writeMarkup (titleMarkup, true);

	VectorForEach (metaElements, MetaElement, me)
		XmlStringDictionary attr;
		if(me.httpEquiv)
			attr.appendEntry (String ("http-equiv"), me.name);
		else
			attr.appendEntry (String ("name"), me.name);
		attr.appendEntry (String ("content"), me.content);

		startElement (String (HtmlTags::kMeta), &attr);
		decIndent ();
	EndFor

	if(!styleElement.isEmpty ())
	{
		String styleMarkup = getIndent ();
		styleMarkup << "<" << HtmlTags::kStyle << ">" << styleElement << "</" << HtmlTags::kStyle << ">";
		writeMarkup (styleMarkup, true);
	}

	return endElement (String (HtmlTags::kHead));
}

//************************************************************************************************
// HtmlBuilder
//************************************************************************************************

HtmlBuilder::HtmlBuilder (TextLineFormat lineFormat, MarkupEncoder* encoder)
: TextBuilder (lineFormat, encoder)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API HtmlBuilder::printFragment (String& result, const TextFragment& fragment)
{
	result.empty ();
	tresult tr = kResultOk;

	switch(fragment.nodeType)
	{
	case Text::kHeading :
		{
			int level = ccl_bound<int> (fragment.argument.asInt (), Text::kH1, Text::kMaxHeadingLevel);
			result << "<h" << level << ">" << unpack (fragment) << "</h" << level << ">" << getLineEnd ();
		}
		break;

	case Text::kParagraph :
		result << "<p>" << getLineEnd ();
		result << unpack (fragment);
		result << "</p>" << getLineEnd ();
		break;

	case Text::kBlockQuote :
		result << "<blockquote>" << getLineEnd ();
		result << unpack (fragment);
		result << "</blockquote>" << getLineEnd ();
		break;

	case Text::kPlainText :
		result << unpack (fragment);
		break;

	case Text::kSoftBreak :
		result << getLineEnd ();
		break;

	case Text::kLineBreak :
		result << "<br>" << getLineEnd ();
		break;
		
	case Text::kHorizontalLine :
		result << "<hr>" << getLineEnd ();
		break;

	case Text::kStrong :
		result << "<strong>" << unpack (fragment) << "</strong>";
		break;

	case Text::kBold :
		result << "<b>" << unpack (fragment) << "</b>";
		break;
	
	case Text::kEmphasis :
		result << "<em>" << unpack (fragment) << "</em>";
		break;

	case Text::kItalic :
		result << "<i>" << unpack (fragment) << "</i>";
		break;

	case Text::kUnderline :
		result << "<u>" << unpack (fragment) << "</u>";
		break;

	case Text::kSubscript :
		result << "<sub>" << unpack (fragment) << "</sub>";
		break;

	case Text::kSuperscript :
		result << "<sup>" << unpack (fragment) << "</sup>";
		break;

	case Text::kFontColor :
		result << "<font color=\"" << VariantString (fragment.argument) << "\">" << unpack (fragment) << "</font>";
		break;

	case Text::kFontSize :
		result << "<font size=\"" << VariantString (fragment.argument) << "\">" << unpack (fragment) << "</font>";
		break;

	case Text::kStyleSpan :
		result << "<span style=\"" << fragment.argument.asString () << "\">" << unpack (fragment) << "</span>";
		break;

	case Text::kAnchor :
		result << "<a name=\"" << fragment.argument.asString () << "\">" << getLineEnd ();
		break;

	case Text::kFragmentLink :
		result << "<a href=\"#" << fragment.argument.asString () << "\">";
		result << unpack (fragment) << "</a>" << getLineEnd ();
		break;

	case Text::kLink :
		result << "<a href=\"" << fragment.argument.asString () << "\">";
		result << unpack (fragment) << "</a>" << getLineEnd ();
		break;

	case Text::kListBegin :
		if(fragment.argument.asInt () == Text::kOrderedList)
			result << "<ol>";
		else
			result << "<ul>";
		result << getLineEnd ();
		break;

	case Text::kListEnd :
		if(fragment.argument.asInt () == Text::kOrderedList)
			result << "</ol>";
		else
			result << "</ul>";
		result << getLineEnd ();
		break;

	case Text::kListItem :
		result << "<li>" << unpack (fragment) << "</li>" << getLineEnd ();
		break;

	case Text::kTable :
		if(UnknownPtr<ITextTable> table = fragment.argument.asUnknown ())
		{
			int rowCount = 0, columnCount = 0;
			table->getSize (rowCount, columnCount);

			result << "<table>" << getLineEnd ();

			if(!table->getTitle ().isEmpty ())
				result << "<caption>" << table->getTitle () << "</caption>" << getLineEnd ();

			for(int row = 0; row < rowCount; row++)
			{
				ITextTable::IRow& tableRow = table->getRow (row);
				result << "<tr>" << getLineEnd ();

				for(int column = 0; column < columnCount; column++)
				{
					ITextTable::ICell& cell = tableRow.getCell (column);
					result << "<td>";
					result << cell.getContent ();
					result << "</td>" << getLineEnd ();
				}

				result << "</tr>" << getLineEnd ();
			}
			result << "</table>" << getLineEnd ();
		}
		break;

	case Text::kCodeBlock :
		{
			result << "<pre><code>" << getLineEnd ();
			result << unpack (fragment) << getLineEnd ();
			result << "</code></pre>" << getLineEnd ();
		}
		break;

	case Text::kCodeInline :
		result << "<code>" << unpack (fragment) << "</code>";
		break;

	default :
		CCL_DEBUGGER ("Unsupported text node type!\n")
		tr = kResultInvalidArgument;
		break;
	}
	return tr;
}
