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
// Filename    : core/extras/web/corewebserver.h
// Description : Embedded HTTP Server
//
//************************************************************************************************

#ifndef _corewebserver_h
#define _corewebserver_h

#include "core/public/corehttp.h"
#include "core/public/corestream.h"
#include "core/public/corevector.h"

namespace Core {
namespace Portable {
namespace HTTP {

using namespace HTTPDefinitions; // import HTTP definitions shared with CCL

// TODO: share with CCL...
const CStringPtr kContentType = "Content-Type";
const CStringPtr kContentLength = "Content-Length";
const CStringPtr kContentDisposition = "Content-Disposition";
const CStringPtr kServer = "Server";
const CStringPtr kConnection = "Connection";

const CStringPtr kMultipartForm = "multipart/form-data";
const CStringPtr kWebForm = "application/x-www-form-urlencoded";

//////////////////////////////////////////////////////////////////////////////////////////////////
// Content Types
//////////////////////////////////////////////////////////////////////////////////////////////////

#define CONTENT_TYPE_HTML	"text/html"
#define CONTENT_TYPE_PNG	"image/png"
#define CONTENT_TYPE_BINARY	"application/octet-stream"

#define CONTENT_FLAG_NONE	0
#define CONTENT_FLAG_SSI	Core::Portable::HTTP::ContentDescriptor::kSSI

//************************************************************************************************
// HTTP::ContentDescriptor
//************************************************************************************************

struct ContentDescriptor
{
	enum Flags
	{
		kSSI = 1<<0
	};

	CStringPtr fileName;
	CStringPtr contentType;
	const uint8* buffer;
	uint32 size;
	int flags;

	bool hasServerSideIncludes () const { return flags & kSSI; }
};

//************************************************************************************************
// HTTP::KeyValueList
//************************************************************************************************

class KeyValueList
{
public:	
	typedef CString64 Key;
	typedef CString128 Value;

	CStringPtr getValue (CStringPtr key) const;
	bool addValue (CStringPtr key, CStringPtr value);
	int64 getIntVale (CStringPtr key) const;
	bool addIntValue (CStringPtr key, int64 value);

	int getCount () const { return entries.count (); }
	CStringPtr getKeyAt (int index) const { return entries.at (index).key.str (); }
	CStringPtr getValueAt (int index) const { return entries.at (index).value.str (); }
	void removeAll () { entries.removeAll (); }

	void dump () const;

protected:
	struct Entry
	{
		Key key;
		Value value;
	};
	static const int kMaxEntryCount = 10;
	FixedSizeVector<Entry, kMaxEntryCount> entries;
};

//************************************************************************************************
// HTTP::HeaderList
//************************************************************************************************

class HeaderList: public KeyValueList
{
public:	
	CStringPtr getContentType () const { return getValue (HTTP::kContentType); }
	int64 getContentLength () const { return getIntVale (HTTP::kContentLength); }

	bool send (IO::Stream& stream) const;
	bool receive (IO::Stream& stream);
};

//************************************************************************************************
// HTTP::URLDecoder
//************************************************************************************************

struct URLDecoder
{
	static void decodeFields (KeyValueList& fields, CStringPtr urlEncodedString, int length);
};

//************************************************************************************************
// HTTP::Response
//************************************************************************************************

class Response
{
public:
	Response ();

	PROPERTY_VARIABLE (int, version, Version)
	PROPERTY_VARIABLE (int, status, Status)
	HeaderList& getHeaders () { return headers; }

	bool send (IO::Stream& stream);

protected:
	HeaderList headers;
};

//************************************************************************************************
// HTTP::Request
//************************************************************************************************

class Request
{
public:
	Request ();

	PROPERTY_VARIABLE (int, version, Version)
	PROPERTY_CSTRING_BUFFER (16, method, Method)
	bool isGET () const { return method == HTTP::kGET; }
	bool isPOST () const { return method == HTTP::kPOST; }
	PROPERTY_CSTRING_BUFFER (STRING_STACK_SPACE_MAX, path, Path)
	const HeaderList& getHeaders () const { return headers; }

	void reset ();
	bool receive (IO::Stream& stream);
	
	void setResponseStatus (int status) { response.setStatus (status); }
	int getResponseStatus () const { return response.getStatus (); }
	HeaderList& getResponseHeaders () { return response.getHeaders (); }
	bool sendResponse (IO::Stream& stream) { return response.send (stream); }

protected:
	HeaderList headers;
	Response response;
};

//************************************************************************************************
// HTTP::IRequestHandler
//************************************************************************************************

struct IRequestHandler
{
	virtual bool handleHTTPRequest (IO::Stream& stream) = 0;
};

//************************************************************************************************
// HTTP::ContentServer
//************************************************************************************************

class ContentServer: public IRequestHandler
{
public:
	ContentServer (CStringPtr serverName, const ContentDescriptor fileList[] = 0, int fileCount = 0);

	// Configuration
	void setFileList (const ContentDescriptor fileList[], int fileCount);
	void setDynamicPageBuffer (uint8* buffer, uint32 size);
	PROPERTY_VARIABLE (CStringPtr, indexPageName, IndexPageName)

	virtual bool handleRequest (Request& request, IO::Stream& stream);

	virtual bool getContent (ContentDescriptor& content, CStringPtr path) const;
	virtual bool getErrorPage (ContentDescriptor& content, int status) const;
	
	virtual CStringPtr resolveVariable (CStringPtr key) const; ///< resolve Server Side Include (SSI) variable

	// IRequestHandler
	bool handleHTTPRequest (IO::Stream& stream) override;

protected:
	CStringPtr serverName;
	const ContentDescriptor* fileList;
	int fileCount;

	const ContentDescriptor* findContent (CStringPtr path) const;
	bool processServerSideIncludes (IO::Stream& stream, const uint8 data[], uint32 size);
	bool sendRawData (IO::Stream& stream, const uint8 data[], uint32 size);

private:
	static const uint32 kMaxPartLength = 512;

	Request pendingRequest;
	uint8* dynamicPageBuffer;
	uint32 dynamicPageBufferSize;
};

//************************************************************************************************
// HTTP::IFormDataReceiver
//************************************************************************************************

struct IFormDataReceiver
{
	virtual void beginPart (CStringPtr name, const HeaderList& headers) = 0;
	virtual void receiveData (const char data[], int length) = 0;
	virtual void endPart (bool lastPart) = 0;
};

//************************************************************************************************
// HTTP::FormReader
//************************************************************************************************

class FormReader
{
public:
	FormReader ();

	const KeyValueList* readForm (IO::Stream& stream, int64 totalLength);

	bool readMultipart (IFormDataReceiver& receiver, IO::Stream& stream, int64 totalLength);

protected:
	// 70 characters (MIME limit) + leading hyphens + trailing hyphens + CRLF
	static const int kMaxBoundaryLength = 70 + 2 + 2 + 2;
	typedef CStringBuffer<kMaxBoundaryLength> BoundaryBuffer;
	static const int kOutputBufferSize = 512;

	int64 remainingLength;
	BoundaryBuffer boundary;
	int boundaryLength;
	char retroBuffer[kMaxBoundaryLength];
	char outputBuffer[kOutputBufferSize];
	HeaderList partHeaders;

	bool readPart (bool& lastPart, IFormDataReceiver& receiver, IO::Stream& stream);
};

} // namespace HTTP
} // namespace Portable
} // namespace Core

#endif // _corewebserver_h
