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
// Filename    : ccl/network/web/webrequest.cpp
// Description : Web Request
//
//************************************************************************************************

#include "ccl/network/web/webrequest.h"

#include "ccl/public/base/istream.h"

#include "ccl/base/storage/urlencoder.h"

using namespace CCL;
using namespace Web;

//************************************************************************************************
// WebHeaderCollection
//************************************************************************************************

DEFINE_CLASS_HIDDEN (WebHeaderCollection, CStringDictionary)

//////////////////////////////////////////////////////////////////////////////////////////////////

WebHeaderCollection::WebHeaderCollection ()
{
	setCaseSensitive (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ICStringDictionary& WebHeaderCollection::getEntries ()
{
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WebHeaderCollection::parseFileName (String& fileName) const
{
	// parse HTTP content-disposition header field parameters according to RFC 5987

	static const String kFileNameKey (CCLSTR ("filename"));
	static const String kFileNameExtendedKey (CCLSTR ("filename*"));
	static const String kUTF8Charset (CCLSTR ("UTF-8"));
	static const String kLatin1Charset (CCLSTR ("ISO-8859-1"));

	String result;
	CStringRef rawValue = lookupValue (Meta::kContentDisposition);
	
	auto getParameter = [&] (StringRef key)
	{
		// parameter = token LWSP "=" LWSP (token / quoted-string)
		// ext-parameter = parmname "*" LWSP "=" LWSP charset  "'" [ language ] "'" value-chars
		
		// start with Latin-1, as every byte sequence is valid; reencode part of the string in UTF-8 later, if necessary
		String entry (Text::kISOLatin1, rawValue);
		AutoPtr<IStringTokenizer> tokenizer (entry.tokenize (CCLSTR (";")));
		if(!tokenizer)
			return false;
			
		uchar delimiter = 0;
		while(!tokenizer->done ())
		{
			StringRef parameter (tokenizer->nextToken (delimiter));
			if(parameter.contains (CCLSTR ("=")) == false)
				continue;
			AutoPtr<IStringTokenizer> paramTokenizer (parameter.tokenize (CCLSTR ("=")));
			if(!paramTokenizer)
				continue;
			String paramName = paramTokenizer->nextToken (delimiter);
			paramName.trimWhitespace ();
			if(paramName.compare (key, Text::kIgnoreCase ) == Text::kEqual)
			{
				if(paramTokenizer->done ())
					continue;
				String value = paramTokenizer->nextToken (delimiter);
				value.trimWhitespace ();
				TextEncoding encoding = Text::kISOLatin1;
				if(paramName.contains (CCLSTR ("*")))
				{
					AutoPtr<IStringTokenizer> charsetTokenizer (parameter.tokenize (CCLSTR ("'"), Text::kPreserveEmptyToken));
					if (!charsetTokenizer)
						return false;
					StringRef charsetPart = charsetTokenizer->nextToken (delimiter);
					TextEncoding encoding = Text::kISOLatin1;
					if(charsetPart.contains (kUTF8Charset, false))
						encoding = Text::kUTF8;
					else if(charsetPart.contains (kLatin1Charset, false) == false)
					{
						// unknown encoding
						ASSERT (0)
					}
					if(charsetTokenizer->done ())
						return false;
					charsetTokenizer->nextToken (delimiter);
					if(charsetTokenizer->done ())
						return false;
					String stringPart = charsetTokenizer->nextToken (delimiter);
					if(encoding != Text::kISOLatin1)
					{
						MutableCString buffer (stringPart, Text::kISOLatin1);
						buffer.toUnicode (stringPart, encoding);
					}
					result = stringPart;
					return true;
				}
				else
				{
					value.replace (CCLSTR ("\""), String::kEmpty);
					result = value;
					return true;
				}
			}
		}
		return false;
	};
	
	bool success = getParameter (kFileNameExtendedKey);
	if(!success)
		success = getParameter (kFileNameKey);

	if(success)
		fileName = UrlEncoder ().decode (result);
	
	return success;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WebHeaderCollection::parseDate (DateTime& date) const
{
	CString result = lookupValue (Meta::kDate);
	if(result.isEmpty ())
		return false;
	
	// Parse date from RFC 7231 (Wed, 01 Jun 2022 16:13:03 GMT)
	// see https://datatracker.ietf.org/doc/html/rfc7231#section-7.1.1.1

	AutoPtr<IStringTokenizer> tokenizer = String (result).tokenize (" :");
	if(!tokenizer.isValid ())
		return false;
	uchar delimiter = 0;
	tokenizer->nextToken (delimiter); // Skip the day of week and ','
	int day = tokenizer->nextToken (delimiter).scanInt ();
	StringRef monthString = tokenizer->nextToken (delimiter);
	int monthNumeric = 0;
	if(monthString == "Jan")
		monthNumeric = 1;
	else if(monthString == "Feb")
		monthNumeric = 2;
	else if(monthString == "Mar")
		monthNumeric = 3;
	else if(monthString == "Apr")
		monthNumeric = 4;
	else if(monthString == "May")
		monthNumeric = 5;
	else if(monthString == "Jun")
		monthNumeric = 6;
	else if(monthString == "Jul")
		monthNumeric = 7;
	else if(monthString == "Aug")
		monthNumeric = 8;
	else if(monthString == "Sep")
		monthNumeric = 9;
	else if(monthString == "Oct")
		monthNumeric = 10;
	else if(monthString == "Nov")
		monthNumeric = 11;
	else if(monthString == "Dec")
		monthNumeric = 12;
	else
		return false;

	int year = tokenizer->nextToken (delimiter).scanInt ();
	int hour = tokenizer->nextToken (delimiter).scanInt ();
	if(hour < 0 || hour > 24)
		return false;
	int minute = tokenizer->nextToken (delimiter).scanInt ();
	if(minute < 0 || minute > 60)
		return false;
	int second = tokenizer->nextToken (delimiter).scanInt ();
	if(second < 0 || second > 60)
		return false;
	if(tokenizer->nextToken (delimiter) != "GMT")
		return false; // its supposed to be "GMT" always

	date = DateTime (Date (year, monthNumeric, day), Time (hour, minute, second));

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WebHeaderCollection::isChunkedTransfer () const
{
	return lookupValue (Meta::kTransferEncoding) == "chunked";
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WebHeaderCollection::setRangeBytes (int64 start, int64 end)
{
	MutableCString entry;
	entry.appendFormat ("bytes=%d-", start);
	if(end > 0)
		entry.appendFormat ("%d", end);
	setEntry (Meta::kRange, entry.str ());
	
	return true;
}

//************************************************************************************************
// WebRequest
//************************************************************************************************

DEFINE_CLASS_HIDDEN (WebRequest, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

WebRequest::WebRequest (IStream* _stream)
: stream (nullptr),
  response (nullptr),
  headers (nullptr)
{
	if(_stream)
		setStream (_stream);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WebRequest::~WebRequest ()
{
	setStream (nullptr);

	if(response)
		response->release ();
	if(headers)
		headers->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebRequest::setStream (IStream* _stream)
{
	take_shared (stream, _stream);

	if(response)
		response->setStream (_stream);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStream* CCL_API WebRequest::getStream ()
{
	return stream;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IWebResponse* CCL_API WebRequest::getWebResponse ()
{
	if(response == nullptr)
	{
		CCL_NOT_IMPL ("WebRequest::getWebResponse not implemented!")
	}
	return response;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IWebHeaderCollection* CCL_API WebRequest::getWebHeaders ()
{
	return headers;
}

//************************************************************************************************
// WebResponse
//************************************************************************************************

DEFINE_CLASS_HIDDEN (WebResponse, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

WebResponse::WebResponse (IStream* _stream)
: stream (nullptr),
  headers (nullptr)
{
	if(_stream)
		setStream (_stream);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WebResponse::~WebResponse ()
{
	setStream (nullptr);
	if(headers)
		headers->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebResponse::setStream (IStream* _stream)
{
	take_shared (stream, _stream);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStream* CCL_API WebResponse::getStream ()
{
	return stream;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IWebHeaderCollection* CCL_API WebResponse::getWebHeaders ()
{
	return headers;
}
