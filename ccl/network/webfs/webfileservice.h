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
// Filename    : ccl/network/webfs/webfileservice.h
// Description : Web File Service
//
//************************************************************************************************

#ifndef _ccl_webfileservice_h
#define _ccl_webfileservice_h

#include "ccl/base/singleton.h"

#include "ccl/public/network/web/iwebfileservice.h"

namespace CCL {

namespace Threading {
interface IThreadPool; }

namespace Web {

class Volume;
class VolumeHandler;

//************************************************************************************************
// WebFileService
//************************************************************************************************

class WebFileService: public Object,
				  	  public IWebFileService,
				  	  public Singleton<WebFileService>
{
public:
	DECLARE_CLASS (WebFileService, Object)

	WebFileService ();
	~WebFileService ();

	VolumeHandler& getVolumes ();

	// IWebFileService
	tresult CCL_API mountFileServer (UrlRef serverUrl, StringRef name, StringRef label, 
									 IWebCredentials* credentials = nullptr, StringRef type = nullptr,
									 IUnknown* serverHandler = nullptr) override;
	tresult CCL_API unmountFileServer (StringRef name, tbool deferred = false) override;
	tresult CCL_API remountFileServer (StringRef name, IWebCredentials* newCredentials, const IUrl* newUrl = nullptr) override;
	tbool CCL_API isMounted (UrlRef serverUrl, IWebCredentials* credentials = nullptr) override;
	tresult CCL_API translateServerUrl (IUrl& webfsUrl, UrlRef serverUrl, IWebCredentials* credentials = nullptr) override;
	tresult CCL_API translateWebfsUrl (IUrl& serverUrl, UrlRef webfsUrl) override;
	tresult CCL_API terminate () override;
	tresult CCL_API openHandler (UrlRef webfsUrl, UIDRef iid, void** object) override;
	IFileDescriptor* CCL_API openFileItem (UrlRef webfsUrl) override;
	tresult CCL_API requestDirectory (IObserver* observer, UrlRef webfsUrl, tbool async = true) override;
	tresult CCL_API discardDirectory (UrlRef webfsUrl, tbool async = true) override;
	tresult CCL_API scheduleTask (IObserver* observer, UrlRef webfsUrl, IFileTask* task) override;
	tresult CCL_API cancelOperation (IObserver* observer) override;
	IRemoteSession* CCL_API openSession (UrlRef webfsUrl) override;
	ISearcher* CCL_API createSearcher (ISearchDescription& description) override;
	tresult CCL_API createDownload (ITransfer*& transfer, UrlRef webfsUrl, UrlRef localPath) override;
	tresult CCL_API createUpload (ITransfer*& transfer, UrlRef webfsUrl, UrlRef localPath) override;
	ITriggerAction* CCL_API createDirectoryChangedAction (UrlRef webfsUrl) override;

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	CLASS_INTERFACE (IWebFileService, Object)

protected:
	VolumeHandler* volumeHandler;
	Threading::IThreadPool* fileWorker;
	int currentInsertPosition;

	MutableCString getClientProtocol (UrlRef url) const;
};

} // namespace Web
} // namespace CCL

#endif // _ccl_webfileservice_h
