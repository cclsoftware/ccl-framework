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
// Filename    : ccl/public/network/web/iwebrequest.h
// Description : Web Request/Response Interface
//
//************************************************************************************************

#ifndef _ccl_iwebrequest_h
#define _ccl_iwebrequest_h

#include "ccl/public/base/iunknown.h"
#include "ccl/public/base/datetime.h"

namespace CCL {

interface IStream;
interface ICStringDictionary;

namespace Web {

interface IWebResponse;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Web Protocol Header Fields
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Meta
{
	DEFINE_STRINGID (kHost, "Host")
	DEFINE_STRINGID (kUserAgent, "User-Agent")
	DEFINE_STRINGID (kAuthorization, "Authorization")
	DEFINE_STRINGID (kContentType, "Content-Type")
	DEFINE_STRINGID (kContentLength, "Content-Length")
	DEFINE_STRINGID (kContentRange, "Content-Range")
	DEFINE_STRINGID (kContentDisposition, "Content-Disposition")
	DEFINE_STRINGID (kContentTransferEncoding, "Content-Transfer-Encoding")
	DEFINE_STRINGID (kDate, "Date")
	DEFINE_STRINGID (kServer, "Server")
	DEFINE_STRINGID (kLocation, "Location")
	DEFINE_STRINGID (kConnection, "Connection")
	DEFINE_STRINGID (kTransferEncoding, "Transfer-Encoding")
	DEFINE_STRINGID (kRange, "Range")
	DEFINE_STRINGID (kIfRange, "If-Range")
	DEFINE_STRINGID (kETag, "ETag")
}

//************************************************************************************************
// IWebHeaderCollection
/** Collection for protocol-specific headers. */
//************************************************************************************************

interface IWebHeaderCollection: IUnknown
{
	/** Get all header entries. */
	virtual ICStringDictionary& getEntries () = 0;

	/** Parse filename from "Content-Disposition" header. */
	virtual tbool CCL_API parseFileName (String& fileName) const = 0;

	/** Parse the response date from the "Date" header. */
	virtual tbool CCL_API parseDate (CCL::DateTime& date) const = 0;

	/** Returns true for chunked transfer ("Transfer-Encoding"). */
	virtual tbool CCL_API isChunkedTransfer () const = 0;

	/** Set range requested by client ("Range"), end = 0 denotes end of file. */
	virtual tbool CCL_API setRangeBytes (int64 start, int64 end = 0) = 0;
	
	DECLARE_IID (IWebHeaderCollection)
};

DEFINE_IID (IWebHeaderCollection, 0x83d03ec8, 0xb69, 0x46d3, 0xbb, 0x32, 0x9d, 0x40, 0xde, 0x4e, 0x62, 0x70)

//************************************************************************************************
// IWebRequest
/** Request interface for protocols like HTTP. */
//************************************************************************************************

interface IWebRequest: IUnknown
{
	/** Get underlying network stream. */
	virtual IStream* CCL_API getStream () = 0;

	/** Get associated response object. */
	virtual IWebResponse* CCL_API getWebResponse () = 0;

	/** Get associated header collection. */
	virtual IWebHeaderCollection* CCL_API getWebHeaders () = 0;

	DECLARE_IID (IWebRequest)
};

DEFINE_IID (IWebRequest, 0x1511bd2, 0x418a, 0x4d16, 0x82, 0xbf, 0xce, 0x91, 0x44, 0x1d, 0x23, 0xde)

//************************************************************************************************
// IWebResponse
/** Response interface for protocols like HTTP. */
//************************************************************************************************

interface IWebResponse: IUnknown
{
	/** Get underlying network stream. */
	virtual IStream* CCL_API getStream () = 0;

	/** Get associated header collection. */
	virtual IWebHeaderCollection* CCL_API getWebHeaders () = 0;

	DECLARE_IID (IWebResponse)
};

DEFINE_IID (IWebResponse, 0x219096f3, 0x4a8a, 0x43c7, 0xae, 0xab, 0x5a, 0x83, 0x74, 0xce, 0xa0, 0xc6)

} // namespace Web
} // namespace CCL

#endif // _ccl_iwebrequest_h
