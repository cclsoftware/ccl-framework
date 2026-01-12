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
// Filename    : ccl/extras/webfs/webfilemethods.cpp
// Description : Web File Methods
//
//************************************************************************************************

#include "ccl/extras/webfs/webfilemethods.h"
#include "ccl/public/extras/iwebfilebrowser.h"

#include "ccl/base/trigger.h"
#include "ccl/base/storage/url.h"
#include "ccl/base/storage/file.h"
#include "ccl/base/collections/container.h"

#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/systemservices.h"

#include "ccl/public/network/web/iwebfileservice.h"
#include "ccl/public/network/web/itransfermanager.h"
#include "ccl/public/network/web/iwebfileclient.h"
#include "ccl/public/netservices.h"

namespace CCL {
namespace Web {

//************************************************************************************************
// Downloadable
//************************************************************************************************

class Downloadable: public Object,
					public IDownloadable,
					public IFilePromise
{
public:
	DECLARE_CLASS (Downloadable, Object)

	Downloadable (UrlRef webfsUrl = Url (), IFileDescriptor* webfsItem = nullptr);

	PROPERTY_OBJECT (Url, webfsUrl, WebFSUrl)
	PROPERTY_SHARED_AUTO (IFileDescriptor, webfsItem, WebFSItem)

	// IDownloadable
	UrlRef CCL_API getSourceUrl () const override;
		
	// IFileDescriptor
	tbool CCL_API getTitle (String& title) const override;
	tbool CCL_API getFileName (String& fileName) const override;
	tbool CCL_API getFileType (FileType& fileType) const override;
	tbool CCL_API getFileSize (int64& fileSize) const override;
	tbool CCL_API getFileTime (DateTime& fileTime) const override;
	tbool CCL_API getMetaInfo (IAttributeList& a) const override;
	
	// IFilePromise
	tbool CCL_API isAsync () const override;
	tresult CCL_API createFile (UrlRef destPath, IProgressNotify* progress = nullptr) override;

	CLASS_INTERFACES (Object)
};

//************************************************************************************************
// InstallFileAction
//************************************************************************************************

class InstallFileAction: public TriggerAction
{
public:
	// TriggerAction
	void CCL_API execute (IObject* target) override;
};

//************************************************************************************************
// RemoveUploadFileAction
//************************************************************************************************

class RemoveUploadFileAction: public TriggerAction
{
public:
	// TriggerAction
	void CCL_API execute (IObject* target) override;
};

static const String kUploadFolderName ("Uploads");

} // namespace Web
} // namespace CCL

using namespace CCL;
using namespace Web;

//************************************************************************************************
// FileMethods
//************************************************************************************************

DEFINE_CLASS_HIDDEN (FileMethods, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileMethods::canDownload (UrlRef webfsUrl)
{
	FileInfo info;
	System::GetFileSystem ().getFileInfo (info, webfsUrl);
	return (info.flags & IWebFileClient::DirEntry::kCanDownload) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDownloadable* FileMethods::createDownloadable (UrlRef webfsUrl)
{
	AutoPtr<IFileDescriptor> webfsItem = !webfsUrl.isFolder () ? System::GetWebFileService ().openFileItem (webfsUrl) : nullptr;
	return NEW Downloadable (webfsUrl, webfsItem);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileMethods::collectVolumes (Container& volumes)
{
	ASSERT (volumes.isObjectCleanup ())
	Url webfsRoot;
	webfsRoot.setProtocol (IWebFileService::kProtocol);
	ForEachFile (System::GetFileSystem ().newIterator (webfsRoot), path)
		VolumeInfo info;
		System::GetFileSystem ().getVolumeInfo (info, *path);
		volumes.add (NEW UrlWithTitle (*path, info.label));
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITransfer* FileMethods::createDownloadForUrl (UrlRef url, UrlRef dstPath)
{
	ITransfer* transfer = nullptr;
	if(url.getProtocol () == IWebFileService::kProtocol)
		System::GetWebFileService ().createDownload (transfer, url, dstPath);
	else
		transfer = System::GetTransferManager ().createTransfer (dstPath, url, ITransfer::kDownload);
	return transfer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileMethods::downloadFile (UrlRef url)
{
	Url dstPath;
	System::GetSystem ().getLocation (dstPath, System::kUserDownloadsFolder);
	downloadFile (url, dstPath);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileMethods::downloadFile (UrlRef url, UrlRef dstPath)
{
	AutoPtr<ITransfer> transfer = createDownloadForUrl (url, dstPath);
	ASSERT (transfer)
	if(transfer)
	{
		ITransfer* existing = System::GetTransferManager ().find (transfer);
		if(!existing || existing->getState () >= ITransfer::kCompleted)
			System::GetTransferManager ().queue (transfer);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileMethods::installFile (UrlRef url, IFileDescriptor& descriptor)
{
	// determine default location
	Url dstPath;
	System::GetFileTypeRegistry ().getHandlers ().getDefaultLocation (dstPath, descriptor);
	if(dstPath.isEmpty ())
		System::GetSystem ().getLocation (dstPath, System::kUserDownloadsFolder);

	AutoPtr<ITransfer> transfer = createDownloadForUrl (url, dstPath);
	ASSERT (transfer)
	if(transfer)
	{
		ITransfer* existing = System::GetTransferManager ().find (transfer);
		if(!existing || existing->getState () >= ITransfer::kCompleted)
		{
			// add finalizer to install file
			transfer->addFinalizer (NEW InstallFileAction);

			System::GetTransferManager ().queue (transfer);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileMethods::canUploadToVolume (UrlRef webfsUrl)
{
	VolumeInfo volumeInfo;
	System::GetFileSystem ().getVolumeInfo (volumeInfo, webfsUrl);
	return (volumeInfo.flags & IWebFileClient::ServerInfo::kCanUploadFiles) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileMethods::canModifySpecificFolders (UrlRef webfsUrl)
{
	VolumeInfo volumeInfo;
	System::GetFileSystem ().getVolumeInfo (volumeInfo, webfsUrl);
	return (volumeInfo.flags & IWebFileClient::ServerInfo::kCanModifySpecific) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileMethods::canUploadToFolder (UrlRef webfsUrl)
{
	FileInfo fileInfo;
	System::GetFileSystem ().getFileInfo (fileInfo, webfsUrl);
	return (fileInfo.flags & IWebFileClient::DirEntry::kCanUpload) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileMethods::collectUploadTargets (Container& targets, UrlRef webfsUrl, int maxDepth)
{
	struct Collector
	{
		int maxDepth;
		Collector (int maxDepth): maxDepth (maxDepth) {}
		void collect (Container& targets, UrlRef webfsUrl, int depth = 1)
		{
			ForEachFile (System::GetFileSystem ().newIterator (webfsUrl), path)
				if(path->isFolder ())
				{
					if(FileMethods ().canUploadToFolder (*path))
					{
						UrlDisplayString displayString (*path, Url::kStringDisplayName);
						targets.add (NEW UrlWithTitle (*path, displayString));
					}
					else if(depth <= maxDepth)
					{
						collect (targets, *path, depth + 1);
					}
				}
			EndFor
		}
	};

	Collector (maxDepth).collect (targets, webfsUrl);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileMethods::canUploadFrom (UrlRef path)
{
	if(path.isFolder ()) // TODO: folder uploads not yet implemented!
		return false;

	if(!System::GetFileSystem ().isLocalFile (path)) // can't transfer from server to server
		return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileMethods::canUploadFromFolder (UrlRef path)
{
	if(path.isFolder () == false)
		return false;
	
	if(!System::GetFileSystem ().isLocalFile (path)) // can't upload from server
		return false;

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileMethods::getUploadFolder (IUrl& path, UrlRef webfsUrl)
{
	System::GetSystem ().getLocation (path, System::kUserContentFolder);
	path.descend (kUploadFolderName, Url::kFolder);

	VolumeInfo info;
	System::GetFileSystem ().getVolumeInfo (info, webfsUrl);
	ASSERT (!info.label.isEmpty ())
	if(!info.label.isEmpty ())
		path.descend (LegalFileName (info.label), Url::kFolder);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileMethods::uploadObject (UrlRef webfsUrl, UrlRef path)
{
	if(!path.isFile ()) // TODO: folder uploads not yet implemented!
		return false;

	// check for customized uploader first
	AutoPtr<Web::ITransfer> transfer;
	if(AutoPtr<IUploader> uploader = System::GetWebFileService ().openHandler<IUploader> (webfsUrl))
	{
		transfer = uploader->createTransferForUpload (webfsUrl, path);
		if(transfer)
			transfer->addFinalizer (System::GetWebFileService ().createDirectoryChangedAction (webfsUrl));
	}
	else
		System::GetWebFileService ().createUpload (transfer, webfsUrl, path);
	
	if(transfer)
	{
		Url uploadStandardFolder;
		if(getUploadFolder (uploadStandardFolder, webfsUrl))
			if(uploadStandardFolder.contains (path))
				transfer->addFinalizer (NEW RemoveUploadFileAction);
	}
	else
		return false;

	System::GetTransferManager ().queue (transfer);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileMethods::canRenameFile (UrlRef webfsUrl)
{
	if(webfsUrl.isRootPath ()) // can't rename volumes
		return false;

	FileInfo info;
	System::GetFileSystem ().getFileInfo (info, webfsUrl);
	return (info.flags & IWebFileClient::DirEntry::kCanRename) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileMethods::canDeleteFile (UrlRef webfsUrl)
{
	if(webfsUrl.isRootPath ()) // can't delete volumes
		return false;

	FileInfo info;
	System::GetFileSystem ().getFileInfo (info, webfsUrl);
	return (info.flags & IWebFileClient::DirEntry::kCanDelete) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileMethods::canCreateFolder (UrlRef webfsUrl)
{
	VolumeInfo info;
	System::GetFileSystem ().getVolumeInfo (info, webfsUrl);
	if(info.flags & IWebFileClient::ServerInfo::kCanCreateFolders)
		return true;
	if(info.flags & IWebFileClient::ServerInfo::kCanModifySpecific)
	{
		FileInfo info;
		System::GetFileSystem ().getFileInfo (info, webfsUrl);
		return (info.flags & IWebFileClient::DirEntry::kCanCreateFolder) != 0;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileMethods::isSameVolume (UrlRef source, UrlRef target)
{
	VolumeInfo sourceVolume;
	VolumeInfo targetVolume;
	System::GetFileSystem ().getVolumeInfo (sourceVolume, source);
	System::GetFileSystem ().getVolumeInfo (targetVolume, target);
	return sourceVolume.type == targetVolume.type
		&& sourceVolume.label == targetVolume.label;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileMethods::acceptsChildrenOnly (UrlRef webfsUrl)
{
	ASSERT (webfsUrl.isFolder ())
	if(webfsUrl.isFolder () == false)
		return false;

	FileInfo info;
	System::GetFileSystem ().getFileInfo (info, webfsUrl);
	return info.flags & IWebFileClient::DirEntry::kAcceptsChildrenOnly;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileMethods::canMoveFolder (UrlRef webfsUrl)
{
	ASSERT (webfsUrl.isFolder ())
	if(webfsUrl.isFolder () == false)
		return false;

	FileInfo info;
	System::GetFileSystem ().getFileInfo (info, webfsUrl);
	return info.flags & IWebFileClient::DirEntry::kCanMove;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileMethods::moveObjectToFolder (UrlRef source, UrlRef folder)
{
	ASSERT (folder.isFolder ())
	if(folder.isFolder () == false)
		return false;
	
	String newFullPath = folder.getPath ();
	Url target (source);
	target.setPath (newFullPath);

	String sourceName;
	source.getName (sourceName);
	target.descend (sourceName);

	return System::GetFileSystem ().moveFile (target, source);
}

//************************************************************************************************
// InstallFileAction
//************************************************************************************************

void CCL_API InstallFileAction::execute (IObject* target)
{
	UnknownPtr<ITransfer> t (target);
	ASSERT (t.isValid ())
	if(!t.isValid ())
		return;

	System::GetFileTypeRegistry ().getHandlers ().openFile (t->getDstLocation ());
}

//************************************************************************************************
// RemoveUploadFileAction
//************************************************************************************************

void CCL_API RemoveUploadFileAction::execute (IObject* target)
{
	UnknownPtr<ITransfer> transfer (target);

	ASSERT (transfer)
	if(!transfer)
		return;

	File uploadFile (transfer->getSrcLocation ());
	bool finalized = false;

	if(uploadFile.exists ())
		finalized = uploadFile.remove ();

	ASSERT (finalized);
}

//************************************************************************************************
// Downloadable
//************************************************************************************************

DEFINE_CLASS_HIDDEN (Downloadable, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

Downloadable::Downloadable (UrlRef webfsUrl, IFileDescriptor* webfsItem)
: webfsUrl (webfsUrl)
{
	setWebFSItem (webfsItem);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Downloadable::queryInterface (UIDRef iid, void** ptr)
{
	if(webfsItem != nullptr)  // hide if descriptor missing
	{
		QUERY_INTERFACE (IFileDescriptor)
		QUERY_INTERFACE (IFilePromise)
	}

	QUERY_INTERFACE (IDownloadable)
	return SuperClass::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UrlRef CCL_API Downloadable::getSourceUrl () const
{
	return webfsUrl;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Downloadable::getTitle (String& title) const
{
	return webfsItem ? webfsItem->getTitle (title) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Downloadable::getFileName (String& fileName) const
{
	return webfsItem ? webfsItem->getFileName (fileName) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Downloadable::getFileType (FileType& fileType) const
{
	return webfsItem ? webfsItem->getFileType (fileType) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Downloadable::getFileSize (int64& fileSize) const
{
	return webfsItem ? webfsItem->getFileSize (fileSize) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Downloadable::getFileTime (DateTime& fileTime) const
{
	return webfsItem ? webfsItem->getFileTime (fileTime) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Downloadable::getMetaInfo (IAttributeList& a) const
{
	return webfsItem ? webfsItem->getMetaInfo (a) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Downloadable::isAsync () const
{
	return true; // downloads are queued
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Downloadable::createFile (UrlRef destPath, IProgressNotify* progress)
{
	FileMethods ().downloadFile (getSourceUrl (), destPath);
	return kResultOk;
}
