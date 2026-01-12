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
// Filename    : ccl/public/network/web/ixmlhttprequest.h
// Description : XMLHttpRequest interface 
//
//************************************************************************************************

#ifndef _ccl_ixmlhttprequest_h
#define _ccl_ixmlhttprequest_h

#include "ccl/public/base/variant.h"
#include "ccl/public/text/cstring.h"
#include "ccl/public/base/iasyncoperation.h"

namespace CCL {

interface IProgressNotify;

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID 
{
	DEFINE_CID (XMLHttpRequest, 0xeabd31c, 0x6cd1, 0x4ff4, 0xa7, 0xc0, 0xb2, 0xb9, 0xf8, 0x54, 0xa7, 0xf3)
}

namespace Web {

interface IWebHeaderCollection;

//************************************************************************************************
// IXMLHttpRequest
/**	Nearly W3C-compliant XMLHttpRequest API definition (see http://www.w3.org/TR/XMLHttpRequest/) 

	Threading Policy: 
	The XMLHttpRequest object itself is not thread-safe. It can be used either in synchronous or
	asynchronous mode. In synchronous mode no events will be signaled.
*/
//************************************************************************************************

interface IXMLHttpRequest: IAsyncInfo
{
	//////////////////////////////////////////////////////////////////////////////////////////////
	// States
	//////////////////////////////////////////////////////////////////////////////////////////////

	DEFINE_ENUM (ReadyState)
	{
		kUnsent = 0,			///< open() has not been called yet.
		kOpened = 1,			///< send() has not been called yet.
		kHeadersReceived = 2,	///< send() has been called and headers are available.
		kLoading = 3,			///< The response entity body is being received.
		kDone = 4				///< The operation is complete.
	};

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Events
	//////////////////////////////////////////////////////////////////////////////////////////////

	DECLARE_STRINGID_MEMBER (kOnLoadStart)			///< When the request starts.
	DECLARE_STRINGID_MEMBER (kOnProgress)			///< While sending and loading data. (arg[0]: progress value)
	DECLARE_STRINGID_MEMBER (kOnAbort)				///< When the request has been aborted. For instance, by invoking the abort() method.
	DECLARE_STRINGID_MEMBER (kOnError)				///< When the request has failed at network level (not application level with HTTP error code)
	DECLARE_STRINGID_MEMBER (kOnLoad)				///< When the request has successfully completed.
	DECLARE_STRINGID_MEMBER (kOnLoadEnd)			///< When the request has completed (either in success or failure).
	DECLARE_STRINGID_MEMBER (kOnReadyStateChange)	///< When the readyState property changes

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Properties
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Returns the current state [IObject "readyState"]. */
	virtual ReadyState CCL_API getReadyState () const = 0;

	/** Represents HTTP response as stream. */
	virtual IStream* CCL_API getResponseStream () const = 0;

	/** Returns the HTTP status code returned by a request [IObject "status"]. */
	virtual int CCL_API getStatus () const = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Methods
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Cancels the current HTTP request. */
	virtual tresult CCL_API abort () = 0;

	/** Initializes the request and specifies the method, URL, and authentication information. */
	virtual tresult CCL_API open (StringID method, UrlRef url, tbool async = true, 
								  StringRef user = nullptr, StringRef password = nullptr, StringRef authType = nullptr) = 0;

	/** Specifies an HTTP request header. */
	virtual tresult CCL_API setRequestHeader (StringID header, StringID value) = 0;

	/**	Sends an HTTP request to the server and receives a response.
		The progress callback interface can be used for synchronous requests. */
	virtual tresult CCL_API send (VariantRef data = 0, IProgressNotify* progress = nullptr) = 0;

	/** Returns all headers from the response. */
	virtual IWebHeaderCollection* CCL_API getAllResponseHeaders () const = 0;
	
	/** Returns the header field value from the response of which the field name matches id. */
	virtual tresult CCL_API getResponseHeader (CString& result, StringID id) const = 0;

	DECLARE_IID (IXMLHttpRequest)
};

DEFINE_IID (IXMLHttpRequest, 0x909be1f, 0x9a47, 0x4767, 0xa3, 0x88, 0x1d, 0xdb, 0xd1, 0x2c, 0xfd, 0x3f)

DEFINE_STRINGID_MEMBER (IXMLHttpRequest, kOnLoadStart, "onloadstart")
DEFINE_STRINGID_MEMBER (IXMLHttpRequest, kOnProgress, "onprogress")
DEFINE_STRINGID_MEMBER (IXMLHttpRequest, kOnAbort, "onabort")
DEFINE_STRINGID_MEMBER (IXMLHttpRequest, kOnError, "onerror")
DEFINE_STRINGID_MEMBER (IXMLHttpRequest, kOnLoad, "onload")
DEFINE_STRINGID_MEMBER (IXMLHttpRequest, kOnLoadEnd, "onloadend")
DEFINE_STRINGID_MEMBER (IXMLHttpRequest, kOnReadyStateChange, "onreadystatechange")

} // namespace Web
} // namespace CCL

#endif // _ccl_ixmlhttprequest_h
