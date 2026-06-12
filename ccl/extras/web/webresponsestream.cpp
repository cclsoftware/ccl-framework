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
// Filename    : ccl/extras/web/webresponsestream.cpp
// Description : Web Response Streams
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/extras/web/webresponsestream.h"

#include "ccl/base/message.h"

#include "ccl/public/text/istringdict.h"

using namespace CCL;
using namespace Web;

//************************************************************************************************
// WebResponseStream
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (WebResponseStream, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

WebResponseStream::WebResponseStream ()
: contentLength (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WebResponseStream::onHeadersUpdated (int64 contentLength, IWebHeaderCollection& headers)
{
	this->contentLength = contentLength;

	if(contentType.isEmpty ())
		contentType = headers.getEntries ().lookupValue (Meta::kContentType);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WebResponseStream::onResponseCompleted ()
{}

//************************************************************************************************
// WebSseStream
//************************************************************************************************

DEFINE_CLASS_HIDDEN (WebSseStream, WebResponseStream)
DEFINE_STRINGID_MEMBER_ (WebSseStream, kEventStreamContentType, "text/event-stream")
DEFINE_STRINGID_MEMBER_ (WebSseStream, kOnMessage, "onMessage")

//////////////////////////////////////////////////////////////////////////////////////////////////

const String WebSseStream::kEventNamePrefix ("event:");
const String WebSseStream::kEventDataPrefix ("data:");
const String WebSseStream::kCommentPrefix (":");
const int WebSseStream::kEventNamePrefixLength (kEventNamePrefix.length ());
const int WebSseStream::kEventDataPrefixLength (kEventDataPrefix.length ());

//////////////////////////////////////////////////////////////////////////////////////////////////

WebSseStream::~WebSseStream ()
{
	cancelSignals ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API WebSseStream::write (const void* buffer, int size)
{
	CCL_PRINTF ("WebSseStream chunk of %d bytes received...\n", size)

	if(size <= 0)
		return size;
	
	// Append incoming chunk to internal buffer
	inBuffer.appendCString (Text::kUTF8, (const char*)buffer, size);

	StringRef nl = String::getLineEnd (Text::kLFLineFormat);
	StringRef cr = String::getLineEnd (Text::kCRLineFormat);
	
	// Process complete lines
	while(true)
	{
		int newlineIndex = inBuffer.index (nl);

		// No complete line available yet
		if(newlineIndex < 0)
			break;

		// Extract the line
		String line = inBuffer.subString (0, newlineIndex);
		inBuffer = inBuffer.subString (newlineIndex + 1, inBuffer.length () - newlineIndex - 1);

		// Remove trailing carriage return
		if(line.endsWith (cr))
			line.truncate (line.length () - 1);

		// Blank line = end of SSE event
		if(line.isEmpty ())
		{
			if(!currentData.isEmpty ())
			{				
				deferSignal (NEW Message (kOnMessage, currentEvent, currentData), kSignalQueueAddAlways);
				
				currentEvent.empty ();
				currentData.empty ();
			}
			continue;
		}

		// Skip comment lines
		if(line.startsWith (kCommentPrefix))
			continue;

		// Event name
		if(line.startsWith (kEventNamePrefix))
		{
			String value = line.subString (kEventNamePrefixLength, line.length () - kEventNamePrefixLength);
			value.trimWhitespace ();
			currentEvent = value;
			continue;
		}

		// Event data
		if(line.startsWith (kEventDataPrefix))
		{
			String value = line.subString (kEventDataPrefixLength, line.length () - kEventDataPrefixLength);

			if(!currentData.isEmpty ())
				currentData.append (nl);

			currentData.append (value);
			continue;
		}
	}

	return size;
}

//************************************************************************************************
// WebMixedResponseStream
//************************************************************************************************

DEFINE_CLASS_HIDDEN (WebMixedResponseStream, WebSseStream)
DEFINE_STRINGID_MEMBER_ (WebMixedResponseStream, kResponseCompleted, "responseCompleted")

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WebMixedResponseStream::isEventStream () const
{
	return contentType == kEventStreamContentType;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API WebMixedResponseStream::write (const void* buffer, int size)
{
	if(isEventStream ())
		return WebSseStream::write (buffer, size);
	else
		return responseData.write (buffer, size);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WebMixedResponseStream::onResponseCompleted ()
{
	responseData.rewind ();
	deferSignal (NEW Message (kResponseCompleted, isEventStream (), static_cast<IMemoryStream*> (&responseData)));
}
