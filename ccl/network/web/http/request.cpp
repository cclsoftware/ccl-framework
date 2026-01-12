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
// Filename    : ccl/network/web/http/request.cpp
// Description : HTTP Request/Response
//
//************************************************************************************************

#include "ccl/network/web/http/request.h"

#include "ccl/public/base/istream.h"
#include "ccl/public/systemservices.h"

using namespace CCL;
using namespace Web;
using namespace HTTP;

//************************************************************************************************
// HTTP::Streamer
//************************************************************************************************

Streamer::Streamer (IStream& stream)
: stream (stream)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Streamer::writeLine (CStringRef line)
{
#if 1
	MutableCString lineWithEnding (line);
	lineWithEnding += "\r\n";
	int length = lineWithEnding.length ();
	return stream.write (lineWithEnding.str (), length) == length;
#else
	int length = line.length ();
	if(stream.write (line.str (), length) != length)
		return false;
	if(stream.write ("\r\n", 2) != 2)
		return false;
	return true;
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Streamer::readLine (MutableCString& line)
{
	CStringWriter<512> writer (line);
	while(1)
	{
		char c = 0;
		if(stream.read (&c, 1) != 1)
			return false;
		
		// CR LF
		if(c == '\r')
		{
			if(stream.read (&c, 1) != 1)
				return false;
			ASSERT (c == '\n')
		}
		
		if(c == '\n')
			break;

		writer.append (c);
	}
	writer.flush ();
	return true;
}

//************************************************************************************************
// HTTP::HeaderList
//************************************************************************************************

DEFINE_CLASS_HIDDEN (HeaderList, WebHeaderCollection)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool HeaderList::hasContentLength () const
{
	return lookupValue (Meta::kContentLength).isEmpty () == false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int HeaderList::getByteSize () const
{
	int size = 0;
	for(int i = 0; i < countEntries (); i++)
	{
		CStringRef key = getKeyAt (i);
		CStringRef value = getValueAt (i);

		size += key.length () + 2 + value.length () + 2; // key + ": " + value + CRLF
	}
	return size;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool HeaderList::send (IStream& stream) const
{
	// "Host: ccl.dev"
	// "User-Agent: Application/1.0"
	Streamer s (stream);
	for(int i = 0; i < countEntries (); i++)
	{
		CStringRef key = getKeyAt (i);
		CStringRef value = getValueAt (i);

		MutableCString line;
		line.appendFormat ("%s: %s", key.str (), value.str ());
		if(!s.writeLine (line))
			return false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool HeaderList::receive (IStream& stream)
{
	removeAll ();

	// "Host: ccl.dev"
	// "User-Agent: Application/1.0"
	Streamer s (stream);
	while(1)
	{
		MutableCString line;
		if(!s.readLine (line))
			return false;

		if(line.isEmpty ()) // blank line
			break;

		int index = line.index (":");
		ASSERT (index != -1) 
		
		// TODO: parse headers spread over multiple lines...

		if(index != -1)
		{
			MutableCString key = line;
			key.truncate (index);

			const char* value = line.str () + index + 1;
			while(*value && *value == 0x20)
				value++;

			setEntry (key, value);
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void HeaderList::getRangeBytes (int64& start, int64& end) const
{
	start = 0;
	end = 0;
	
	// looks like "bytes=1024-4096" or "bytes=1024-"
	MutableCString entry = getRange ();
	if(entry.isEmpty () == false)
	{
		int nConversions = ::sscanf (entry.str (), "bytes=%" FORMAT_INT64 "d-%" FORMAT_INT64 "d/", &start, &end);
		ASSERT (nConversions >= 1)
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void HeaderList::getContentRangeBytes (int64& start, int64& end, int64& length) const
{
	start = 0;
	end = 0;
	length = 0;
	
	// looks like "bytes 0-1023/146515"
	MutableCString entry = getContentRange ();
	if(entry.isEmpty () == false)
	{
		int nConversions = ::sscanf (entry.str (), "bytes %" FORMAT_INT64 "d-%" FORMAT_INT64 "d/%" FORMAT_INT64 "d" , &start, &end, &length);
		ASSERT (nConversions == 3)
	}
}

//************************************************************************************************
// HTTP::Request
//************************************************************************************************

DEFINE_CLASS_HIDDEN (Request, WebRequest)

//////////////////////////////////////////////////////////////////////////////////////////////////

Request::Request (IStream* _stream)
: WebRequest (_stream),
  version (HTTP::kV1_0)
{
	ASSERT (response == nullptr)
	response = NEW Response (_stream);
	ASSERT (headers == nullptr)
	headers = NEW HeaderList;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HeaderList& Request::getHeaders () const
{ 
	ASSERT (headers != nullptr)
	return *(HeaderList*)headers;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Response& Request::getResponse ()
{
	ASSERT (response != nullptr)
	return *(Response*)response;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Request::reset ()
{
	version = HTTP::kV1_0;
	method.empty ();
	path.empty ();
	getHeaders ().removeAll ();
	getResponse ().reset ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Request::send () const
{
	ASSERT (stream != nullptr)
	if(!stream)
		return false;

	// "GET /index.html HTTP/1.0"
	MutableCString line;
	line.appendFormat ("%s %s %s", method.str (), path.str (), HTTP::getVersionString (version));
	if(!Streamer (*stream).writeLine (line))
		return false;

	// Headers
	if(!getHeaders ().send (*stream))
		return false;

	// Blank line
	return Streamer (*stream).writeLine ("");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Request::receive ()
{
	ASSERT (stream != nullptr)
	if(!stream)
		return false;

	reset (); // clear old content

	MutableCString line;
	if(!Streamer (*stream).readLine (line))
		return false;

	// "GET /index.html HTTP/1.0"
	MutableCString versionString;
	int part = 1;
	for(int i = 0; line[i] != 0; i++)
	{
		char c = line[i];
		if(c == 0x20) // space
			part++;
		else 
		switch(part)
		{
		case 1  : method.append (c); break;
		case 2  : path.append (c); break;
		case 3  : versionString.append (c); break;
		default :
			return false;
		}
	}

	version = HTTP::getVersionNumber (versionString);
	if(method.isEmpty () || path.isEmpty () || versionString.isEmpty ())
		return false;

	// Headers
	return getHeaders ().receive (*stream);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Request::dump () const
{
	CString versionString = HTTP::getVersionString (version);
	Debugger::printf ("HTTP Request: method = %s path = %s version = %s\n", method.str (), path.str (), versionString.str ());
	Debugger::println ("Headers follow...");
	getHeaders ().dump ();
}

//************************************************************************************************
// HTTP::Response
//************************************************************************************************

DEFINE_CLASS_HIDDEN (Response, WebResponse)

//////////////////////////////////////////////////////////////////////////////////////////////////

Response::Response (IStream* stream)
: WebResponse (stream),
  version (HTTP::kV1_0),
  status (0)
{
	ASSERT (headers == nullptr)
	headers = NEW HeaderList;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HeaderList& Response::getHeaders () const
{
	ASSERT (headers != nullptr)
	return *(HeaderList*)headers;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Response::reset ()
{
	status = 0;
	version = HTTP::kV1_0;
	getHeaders ().removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Response::send () const
{
	ASSERT (stream != nullptr)
	if(!stream)
		return false;

	MutableCString versionString = HTTP::getVersionString (version);
	MutableCString statusString = HTTP::getStatusString (status);

	// "HTTP/1.0 200 OK"
	MutableCString line;
	line.appendFormat ("%s %d %s", versionString.str (), status, statusString.str ());
	if(!Streamer (*stream).writeLine (line))
		return false;

	// Headers
	if(!getHeaders ().send (*stream))
		return false;

	// Blank line
	return Streamer (*stream).writeLine ("");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Response::receive ()
{
	ASSERT (stream != nullptr)
	if(!stream)
		return false;

	reset (); // clear old content

	MutableCString line;
	if(!Streamer (*stream).readLine (line))
		return false;

	// "HTTP/1.0 200 OK"
	int spacePos = line.index (" ");
	if(spacePos == -1)
		return false;

	MutableCString versionString = line;
	versionString.truncate (spacePos);
	version = HTTP::getVersionNumber (versionString);

	::sscanf (line.str () + spacePos + 1, "%d", &status);

	// Headers
	return getHeaders ().receive (*stream);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Response::dump () const
{
	CString statusString = HTTP::getStatusString (status);
	CString versionString = HTTP::getVersionString (version);
	Debugger::printf ("HTTP Response: status = %d \"%s\" version = %s\n", status, statusString.str (), versionString.str ());
	Debugger::println ("Headers follow...");
	getHeaders ().dump ();
}
