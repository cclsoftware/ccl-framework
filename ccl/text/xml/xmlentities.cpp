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
// Filename    : ccl/text/xml/xmlentities.cpp
// Description : XML Entities
//
//************************************************************************************************

#include "ccl/text/xml/xmlentities.h"

#include "ccl/public/base/cclmacros.h"
#include "ccl/public/base/debug.h"

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////
// XML Entity Definitions
//////////////////////////////////////////////////////////////////////////////////////////////////

// HTML entities
// http://en.wikipedia.org/wiki/List_of_XML_and_HTML_character_entity_references
// http://www.w3.org/TR/xhtml1/DTD/xhtml-lat1.ent

struct XmlEntity
{
	uchar character;
	const char* entity;
};

static const XmlEntity standard_entities[] =
{
	{'"',		"quot"},		// quotation mark
	{'&',		"amp"},			// ampersand
	{'\'',		"apos"},		// apostrophe
	{'<',		"lt"},			// less-than sign
	{'>',		"gt"}			// greater-than sign
};

static const XmlEntity built_in_extra_entities[] =
{
	{'\t',		"tab"},			// tab
	{'\n',		"nl"},			// new line
	{'\r',		"cr"},			// carriage return
	{0x00A0,	"nbsp"},		// no-break space
	{0x00A9,	"copy"},		// copyright sign
	{0x00AE,	"reg"},			// registered sign 
	{0x20AC,	"euro"},		// euro sign
	{0x2122,	"trade"},		// trademark sign
	{0x2764,	"heart"}		// heart symbol
};

static const XmlEntity linebreak_entities_encode[] =
{
	// line breaks only survice parsing with expat when encoded like this (expat converts &nl; and &cr; to space)
	// decoding seems to happen already in expat
	{'\n',		"#10"},			// new line
	{'\r',		"#13"},			// carriage return
	{'\t',		"#9"}			// tab
};

//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename Writer> 
bool encodeChar (Writer& writer, String& string, uchar c, const XmlEntity entites[], int count)
{
	bool encoded = false;
	for(int i = 0; i < count; i++)
	{
		const XmlEntity& e = entites[i];
		if(c == e.character)
		{
			writer.flush ();
			string.appendASCII ("&");
			string.appendASCII (e.entity);
			string.appendASCII (";");
			encoded = true;
			break;
		}
	}
	return encoded;
}

template <typename Writer> 
bool encodeChar (Writer& writer, String& string, uchar c)
{
	return	encodeChar (writer, string, c, standard_entities, ARRAY_COUNT (standard_entities)) ||
			encodeChar (writer, string, c, linebreak_entities_encode, ARRAY_COUNT (linebreak_entities_encode)) ||
			encodeChar (writer, string, c, built_in_extra_entities, ARRAY_COUNT (built_in_extra_entities));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static bool decodeEntity (uchar& c, StringID name, const XmlEntity entites[], int count)
{
	bool decoded = false;
	for(int i = 0; i < count; i++)
	{
		const XmlEntity& e = entites[i];
		if(name == e.entity)
		{
			c = e.character;
			decoded = true;
			break;
		}
	}
	return decoded;
}

static bool decodeEntity (uchar& c, StringID name)
{
	return	decodeEntity (c, name, standard_entities, ARRAY_COUNT (standard_entities)) ||
			decodeEntity (c, name, built_in_extra_entities, ARRAY_COUNT (built_in_extra_entities));
}

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// XmlEntities
//************************************************************************************************

MutableCString XmlEntities::makeBuiltInDTD (TextLineFormat lineFormat)
{
	MutableCString text;
	CStringRef lineEnd = CString::getLineEnd (lineFormat);
	for(int i = 0; i < ARRAY_COUNT (built_in_extra_entities); i++)
	{
		const XmlEntity& e = built_in_extra_entities[i];

		// <!ENTITY nbsp "&#160;">
		text += "<!ENTITY ";
		text += e.entity;
		text += " \"&#";
		text.appendFormat ("%d", (int)e.character);
		text += ";\">";
		text += lineEnd;
	}
	return text;
}


//////////////////////////////////////////////////////////////////////////////////////////////////

String XmlEntities::encode (StringRef text)
{
	String result;
	StringWriter<512> writer (result);

	StringChars chars (text);
	int length = text.length ();
	for(int i = 0; i < length; i++)
	{
		uchar c = chars[i];
		if(c && encodeChar (writer, result, c) == false)    // Can happen with CFStrings
		{
			if(c < 32) // ASCII control character -> illegal in XML, except tab, carriage return and line feed!
			{
				CCL_DEBUGGER ("Illegal character encountered!")
				
				#if 0
				writer.flush ();
				MutableCString entity;
				entity.appendFormat ("&#%d;", (int)c);
				result.appendASCII (entity);
				#endif
			}
			else
				writer.append (c);
		}
	}

	writer.flush ();
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString XmlEntities::encodeToASCII (StringRef text)
{
	String result;
	StringWriter<512> writer (result);

	StringChars chars (text);
	int length = text.length ();
	for(int i = 0; i < length; i++)
	{
		uchar c = chars[i];
		if(encodeChar (writer, result, c) == false)
		{
			if(c > 31 && c < 127) // printable ASCII (except xml entities)
				writer.append (c);
			else
			{
				writer.flush ();
				MutableCString entity;
				entity.appendFormat ("&#%d;", (int)c);
				result.appendASCII (entity);
			}
		}
	}

	writer.flush ();
	return MutableCString (result);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String XmlEntities::decode (StringRef text)
{
	String result;
	StringWriter<512> writer (result);

	StringChars chars (text);
	int length = text.length ();
	for(int i = 0; i < length; i++)
	{
		uchar c = chars[i];
		if(c != '&')
		{
			writer.append (c);
			continue;
		}

		// Special case: do not decode '& ' and '&[non-ascii]'.
		// Peek next char to preserve loop index.
		uchar next = chars[i + 1];
		if(next == ' ' || !isascii (next))
		{
			writer.append ('&');
			continue;
		}

		int j = i;
		MutableCString entity;
		bool terminated = false;
		while(++j < length)
		{
			c = chars[j];
			if(c == ';')
			{
				terminated = true;
				break;
			}
			entity += c;
		}
		if(!terminated)
		{
			writer.append ('&');
			continue;
		}

		// Attempt to decode from number or name encoded format.
		// For sscanf, process terminated entities only as strings such as '#38test'
		// cause false positives, resulting in 'test' not being added to the output.
		bool decoded = false;
		if(entity.startsWith ("#"))
		{
			int value = 0;
			if(::sscanf (entity.str () + 1, "%d", &value) == 1)
			{
				c = (uchar)value;
				decoded = true;
			}
		}
		else
			decoded = decodeEntity (c, entity);

		// Append decoded char and skip entity substring
		// or append original string after '&' as is.
		if(decoded)
		{
			writer.append (c);
			i = j;
		}
		else
			writer.append ('&');
	}

	writer.flush ();
	return result;
}

//************************************************************************************************
// XmlEncodings
//************************************************************************************************

const char* XmlEncodings::getEncoding (TextEncoding encoding)
{
	ASSERT (encoding != Text::kUnknownEncoding)

	static const struct { TextEncoding encoding; const char* xml; } knownXmlEncodings[] =
	{
		{Text::kASCII,		"US-ASCII"},
		{Text::kISOLatin1,	"ISO-8859-1"},
		{Text::kUTF8,		"UTF-8"},
		{Text::kUTF16LE,	"UTF-16"},
		{Text::kUTF16BE,	"UTF-16"}
	};

	const char* xmlEncoding = nullptr;
	for(int i = 0; i < ARRAY_COUNT (knownXmlEncodings); i++)
		if(encoding == knownXmlEncodings[i].encoding)
		{
			xmlEncoding = knownXmlEncodings[i].xml;
			break;
		}

	return xmlEncoding;
}
