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
// Filename    : core/public/corehttp.h
// Description : HTTP Definitions
//
//************************************************************************************************

#ifndef _corehttp_h
#define _corehttp_h

#include "core/public/coremacros.h"
#include "core/public/corestringbuffer.h"

namespace Core {

namespace HTTPDefinitions
{
	//////////////////////////////////////////////////////////////////////////////////////////////
	// Version
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** HTTP versions. */
	enum Version
	{
		kV1_0  = 100,	///< "HTTP/1.0"
		kV1_1  = 110	///< "HTTP/1.1"
	};

	const CStringPtr kV1_0_String = "HTTP/1.0";
	const CStringPtr kV1_1_String = "HTTP/1.1";

	/** Get string by version number. */
	static CStringPtr getVersionString (int version)
	{
		if(version == kV1_1)
			return kV1_1_String;
		ASSERT (version == kV1_0)
		return kV1_0_String;
	}

	/** Get version number by string. */
	static int getVersionNumber (CStringPtr string)
	{
		if(ConstString (string) == kV1_1_String)
			return kV1_1;
		ASSERT (ConstString (string) == kV1_0_String)
		return kV1_0;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Methods
	//////////////////////////////////////////////////////////////////////////////////////////////

	const CStringPtr kGET = "GET";
	const CStringPtr kHEAD = "HEAD";
	const CStringPtr kPOST = "POST";
	const CStringPtr kPUT = "PUT";
	const CStringPtr kPATCH = "PATCH";
	const CStringPtr kDELETE = "DELETE";

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Status Codes
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** HTTP status codes. */
	enum StatusCodes
	{
		// 1xx indicates an informational message only
		kContinue			= 100,
		kSwitchingProtocols = 101,
		
		// 2xx indicates success of some kind
		kOK					= 200,
		kCreated			= 201,
		kNoContent			= 204,
		kPartialContent		= 206,
		kMultipleStatus		= 207,
		
		// 3xx redirects the client to another URL
		kMultipleChoices	= 300,
		kMovedPermanently	= 301,
		kMovedTemporarily	= 302, // aka "Found"
		kTemporaryRedirect	= 307,
		kPermanentRedirect	= 308,
		
		// 4xx indicates an error on the client's part
		kBadRequest			= 400,
		kUnauthorized		= 401,
		kForbidden			= 403,
		kNotFound			= 404,
		kMethodNotAllowed	= 405,
		kPayloadTooLarge    = 413,

		// 5xx indicates an error on the server's part
		kServerError		= 500,
		kNotImplemented		= 501,
		kServiceUnavailable	= 503,
		kInsufficientSpace  = 507
	};

	/** Check for success code. */
	static bool isSuccessStatus (int status)
	{
		return status >= kOK && status < kMultipleChoices;
	}

	/** Check for error code. */
	static bool isErrorStatus (int status)
	{
		return status >= kBadRequest;
	}

	/** Check for redirect status. */
	static bool isRedirectStatus (int status)
	{
		return	status == kMovedPermanently || status == kMovedTemporarily ||
				status == kTemporaryRedirect || status == kPermanentRedirect;
	}

	/** Check for authorization error. */
	static bool isUnauthorized (int status)
	{
		return status == kUnauthorized;
	}

	/** Get description by status code. */
	static CStringPtr getStatusString (int status)
	{
		static const struct { int status; CStringPtr text; } descriptions[] =
		{
			{kContinue,				"Continue"},
			{kSwitchingProtocols,	"Switching Protocols"},

			{kOK,					"OK"},
			{kCreated,				"Created"},
			{kNoContent,			"No Content"},
			{kPartialContent,		"Partial Content"},
			{kMultipleStatus,		"Multiple Status"},
			
			{kMultipleChoices,		"Multiple Choices"},
			{kMovedPermanently,		"Moved Permanently"},
			{kMovedTemporarily,		"Moved Temporarily"},
			{kTemporaryRedirect,	"Temporary Redirect"},
			{kPermanentRedirect,	"Permanent Redirect"},

			{kBadRequest,			"Bad Request"},
			{kUnauthorized,			"Unauthorized"},
			{kForbidden,			"Forbidden"},
			{kNotFound,				"Not Found"},
			{kMethodNotAllowed,		"Method not allowed"},

			{kServerError,			"Server Error"},
			{kNotImplemented,		"Not Implemented"},
			{kServiceUnavailable,	"Service Unavailable"},
			{kInsufficientSpace,	"Insufficient Space"}
		};

		for(int i = 0; i < ARRAY_COUNT (descriptions); i++)
			if(status == descriptions[i].status)
				return descriptions[i].text;

		return "Unknown";
	}
}

} // namespace Core 

#endif // _corehttp_h
