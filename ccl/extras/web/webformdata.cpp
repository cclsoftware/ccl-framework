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
// Filename    : ccl/extras/web/webformdata.cpp
// Description : Web Form Data
//
//************************************************************************************************

// Content-Type: multipart/form-data
// http://www.w3.org/TR/html401/interact/forms.html#h-17.13.4.2
// http://www.w3.org/Protocols/rfc1341/7_2_Multipart.html

#include "ccl/extras/web/webformdata.h"

#include "ccl/base/storage/urlencoder.h"

#include "ccl/public/base/memorystream.h"
#include "ccl/public/base/multiplexstream.h"

#define CRLF "\r\n"

using namespace CCL;
using namespace Web;

//************************************************************************************************
// FormData
//************************************************************************************************

StringID FormData::getContentType ()
{
	return CSTR ("application/x-www-form-urlencoded");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStream* FormData::createStream (const IStringDictionary& parameters)
{
	String encoded = UrlEncoder (UrlEncoder::kWebForm).encode (parameters);

	AutoPtr<MemoryStream> memStream = NEW MemoryStream;
	int size = encoded.length ();
	memStream->allocateMemory (size + 1);
	char* ptr = (char*)memStream->getMemoryAddress ();
	encoded.toASCII (ptr, size + 1);
	memStream->setBytesWritten (size);
	
	return memStream.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStream* FormData::createStream (const ICStringDictionary& parameters)
{
	MutableCString encoded = UrlEncoder (UrlEncoder::kWebForm).encode (parameters);

	AutoPtr<MemoryStream> memStream = NEW MemoryStream;
	memStream->write (encoded.str (), encoded.length ());
	memStream->rewind ();

	return memStream.detach ();
}

//************************************************************************************************
// MultipartFormData
//************************************************************************************************

MultipartFormData::MultipartFormData ()
: multiplexStream (*NEW MultiplexStream)
{
	boundary.appendFormat ("----------------------------XXBOUNDARY%04X", ::rand ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MultipartFormData::~MultipartFormData ()
{
	multiplexStream.release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString MultipartFormData::getContentType () const
{
	MutableCString contentType;
	contentType.appendFormat ("multipart/form-data; boundary=%s", boundary.str ());
	return contentType;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStream* MultipartFormData::createStream () const
{
	return return_shared (&multiplexStream);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MultipartFormData::appendText (StringID text)
{
	int size = text.length ();
	AutoPtr<MemoryStream> streamPart = NEW MemoryStream;
	streamPart->write (text.str (), size);
	multiplexStream.addStream (streamPart, size);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MultipartFormData::addField (StringID name, StringRef value, bool end)
{
	MutableCString cString (value, Text::kUTF8);
	addField (name, cString, end);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MultipartFormData::addField (StringID name, StringID value, bool end)
{
	MutableCString text;

	// 1) start boundary
	text.appendFormat ("--%s" CRLF, boundary.str ());

	// 2) headers
	text.appendFormat ("Content-Disposition: form-data; name=\"%s\"" CRLF, name.str ());

	// 3) blank line
	text += CRLF; 

	// 4) data
	text += value;

	// 5) blank line
	text += CRLF; 

	// 6) end boundary
	if(end)
		text.appendFormat ("--%s--" CRLF, boundary.str ());

	appendText (text); // add to multiplex stream
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MultipartFormData::addFile (StringID name, StringID fileName, IStream* file, int64 fileSize, bool end)
{
	MutableCString text;

	// 1) start boundary
	text.appendFormat ("--%s" CRLF, boundary.str ());

	// 2) headers
	text.appendFormat ("Content-Disposition: form-data; name=\"%s\"; filename=\"%s\"" CRLF, name.str (), fileName.str ());
	text.appendFormat ("Content-Transfer-Encoding: binary" CRLF);
	text.appendFormat ("Content-Type: application/octet-stream" CRLF);

	// 3) blank line
	text += CRLF; 

	appendText (text); // add to multiplex stream
	text.empty ();

	// 4) data
	multiplexStream.addStream (file, fileSize); // add to multiplex stream

	// 5) blank line
	text += CRLF; 

	// 6) end boundary
	if(end)
		text.appendFormat ("--%s--" CRLF, boundary.str ());

	appendText (text); // add to multiplex stream
}
