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
// Filename    : ccl/public/network/web/iwebsocket.h
// Description : WebSocket interface 
//
//************************************************************************************************

#ifndef _ccl_iwebsocket_h
#define _ccl_iwebsocket_h

#include "ccl/public/base/variant.h"

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID 
{
	DEFINE_CID (WebSocket, 0x3e8ea54b, 0xe756, 0x4eb6, 0xba, 0x89, 0x6a, 0x57, 0x3b, 0xc0, 0xc8, 0xb4)
}

namespace Web {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Web Socket Definitions
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Meta
{
	// WebSocket URI schemes
	DEFINE_STRINGID (kWS, "ws")		///< not secure
	DEFINE_STRINGID (kWSS, "wss")	///< secure
}

//************************************************************************************************
// IWebSocket
//************************************************************************************************

interface IWebSocket: IUnknown
{
	//////////////////////////////////////////////////////////////////////////////////////////////
	// States
	//////////////////////////////////////////////////////////////////////////////////////////////

	DEFINE_ENUM (ReadyState)
	{
		kConnecting = 0,
		kOpen = 1,
		kClosing = 2,
		kClosed = 3
	};

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Events
	//////////////////////////////////////////////////////////////////////////////////////////////

	DECLARE_STRINGID_MEMBER (kOnOpen)				///< When connection is opened
	DECLARE_STRINGID_MEMBER (kOnClose)				///< When connection is closed
	DECLARE_STRINGID_MEMBER (kOnMessage)			///< When a message is received, args[0]: data (string or memory stream)
	DECLARE_STRINGID_MEMBER (kOnError)				///< When an error occurred
	DECLARE_STRINGID_MEMBER (kOnReadyStateChange)	///< When the readyState property changes

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Properties
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Returns the current state [IObject "readyState"]. */
	virtual ReadyState CCL_API getReadyState () const = 0;

	/** Get number of bytes queued [IObject "bufferedAmount"]. */
	virtual uint32 CCL_API getBufferedAmount () const = 0;

	/** Get extensions selected by server [IObject "extensions"]. */
	virtual StringRef CCL_API getExtensions () const = 0;
	
	/** Get sub-protocl selected by server [IObject "protocol"]. */
	virtual StringRef CCL_API getProtocol () const = 0;

	/** Get URL passed to open method [IObject "url"]. */
	virtual UrlRef CCL_API getUrl () const = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Methods
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Open connection. */
	virtual tresult CCL_API open (UrlRef url, VariantRef protocols = 0) = 0;

	/** Close connection. */
	virtual tresult CCL_API close (int code = 0, StringRef reason = nullptr) = 0;

	/**	Send data (string or binary). */
	virtual tresult CCL_API send (VariantRef data) = 0;

	DECLARE_IID (IWebSocket)
};

DEFINE_IID (IWebSocket, 0x6a06c18a, 0xc73c, 0x42a2, 0xa2, 0xa6, 0x63, 0xb7, 0x5, 0x50, 0x93, 0xe3)
DEFINE_STRINGID_MEMBER (IWebSocket, kOnOpen, "onopen")
DEFINE_STRINGID_MEMBER (IWebSocket, kOnClose, "onclose")
DEFINE_STRINGID_MEMBER (IWebSocket, kOnMessage, "onmessage")
DEFINE_STRINGID_MEMBER (IWebSocket, kOnError, "onerror")
DEFINE_STRINGID_MEMBER (IWebSocket, kOnReadyStateChange, "onreadystatechange")

} // namespace Web
} // namespace CCL

#endif // _ccl_iwebsocket_h
