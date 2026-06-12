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
// Filename    : ccl/extras/web/webresponsestream.h
// Description : Web Response Streams
//
//************************************************************************************************

#ifndef _ccl_webresponsestream_h
#define _ccl_webresponsestream_h

#include "ccl/base/object.h"

#include "ccl/public/base/memorystream.h"
#include "ccl/public/text/cclstring.h"
#include "ccl/public/text/cstring.h"

#include "ccl/public/network/web/iwebrequest.h"

namespace CCL {
namespace Web {

//************************************************************************************************
// WebResponseStream
/**	Base class for custom stream to parse HTTP response at lower level.	
	
	Threading Policy: 
	Stream writes and headers are received on a background thread. */
//************************************************************************************************

class WebResponseStream: public Object,
						 public IStream,
						 public IWebResponseSink
{
public:
	DECLARE_CLASS_ABSTRACT (WebResponseStream, Object)

	WebResponseStream ();

	PROPERTY_VARIABLE (int64, contentLength, ContentLength)
	PROPERTY_MUTABLE_CSTRING (contentType, ContentType)

	CLASS_INTERFACE2 (IStream, IWebResponseSink, Object)
	
protected:
	// IWebResponseSink
	void CCL_API onHeadersUpdated (int64 contentLength, IWebHeaderCollection& headers) override;
	void CCL_API onResponseCompleted () override;

	// IStream not implemented:
	int CCL_API read (void* buffer, int size) override	{ ASSERT (0) return -1; }
	int64 CCL_API tell () override						{ ASSERT (0) return -1; }
	tbool CCL_API isSeekable () const override			{ ASSERT (0) return false; }
	int64 CCL_API seek (int64 pos, int mode) override	{ ASSERT (0) return -1; }
};

//************************************************************************************************
// WebSseStream
/**	Handles HTTP SSE (Server-sent Events).
	Pass as response stream to HTTP request and it signals when messages are received. */
//************************************************************************************************

class WebSseStream: public WebResponseStream
{
public:
	DECLARE_CLASS (WebSseStream, WebResponseStream)

	~WebSseStream ();

	/** Event stream content type. */
	DECLARE_STRINGID_MEMBER (kEventStreamContentType)

	/**	Signaled when a complete SSE message is parsed from the stream.
		args[0]: current event name (String)
		args[1]: current event data (String) */
	DECLARE_STRINGID_MEMBER (kOnMessage)

	// WebResponseStream
	int CCL_API write (const void* buffer, int size) override;

private:
	static const String kEventNamePrefix;
	static const String kEventDataPrefix;
	static const String kCommentPrefix;
	static const int kEventNamePrefixLength;
	static const int kEventDataPrefixLength;
	
	String inBuffer;
	String currentData;
	String currentEvent;
};

//************************************************************************************************
// WebMixedResponseStream
/**	Handles response that's either HTTP SSE or other data. */
//************************************************************************************************

class WebMixedResponseStream: public WebSseStream
{
public:
	DECLARE_CLASS (WebMixedResponseStream, WebSseStream)

	bool isEventStream () const;

	/**	Signaled when response is complete.
		args[0]: event stream? (tbool)
		args[1]: response data (IMemoryStream) */
	DECLARE_STRINGID_MEMBER (kResponseCompleted)

	// WebSseStream
	int CCL_API write (const void* buffer, int size) override;

protected:
	MemoryStream responseData;

	// WebSseStream
	void CCL_API onResponseCompleted () override;
};

} // namespace Web
} // namespace CCL

#endif // _ccl_webresponsestream_h
