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
// Filename    : ccl/network/web/webservice.h
// Description : Web Service
//
//************************************************************************************************

#ifndef _ccl_webservice_h
#define _ccl_webservice_h

#include "ccl/base/singleton.h"
#include "ccl/public/collections/linkedlist.h"

#include "ccl/public/network/web/iwebservice.h"
#include "ccl/public/network/web/iwebprotocol.h"

namespace CCL {
namespace Web {

//************************************************************************************************
// WebService
//************************************************************************************************

class WebService: public Object,
				  public IWebService,
				  public IWebProtocolRegistrar,
				  public Singleton<WebService>
{
public:
	DECLARE_CLASS (WebService, Object)

	WebService ();
	~WebService ();

	// IWebService
	IWebClient* CCL_API createClient (StringID protocol) override;
	IWebServer* CCL_API createServer (StringID protocol) override;
	IWebNewsReader* CCL_API createReader () override;
	IWebCredentials* CCL_API createCredentials () override;
	IWebHeaderCollection* CCL_API createHeaderCollection () override;
	tresult CCL_API downloadData (UrlRef remotePath, IStream& localStream, 
					IWebCredentials* credentials = nullptr, IWebHeaderCollection* headers = nullptr, 
					IProgressNotify* progress = nullptr, int* status = nullptr) override;
	tresult CCL_API downloadInBackground (IObserver* observer, UrlRef remotePath, IStream& localStream,
					IWebCredentials* credentials = nullptr, IWebHeaderCollection* headers = nullptr) override;
	tresult CCL_API uploadData (UrlRef remotePath, IStream& localStream, IWebHeaderCollection* headers, IStream& response, 
					StringID method = nullptr, IWebCredentials* credentials = nullptr, IProgressNotify* progress = nullptr, int* status = nullptr) override;
	tresult CCL_API uploadInBackground (IObserver* observer, UrlRef remotePath, IStream& localStream, IWebHeaderCollection* headers,
					StringID method = nullptr, IWebCredentials* credentials = nullptr) override;
	tresult CCL_API cancelOperation (IObserver* observer) override;
	tresult CCL_API cancelOnExit () override;
	tresult CCL_API setUserAgent (StringRef userAgent) override;

	// IWebProtocolRegistrar
	tresult CCL_API registerProtocol (IWebClientProtocol* protocol) override;
	tresult CCL_API unregisterProtocol (IWebClientProtocol* protocol) override;
	tresult CCL_API registerProtocolPlugins () override;
	tresult CCL_API unregisterProtocolPlugins () override;

	CLASS_INTERFACE2 (IWebService, IWebProtocolRegistrar, Object)

protected:
	LinkedList<IWebClientProtocol*> protocols;
	LinkedList<IWebClientProtocol*> protocolPlugins;
};

} // namespace Web
} // namespace CCL

#endif // _ccl_webservice_h
