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
// Filename    : ccl/network/webfs/webfilesystem.cpp
// Description : Web File System
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/network/webfs/webfilesystem.h"

#include "ccl/base/message.h"
#include "ccl/base/signalsource.h"
#include "ccl/base/storage/file.h"
#include "ccl/base/storage/attributes.h"
#include "ccl/base/collections/objectarray.h"
#include "ccl/base/collections/stringdictionary.h"

#include "ccl/public/system/cclerror.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/systemservices.h"

#include "ccl/public/network/web/iwebfileservice.h"
#include "ccl/public/network/web/iwebfiletask.h"
#include "ccl/public/netservices.h"

namespace CCL {
namespace Web {

//************************************************************************************************
// FileSystem::FileIterator
//************************************************************************************************

class FileSystem::FileIterator: public Unknown,
								public IFileIterator
{
public:
	FileIterator (DirectoryEntry& entry, UrlRef basePath, int mode);

	// IFileIterator
	const IUrl* CCL_API next () override;

	CLASS_INTERFACE (IFileIterator, Unknown)

protected:
	ObjectArray paths;
	AutoPtr<Iterator> iter;
};

//************************************************************************************************
// FileSystem::Operation
//************************************************************************************************

class FileSystem::Operation: public Object,
							 public IFileTask
{
public:
	enum Type { kCreateFolder, kRemoveFile, kMoveFile };

	Operation (Type type);

	PROPERTY_VARIABLE (Type, type, Type)
	PROPERTY_OBJECT (Url, targetUrl, TargetUrl)

	// IFileTask
	tresult CCL_API perform (UrlRef webfsUrl, IRemoteSession& session) override;
	
	CLASS_INTERFACE (IFileTask, Object)

protected:
	void refreshDirectory (UrlRef path);
};

//************************************************************************************************
// RootFileSystem::VolumeIterator
//************************************************************************************************

class RootFileSystem::VolumeIterator: public Unknown,
									  public IFileIterator
{
public:
	VolumeIterator (VolumeHandler& handler, int mode);

	// IFileIterator
	const IUrl* CCL_API next () override;

	CLASS_INTERFACE (IFileIterator, Unknown)

protected:
	ObjectArray paths;
	AutoPtr<Iterator> iter;
};

//************************************************************************************************
// RootFileSystem::VolumeUpdateTask
//************************************************************************************************

class RootFileSystem::VolumeUpdateTask: public Object,
										public IFileTask
{
public:
	VolumeUpdateTask (VolumeHandler& handler);

	// IFileTask
	tresult CCL_API perform (UrlRef webfsUrl, IRemoteSession& session) override;
	
	CLASS_INTERFACE (IFileTask, Object)

protected:
	VolumeHandler& handler;
};

} // namespace Web
} // namespace CCL

using namespace CCL;
using namespace Web;

//************************************************************************************************
// VolumeHandler
//************************************************************************************************

DEFINE_CLASS_HIDDEN (VolumeHandler, MountProtocolHandler)

//////////////////////////////////////////////////////////////////////////////////////////////////

VolumeHandler::VolumeHandler ()
{
	rootFs = NEW RootFileSystem (*this);
	
	lock = System::CreateAdvancedLock (ClassID::ReadWriteLock);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API VolumeHandler::getProtocol () const
{
	return IWebFileService::kProtocol;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Volume* VolumeHandler::find (StringRef name) const
{
	ForEach (mountPoints, Volume, volume)
		if(volume->getName () == name)
			return volume;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VolumeHandler::addVolume (Volume* volume, int position)
{
	Threading::AutoLock autoLock (lock, Threading::ILockable::kWrite);

	// make unique volume name
	int index = 1;
	String name (volume->getName ());
	Volume* existing = nullptr;
	while((existing = find (name)) != nullptr)
		name = String () << volume->getName () << index++;
	
	volume->setName (name);
	
	if(index > 1)
		volume->setLabel (String () << volume->getLabel () << " (" << index << ")");

	volume->getFs ()->setLock (lock);

	bool added = false;
	if(position == 0)
	{
		mountPoints.prepend (volume);
		added = true;
	}
	else if(position >= 1)
		added = mountPoints.insertAt (position, volume);

	if(!added)
		mountPoints.add (volume);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool VolumeHandler::removeVolume (StringRef name)
{
	Threading::AutoLock autoLock (lock, Threading::ILockable::kWrite);

	Volume* volume = find (name);
	if(volume == nullptr)
		return false;

	mountPoints.remove (volume);
	volume->release ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VolumeHandler::removeAll ()
{
	Threading::AutoLock autoLock (lock, Threading::ILockable::kWrite);

	mountPoints.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Volume* VolumeHandler::openVolume (StringRef name)
{
	Threading::AutoLock autoLock (lock, Threading::ILockable::kRead);

	return return_shared (find (name));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Volume* VolumeHandler::openWithServerUrl (UrlRef serverUrl, IWebCredentials* credentials, bool exact)
{
	Threading::AutoLock autoLock (lock, Threading::ILockable::kRead);

	ForEach (mountPoints, Volume, volume)
		if(volume->isEqual (serverUrl, credentials, exact))
			return return_shared (volume);
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int VolumeHandler::getVolumePosition (Volume* volume)
{
	Threading::AutoLock autoLock (lock, Threading::ILockable::kRead);

	return mountPoints.index (volume);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VolumeHandler::getVolumeLocations (Container& paths, bool wantHidden)
{
	Threading::AutoLock autoLock (lock, Threading::ILockable::kRead);

	ForEach (mountPoints, Volume, volume)
		if(volume->isHidden () && !wantHidden) // ignore hidden volumes
			continue;

		Url* path = NEW Url (nullptr, Url::kFolder);
		path->setProtocol (getProtocol ());
		path->setHostName (volume->getName ());
		paths.add (path);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool VolumeHandler::getVolumeInfo (VolumeInfo& info, StringRef name)
{
	Threading::AutoLock autoLock (lock, Threading::ILockable::kRead);

	info = VolumeInfo ();
	if(Volume* volume = find (name))
	{
		info.type = VolumeInfo::kRemote;
		info.subType = volume->getType ();
		info.flags = volume->getFlags ();
		info.label = volume->getLabel ();
		info.serialNumber = volume->getName ();
		info.bytesTotal = volume->getBytesTotal ();
		info.bytesFree = volume->getBytesFree (); 
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFileSystem* CCL_API VolumeHandler::getMountPoint (StringRef name)
{
	if(name.isEmpty ())
		return rootFs;
	else
		return SuperClass::getMountPoint (name);
}

//************************************************************************************************
// DirectoryEntry
//************************************************************************************************

DEFINE_CLASS_HIDDEN (DirectoryEntry, ObjectNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

DirectoryEntry::DirectoryEntry ()
: contentLength (0),
  directory (false),
  cached (false),
  flags (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DirectoryEntry::assign (const IWebFileClient::DirEntry& entry, IUnknown* object)
{
	setName (entry.name);
	setCreationTime (entry.creationDate);
	setModifiedTime (entry.modifiedDate);
	setContentType (entry.contentType);
	setContentLength (entry.contentLength);
	setDirectory (entry.directory != 0);
	setFlags (entry.flags);
	setObject (object);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DirectoryEntry::assignSearchResult (UrlRef webfsUrl, const IWebFileSearchClient::ResultEntry& entry, IUnknown* object)
{
	assign (entry, object);

	// result entry contains the absolute path, but we want the name part only here
	String name;
	webfsUrl.getName (name);
	setName (name);
}

//************************************************************************************************
// Volume
//************************************************************************************************

DEFINE_CLASS_HIDDEN (Volume, MountPoint)

//////////////////////////////////////////////////////////////////////////////////////////////////

Volume::Volume (StringRef name)
: MountPoint (name),
  webFs (nullptr),
  flags (0),
  bytesTotal (0),
  bytesFree (0),
  hidden (false)
{
	rootDir = NEW DirectoryEntry;
	rootDir->setDirectory (true);

	fileSys = webFs = NEW FileSystem (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Volume::~Volume ()
{
	// TODO: this call might block!!!
	if(canConnect ())
		disconnect ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Volume::setName (StringRef _name)
{
	name = _name;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FileSystem* Volume::getFs () const
{
	return webFs;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Volume::canConnect () const
{
	return client != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Volume::isEqual (UrlRef _serverUrl, IWebCredentials* _credentials, bool exact) const
{
	bool matches = false;
	if(exact)
		matches = serverUrl.isEqualUrl (_serverUrl) != 0;
	else
		matches = serverUrl.getProtocol () == _serverUrl.getProtocol () && 
				  serverUrl.getHostName () == _serverUrl.getHostName ();

	if(matches)
	{
		if(!credentials && !_credentials)
			return true;
		
		if(credentials && _credentials)
		{
			if(!credentials->getUserName ().isEmpty ())
				return credentials->getUserName () == _credentials->getUserName ();
			else
			{
				Attributes otherAttributes;
				_credentials->getAttributes (otherAttributes);
				Attributes attributes;
				credentials->getAttributes (attributes);

				return attributes.equals (otherAttributes);
			}
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IWebFileClient* Volume::connect ()
{
	UnknownPtr<IWebClient> c (client);
	ASSERT (c)
	if(c && !c->isConnected ())
	{
		tresult tr = c->connect (serverUrl.getHostName ());
		if(tr != kResultOk)
			return nullptr;
			
		IWebFileClient::ServerInfo info;
		String rootPath (getRemotePath (Url ())); // use empty WebFS URL for conversion
		if(client->getServerInfo (rootPath, info) == kResultOk)
		{
			bytesTotal = info.bytesTotal;
			bytesFree = info.bytesFree;
			flags = info.flags;
		}
	}
	return client;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Volume::disconnect ()
{
	UnknownPtr<IWebClient> c (client);
	ASSERT (c)
	if(c && c->isConnected ())
		c->disconnect ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String Volume::getRemotePath (UrlRef webfsUrl) const
{
	String remotePath;
	remotePath << Url::strPathChar;

	if(!serverUrl.getPath ().isEmpty ())
		remotePath << serverUrl.getPath () << Url::strPathChar;

	remotePath << webfsUrl.getPath ();

	// end slash for folders
	if(webfsUrl.isFolder () && !remotePath.endsWith (Url::strPathChar))
		remotePath << Url::strPathChar;
	return remotePath;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Volume::getWebfsUrl (IUrl& webfsUrl, StringRef _remotePath, int type)
{
	webfsUrl.setProtocol (IWebFileService::kProtocol);
	webfsUrl.setHostName (getName ());

	// remove leading slash
	String relativePath (_remotePath);
	if(relativePath.startsWith (Url::strPathChar))
		relativePath.remove (0, 1);

	// remove the path part used to mount the volume
	String mountedPath (getServerUrl ().getPath ());
	if(!mountedPath.isEmpty () && relativePath.startsWith (mountedPath))
		relativePath.remove (0, mountedPath.length ());

	// append trailing slash if type is set explicitly
	if(type == Url::kFolder && !relativePath.endsWith (Url::strPathChar))
		relativePath << Url::strPathChar;

	webfsUrl.setPath (relativePath, Url::kDetect); // detect folder via trailing slash
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Volume::getFullUrl (Url& result, UrlRef webfsUrl) const
{
	result = serverUrl;
	result.descend (webfsUrl.getPath (), webfsUrl.getType ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String Volume::getDisplayString (UrlRef webfsUrl) const
{
	return String () << getLabel () << Url::strPathChar << webfsUrl.getPath ();
}

//************************************************************************************************
// FileSystem
//************************************************************************************************

FileSystem::FileSystem (Volume& volume)
: volume (volume)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult FileSystem::updateDirectory (UrlRef webfsUrl, IProgressNotify* progress)
{
	AutoPtr<IWebFileClient::IDirIterator> iter;

	if(volume.canConnect ())
	{
		IWebFileClient* client = volume.connect ();
		if(client == nullptr)
			return kResultFailed;

		String dirPath = volume.getRemotePath (webfsUrl);
		iter = client->openDirectory (dirPath, progress);
		if(iter == nullptr)
		{
			ccl_raise (String ()); // trigger second attempt if underlying socket does not raise errors
			return kResultFailed;
		}
	}

	addToDirectory (webfsUrl, iter); // iter can be null if volume is not connectable
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileSystem::addToDirectory (UrlRef webfsUrl, const IWebFileClient::IDirIterator* iter)
{
	ObjectArray entries;
	entries.objectCleanup (true);

	if(iter)
	{
		const IWebFileClient::DirEntry* e = nullptr;
		for(int index = 0; e = iter->getEntry (index); index++)
		{
			IUnknown* object = iter->getObject (index);

			DirectoryEntry* entry = NEW DirectoryEntry;
			entry->assign (*e, object);
			entries.add (entry);
		}
	}

	addToDirectory (webfsUrl, entries);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileSystem::addToDirectory (UrlRef webfsUrl, Container& entries)
{
	Threading::AutoLock autoLock (lock, Threading::ILockable::kWrite);

	DirectoryEntry* parent = createEntry (webfsUrl);
	ASSERT (parent)
	parent->removeAll ();
	parent->setCached (true);

	ForEach (entries, DirectoryEntry, entry)
		parent->addChild (return_shared (entry));
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult FileSystem::discardDirectory (UrlRef webfsUrl)
{
	Threading::AutoLock autoLock (lock, Threading::ILockable::kWrite);

	DirectoryEntry* entry = findEntry (webfsUrl);
	if(entry == nullptr)
		return kResultFalse;

	ASSERT (entry->isDirectory ())
	entry->removeAll ();
	entry->setCached (false);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult FileSystem::insertSearchResult (const IWebFileSearchClient::IResultIterator& iter, IUnknownList* outItems)
{
	Threading::AutoLock autoLock (lock, Threading::ILockable::kWrite);

	const IWebFileClient::DirEntry* e = nullptr;
	for(int index = 0; e = iter.getEntry (index); index++)
	{
		IUnknown* object = iter.getObject (index);

		Url url;
		volume.getWebfsUrl (url, e->name, e->directory ? Url::kFolder : Url::kFile);
		ASSERT (url.isFolder () || !e->directory)

		DirectoryEntry* entry = createEntry (url);
		ASSERT (entry)
		entry->assignSearchResult (url, *e, object);

		if(outItems)
			outItems->add (static_cast<IObject*> (url.clone ()));
	}

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult FileSystem::insertSearchResult (const IWebFileSearchClient::ResultEntry& e, IUnknown* object, IUnknown** outItem)
{
	Threading::AutoLock autoLock (lock, Threading::ILockable::kWrite);

	Url url;
	volume.getWebfsUrl (url, e.name, e.directory ? Url::kFolder : Url::kFile);
	ASSERT (url.isFolder () || !e.directory)

	DirectoryEntry* entry = createEntry (url);
	ASSERT (entry)
	entry->assignSearchResult (url, e, object);

	if(outItem)
		*outItem = static_cast<IObject*> (url.clone ());
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DirectoryEntry* FileSystem::findEntry (UrlRef url)
{
	if(url.getPath ().isEmpty ())
	{
		ASSERT (url.isFolder ())
		return volume.getRootDirectory ();
	}
	else
	{
		DirectoryEntry* entry = unknown_cast<DirectoryEntry> (volume.getRootDirectory ()->lookupChild (url.getPath ()));
		if(entry)
		{
			ASSERT (entry->isDirectory () == url.isFolder ())
			if(entry->isDirectory () == url.isFolder ())
				return entry;
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DirectoryEntry* FileSystem::createEntry (UrlRef url)
{
	if(DirectoryEntry* existing = findEntry (url))
		return existing;

	DirectoryEntry* current = volume.getRootDirectory ();
	AutoPtr<IStringTokenizer> iter = url.getPath ().tokenize (Url::strPathChar);
	if(iter) while(!iter->done ())
	{
		uchar delimiter = 0;
		StringRef name = iter->nextToken (delimiter);
		
		bool isFolder = true;
		if(iter->done () && url.isFile ()) // last one is the file's name
			isFolder = false;

		// check if folder already exists
		DirectoryEntry* newEntry = nullptr;
		if(isFolder)
			newEntry = current->findChildNode<DirectoryEntry> (name);

		if(newEntry == nullptr)
		{
			newEntry = NEW DirectoryEntry;
			newEntry->setDirectory (isFolder);
			newEntry->setName (name);
			current->addChild (newEntry);
		}

		current = newEntry;
	}

	ASSERT (current != volume.getRootDirectory ())
	return current;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileSystem::fileExists (UrlRef url)
{
	Threading::AutoLock autoLock (lock, Threading::ILockable::kRead);

	DirectoryEntry* entry = findEntry (url);
	if(entry == nullptr)
		return false;
	
	// check if directory content has been cached
	if(entry->isDirectory ())
		return entry->isCached ();

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileSystem::getFileInfo (FileInfo& info, UrlRef url)
{
	Threading::AutoLock autoLock (lock, Threading::ILockable::kRead);

	info = FileInfo ();
	DirectoryEntry* entry = findEntry (url);
	if(entry == nullptr)
		return false;
	
	info.flags = entry->getFlags ();
	info.fileSize = entry->getContentLength ();
	info.createTime = entry->getCreationTime ();
	info.modifiedTime = info.accessTime = entry->getModifiedTime ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFileDescriptor* FileSystem::openFileItemInternal (UrlRef url, bool allowDefault)
{
	Threading::AutoLock autoLock (lock, Threading::ILockable::kRead);

	DirectoryEntry* entry = findEntry (url);
	if(entry == nullptr)
		return nullptr;

	if(IFileDescriptor* item = UnknownPtr<IFileDescriptor> (entry->getObject ()).detach ())
		return item;

	if(!allowDefault)
		return nullptr;

	FileDescriptor* descriptor = NEW FileDescriptor (entry->getName (), entry->getContentLength ());
	descriptor->setFileTime (entry->getModifiedTime ());
	return descriptor;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFileDescriptor* CCL_API FileSystem::openFileItem (UrlRef url)
{
	return openFileItemInternal (url, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFileIterator* CCL_API FileSystem::newIterator (UrlRef url, int mode)
{
	Threading::AutoLock autoLock (lock, Threading::ILockable::kRead);

	DirectoryEntry* entry = findEntry (url);
	if(entry == nullptr)
		return nullptr;

	// check if directory content has been cached
	if(!entry->isCached ())
		return nullptr;

	return NEW FileIterator (*entry, url, mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileSystem::createFolder (UrlRef url)
{
	#if 0
	if(!System::IsInMainThread ())
	{
		IWebFileClient* client = volume.connect ();
		if(client == 0)
			return false;

		return client->makeDirectory (volume.getRemotePath (url)) == kResultOk;
	}
	#endif

	System::GetWebFileService ().scheduleTask (nullptr, url, AutoPtr<Operation> (NEW Operation (Operation::kCreateFolder)));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileSystem::removeFile (UrlRef url, int mode)
{
	#if 0
	if(!System::IsInMainThread ())
	{
		IWebFileClient* client = volume.connect ();
		if(client == 0)
			return false;

		return client->deleteResource (volume.getRemotePath (url)) == kResultOk;
	}
	#endif

	System::GetWebFileService ().scheduleTask (nullptr, url, AutoPtr<Operation> (NEW Operation (Operation::kRemoveFile)));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileSystem::removeFolder (UrlRef url, int mode)
{
	if(mode & kDeleteRecursively)
	{
		CCL_NOT_IMPL ("Recursive delete not implemented!\n")
		return false;
	}

	return removeFile (url, mode);
}

//************************************************************************************************
// FileSystem::FileIterator
//************************************************************************************************

FileSystem::FileIterator::FileIterator (DirectoryEntry& entry, UrlRef basePath, int mode)
{
	paths.objectCleanup (true);
	
	bool wantFiles = (mode & IFileIterator::kFiles) != 0;
	bool wantFolders = (mode & IFileIterator::kFolders) != 0;
	//bool wantHidden = (mode & IFileIterator::kIgnoreHidden) == 0; not used!

	ForEach (entry, DirectoryEntry, e)
		if(!e->isDirectory () && !wantFiles)
			continue;
		if(e->isDirectory () && !wantFolders)
			continue;

		Url* path = NEW Url (basePath);

		if(IFileDescriptor* descriptor = UnknownPtr<IFileDescriptor> (e->getObject ()))
		{
			String displayName;
			if(descriptor->getFileName (displayName))
				path->getParameters ().setEntry (CCLSTR (UrlParameter::kDisplayName), displayName);
		}
		
		path->descend (e->getName (), e->isDirectory () ? Url::kFolder : Url::kFile);
		paths.add (path);
	EndFor

	iter = paths.newIterator ();
	ASSERT (iter)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IUrl* CCL_API FileSystem::FileIterator::next ()
{
	if(iter == nullptr)
		return nullptr;
	return (Url*)iter->next ();
}

//************************************************************************************************
// FileSystem::Operation
//************************************************************************************************

FileSystem::Operation::Operation (Type type)
: type (type)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API FileSystem::Operation::perform (UrlRef webfsUrl, IRemoteSession& session)
{
	tbool succeeded = false;

	switch(type)
	{
	case kCreateFolder :
		succeeded = session.getFileSystem ().createFolder (webfsUrl);
		break;

	case kRemoveFile :
		succeeded = session.getFileSystem ().removeFile (webfsUrl);
		break;

	case kMoveFile :
		{
			String sourcePath, destPath;
			session.getRemotePath (sourcePath, webfsUrl);
			session.getRemotePath (destPath, targetUrl);
			String resultPath;
			succeeded = session.getClient ().moveResource (resultPath, sourcePath, destPath) == kResultOk;
		}
		break;
	}

	if(!succeeded)
		return kResultFailed;

	// update cached entries
	Url parentFolder (webfsUrl);
	parentFolder.ascend ();
	refreshDirectory (parentFolder);

	if(type == kMoveFile)
	{
		Url parentFolder2 (targetUrl);
		parentFolder2.ascend ();
		if(!parentFolder.contains (parentFolder2))
			refreshDirectory (parentFolder2);
	}

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileSystem::Operation::refreshDirectory (UrlRef path)
{
	#if 0 // Not here, new request is done by application in response to signal
	System::GetWebFileService ().discardDirectory (path);
	System::GetWebFileService ().requestDirectory (0, path, false);
	#endif

	AutoPtr<Url> path2 = NEW Url (path);
	SignalSource (Signals::kWebFiles).deferSignal (NEW Message (Signals::kDirectoryChanged, path2->asUnknown ()));
}

//************************************************************************************************
// RootFileSystem
//************************************************************************************************

RootFileSystem::RootFileSystem (VolumeHandler& handler)
: handler (handler)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API RootFileSystem::isCaseSensitive ()
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFileIterator* CCL_API RootFileSystem::newIterator (UrlRef url, int mode)
{
	if((mode & IFileIterator::kFolders) == 0)
		return nullptr;

	return NEW VolumeIterator (handler, mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API RootFileSystem::getVolumeInfo (VolumeInfo& info, UrlRef rootUrl)
{
	// hidden feature for asynchronous volume information update
	bool asyncUpdateNeeded = (info.type & IVolumeFileSystem::kSuppressSlowVolumeInfo) != 0;
	
	if(handler.getVolumeInfo (info, rootUrl.getHostName ()))
	{
		if(asyncUpdateNeeded)
			System::GetWebFileService ().scheduleTask (nullptr, rootUrl, AutoPtr<IFileTask> (NEW VolumeUpdateTask (handler)));
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API RootFileSystem::isLocalFile (UrlRef url)
{
	return false; // files aren't local
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API RootFileSystem::isHiddenFile (UrlRef url)
{
	String name;
	url.getName (name, true);
	return name.startsWith (".", true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API RootFileSystem::isWriteProtected (UrlRef url)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API RootFileSystem::moveFile (UrlRef dstPath, UrlRef srcPath, int mode, IProgressNotify* progress)
{
	AutoPtr<FileSystem::Operation> op = NEW FileSystem::Operation (FileSystem::Operation::kMoveFile);
	op->setTargetUrl (dstPath);
	System::GetWebFileService ().scheduleTask (nullptr, srcPath, op);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API RootFileSystem::copyFile (UrlRef dstPath, UrlRef srcPath, int mode, IProgressNotify* progress)
{
	return false;
}

//************************************************************************************************
// RootFileSystem::VolumeIterator
//************************************************************************************************

RootFileSystem::VolumeIterator::VolumeIterator (VolumeHandler& handler, int mode)
{
	bool wantHidden = (mode & kIgnoreHidden) == 0;
	
	paths.objectCleanup (true);
	handler.getVolumeLocations (paths, wantHidden);
	iter = paths.newIterator ();
	ASSERT (iter)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IUrl* CCL_API RootFileSystem::VolumeIterator::next ()
{
	if(iter == nullptr)
		return nullptr;
	return (Url*)iter->next ();
}

//************************************************************************************************
// RootFileSystem::VolumeUpdateTask
//************************************************************************************************

RootFileSystem::VolumeUpdateTask::VolumeUpdateTask (VolumeHandler& handler)
: handler (handler)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API RootFileSystem::VolumeUpdateTask::perform (UrlRef webfsUrl, IRemoteSession& session)
{
	String remotePath;
	session.getRemotePath (remotePath, webfsUrl);
	
	IWebFileClient::ServerInfo serverInfo;
	if(session.getClient ().getServerInfo (remotePath, serverInfo) == kResultOk)
	{
		AutoPtr<Volume> volume = handler.openVolume (webfsUrl.getHostName ());
		if(volume)
		{
			volume->setBytesTotal (serverInfo.bytesTotal);
			volume->setBytesFree (serverInfo.bytesFree);
		
			SignalSource (Signals::kWebFiles).deferSignal (NEW Message (Signals::kVolumeInfoChanged, webfsUrl.getHostName ()));
		}
	}

	return kResultOk;
}
