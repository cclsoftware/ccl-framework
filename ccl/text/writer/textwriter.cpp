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
// Filename    : ccl/text/writer/textwriter.cpp
// Description : Text Writer
//
//************************************************************************************************

#include "ccl/text/writer/textwriter.h"

#include "ccl/text/writer/markupencoder.h"

using namespace CCL;

//************************************************************************************************
// TextWriter
//************************************************************************************************

const String TextWriter::strIndent = CCLSTR ("\t");
const String TextWriter::strSpace = CCLSTR (" ");

//////////////////////////////////////////////////////////////////////////////////////////////////

TextWriter::TextWriter ()
: lineFormat (Text::kSystemLineFormat),
  streamer (nullptr),
  indent (0),
  indentDisabled (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

TextWriter::~TextWriter ()
{
	if(streamer)
		streamer->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TextWriter::setDocumentLineFormat (TextLineFormat lineFormat)
{
	setLineFormat (lineFormat);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API TextWriter::beginDocument (IStream& stream, TextEncoding encoding)
{
	ASSERT (streamer == nullptr)
	if(streamer != nullptr)
		return kResultUnexpected;

	streamer = NEW TextStreamer (stream, encoding, lineFormat);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API TextWriter::endDocument ()
{
	if(streamer)
		streamer->release ();
	streamer = nullptr;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API TextWriter::writeLine (StringRef text)
{
	ASSERT (streamer != nullptr)
	if(streamer == nullptr)
		return kResultUnexpected;

	if(indent > 0)
		if(!streamer->writeString (getIndent (), false))
			return kResultFalse;

	if(!streamer->writeString (text, true))
		return kResultFalse;

	return kResultOk;
}

//************************************************************************************************
// MarkupWriter
//************************************************************************************************

MarkupWriter::MarkupWriter (MarkupEncoder* encoder)
: encoder (encoder)
{
	ASSERT (encoder != nullptr)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MarkupWriter::~MarkupWriter ()
{
	encoder->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String MarkupWriter::encodeEntities (StringRef text)
{
	ASSERT (encoder != nullptr)
	return encoder->encode (text);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API MarkupWriter::setDocumentLineFormat (TextLineFormat lineFormat)
{
	TextWriter::setDocumentLineFormat (lineFormat);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MarkupWriter::beginDocument (IStream& stream, TextEncoding encoding)
{
	return TextWriter::beginDocument (stream, encoding);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MarkupWriter::endDocument ()
{
	return TextWriter::endDocument ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MarkupWriter::writeLine (StringRef text)
{
	String encodedText = encodeEntities (text);
	return TextWriter::writeLine (encodedText);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MarkupWriter::writeMarkup (StringRef markup, tbool appendNewline)
{
	ASSERT (streamer != nullptr)
	if(streamer == nullptr)
		return kResultUnexpected;

	if(!streamer->writeString (markup, appendNewline))
		return kResultFalse;

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MarkupWriter::encode (String& result, StringRef text)
{
	ASSERT (encoder != nullptr)
	result = encoder->encode (text);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MarkupWriter::encode (MutableCString& result, StringRef text)
{
	ASSERT (encoder != nullptr)
	result = encoder->encodeToASCII (text);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MarkupWriter::decode (String& result, StringRef text)
{
	ASSERT (encoder != nullptr)
	result = encoder->decode (text);
	return kResultOk;
}

//************************************************************************************************
// SgmlWriter
//************************************************************************************************

SgmlWriter::SgmlWriter (MarkupEncoder* encoder)
: MarkupWriter (encoder)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SgmlWriter::beginDocument (IStream& stream, TextEncoding encoding)
{
	return MarkupWriter::beginDocument (stream, encoding);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SgmlWriter::writeDocType (StringRef name, StringRef pubid, StringRef sysid, StringRef subset)
{
	String docType;
	StringRef lineEnd = String::getLineEnd (lineFormat);
	docType << "<!DOCTYPE " << name << " ";

	if(!pubid.isEmpty ())
	{
		docType << "PUBLIC \"" << pubid << "\"";
		if(!sysid.isEmpty ())
			docType << lineEnd << "  ";
	}

	if(!sysid.isEmpty ())
	{
		if(pubid.isEmpty ())
			docType << "SYSTEM ";
		docType << "\"" << sysid << "\"";
	}

	if(!subset.isEmpty ())
	{
		docType << " [" << lineEnd;
		docType << subset << "]";
	}

	docType << ">";

	return writeMarkup (docType, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SgmlWriter::setShouldIndent (tbool state)
{
	setIndentDisabled (state == 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SgmlWriter::startElement (StringRef name, const IStringDictionary* attributes)
{
	ASSERT (streamer != nullptr)
	if(streamer == nullptr)
		return kResultUnexpected;

	String line = getIndent ();
	line << "<" << name;

	if(attributes)
	{
		if(!streamer->writeString (line, false))
			return kResultFalse;

		if(!writeAttributesString (attributes, name.length () + 1))
			return kResultFalse;

		line.empty ();
	}

	line << ">";
	incIndent ();

	return streamer->writeString (line, true) ? kResultOk : kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SgmlWriter::endElement (StringRef name)
{
	ASSERT (streamer != nullptr)
	if(streamer == nullptr)
		return kResultUnexpected;

	decIndent ();
	String line = getIndent ();
	line << "</" << name << ">";
	return streamer->writeString (line, true) ? kResultOk : kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SgmlWriter::writeElement (StringRef name, StringRef value)
{
	return writeElement (name, nullptr, value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SgmlWriter::writeElement (StringRef name, const IStringDictionary* attributes, StringRef value)
{
	ASSERT (streamer != nullptr)
	if(streamer == nullptr)
		return kResultUnexpected;

	String line = getIndent ();
	line << "<" << name;

	if(attributes)
	{
		if(!streamer->writeString (line, false))
			return kResultFalse;

		if(!writeAttributesString (attributes, name.length () + 1))
			return kResultFalse;

		line.empty ();
	}

	if(!value.isEmpty ())
		line << ">" << encodeEntities (value) << "</" << name << ">";
	else
		line << "/>";

	return streamer->writeString (line, true) ? kResultOk : kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SgmlWriter::writeValue (StringRef value)
{
	ASSERT (streamer != nullptr)
	if(streamer == nullptr)
		return kResultUnexpected;

	String line = getIndent ();
	line << encodeEntities (value);
	return streamer->writeString (line, true) ? kResultOk : kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SgmlWriter::writeComment (StringRef text)
{
	ASSERT (streamer != nullptr)
	if(streamer == nullptr)
		return kResultUnexpected;

	String commentBegin = getIndent ();
	commentBegin << "<!-- ";
	if(!streamer->writeString (commentBegin, false))
		return kResultFalse;

	String encodedText = encodeEntities (text);
	if(!streamer->writeString (encodedText, false))
		return kResultFalse;

	String commentEnd;
	commentEnd << " -->";
	if(!streamer->writeString (commentEnd, true))
		return kResultFalse;

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API SgmlWriter::getCurrentDepth () const
{
	return currentIndent ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SgmlWriter::writeAttributesString (const IStringDictionary* attributes, int offset)
{
	ASSERT (attributes != nullptr)
	if(attributes == nullptr)
		return false;

	String line;
	int count = attributes->countEntries ();
	for(int i = 0; i < count; i++)
	{
		StringRef key = attributes->getKeyAt (i);
		StringRef value = attributes->getValueAt (i);

		line << " " << key << "=\"";
		line << encodeEntities (value);
		line << "\"";

		// break line if too long...
		if(i < count - 1 && getIndent ().length () + offset + line.length () >= kMaxLineLength)
		{
			if(!streamer->writeString (line, true))
				return false;

			line = getIndent ();
			line.append (strSpace, offset);
		}
	}

	return streamer->writeString (line, false);
}
