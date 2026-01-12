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
// Filename    : ccl/network/web/webservice.cpp
// Description : Web Service
//
//************************************************************************************************

#define DEBUG_LOG 1

#include "ccl/network/web/webservice.h"
#include "ccl/network/web/xmlnewsreader.h"
#include "ccl/network/web/localclient.h"

#include "ccl/network/web/http/client.h"
#include "ccl/network/web/http/server.h"
#include "ccl/network/web/xmlhttprequest.h"
#include "ccl/network/web/websocket.h"

#include "ccl/base/message.h"
#include "ccl/base/storage/url.h"

#include "ccl/public/base/iprogress.h"
#include "ccl/public/base/memorystream.h"
#include "ccl/public/system/ithreadpool.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/plugservices.h"
#include "ccl/public/netservices.h"

namespace CCL {
namespace Web {

//************************************************************************************************
// WebWorkItem
//************************************************************************************************

class WebWorkItem: public Object,
				   public Threading::AbstractWorkItem,
				   public AbstractProgressNotify
{
public:
	WebWorkItem (IObserver* observer, UrlRef remotePath, IStream& localStream, IWebCredentials* credentials);

	static void cancelOnExit ();

	// IWorkItem
	void CCL_API cancel () override;

	// IProgressNotify
	tbool CCL_API isCanceled () override;
	void CCL_API updateProgress (const State& state) override;

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	CLASS_INTERFACE2 (IWorkItem, IProgressNotify, Object)

protected:
	static bool exiting;

	IObserver* observer;
	AutoPtr<Url> remotePath;
	SharedPtr<IStream> localStream;
	SharedPtr<IWebCredentials> credentials;
	bool canceled;

	void sendInitialNotification ();
};

//************************************************************************************************
// DownloadWork
//************************************************************************************************

class DownloadWork: public WebWorkItem
{
public:
	DownloadWork (IObserver* observer, UrlRef remotePath, IStream& localStream, IWebCredentials* credentials,
				  IWebHeaderCollection* headers);

	// WebWorkItem
	void CCL_API work () override;

protected:
	SharedPtr<IWebHeaderCollection> headers;
};

//************************************************************************************************
// UploadWork
//************************************************************************************************

class UploadWork: public WebWorkItem
{
public:
	UploadWork (IObserver* observer, UrlRef remotePath, IStream& localStream, 
				IWebHeaderCollection* headers, StringID method, IWebCredentials* credentials);

	// WebWorkItem
	void CCL_API work () override;

protected:
	SharedPtr<IWebHeaderCollection> headers;
	MutableCString method;
	AutoPtr<MemoryStream> responseStream;
};

} // namespace Web
} // namespace CCL

using namespace CCL;
using namespace Web;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Network Services API
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT IWebService& CCL_API System::CCL_ISOLATED (GetWebService) ()
{
	return WebService::instance ();
}

//************************************************************************************************
// WebService
//************************************************************************************************

DEFINE_CLASS_HIDDEN (WebService, Object)
DEFINE_SINGLETON (WebService)

//////////////////////////////////////////////////////////////////////////////////////////////////

WebService::WebService ()
{
	XMLHttpRequest::forceLinkage ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WebService::~WebService ()
{
	ASSERT (protocols.isEmpty ())
	ASSERT (protocolPlugins.isEmpty ())
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IWebClient* CCL_API WebService::createClient (StringID protocol)
{
	if(protocol.compare (Meta::kHTTP, false) == 0)
		return NEW HTTP::Client;

	if(protocol.compare (Meta::kHTTPS, false) == 0)
		return NEW HTTP::Client (true);

	if(LocalClient* client = LocalClient::create (protocol))
		return client;

	ListForEach (protocols, IWebClientProtocol*, p)
		if(p->getProtocol () == protocol)
			return p->createClient ();
	EndFor

	CCL_DEBUGGER ("Unknown Client Protocol!")
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IWebServer* CCL_API WebService::createServer (StringID protocol)
{
	if(protocol.compare (Meta::kHTTP, false) == 0)
		return NEW HTTP::Server;

	CCL_DEBUGGER ("Unknown Server Protocol!")
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IWebNewsReader* CCL_API WebService::createReader ()
{
	return NEW XmlNewsReader;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IWebCredentials* CCL_API WebService::createCredentials ()
{
	return NEW WebCredentials;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IWebHeaderCollection* CCL_API WebService::createHeaderCollection ()
{
	return NEW WebHeaderCollection;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WebService::downloadData (UrlRef remotePath, IStream& localStream, 
										  IWebCredentials* credentials, IWebHeaderCollection* headers,
										  IProgressNotify* progress, int* status)
{
	if(status)
		*status = 0;

	tresult result = kResultFailed;
	AutoPtr<IWebClient> client = createClient (MutableCString (remotePath.getProtocol ()));
	if(client)
	{
		client->setCredentials (credentials);
		result = client->connect (remotePath.getHostName ());
		if(result == kResultOk)
		{
			String path = UrlUtils::toResourcePath (remotePath);

			result = client->downloadData (path, localStream, headers, progress);
			if(status)
				*status = client->getLastStatus ();
		}
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WebService::downloadInBackground (IObserver* observer, UrlRef remotePath, IStream& localStream,
												  IWebCredentials* credentials, IWebHeaderCollection* headers)
{
	ASSERT (observer != nullptr)
	if(!observer)
		return kResultInvalidArgument;

	System::GetThreadPool ().scheduleWork (NEW DownloadWork (observer, remotePath, localStream, credentials, headers));
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WebService::uploadData (UrlRef remotePath, IStream& localStream, IWebHeaderCollection* headers, IStream& response, 
										StringID method, IWebCredentials* credentials, IProgressNotify* progress, int* status)
{
	if(status)
		*status = 0;

	tresult result = kResultFailed;
	AutoPtr<IWebClient> client = createClient (MutableCString (remotePath.getProtocol ()));
	if(client)
	{
		client->setCredentials (credentials);
		result = client->connect (remotePath.getHostName ());
		if(result == kResultOk)
		{
			String path = UrlUtils::toResourcePath (remotePath);

			result = client->uploadData (headers, localStream, path, response, method, progress);
			if(status)
				*status = client->getLastStatus ();
		}
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WebService::uploadInBackground (IObserver* observer, UrlRef remotePath, IStream& localStream, 
												IWebHeaderCollection* headers,
												StringID method, IWebCredentials* credentials)
{
	ASSERT (observer != nullptr)
	if(!observer)
		return kResultInvalidArgument;

	System::GetThreadPool ().scheduleWork (NEW UploadWork (observer, remotePath, localStream, headers, method, credentials));
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WebService::cancelOperation (IObserver* observer)
{
	ASSERT (observer != nullptr)
	if(!observer)
		return kResultInvalidArgument;

	System::GetThreadPool ().cancelWork (observer, true);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WebService::cancelOnExit ()
{
	WebWorkItem::cancelOnExit ();
	WebSocket::cancelOnExit ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WebService::setUserAgent (StringRef userAgent)
{
	ASSERT (HTTP::Client::getUserAgent (false).isEmpty ())
	if(!HTTP::Client::getUserAgent (false).isEmpty ()) // must be called only once!
		return kResultFailed;

	HTTP::Client::setUserAgent (MutableCString (userAgent, Text::kUTF8));
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WebService::registerProtocol (IWebClientProtocol* protocol)
{
	ASSERT (protocol != nullptr)
	if(protocol == nullptr)
		return kResultInvalidPointer;

	// check for duplicates
	ListForEach (protocols, IWebClientProtocol*, p)
		if(p->getProtocol () == protocol->getProtocol ())
		{
			CCL_DEBUGGER ("Multiple web client protocols with same identifier!!!\n")
			return kResultAlreadyExists;
		}
	EndFor

	protocols.append (protocol);
	protocol->retain ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WebService::unregisterProtocol (IWebClientProtocol* protocol)
{
	ASSERT (protocol != nullptr && protocols.contains (protocol))
	if(protocol == nullptr)
		return kResultInvalidPointer;
	if(!protocols.contains (protocol))
		return kResultInvalidArgument;

	protocols.remove (protocol);
	protocol->release ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WebService::registerProtocolPlugins ()
{
	auto protocolLoaded = [this] (const IClassDescription& description)
	{
		for(IWebClientProtocol* existing : protocols)
			if(ccl_classof (existing) == &description)
				return true;
		return false;
	};

	ForEachPlugInClass (PLUG_CATEGORY_WEBCLIENTPROTOCOL, description)
		if(protocolLoaded (description)) // instantiate only once when called multiple times
			continue;
		
		if(auto* protocol = ccl_new<IWebClientProtocol> (description.getClassID ()))
		{
			if(registerProtocol (protocol) == kResultOk)
				protocolPlugins.append (protocol);
			else
				ccl_release (protocol);
		}
	EndFor
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WebService::unregisterProtocolPlugins ()
{
	ListForEach (protocolPlugins, IWebClientProtocol*, p)
		unregisterProtocol (p);
		ccl_release (p);
	EndFor
	protocolPlugins.removeAll ();

	return kResultOk;
}

//************************************************************************************************
// WebWorkItem
//************************************************************************************************

bool WebWorkItem::exiting = false;
void WebWorkItem::cancelOnExit ()
{
	exiting = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WebWorkItem::WebWorkItem (IObserver* observer, UrlRef remotePath, IStream& localStream, IWebCredentials* credentials)
: AbstractWorkItem (observer),
  observer (observer),
  remotePath (NEW Url (remotePath)),
  localStream (&localStream),
  credentials (credentials),
  canceled (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WebWorkItem::cancel ()
{
	canceled = true; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WebWorkItem::isCanceled ()
{
	if(exiting)
	{
		CCL_PRINTLN ("Web work canceled on exit")
		return true;
	}
	return canceled;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebWorkItem::sendInitialNotification ()
{
	updateProgress (IProgressNotify::State (0., IProgressNotify::kIndeterminate));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WebWorkItem::updateProgress (const State& state)
{
	if(!canceled)
	{
		Message* m = NEW Message (Meta::kBackgroundProgressNotify, state.value, state.flags);
		m->post (observer, -1); // -1: collect similar messages
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WebWorkItem::notify (ISubject* subject, MessageRef msg)
{
	if(msg == Meta::kContentLengthNotify)
		if(!canceled)
			(NEW Message (Meta::kContentLengthNotify, msg[0], msg[1]))->post (observer, -1);
}

//************************************************************************************************
// DownloadWork
//************************************************************************************************

DownloadWork::DownloadWork (IObserver* observer, UrlRef remotePath, IStream& localStream, 
							IWebCredentials* credentials, IWebHeaderCollection* headers)
: WebWorkItem (observer, remotePath, localStream, credentials),
  headers (headers)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DownloadWork::work ()
{
	sendInitialNotification (); // let observer know we are about to start now

	int status = 0;
	tresult result = WebService::instance ().downloadData (*remotePath, *localStream, credentials, 
														   headers, this, &status);
	if(!canceled)
	{
		localStream.release (); // ensure local stream is closed before notification!

		Message* m = NEW Message (Meta::kDownloadComplete, result, status);
		m->post (observer);
	}
}

//************************************************************************************************
// UploadWork
//************************************************************************************************

UploadWork::UploadWork (IObserver* observer, UrlRef remotePath, IStream& localStream, 
						IWebHeaderCollection* headers, 
						StringID method, IWebCredentials* credentials)
: WebWorkItem (observer, remotePath, localStream, credentials),
  headers (headers),
  method (method),
  responseStream (NEW MemoryStream)
{}
  
//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API UploadWork::work ()
{
	sendInitialNotification (); // let observer know we are about to start now

	int status = 0;
	tresult result = WebService::instance ().uploadData (*remotePath, *localStream, headers, *responseStream, 
														  method, credentials, this, &status);
	if(!canceled)
	{
		localStream.release (); // ensure local stream is closed before notification!

		IStream* response = nullptr;
		if(responseStream->getBytesWritten () > 0)
			response = responseStream;

		Message* m = NEW Message (Meta::kUploadComplete, result, status, response);
		m->post (observer);
	}
}
