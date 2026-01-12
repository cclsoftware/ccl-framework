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
// Filename    : ccl/public/network/web/iwebservice.h
// Description : Web Service Interface
//
//************************************************************************************************

#ifndef _ccl_iwebservice_h
#define _ccl_iwebservice_h

#include "ccl/public/base/iunknown.h"
#include "ccl/public/text/cstring.h"

namespace CCL {

interface IStream;
interface IObserver;
interface IProgressNotify;

namespace Web {

interface IWebClient;
interface IWebServer;
interface IWebNewsReader;
interface IWebCredentials;
interface IWebHeaderCollection;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Web Service Definitions
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Meta
{
	// *** Protocols ***
	DEFINE_STRINGID (kHTTP, "http")
	DEFINE_STRINGID (kHTTPS, "https")

	// *** Content Types ***
	DEFINE_STRINGID (kFormContentType, "application/x-www-form-urlencoded")
	DEFINE_STRINGID (kMultipartFormData, "multipart/form-data")
	DEFINE_STRINGID (kBinaryContentType, "application/octet-stream")

	// *** Messages ***
	/** arg[0]: tresult, arg[1]: status (int, optional) */
	DEFINE_STRINGID (kDownloadComplete, "downloadComplete")

	/** arg[0]: tresult, args[1]: status (int, optional), arg[2]: response (IStream, optional) */
	DEFINE_STRINGID (kUploadComplete, "uploadComplete")

	/**	arg[0]: content length (bytes) args[1]: protocol headers (IWebHeaderCollection, optional).
		Can be sent multiple times for chunked transfers, check with IWebHeaderCollection::isChunkedTransfer(). */
	DEFINE_STRINGID (kContentLengthNotify, "contentLengthNotify")

	/** args[0]: progress value (float), args[1]: progress flags (int) */
	DEFINE_STRINGID (kBackgroundProgressNotify, "progressNotify")
}

//************************************************************************************************
// IWebService
/** Service interface for web-aware applications. */
//************************************************************************************************

interface IWebService: IUnknown
{
	//////////////////////////////////////////////////////////////////////////////////////////////
	// Configuration
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Set user agent string used in HTTP transactions. */
	virtual tresult CCL_API setUserAgent (StringRef userAgent) = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Factory Methods
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Create client instance for given protocol (e.g. HTTP). */
	virtual IWebClient* CCL_API createClient (StringID protocol) = 0;

	/** Create server instance for given protocol (e.g. HTTP). */
	virtual IWebServer* CCL_API createServer (StringID protocol) = 0;

	/** Create feed reader (Atom/RSS). */
	virtual IWebNewsReader* CCL_API createReader () = 0;

	/** Create credentials object. */
	virtual IWebCredentials* CCL_API createCredentials () = 0;

	/** Create header collection object. */
	virtual IWebHeaderCollection* CCL_API createHeaderCollection () = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Uploads/Downloads
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Download remote resource to local storage. Returns result at network level, use status for application level. */
	virtual tresult CCL_API downloadData (UrlRef remotePath, IStream& localStream,
							IWebCredentials* credentials = nullptr, IWebHeaderCollection* headers = nullptr,
							IProgressNotify* progress = nullptr, int* status = nullptr) = 0;

	/** Download remote resource asynchronously. Observer receives kBackgroundProgressNotify + kDownloadComplete messages. */
	virtual tresult CCL_API downloadInBackground (IObserver* observer, UrlRef remotePath, IStream& localStream,
							IWebCredentials* credentials = nullptr, IWebHeaderCollection* headers = nullptr) = 0;

	/** Upload data. Headers must include content type. Returns result at network level, use status for application level. */
	virtual tresult CCL_API uploadData (UrlRef remotePath, IStream& data, IWebHeaderCollection* headers, IStream& response,
							StringID method = nullptr, IWebCredentials* credentials = nullptr, IProgressNotify* progress = nullptr, int* status = nullptr) = 0;

	/** Upload data asynchronously. Headers must include content type. Observer receives kBackgroundProgressNotify + kUploadComplete messages. */
	virtual tresult CCL_API uploadInBackground (IObserver* observer, UrlRef remotePath, IStream& localStream, IWebHeaderCollection* headers,
							StringID method = nullptr, IWebCredentials* credentials = nullptr) = 0;

	/** Cancel asynchronous upload or download. */
	virtual tresult CCL_API cancelOperation (IObserver* observer) = 0;

	/** Mark all asynchronous operations canceled on program exit.
		Note that this doesn't wait for them to be finished so cancelOperation() still needs to be called individually
		but with the benefit of operations not waiting on each other. */
	virtual tresult CCL_API cancelOnExit () = 0;

	DECLARE_IID (IWebService)
};

DEFINE_IID (IWebService, 0x28915aa, 0xed87, 0x4ea3, 0xa6, 0xb0, 0x68, 0xb1, 0xfb, 0x19, 0x2d, 0xf)

} // namespace Web
} // namespace CCL

#endif // _ccl_iwebservice_h
