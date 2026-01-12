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
// Filename    : ccl/base/storage/urlencoder.cpp
// Description : Url Encoder
//
//************************************************************************************************

#include "ccl/base/storage/urlencoder.h"

using namespace CCL;

//************************************************************************************************
// UrlEncoder
//************************************************************************************************

UrlEncoder::UrlEncoder (Scheme scheme, TextEncoding textEncoding)
: scheme (scheme),
  textEncoding (textEncoding)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Encoding
//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString UrlEncoder::encode (CStringRef string)
{
	MutableCString result;
	CStringWriter<kBufferSize> writer (result);
	Core::URLEncoding::encode (writer, string, static_cast<Core::URLEncoding::Scheme> (scheme));
	writer.flush ();
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String UrlEncoder::encode (StringRef string)
{
	return String (encode (MutableCString (string, textEncoding)));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString UrlEncoder::encodePathComponents (CStringRef string)
{
	return MutableCString (encodePathComponents (String (textEncoding, string)), textEncoding); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String UrlEncoder::encodePathComponents (StringRef string)
{
	return convertPathComponents<&UrlEncoder::encode> (string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class Text, class Dictionary>
Text UrlEncoder::encode (const Dictionary& parameters)
{
	// See http://www.w3.org/TR/html4/interact/forms.html#h-17.13.4.1

	Text result;
	int count = parameters.countEntries ();
	for(int i = 0; i < count; i++)
	{
		Text key (encode (parameters.getKeyAt (i)));
		Text value (encode (parameters.getValueAt (i)));

		if(i > 0)
			result.append ("&");

		ASSERT (!key.isEmpty ())
		result.append (key);

		// The equals sign may be omitted if the value is an empty string.
		if(!value.isEmpty ())
		{
			result.append ("=");
			result.append (value);
		}
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString UrlEncoder::encode (const ICStringDictionary& parameters)
{
	return encode<MutableCString, ICStringDictionary> (parameters);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String UrlEncoder::encode (const IStringDictionary& parameters)
{
	return encode<String, IStringDictionary> (parameters);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Decoding
//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString UrlEncoder::decode (CStringRef string)
{
	MutableCString result;
	CStringWriter<kBufferSize> writer (result);
	Core::URLEncoding::decode (writer, string);
	writer.flush ();
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String UrlEncoder::decode (StringRef string)
{
	String result;
	result.appendCString (textEncoding, decode (MutableCString (string)));
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString UrlEncoder::decodePathComponents (CStringRef string)
{
	return MutableCString (decodePathComponents (String (textEncoding, string)), textEncoding);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String UrlEncoder::decodePathComponents (StringRef string)
{
	return convertPathComponents<&UrlEncoder::decode> (string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class Dictionary, class TextRef, class Text>
Dictionary& UrlEncoder::decode (Dictionary& parameters, TextRef string)
{
	parameters.removeAll ();

	// http://en.wikipedia.org/wiki/Query_string
	// field1=value1&field2=value2&field3=value3...
	// The series of pairs is separated by the ampersand, '&' (or semicolon, ';' for URLs embedded in HTML)
	// Within each pair, the field name and value are separated by an equals sign, '='.
	// The equals sign may be omitted if the value is an empty string.

	Text prevString;
	uchar prevDelimiter = 0;
	ForEachStringToken (String (string), CCLSTR ("&;="), token)
		Text string = decode (Text (token));
		uchar delimiter = __delimiter;

		bool endOfPair = delimiter == '&' || delimiter == ';' || delimiter == 0;
		if(endOfPair)
		{
			if(prevDelimiter == '=')
			{
				ASSERT (!prevString.isEmpty ())
				parameters.appendEntry (prevString, string);
			}
			else
			{
				ASSERT (!string.isEmpty ())
				parameters.appendEntry (string, nullptr);
			}
		}

		prevString = string;
		prevDelimiter = delimiter;
	EndFor
	return parameters;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStringDictionary& UrlEncoder::decode (IStringDictionary& parameters, StringRef string)
{
	return decode<IStringDictionary, StringRef, String> (parameters, string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ICStringDictionary& UrlEncoder::decode (ICStringDictionary& parameters, CStringRef string)
{
	return decode<ICStringDictionary, CStringRef, MutableCString> (parameters, string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Helpers
//////////////////////////////////////////////////////////////////////////////////////////////////

template<UrlEncoder::ConvertMethod convert>
String UrlEncoder::convertPathComponents (StringRef inPath)
{
	// https://tools.ietf.org/html/rfc3986#section-2.2
	// A subset of the reserved characters (gen-delims) is used as
	// delimiters of the generic URI components
	static const String kGenericDelimiters (":/?#[]@");

	String outPath;
	uchar prevDelimiter = 0;
	ForEachStringToken (inPath, kGenericDelimiters, token)
		String converted = (this->*convert) (token);
		if(!outPath.isEmpty ())
		{
			uchar delimiterString[2] = {prevDelimiter, 0};
			outPath << delimiterString;
		}
		outPath << converted;
		prevDelimiter = __delimiter;
	EndFor

	// handle leading + trailing slash
	static const String kPathChar (CCLSTR ("/"));
	if(inPath.startsWith (kPathChar))
		outPath.insert (0, kPathChar);
	if(inPath.endsWith (kPathChar) && inPath.length () > 1)
		outPath << kPathChar;

	return outPath;
}
