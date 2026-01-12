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
// Filename    : ccl/public/network/web/iwebclient.h
// Description : Web Client Interface
//
//************************************************************************************************

#ifndef _ccl_iwebclient_h
#define _ccl_iwebclient_h

#include "ccl/public/base/iunknown.h"
#include "ccl/public/text/cstring.h"

namespace CCL {

interface IStream;
interface IProgressNotify;

namespace Web {

interface IWebCredentials;
interface IWebHeaderCollection;

//************************************************************************************************
// IWebClient
/** Client interface for protocols like HTTP. */
//************************************************************************************************

interface IWebClient: IUnknown
{
	/** Supply credentials. */
	virtual void CCL_API setCredentials (IWebCredentials* credentials) = 0;

	/** Connect to given host. */
	virtual tresult CCL_API connect (StringRef hostname) = 0;

	/** Disconnect from host. */
	virtual tresult CCL_API disconnect () = 0;

	/** Check if client is currently connected. */
	virtual tbool CCL_API isConnected () = 0;

	/** Get most recent status code of underlying protocol. */
	virtual int CCL_API getLastStatus () = 0;

	/** Download data from remote to local storage. */
	virtual tresult CCL_API downloadData (StringRef remotePath, 
										  IStream& localStream,
										  IWebHeaderCollection* headers = nullptr,
										  IProgressNotify* progress = nullptr) = 0;

	/** Upload data from local to remote storage. Headers must include content type. */
	virtual tresult CCL_API uploadData (IWebHeaderCollection* headers,
										IStream& localStream,
										StringRef remotePath, 
										IStream& responseStream,
										StringID method = nullptr,
										IProgressNotify* progress = nullptr) = 0;

	/** Set option for the web client operation */
	virtual tresult CCL_API setOption (StringID optionId, VariantRef value) = 0;

	/** Client options */
	DECLARE_STRINGID_MEMBER (kUncached)
	DECLARE_STRINGID_MEMBER (kSilent)
	
	DECLARE_IID (IWebClient)
};

DEFINE_IID (IWebClient, 0x5cda5c33, 0xd396, 0x4206, 0x9f, 0x2e, 0xb, 0xa, 0x2b, 0xbd, 0xc7, 0x52)
DEFINE_STRINGID_MEMBER (IWebClient, kUncached, "uncached")
DEFINE_STRINGID_MEMBER (IWebClient, kSilent, "silent")

} // namespace Web
} // namespace CCL

#endif // _ccl_iwebclient_h
