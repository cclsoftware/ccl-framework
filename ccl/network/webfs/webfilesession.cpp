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
// Filename    : ccl/network/webfs/webfilesession.cpp
// Description : WebFS Remote Session
//
//************************************************************************************************

#include "ccl/network/webfs/webfilesession.h"
#include "ccl/network/webfs/webfilesystem.h"

#include "ccl/public/text/istringdict.h"
#include "ccl/public/base/memorystream.h"
#include "ccl/public/system/ifileitem.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/systemservices.h"

#include "ccl/public/network/web/iwebrequest.h"
#include "ccl/public/network/web/iwebservice.h"
#include "ccl/public/netservices.h"

namespace CCL {
namespace Web {

//************************************************************************************************
// RemoteSession::RemotePath
//************************************************************************************************

class RemoteSession::RemotePath: public String
{
public:
	RemotePath (RemoteSession& session, UrlRef webfsUrl)
	{ session.getRemotePath (*this, webfsUrl); }
};

//************************************************************************************************
// RemoteSession::FileIterator
//************************************************************************************************

class RemoteSession::FileIterator: public Unknown,
								   public IFileIterator
{
public:
	FileIterator (IWebFileClient::IDirIterator* iter, UrlRef webfsUrl, int mode);

	// IFileIterator
	const IUrl* CCL_API next () override;

	CLASS_INTERFACE (IFileIterator, Unknown)

protected:
	AutoPtr<IWebFileClient::IDirIterator> iter;
	int index;
	bool wantFiles, wantFolders;
	Url webfsUrl;
	Url nextUrl;
};

} // namespace Web
} // namespace CCL

using namespace CCL;
using namespace Web;

//************************************************************************************************
// RemoteSession
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (RemoteSession, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

RemoteSession::RemoteSession (Volume& volume, IWebFileClient& client, bool ownsConnection)
: volume (volume),
  client (client),
  ownsConnection (ownsConnection)
{
	volume.retain ();
	client.retain ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

RemoteSession::~RemoteSession ()
{
	if(ownsConnection)
	{
		UnknownPtr<IWebClient> c (&client);
		ASSERT (c)
		if(c && c->isConnected ())
			c->disconnect ();
	}

	volume.release ();
	client.release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IWebFileClient& CCL_API RemoteSession::getClient ()
{
	return client;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFileSystem& CCL_API RemoteSession::getFileSystem ()
{
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API RemoteSession::getRemotePath (String& remotePath, UrlRef webfsUrl)
{
	remotePath = volume.getRemotePath (webfsUrl);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API RemoteSession::getWebfsUrl (IUrl& webfsUrl, StringRef remotePath)
{
	volume.getWebfsUrl (webfsUrl, remotePath);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API RemoteSession::downloadFile (UrlRef webfsUrl, UrlRef localPath, IProgressNotify* progress)
{
	AutoPtr<IStream> localStream = System::GetFileSystem ().openStream (localPath, IStream::kCreateMode);
	if(!localStream)
		return false;

	RemotePath remotePath (*this, webfsUrl);
	UnknownPtr<IWebClient> webClient (&client);
	ASSERT (webClient)
	return webClient->downloadData (remotePath, *localStream, nullptr, progress) == kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API RemoteSession::uploadFile (UrlRef localPath, UrlRef webfsUrl, IProgressNotify* progress)
{
	// try to create folder structure first
	createFolder (webfsUrl);

	AutoPtr<IStream> localStream = System::GetFileSystem ().openStream (localPath, IStream::kOpenMode);
	if(!localStream)
		return false;

	AutoPtr<Web::IWebHeaderCollection> headers = System::GetWebService ().createHeaderCollection ();
	headers->getEntries ().setEntry (Web::Meta::kContentType, Meta::kBinaryContentType);
	RemotePath remotePath (*this, webfsUrl);
	MemoryStream response;

	UnknownPtr<IWebClient> webClient (&client);
	ASSERT (webClient)
	return webClient->uploadData (headers, *localStream, remotePath, response, nullptr, progress) == kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStream* CCL_API RemoteSession::openStream (UrlRef webfsUrl, int mode, IUnknown* context)
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API RemoteSession::fileExists (UrlRef webfsUrl)
{
	RemotePath remotePath (*this, webfsUrl);
	IWebFileClient::DirEntry unused;
	return client.getFileInfo (remotePath, unused) == kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API RemoteSession::getFileInfo (FileInfo& info, UrlRef webfsUrl)
{
	RemotePath remotePath (*this, webfsUrl);

	IWebFileClient::DirEntry entry;
	if(client.getFileInfo (remotePath, entry) != kResultOk)
		return false;

	info.flags = 0;
	info.fileSize = entry.contentLength;
	info.createTime = entry.creationDate;
	info.modifiedTime = entry.modifiedDate;
	info.accessTime = info.modifiedTime;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API RemoteSession::removeFile (UrlRef webfsUrl, int mode)
{
	ASSERT ((mode & kDeleteToTrashBin) == 0)

	RemotePath remotePath (*this, webfsUrl);
	return client.deleteResource (remotePath) == kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API RemoteSession::renameFile (UrlRef webfsUrl, StringRef newName, int mode)
{
	Url newWebfsUrl (webfsUrl);
	newWebfsUrl.setName (newName, webfsUrl.getType ());

	RemotePath sourcePath (*this, webfsUrl);
	RemotePath destPath (*this, newWebfsUrl);

	String resultPath;
	return client.moveResource (resultPath, sourcePath, destPath) == kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFileIterator* CCL_API RemoteSession::newIterator (UrlRef webfsUrl, int mode)
{
	// TODO: IProgressNotify for cancelation not available here!!!
	RemotePath remotePath (*this, webfsUrl);
	IWebFileClient::IDirIterator* iter = client.openDirectory (remotePath, nullptr);
	return iter ? NEW FileIterator (iter, webfsUrl, mode) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool RemoteSession::createParentFolder (UrlRef webfsUrl)
{
	Url parent (webfsUrl);
	if(!parent.ascend ())
		return false;

	return createFolder (parent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API RemoteSession::createFolder (UrlRef webfsUrl)
{
	if(webfsUrl.isFile ())
		return createParentFolder (webfsUrl);

	if(fileExists (webfsUrl))
		return true;

	if(!createParentFolder (webfsUrl))
		return false;

	Url parent (webfsUrl);
	parent.ascend ();
	RemotePath remotePath (*this, parent);

	String name, result;
	webfsUrl.getName (name);
	return client.makeDirectory (result, remotePath, name) == kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API RemoteSession::removeFolder (UrlRef webfsUrl, int mode)
{
	if(mode & kDeleteRecursively)
	{
		AutoPtr<IFileIterator> iter = newIterator (webfsUrl);
		if(iter)
		{
			const IUrl* url;
			while((url = iter->next ()) != nullptr)
			{
				if(url->isFolder ())
					removeFolder (*url, mode);
				else if(url->isFile ())
					removeFile (*url, mode & ~kDeleteRecursively);
			}
		}
	}

	return removeFile (webfsUrl, mode & ~kDeleteRecursively);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API RemoteSession::isCaseSensitive ()
{
	return true;
}

//************************************************************************************************
// RemoteSession::FileIterator
//************************************************************************************************

RemoteSession::FileIterator::FileIterator (IWebFileClient::IDirIterator* iter, UrlRef webfsUrl, int mode)
: iter (iter),
  index (0),
  wantFiles ((mode & IFileIterator::kFiles) != 0),
  wantFolders ((mode & IFileIterator::kFolders) != 0),
  webfsUrl (webfsUrl)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IUrl* CCL_API RemoteSession::FileIterator::next ()
{
	while(1)
	{
		const IWebFileClient::DirEntry* e = iter->getEntry (index++);
		if(e == nullptr)
			return nullptr;

		if(!e->directory && !wantFiles)
			continue;
		if(e->directory && !wantFolders)
			continue;

		String name;
		if(IFileDescriptor* descriptor = UnknownPtr<IFileDescriptor> (iter->getObject (index)))
			descriptor->getFileName (name);
		if(name.isEmpty ())
			name = e->name;

		nextUrl = webfsUrl;
		nextUrl.descend (name, e->directory ? Url::kFolder : Url::kFile);
		break;
	}

	return &nextUrl;
}
