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
// Filename    : core/extras/web/corewebserver.cpp
// Description : Embedded HTTP Server
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "core/extras/web/corewebserver.h"

#include "core/portable/corehtmlwriter.h"
#include "core/system/coredebug.h"

#include "core/public/coreprimitives.h"
#include "core/public/corememstream.h"
#include "core/public/coreurlencoding.h"

namespace Core {
namespace Portable {
namespace HTTP {

//************************************************************************************************
// HTTP::LineBuffer
//************************************************************************************************

typedef CStringBuffer<STRING_STACK_SPACE_MAX> LineBuffer;

#define CRLF "\r\n"
#define DEFAULT_VERSION HTTP::kV1_1

//************************************************************************************************
// HTTP::StreamAccessor
//************************************************************************************************

class StreamAccessor
{
public:
	StreamAccessor (IO::Stream& stream);

	bool writeString (CStringPtr line);
	bool writeLine (CStringPtr line);
	bool readLine (char line[], int maxLength);
	
	template <class StringType> bool readLine (StringType& string)
	{
		return readLine (string.getBuffer (), string.getSize ());
	}

protected:
	IO::Stream& stream;
};

} // namespace HTTP
} // namespace Portable
} // namespace Core

using namespace Core;
using namespace Portable;
using namespace HTTP;

//************************************************************************************************
// HTTP::FormReader
//************************************************************************************************

FormReader::FormReader ()
: remainingLength (0),
  boundaryLength (0)
{
	#if DEBUG // easier on the eyes in debugger when cleared
	::memset (retroBuffer, 0, sizeof(retroBuffer));
	::memset (outputBuffer, 0, sizeof(outputBuffer));
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const KeyValueList* FormReader::readForm (IO::Stream& stream, int64 totalLength)
{
	// ATTENTION: This is a very simple implementation working for small forms only!
	int toRead = (int)get_min<int64> (kOutputBufferSize, totalLength);
	int numRead = stream.readBytes (outputBuffer, toRead);
	if(numRead != toRead)
		return 0;

	partHeaders.removeAll ();
	URLDecoder::decodeFields (partHeaders, outputBuffer, numRead);

	// skip remaining bytes
	totalLength -= numRead;
	while(totalLength > 0)
	{
		char c = 0;
		if(stream.readBytes (&c, 1) != 1)
			return 0;
		totalLength--;
	}

	return &partHeaders;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FormReader::readMultipart (IFormDataReceiver& receiver, IO::Stream& stream, int64 totalLength)
{
	/*
		Multipart MIME Format:

		--boundary + CRLF
		[headers + CRLF]
		[value1 + CRLF]
		--boundary + CRLF
		[headers + CRLF]
		[value2 + CRLF]
		--boundary-- + CRLF
	*/

	remainingLength = totalLength;

	// read first boundary
	int64 start = stream.getPosition ();
	if(!StreamAccessor (stream).readLine (boundary))
		return false;
	remainingLength -= stream.getPosition () - start;

	boundary.insert (0, CRLF); // data is followed by CR LF before boundary
	boundaryLength = boundary.length ();

	bool lastPart = false;
	while(!lastPart)
	{
		if(!readPart (lastPart, receiver, stream))
			return false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FormReader::readPart (bool& lastPart, IFormDataReceiver& receiver, IO::Stream& stream)
{
	// part headers
	int64 start = stream.getPosition ();
	partHeaders.removeAll ();
	if(!partHeaders.receive (stream))
		return false;
	remainingLength -= stream.getPosition () - start;

	// parse part name from content disposition header
	// see: https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Content-Disposition
	HeaderList::Value partName;
	ConstString contentDisposition (partHeaders.getValue (HTTP::kContentDisposition));
	if(!contentDisposition.isEmpty ())
	{
		ConstString namePrefix ("name=\"");
		int startIndex = contentDisposition.index (namePrefix);
		if(startIndex != -1)
		{
			ConstString nameStart (contentDisposition.str () + startIndex + namePrefix.length ());
			int endIndex = nameStart.index ("\"");
			if(endIndex != -1)
				partName.append (nameStart, endIndex);	
		}
	}

	receiver.beginPart (partName, partHeaders);

	// part data
	int retroCount = 0;
	int outputCount = 0;
	bool endOfPart = false;
	while(remainingLength > 0 && !endOfPart)
	{
		char c = 0;
		if(stream.readBytes (&c, 1) != 1)
			return false;

		remainingLength--;

		bool byteUsed = false;
		if(retroCount < boundaryLength)
		{
			retroBuffer[retroCount++] = c;
			byteUsed = true;
		}

		if(retroCount >= boundaryLength)
		{
			endOfPart = ::memcmp (retroBuffer, boundary.str (), boundaryLength) == 0;
			if(endOfPart == false && byteUsed == false)
			{
				outputBuffer[outputCount++] = retroBuffer[0];
				if(outputCount >= kOutputBufferSize)
				{
					receiver.receiveData (outputBuffer, outputCount);
					outputCount = 0;
				}

				::memmove (retroBuffer, retroBuffer + 1, boundaryLength-1);
				retroBuffer[boundaryLength-1] = c;

				// check again
				endOfPart = ::memcmp (retroBuffer, boundary.str (), boundaryLength) == 0;
			}
		}
	}

	// flush output buffer
	if(outputCount > 0)
		receiver.receiveData (outputBuffer, outputCount);

	// read two more bytes (either CR LF or "--" for last boundary)
	if(remainingLength < 2)
		return false;

	char temp[2] = {0};
	if(stream.readBytes (temp, 2) != 2)
		return false;

	remainingLength -= 2;

	lastPart = ::memcmp (temp, "--", 2) == 0 || remainingLength <= 2;

	if(lastPart)
	{
		if(remainingLength != 2 || stream.readBytes (&temp, 2) != 2)
		{
			// We didn't receive the entire data. Too bad!
			return false;
		}

		remainingLength = 0;
	}

	receiver.endPart (lastPart);
	return true;
}

//************************************************************************************************
// HTTP::ContentServer
//************************************************************************************************

ContentServer::ContentServer (CStringPtr serverName, const ContentDescriptor fileList[], int fileCount)
: serverName (serverName),
  fileList (fileList),
  fileCount (fileCount),
  dynamicPageBuffer (0),
  dynamicPageBufferSize (0),
  indexPageName ("index.html")
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ContentServer::setFileList (const ContentDescriptor _fileList[], int _fileCount)
{
	fileList = _fileList;
	fileCount = _fileCount;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ContentServer::setDynamicPageBuffer (uint8* buffer, uint32 size)
{
	dynamicPageBuffer = buffer;
	dynamicPageBufferSize = size;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ContentServer::handleHTTPRequest (IO::Stream& stream)
{
	Request& request = pendingRequest; // use request member to limit stack consumption
	if(!request.receive (stream))
		return false;

	#if DEBUG_LOG
	CORE_PRINTF ("HTTP request method = %s path = %s\n", request.getMethod ().str (), request.getPath ().str ())
	for(int i = 0; i < request.getHeaders ().getCount (); i++)
		CORE_PRINTF ("\tHeader '%s' = '%s'\n", request.getHeaders ().getKeyAt (i), request.getHeaders ().getValueAt (i))
	#endif

	request.getResponseHeaders ().addValue (HTTP::kServer, serverName);
	request.getResponseHeaders ().addValue (HTTP::kConnection, "close");

	bool result = handleRequest (request, stream);
	CORE_PRINTF ("=> Result = %s Status = %d\n", result ? "true" : "false", request.getResponseStatus ())

	request.reset ();
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ContentServer::handleRequest (Request& request, IO::Stream& stream)
{
	int errorStatus = 0;
	HeaderList& responseHeaders = request.getResponseHeaders ();

	if(request.getMethod () == HTTP::kGET)
	{
		ContentDescriptor fileData = {0};
		if(getContent (fileData, request.getPath ()))
		{
			if(fileData.hasServerSideIncludes ())
			{
				ASSERT (dynamicPageBuffer != 0)
				if(dynamicPageBuffer != 0)
				{
					IO::MemoryStream ms (dynamicPageBuffer, dynamicPageBufferSize);
					ms.setBytesWritten (0);
					if(processServerSideIncludes (ms, fileData.buffer, fileData.size))
					{
						uint32 processedSize = ms.getBytesWritten ();

						request.setResponseStatus (HTTP::kOK);
						responseHeaders.addValue (HTTP::kContentType, fileData.contentType);
						responseHeaders.addIntValue (HTTP::kContentLength, processedSize);
						return request.sendResponse (stream) && sendRawData (stream, dynamicPageBuffer, processedSize);
					}
				}

				errorStatus = HTTP::kServerError;
			}
			else
			{
				request.setResponseStatus (HTTP::kOK);
				responseHeaders.addValue (HTTP::kContentType, fileData.contentType);
				responseHeaders.addIntValue (HTTP::kContentLength, fileData.size);
				return request.sendResponse (stream) && sendRawData (stream, fileData.buffer, fileData.size);
			}
		}
		else
			errorStatus = HTTP::kNotFound;
	}
	else
		errorStatus = HTTP::kMethodNotAllowed;

	request.setResponseStatus (errorStatus);
	ContentDescriptor errorPage = {0};
	if(getErrorPage (errorPage, errorStatus))
	{
		ASSERT (errorPage.hasServerSideIncludes () == false)
		responseHeaders.addValue (HTTP::kContentType, errorPage.contentType);
		responseHeaders.addIntValue (HTTP::kContentLength, errorPage.size);
		return request.sendResponse (stream) && sendRawData (stream, errorPage.buffer, errorPage.size);
	}
	else
	{
		responseHeaders.addIntValue (HTTP::kContentLength, 0);
		return request.sendResponse (stream);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr ContentServer::resolveVariable (CStringPtr _key) const
{
	ConstString key (_key);
	if(key == "${SERVER_NAME}")
		return serverName;
	if(key == "${BUILD_DATE}")
		return __DATE__;
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ContentDescriptor* ContentServer::findContent (CStringPtr _path) const
{
	ConstString path (_path);
	for(int i = 0; i < fileCount; i++)
		if(path == fileList[i].fileName)
			return &fileList[i];
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ContentServer::getContent (ContentDescriptor& fileData, CStringPtr path) const
{
	const ContentDescriptor* content = 0;	
	if(ConstString (path) == "/")
		content = findContent (indexPageName);
	else
		content = findContent (path);

	if(content != 0)
	{
		fileData = *content;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ContentServer::getErrorPage (ContentDescriptor& content, int status) const
{
	if(dynamicPageBuffer == 0)
		return false;

	CString256 statusString;
	statusString.appendInteger (status);
	statusString += " - ";
	statusString += HTTP::getStatusString (status);
	statusString += "\n";

	IO::MemoryStream ms (dynamicPageBuffer, dynamicPageBufferSize);
	ms.setBytesWritten (0);
	HtmlWriter (ms).beginDocument (serverName).write (statusString).endDocument ();
	
	content.contentType = CONTENT_TYPE_HTML;
	content.fileName = "error.html";
	content.buffer = dynamicPageBuffer;
	content.size = ms.getBytesWritten ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ContentServer::processServerSideIncludes (IO::Stream& stream, const uint8 data[], uint32 size)
{
	struct PartSender
	{
		const uint8* part;
		uint32 length;
		uint32 maxPartLength;
			
		PartSender (const uint8* start, uint32 maxPartLength)
		: part (start),
		  length (0),
		  maxPartLength (maxPartLength)
		{}

		void begin (const uint8* start)
		{
			part = start;
			length = 0;
		}

		bool advance ()
		{
			length++;
			return length < maxPartLength;
		}

		bool flush (IO::Stream& stream)
		{
			if(length > 0)
			{
				if(stream.writeBytes (part, length) != length)
					return false;
				length = 0;
			}
			return true;
		}
	};

	const uint8* src = data;
	PartSender partSender (src, kMaxPartLength);
	for(uint32 remaining = size; remaining > 0; remaining--, src++)
	{
		static const char kVarPrefix[] = "<!--#echo var=\"";
		const uint32 kVarPrefixLength = sizeof(kVarPrefix) - 1;

		// check for variable
		if(remaining > kVarPrefixLength && ::memcmp (src, kVarPrefix, kVarPrefixLength) == 0)
		{
			if(!partSender.flush (stream))
				return false;

			src += kVarPrefixLength;
			remaining -= kVarPrefixLength;
					
			CString64 varName;
			while(*src != '\"' && remaining > 0)
			{
				varName += (char)*src++;
				remaining--;
			}

			// variable ends with '" -->'
			while(*src != '>' && remaining > 0)
			{
				src++;
				remaining--;
			}

			ConstString varValue = resolveVariable (varName);
			CORE_PRINTF ("SSI resolved variable '%s' to '%s'\n", varName.str (), varValue.str ())
			if(!varValue.isEmpty ())
			{
				int varLength = varValue.length ();
				if(stream.writeBytes (varValue.str (), varLength) != varLength)
					return false;
			}

			partSender.begin (src + 1); // re-start after '>'
		}
		else
		{
			if(!partSender.advance ())
			{
				if(!partSender.flush (stream))
					return false;					
				partSender.begin (src + 1);
			}
		}
	}
	return partSender.flush (stream);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ContentServer::sendRawData (IO::Stream& stream, const uint8 data[], uint32 size)
{
	uint32 partCount = size / kMaxPartLength;
	if(size % kMaxPartLength)
		partCount++;

	const uint8* src = data;
	uint32 remaining = size;
	for(uint32 i = 0; i < partCount; i++)
	{
		uint32 partLength = get_min (remaining, kMaxPartLength);
		if(stream.writeBytes (src, partLength) != partLength)
			return false;

		src += partLength;
		remaining -= partLength;
	}		
	return true;
}

//************************************************************************************************
// HTTP::Request
//************************************************************************************************

Request::Request ()
: version (DEFAULT_VERSION)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Request::reset ()
{
	version = DEFAULT_VERSION;
	method = "";
	path = "";
	headers.removeAll ();
	response.setVersion (DEFAULT_VERSION);
	response.setStatus (0);
	response.getHeaders ().removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Request::receive (IO::Stream& stream)
{
	LineBuffer line;
	if(!StreamAccessor (stream).readLine (line))
		return false;

	// "GET /index.html HTTP/1.0"
	CString32 versionString;
	int part = 1;
	for(int i = 0; line[i] != 0; i++)
	{
		char c = line[i];
		if(c == 0x20) // space
			part++;
		else
		{
			switch(part)
			{
			case 1 : method.append (c); break;
			case 2 : path.append (c); break;
			case 3 : versionString.append (c); break;
			default: return false;
			}
		}
	}

	version = HTTP::getVersionNumber (versionString);
	if(method.isEmpty () || path.isEmpty () || versionString.isEmpty ())
		return false;

	// TODO: URL decoding for path...

	// Headers
	return headers.receive (stream);
}

//************************************************************************************************
// HTTP::Response
//************************************************************************************************

Response::Response ()
: version (DEFAULT_VERSION),
  status (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Response::send (IO::Stream& stream)
{
	CStringPtr versionString = HTTP::getVersionString (version);
	CStringPtr statusString = HTTP::getStatusString (status);

	// "HTTP/1.0 200 OK"
	LineBuffer line;
	line.appendFormat ("%s %d %s", versionString, status, statusString);
	if(!StreamAccessor (stream).writeLine (line))
		return false;

	// Headers
	if(!headers.send (stream))
		return false;

	// Blank line
	return StreamAccessor (stream).writeLine ("");
}

//************************************************************************************************
// HTTP::KeyValueList
//************************************************************************************************

CStringPtr KeyValueList::getValue (CStringPtr key) const
{
	for(int i = 0; i < entries.count (); i++)
		if(entries[i].key == key)
			return entries[i].value.str ();
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool KeyValueList::addValue (CStringPtr key, CStringPtr value)
{
	if(entries.isFull ())
		return false;

	Entry& entry = entries[entries.count ()];
	entry.key = key;
	entry.value = value;
	entries.setCount (entries.count ()+1);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 KeyValueList::getIntVale (CStringPtr key) const
{
	int64 intValue = 0;
	ConstString string (getValue (key));
	string.getIntValue (intValue);
	return intValue;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool KeyValueList::addIntValue (CStringPtr key, int64 value)
{
	CString64 string;
	string.appendFormat ("%" FORMAT_INT64 "d", value);
	return addValue (key, string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void KeyValueList::dump () const
{
	for(int i = 0; i < entries.count (); i++)
		DebugPrintf ("'%s' = '%s'\n", entries[i].key.str (), entries[i].value.str ());
}

//************************************************************************************************
// HTTP::HeaderList
//************************************************************************************************

bool HeaderList::send (IO::Stream& stream) const
{
	// "Host: ccl.dev"
	// "User-Agent: Application/1.0"
	StreamAccessor s (stream);
	for(int i = 0; i < entries.count (); i++)
	{
		LineBuffer line;
		line.appendFormat ("%s: %s" CRLF, entries[i].key.str (), entries[i].value.str ());
		if(!s.writeString (line))
			return false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool HeaderList::receive (IO::Stream& stream)
{
	entries.removeAll ();

	// "Host: ccl.dev"
	// "User-Agent: Application/1.0"
	StreamAccessor s (stream);
	while(1)
	{
		LineBuffer line;
		if(!s.readLine (line))
			return false;

		if(line.isEmpty ()) // blank line
			break;

		int index = line.index (":");
		ASSERT (index != -1) 
		// TODO: parse headers spread over multiple lines!!!

		if(index != -1)
		{
			Entry entry;
			entry.key = line;
			entry.key.truncate (index);

			CStringPtr value = line.str () + index + 1;
			while(*value && *value == 0x20)
				value++;

			entry.value = value;
			entries.add (entry);
		}
	}
	return true;
}

//************************************************************************************************
// HTTP::URLDecoder
//************************************************************************************************

void URLDecoder::decodeFields (KeyValueList& fields, CStringPtr urlEncodedString, int length)
{
	CStringPtr pairStart = urlEncodedString;
	int i = 0;
	for(CStringPtr s = pairStart; i <= length; s++, i++) // will break at null, see below
	{
		char c = i < length ? *s : 0;
		if(c == '&' || c == ';' || c == 0)
		{
			CStringPtr valueStart = 0;
			for(CStringPtr s2 = pairStart; s2 < s; s2++)
				if(*s2 == '=')
				{
					valueStart = s2 + 1;
					break;
				}

			KeyValueList::Key key, key2;
			KeyValueList::Value value, value2;
			if(valueStart != 0)
			{
				key.append (pairStart, (int)(valueStart - pairStart - 1));
				value.append (valueStart, (int)(s - valueStart));
			}
			else // value is empty
				key.append (pairStart, (int)(s - pairStart));

			if(!key.isEmpty ())
			{
				URLEncoding::decode (key2, key);
				URLEncoding::decode (value2, value);
				fields.addValue (key2, value2);
			}

			if(c == 0)
				break;
			else
				pairStart = s + 1;
		}	
	}
}

//************************************************************************************************
// HTTP::StreamAccessor
//************************************************************************************************

StreamAccessor::StreamAccessor (IO::Stream& stream)
: stream (stream)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StreamAccessor::writeString (CStringPtr line)
{
	int length = ConstString (line).length ();
	return stream.writeBytes (line, length) == length;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StreamAccessor::writeLine (CStringPtr line)
{
	LineBuffer lineWithEnding = line;	
	lineWithEnding += CRLF;
	return writeString (lineWithEnding);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StreamAccessor::readLine (char line[], int maxLength)
{
	int charCount = 0;
	while(1)
	{
		char c = 0;
		if(stream.readBytes (&c, 1) != 1)
			return false;
		
		// CR LF
		if(c == '\r')
		{
			if(stream.readBytes (&c, 1) != 1)
				return false;
			ASSERT (c == '\n')
		}
		
		if(c == '\n')
			break;

		if(charCount < maxLength)
			line[charCount++] = c;
	}

	line[get_min (charCount, maxLength)] = '\0';
	return true;
}
