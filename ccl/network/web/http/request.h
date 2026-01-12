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
// Filename    : ccl/network/web/http/request.h
// Description : HTTP Request/Response
//
//************************************************************************************************

#ifndef _ccl_httprequest_h
#define _ccl_httprequest_h

#include "ccl/network/web/webrequest.h"

#include "ccl/public/network/web/httpstatus.h"

namespace CCL {
namespace Web {
namespace HTTP {

class Response;

//************************************************************************************************
// HTTP::Streamer
//************************************************************************************************

class Streamer
{
public:
	Streamer (IStream& stream);

	bool writeLine (CStringRef line);
	bool readLine (MutableCString& line);

protected:
	IStream& stream;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// HTTP Header Macros
//////////////////////////////////////////////////////////////////////////////////////////////////

#define DEFINE_HTTPHEADER(key, Method) \
CStringRef get##Method () const { return lookupValue (key); } \
void set##Method (CStringRef value) { setEntry (key, value); }

#define DEFINE_HTTPHEADER_INT(key, Method) \
int64 get##Method () const { int64 iValue = 0; lookupValue (key).getIntValue (iValue); return iValue; } \
void set##Method (int64 value) { MutableCString string; string.appendFormat ("%" FORMAT_INT64"d", value); setEntry (key, string); }

//************************************************************************************************
// HTTP::HeaderList
//************************************************************************************************

class HeaderList: public WebHeaderCollection
{
public:
	DECLARE_CLASS (HeaderList, WebHeaderCollection)

	DEFINE_HTTPHEADER (Meta::kHost, Host)
	DEFINE_HTTPHEADER (Meta::kUserAgent, UserAgent)
	DEFINE_HTTPHEADER (Meta::kAuthorization, Authorization)

	DEFINE_HTTPHEADER (Meta::kContentType, ContentType)
	DEFINE_HTTPHEADER_INT (Meta::kContentLength, ContentLength)
	bool hasContentLength () const;
	DEFINE_HTTPHEADER (Meta::kContentRange, ContentRange)

	DEFINE_HTTPHEADER (Meta::kContentDisposition, ContentDisposition)
	DEFINE_HTTPHEADER (Meta::kContentTransferEncoding, ContentTransferEncoding)

	DEFINE_HTTPHEADER (Meta::kDate, Date)
	DEFINE_HTTPHEADER (Meta::kServer, Server)
	DEFINE_HTTPHEADER (Meta::kLocation, Location)
	DEFINE_HTTPHEADER (Meta::kConnection, Connection)
	DEFINE_HTTPHEADER (Meta::kTransferEncoding, TransferEncoding)
	
	DEFINE_HTTPHEADER (Meta::kIfRange, IfRange)
	DEFINE_HTTPHEADER (Meta::kRange, Range)
	
	int getByteSize () const;
	bool send (IStream& stream) const;
	bool receive (IStream& stream);

	void getRangeBytes (int64& start, int64& end) const;
	void getContentRangeBytes (int64& start, int64& end, int64& length) const;
};

//************************************************************************************************
// HTTP::Request
//************************************************************************************************

class Request: public WebRequest
{
public:
	DECLARE_CLASS (Request, WebRequest)

	Request (IStream* stream = nullptr);

	PROPERTY_VARIABLE (int, version, Version)
	PROPERTY_MUTABLE_CSTRING (method, Method)
	PROPERTY_MUTABLE_CSTRING (path, Path)

	HeaderList& getHeaders () const;
	Response& getResponse ();
	
	void reset ();
	bool send () const;
	bool receive ();
	
	void dump () const;
};

//************************************************************************************************
// HTTP::Response
//************************************************************************************************

class Response: public WebResponse
{
public:
	DECLARE_CLASS (Response, WebResponse)

	Response (IStream* stream = nullptr);

	PROPERTY_VARIABLE (int, version, Version)
	PROPERTY_VARIABLE (int, status, Status)

	HeaderList& getHeaders () const;

	void reset ();
	bool send () const;
	bool receive ();

	void dump () const;
};

} // namespace HTTP
} // namespace Web
} // namespace CCL

#endif // _ccl_httprequest_h
