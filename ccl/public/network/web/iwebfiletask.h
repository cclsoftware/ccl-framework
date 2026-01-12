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
// Filename    : ccl/public/network/web/iwebfiletask.h
// Description : Web File Task Interface
//
//************************************************************************************************

#ifndef _ccl_iwebfiletask_h
#define _ccl_iwebfiletask_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

interface IFileSystem;
interface IProgressNotify;

namespace Web {

interface IWebFileClient;

//************************************************************************************************
// IRemoteSession
//************************************************************************************************

interface IRemoteSession: IUnknown
{
	/** Get file client interface. */
	virtual IWebFileClient& CCL_API getClient () = 0;

	/** Get file system interface. */
	virtual IFileSystem& CCL_API getFileSystem () = 0;

	/** Translate WebFS URL to remote path. */
	virtual void CCL_API getRemotePath (String& remotePath, UrlRef webfsUrl) = 0;

	/** Translate remote path to WebFS URL. */
	virtual void CCL_API getWebfsUrl (IUrl& webfsUrl, StringRef remotePath) = 0;

	/** Download file from remote to local system. */
	virtual tbool CCL_API downloadFile (UrlRef webfsUrl, UrlRef localPath, IProgressNotify* progress = nullptr) = 0;

	/** Upload file from local to remote system. */
	virtual tbool CCL_API uploadFile (UrlRef localPath, UrlRef webfsUrl, IProgressNotify* progress = nullptr) = 0;

	DECLARE_IID (IRemoteSession)
};

DEFINE_IID (IRemoteSession, 0x46bad4a6, 0xceb3, 0x4d1a, 0xbb, 0x3e, 0xc0, 0x66, 0xc6, 0xb5, 0xc0, 0x65)

//************************************************************************************************
// IFileTask
//************************************************************************************************

interface IFileTask: IUnknown
{
	/** Perform file task within given session. */
	virtual tresult CCL_API perform (UrlRef webfsUrl, IRemoteSession& session) = 0;

	DECLARE_IID (IFileTask)
};

DEFINE_IID (IFileTask, 0x104535ce, 0xbb83, 0x48af, 0xa2, 0x2c, 0x9d, 0x7d, 0x1f, 0x82, 0x6c, 0xda)

} // namespace Web
} // namespace CCL

#endif // _ccl_iwebfiletask_h
