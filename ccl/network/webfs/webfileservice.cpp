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
// Filename    : ccl/network/webfs/webfileservice.cpp
// Description : Web File Service
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/network/webfs/webfileservice.h"
#include "ccl/network/webfs/webfilesystem.h"
#include "ccl/network/webfs/webfilesession.h"
#include "ccl/network/webfs/webfilesearcher.h"
#include "ccl/network/web/transfermanager.h"

#include "ccl/base/message.h"
#include "ccl/base/signalsource.h"

#include "ccl/public/base/iprogress.h"
#include "ccl/public/system/ifileitem.h"
#include "ccl/public/system/ithreadpool.h"
#include "ccl/public/system/cclerror.h"
#include "ccl/public/network/web/iwebservice.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/netservices.h"

namespace CCL {
namespace Web {

//************************************************************************************************
// FileRequest
//************************************************************************************************

class FileRequest: public Object,
				   public Threading::AbstractWorkItem,
				   public AbstractProgressNotify
{
public:
	FileRequest (IObserver* observer, UrlRef webfsUrl);

	// IWorkItem
	void CCL_API cancel () override;

	// IProgressNotify
	tbool CCL_API isCanceled () override;

	CLASS_INTERFACE2 (IWorkItem, IProgressNotify, Object)

protected:
	IObserver* observer;
	Url webfsUrl;
	bool canceled;

	Volume* openVolume ();
};

//************************************************************************************************
// DiscardDirectoryRequest
//************************************************************************************************

class DiscardDirectoryRequest: public FileRequest
{
public:
	DiscardDirectoryRequest (UrlRef webfsUrl);

	// FileRequest
	void CCL_API work () override;
};


//************************************************************************************************
// GetDirectoryRequest
//************************************************************************************************

class GetDirectoryRequest: public FileRequest
{
public:
	GetDirectoryRequest (IObserver* observer, UrlRef webfsUrl);

	PROPERTY_VARIABLE (tresult, workResult, WorkResult)
	
	// FileRequest
	void CCL_API work () override;
};

//************************************************************************************************
// FileTaskRequest
//************************************************************************************************

class FileTaskRequest: public FileRequest
{
public:
	FileTaskRequest (IObserver* observer, UrlRef webfsUrl, IFileTask* task);

	// FileRequest
	void CCL_API work () override;

protected:
	SharedPtr<IFileTask> task;
};

//************************************************************************************************
// DirectoryChangedAction
//************************************************************************************************

class DirectoryChangedAction: public TriggerAction
{
public:
	DirectoryChangedAction (UrlRef webfsUrl);

	PROPERTY_OBJECT (Url, webfsUrl, WebFSUrl)

	// TriggerAction
	void CCL_API execute (IObject* target) override;
};

} // namespace Web
} // namespace CCL

using namespace CCL;
using namespace Web;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Network Services API
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT IWebFileService& CCL_API System::CCL_ISOLATED (GetWebFileService) ()
{
	return WebFileService::instance ();
}

//************************************************************************************************
// WebFileService
//************************************************************************************************

static const CString kUnmountFileServer ("unmountFileServer");

DEFINE_CLASS_HIDDEN (WebFileService, Object)
DEFINE_SINGLETON (WebFileService)

//////////////////////////////////////////////////////////////////////////////////////////////////

WebFileService::WebFileService ()
: volumeHandler (NEW VolumeHandler),
  fileWorker (nullptr),
  currentInsertPosition (-1)
{
	UnknownPtr<IProtocolHandlerRegistry> registry (&System::GetFileSystem ());
	ASSERT (registry != nullptr)
	if(registry)
		registry->registerProtocol (volumeHandler);

	// Note: All asynchronous requests except data upload/downloads are serialized to a single thread!
	fileWorker = System::CreateThreadPool ({1, Threading::kPriorityBelowNormal, "WebFileService"});
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WebFileService::~WebFileService ()
{
	cancelSignals ();

	ASSERT (fileWorker == nullptr) // terminate has to be called!
	safe_release (fileWorker);

	UnknownPtr<IProtocolHandlerRegistry> registry (&System::GetFileSystem ());
	if(registry)
		registry->unregisterProtocol (volumeHandler);

	volumeHandler->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

VolumeHandler& WebFileService::getVolumes ()
{
	return *volumeHandler;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WebFileService::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kUnmountFileServer)
	{
		String name (msg[0].asString ());
		unmountFileServer (name, false);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString WebFileService::getClientProtocol (UrlRef url) const
{
	return MutableCString (url.getProtocol ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WebFileService::mountFileServer (UrlRef serverUrl, StringRef name, StringRef label, 
												 IWebCredentials* credentials, StringRef type, 
												 IUnknown* serverHandler)
{
	ASSERT (System::IsInMainThread ())
	if(!System::IsInMainThread ())
		return kResultWrongThread;

	ASSERT (!name.isEmpty () && !label.isEmpty ())
	if(name.isEmpty () || label.isEmpty ())
		return kResultInvalidArgument;

	UnknownPtr<IWebFileClient> client2;
	if(!serverUrl.isEmpty ()) // allow mount with empty URL for later remount
	{
		MutableCString protocol (getClientProtocol (serverUrl));
		AutoPtr<IWebClient> client = System::GetWebService ().createClient (protocol);
		ASSERT (client != nullptr)
		if(client == nullptr)
			return kResultInvalidArgument;

		client->setCredentials (credentials);

		client2 = client;
		ASSERT (client2)
	}

	Volume* volume = NEW Volume (name);
	volume->setType (type);
	volume->setLabel (label);
	volume->setServerUrl (serverUrl);
	volume->setCredentials (credentials);
	volume->setClient (client2);
	volume->setServerHandler (serverHandler);

	volumeHandler->addVolume (volume, currentInsertPosition);

	SignalSource (Signals::kWebFiles).signal (Message (Signals::kVolumesChanged, name, CCLSTR (Signals::kVolumeChangeMounted)));
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WebFileService::unmountFileServer (StringRef name, tbool deferred)
{
	if(deferred)
	{
		(NEW Message (kUnmountFileServer, name))->post (this);
		return kResultOk;
	}

	ASSERT (System::IsInMainThread ())
	if(!System::IsInMainThread ())
		return kResultWrongThread;

	bool removed = volumeHandler->removeVolume (name);

	if(removed && currentInsertPosition == -1) // suppress unmount signal during remount
		SignalSource (Signals::kWebFiles).signal (Message (Signals::kVolumesChanged, name, CCLSTR (Signals::kVolumeChangeUnmounted)));

	return removed ? kResultOk : kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WebFileService::remountFileServer (StringRef name, IWebCredentials* newCredentials, const IUrl* newUrl)
{
	ASSERT (System::IsInMainThread ())
	if(!System::IsInMainThread ())
		return kResultWrongThread;

	String label, type;
	SharedPtr<IUnknown> serverHandler;
	int position = -1;
	Url serverUrl;

	{
		AutoPtr<Volume> volume = volumeHandler->openVolume (name);
		if(!volume.isValid ())
			return kResultInvalidArgument;

		label = volume->getLabel ();
		type = volume->getType ();
		serverUrl = volume->getServerUrl ();
		serverHandler = volume->getServerHandler ();
		position = volumeHandler->getVolumePosition (volume);
	}

	ScopedVar<int> insertScope (currentInsertPosition, position); // hidden argument!

	tresult result = unmountFileServer (name);
	if(result != kResultOk)
		return result;

	if(newUrl)
		serverUrl.assign (*newUrl);

	return mountFileServer (serverUrl, name, label, newCredentials, type, serverHandler);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WebFileService::isMounted (UrlRef serverUrl, IWebCredentials* credentials)
{
	AutoPtr<Volume> volume = volumeHandler->openWithServerUrl (serverUrl, credentials, true);
	return volume.isValid ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WebFileService::translateServerUrl (IUrl& webfsUrl, UrlRef serverUrl, IWebCredentials* credentials)
{
	// Note: This method behaves ambiguous if multiple servers with the same host name
	// but differently deep paths are mounted at the same time!
	AutoPtr<Volume> volume = volumeHandler->openWithServerUrl (serverUrl, credentials, false);
	if(!volume)
		return kResultFailed;

	volume->getWebfsUrl (webfsUrl, serverUrl.getPath (), serverUrl.getType ());
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WebFileService::translateWebfsUrl (IUrl& serverUrl, UrlRef webfsUrl)
{
	AutoPtr<Volume> volume = volumeHandler->openVolume (webfsUrl.getHostName ());
	if(!volume)
		return kResultFailed;

	Url srcUrl;
	volume->getFullUrl (srcUrl, webfsUrl);
	srcUrl.setProtocol (String (getClientProtocol (srcUrl))); // override with client protocol

	serverUrl.assign (srcUrl);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WebFileService::terminate ()
{
	ASSERT (System::IsInMainThread ())

	volumeHandler->removeAll ();

	SignalSource (Signals::kWebFiles).signal (Message (Signals::kVolumesChanged));

	safe_release (fileWorker);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WebFileService::openHandler (UrlRef webfsUrl, UIDRef iid, void** object)
{
	if(!object)
		return kResultInvalidPointer;

	AutoPtr<Volume> volume = volumeHandler->openVolume (webfsUrl.getHostName ());
	if(volume)
		if(IUnknown* handler = volume->getServerHandler ())
			return handler->queryInterface (iid, object);
	
	*object = nullptr;
	return kResultNoInterface;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFileDescriptor* CCL_API WebFileService::openFileItem (UrlRef webfsUrl)
{
	AutoPtr<Volume> volume = volumeHandler->openVolume (webfsUrl.getHostName ());
	if(volume)
		return volume->getFs ()->openFileItemInternal (webfsUrl, true);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WebFileService::requestDirectory (IObserver* observer, UrlRef webfsUrl, tbool async)
{
	ASSERT (async || !System::IsInMainThread ())
	if(!async && System::IsInMainThread ())
		return kResultWrongThread;

	if(async)
	{	
		ASSERT (observer != nullptr)
		if(!observer)
			return kResultInvalidPointer;

		ASSERT (fileWorker)
		if(!fileWorker) // called after terminate()
			return kResultUnexpected;

		fileWorker->scheduleWork (NEW GetDirectoryRequest (observer, webfsUrl));
		return kResultOk;
	}
	else
	{
		GetDirectoryRequest request (nullptr, webfsUrl);
		request.work ();
		return request.getWorkResult ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WebFileService::discardDirectory (UrlRef webfsUrl, tbool async)
{
	tresult result = kResultFailed;
	if(async)
	{
		fileWorker->scheduleWork (NEW DiscardDirectoryRequest (webfsUrl));
		result = kResultOk;
	}
	else if(AutoPtr<Volume> volume = volumeHandler->openVolume (webfsUrl.getHostName ()))
		result = volume->getFs ()->discardDirectory (webfsUrl);

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WebFileService::scheduleTask (IObserver* observer, UrlRef webfsUrl, IFileTask* task)
{
	ASSERT (/*observer != nullptr &&*/ task != nullptr)
	if(/*!observer ||*/ !task)
		return kResultInvalidPointer;

	ASSERT (fileWorker)
	if(!fileWorker) // called after terminate()
		return kResultUnexpected;

	fileWorker->scheduleWork (NEW FileTaskRequest (observer, webfsUrl, task));
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WebFileService::cancelOperation (IObserver* observer)
{
	ASSERT (observer != nullptr)
	if(!observer)
		return kResultInvalidPointer;

	if(fileWorker)
		fileWorker->cancelWork (observer, true);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IRemoteSession* CCL_API WebFileService::openSession (UrlRef webfsUrl)
{
	ASSERT (!System::IsInMainThread ()) // should not be called from main thread!

	AutoPtr<Volume> volume = volumeHandler->openVolume (webfsUrl.getHostName ());
	if(!volume)
		return nullptr;

	UrlRef serverUrl = volume->getServerUrl ();
	if(serverUrl.isEmpty ())
		return nullptr;

	MutableCString protocol (getClientProtocol (serverUrl));
	AutoPtr<IWebClient> client = System::GetWebService ().createClient (protocol);
	ASSERT (client != nullptr)
	if(client == nullptr)
		return nullptr;

	UnknownPtr<IWebFileClient> client2 (client);
	ASSERT (client2 != nullptr)
	if(client2 == nullptr)
		return nullptr;

	client->setCredentials (volume->getCredentials ());
	if(client->connect (serverUrl.getHostName ()) != kResultOk)
		return nullptr;

	return NEW RemoteSession (*volume, *client2, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ISearcher* CCL_API WebFileService::createSearcher (ISearchDescription& description)
{
	return NEW FileSearcher (description);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WebFileService::createDownload (ITransfer*& transfer, UrlRef webfsUrl, UrlRef localPath)
{
	transfer = nullptr;
	ASSERT (webfsUrl.isFile ())
	if(!webfsUrl.isFile ())
		return kResultInvalidArgument;

	AutoPtr<Volume> volume = volumeHandler->openVolume (webfsUrl.getHostName ());
	if(!volume)
		return kResultFailed;

	String fileName; // the local file name
	bool fileNameNedeed = false;

	// 1) file name is determined by caller
	if(localPath.isFile ())
		localPath.getName (fileName);
	
	// 2) try to determine filename via descriptor
	if(fileName.isEmpty ()) 
		if(AutoPtr<IFileDescriptor> webfsItem = volume->getFs ()->openFileItemInternal (webfsUrl, false))
			webfsItem->getFileName (fileName);

	#if 0
	// 3) use raw filename as fallback - no, don't do this as it might reveal underlying cryptic paths
	if(fileName.isEmpty ())
		webfsUrl.getName (fileName);
	#else
	if(fileName.isEmpty ())
	{
		fileName = TransferManager::kDownloadPartFileName;
		fileNameNedeed = true;
	}
	#endif

	FileInfo fileInfo;
	volume->getFs ()->getFileInfo (fileInfo, webfsUrl);

	Url srcUrl;
	volume->getFullUrl (srcUrl, webfsUrl);
	srcUrl.setProtocol (String (getClientProtocol (srcUrl))); // override with client protocol

	Url dstUrl (localPath);
	if(dstUrl.isFolder ())
		dstUrl.descend (fileName);

	Transfer* t = NEW Transfer (Transfer::kDownload);
	t->setName (fileName);
	t->setFileNameNeeded (fileNameNedeed);
	t->setSize (fileInfo.fileSize);
	//t->setSrcTitle (volume->getDisplayString (webfsUrl));
	t->setSrcTitle (volume->getLabel ());
	t->setDstTitle (UrlDisplayString (localPath));
	t->setSrcUrl (srcUrl);
	t->setDstUrl (dstUrl);
	t->setWebCredentials (volume->getCredentials ());

	transfer = t;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WebFileService::createUpload (ITransfer*& transfer, UrlRef webfsUrl, UrlRef localPath)
{
	transfer = nullptr;
	ASSERT (localPath.isFile ())
	if(!localPath.isFile ())
		return kResultInvalidArgument;

	AutoPtr<Volume> volume = volumeHandler->openVolume (webfsUrl.getHostName ());
	if(!volume)
		return kResultFailed;

	String fileName;
	localPath.getName (fileName);
	FileInfo fileInfo;
	System::GetFileSystem ().getFileInfo (fileInfo, localPath);

	Url dstUrl;
	volume->getFullUrl (dstUrl, webfsUrl);
	dstUrl.setProtocol (String (getClientProtocol (dstUrl))); // override with client protocol
	if(dstUrl.isFolder ())
		dstUrl.descend (fileName);

	Transfer* t = NEW Transfer (Transfer::kUpload);
	t->setName (fileName);
	t->setSize (fileInfo.fileSize);
	t->setSrcTitle (UrlDisplayString (localPath));
	//t->setDstTitle (volume->getDisplayString (webfsUrl));
	t->setDstTitle (volume->getLabel ());
	t->setSrcUrl (localPath);
	t->setDstUrl (dstUrl);
	t->setWebCredentials (volume->getCredentials ());

	// add finalizer for directory refresh
	// TODO: try to use kFileCreated signal instead???
	t->addFinalizer (NEW DirectoryChangedAction (webfsUrl));

	transfer = t;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITriggerAction* CCL_API WebFileService::createDirectoryChangedAction (UrlRef webfsUrl)
{
	return NEW DirectoryChangedAction (webfsUrl);
}

//************************************************************************************************
// DirectoryChangedAction
//************************************************************************************************

DirectoryChangedAction::DirectoryChangedAction (UrlRef _webfsUrl)
: webfsUrl (_webfsUrl)
{
	if(!webfsUrl.isFolder ())
		webfsUrl.ascend ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DirectoryChangedAction::execute (IObject* target)
{
	Transfer* transfer = unknown_cast<Transfer> (target);
	ASSERT (transfer != nullptr)
	if(transfer && transfer->getState () == Transfer::kCompleted)
		SignalSource (Signals::kWebFiles).signal (Message (Signals::kDirectoryChanged, static_cast<IUrl*> (&webfsUrl)));
}

//************************************************************************************************
// FileTaskRequest
//************************************************************************************************

FileTaskRequest::FileTaskRequest (IObserver* observer, UrlRef webfsUrl, IFileTask* task)
: FileRequest (observer, webfsUrl),
  task (task)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API FileTaskRequest::work ()
{
	tresult result = kResultFailed;
	AutoPtr<Volume> volume = openVolume ();
	ASSERT (volume)
	if(volume)
		if(IWebFileClient* client = volume->connect ())
		{
			RemoteSession session (*volume, *client);
			for(int attempt = 1; attempt <= 2; attempt++)
			{
				#if DEBUG_LOG
				if(attempt == 2)
					CCL_PRINTLN ("[WebFS] Second attempt to perform file task.")
				#endif

				ErrorContextGuard errorContext;
				result = task->perform (webfsUrl, session);
				if(!errorContext.hasErrors ())
					break;
			}
		}

	if(!canceled && observer)
	{
		Message* m = NEW Message (Meta::kFileTaskCompleted, result);
		m->post (observer);
	}
}

//************************************************************************************************
// DiscardDirectoryRequest
//************************************************************************************************

DiscardDirectoryRequest::DiscardDirectoryRequest (UrlRef webfsUrl)
: FileRequest (nullptr, webfsUrl)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DiscardDirectoryRequest::work ()
{
	AutoPtr<Volume> volume = openVolume ();
	ASSERT (volume)
	if(volume)
		volume->getFs ()->discardDirectory (webfsUrl);
}

//************************************************************************************************
// GetDirectoryRequest
//************************************************************************************************

GetDirectoryRequest::GetDirectoryRequest (IObserver* observer, UrlRef webfsUrl)
: FileRequest (observer, webfsUrl),
  workResult (kResultFalse)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API GetDirectoryRequest::work ()
{
	tresult result = kResultFailed;
	AutoPtr<Volume> volume = openVolume ();
	ASSERT (volume)
	if(volume)
	{
		for(int attempt = 1; attempt <= 2; attempt++)
		{
			#if DEBUG_LOG
			if(attempt == 2)
				CCL_PRINTLN ("[WebFS] Second attempt to get directory.")
			#endif

			ErrorContextGuard errorContext;
			result = volume->getFs ()->updateDirectory (webfsUrl, this);
			if(!errorContext.hasErrors ())
				break;
		}
	}

	if(!canceled && observer)
	{
		Message* m = NEW Message (Meta::kGetDirectoryCompleted, result);
		m->post (observer);
	}

	setWorkResult (result);
}

//************************************************************************************************
// FileRequest
//************************************************************************************************

FileRequest::FileRequest (IObserver* observer, UrlRef webfsUrl)
: AbstractWorkItem (observer),
  observer (observer),
  webfsUrl (webfsUrl),
  canceled (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Volume* FileRequest::openVolume ()
{
	VolumeHandler& handler = WebFileService::instance ().getVolumes ();
	String name = webfsUrl.getHostName ();
	return handler.openVolume (name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API FileRequest::cancel ()
{
	canceled = true; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileRequest::isCanceled ()
{
	return canceled; 
}
