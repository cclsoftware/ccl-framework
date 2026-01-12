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
// Filename    : ccl/network/webfs/webfilesession.h
// Description : WebFS Remote Session
//
//************************************************************************************************

#ifndef _ccl_webfilesession_h
#define _ccl_webfilesession_h

#include "ccl/base/object.h"

#include "ccl/public/system/ifilesystem.h"

#include "ccl/public/network/web/iwebfiletask.h"

namespace CCL {
namespace Web {

class Volume;

//************************************************************************************************
// RemoteSession
//************************************************************************************************

class RemoteSession: public Object,
					 public IRemoteSession,
					 public IFileSystem
{
public:
	DECLARE_CLASS_ABSTRACT (RemoteSession, Object)

	RemoteSession (Volume& volume, IWebFileClient& client, bool ownsConnection = false);
	~RemoteSession ();

	// IRemoteSession
	IWebFileClient& CCL_API getClient () override;
	IFileSystem& CCL_API getFileSystem () override;
	void CCL_API getRemotePath (String& remotePath, UrlRef webfsUrl) override;
	void CCL_API getWebfsUrl (IUrl& webfsUrl, StringRef remotePath) override;
	tbool CCL_API downloadFile (UrlRef webfsUrl, UrlRef localPath, IProgressNotify* progress = nullptr) override;
	tbool CCL_API uploadFile (UrlRef localPath, UrlRef webfsUrl, IProgressNotify* progress = nullptr) override;

	// IFileSystem
	IStream* CCL_API openStream (UrlRef url, int mode = IStream::kOpenMode, IUnknown* context = nullptr) override;
	tbool CCL_API fileExists (UrlRef url) override;
	tbool CCL_API getFileInfo (FileInfo& info, UrlRef url) override;
	tbool CCL_API removeFile (UrlRef url, int mode = 0) override;
	tbool CCL_API renameFile (UrlRef url, StringRef newName, int mode = 0) override;
	IFileIterator* CCL_API newIterator (UrlRef url, int mode = IFileIterator::kAll) override;
	tbool CCL_API createFolder (UrlRef url) override;
	tbool CCL_API removeFolder (UrlRef url, int mode = 0) override;
	tbool CCL_API isCaseSensitive () override;

	CLASS_INTERFACE2 (IRemoteSession, IFileSystem, Object)

protected:
	class RemotePath;
	class FileIterator;

	Volume& volume;
	IWebFileClient& client;
	bool ownsConnection;

	tbool createParentFolder (UrlRef webfsUrl);
};

} // namespace Web
} // namespace CCL

#endif // _ccl_webfilesession_h
