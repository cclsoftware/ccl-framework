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
// Filename    : ccl/system/packaging/packagehandler.cpp
// Description : Package Handler
//
//************************************************************************************************

#include "ccl/system/packaging/packagehandler.h"
#include "ccl/system/packaging/folderpackage.h"
#include "ccl/system/packaging/packagefile.h"
#include "ccl/system/packaging/zipfile.h"
#include "ccl/system/packaging/sectionstream.h"

#include "ccl/base/storage/file.h"
#include "ccl/base/storage/packageinfo.h"
#include "ccl/base/signalsource.h"
#include "ccl/base/message.h"

#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/iprotocolhandler.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/systemservices.h"

namespace CCL {

//************************************************************************************************
// PackageRootFileSystem
//************************************************************************************************

class PackageRootFileSystem: public Object,
							 public IVolumeFileSystem
{
public:
	PackageRootFileSystem (PackageProtocolHandler& handler);

	// IFileSystem
	IStream* CCL_API openStream (UrlRef url, int mode = IStream::kOpenMode, IUnknown* context = nullptr) override { return nullptr; }
	tbool CCL_API fileExists (UrlRef url) override { return false; }
	tbool CCL_API getFileInfo (FileInfo& info, UrlRef url) override { return false; }
	tbool CCL_API removeFile (UrlRef url, int mode = 0) override { return false; }
	tbool CCL_API renameFile (UrlRef url, StringRef newName, int mode = 0) override { return false; }
	IFileIterator* CCL_API newIterator (UrlRef url, int mode = IFileIterator::kAll) override;
	tbool CCL_API createFolder (UrlRef url) override { return false; }
	tbool CCL_API removeFolder (UrlRef url, int mode = 0) override { return false; }
	tbool CCL_API isCaseSensitive () override { return true; }

	// IVolumeFileSystem
	tbool CCL_API getVolumeInfo (VolumeInfo& info, UrlRef rootUrl) override;
	tbool CCL_API isLocalFile (UrlRef url) override;
	tbool CCL_API isHiddenFile (UrlRef url) override;
	tbool CCL_API isWriteProtected (UrlRef url) override;
	tbool CCL_API moveFile (UrlRef dstPath, UrlRef srcPath, int mode = 0, IProgressNotify* progress = nullptr) override;
	tbool CCL_API copyFile (UrlRef dstPath, UrlRef srcPath, int mode = 0, IProgressNotify* progress = nullptr) override;

	CLASS_INTERFACE2 (IFileSystem, IVolumeFileSystem, Object)

protected:
	PackageProtocolHandler& handler;
};

//************************************************************************************************
// PackageRootIterator
//************************************************************************************************

class PackageRootIterator: public Unknown,
						   public IFileIterator
{
public:
	PackageRootIterator (PackageProtocolHandler& handler, int mode);

	// IFileIterator
	const IUrl* CCL_API next () override;

	CLASS_INTERFACE (IFileIterator, Unknown)

protected:
	ObjectList paths;
	AutoPtr<Iterator> iter;
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// System Service APIs
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT IPackageHandler& CCL_API System::CCL_ISOLATED (GetPackageHandler) ()
{
	return PackageHandler::instance ();
}

//************************************************************************************************
// PackageProtocolHandler
//************************************************************************************************

PackageProtocolHandler::PackageEntry::PackageEntry (StringRef name, IPackageFile* package, int options)
: MountPoint (name, package ? package->getFileSystem () : nullptr),
  package (package),
  options (options)
{
	if(package)
		package->retain ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PackageProtocolHandler::PackageEntry::~PackageEntry ()
{
	safe_release (fileSys); // let package be the last one to be released
	if(package)
		package->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 PackageProtocolHandler::PackageEntry::getUseCount () const
{
	if(FileArchive* archive = unknown_cast<FileArchive> (package))
		return archive->getUseCount ();
	if(FolderPackage* folder = unknown_cast<FolderPackage> (package))
		return folder->getUseCount ();
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (PackageProtocolHandler, MountProtocolHandler)

//////////////////////////////////////////////////////////////////////////////////////////////////

PackageProtocolHandler::PackageProtocolHandler ()
{
	rootFileSystem = NEW PackageRootFileSystem (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageProtocolHandler::addPackage (StringRef name, IPackageFile* package, int options)
{
	Threading::ScopedLock scopedLock (lock);

	mountPoints.add (NEW PackageEntry (name, package, options));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageProtocolHandler::removePackage (IPackageFile* package)
{
	Threading::ScopedLock scopedLock (lock);

	ForEach (mountPoints, PackageEntry, entry)
		if(entry->getPackage () == package)
		{
			mountPoints.remove (entry);
			entry->release ();
			return true;
		}
	EndFor
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageProtocolHandler::collectPaths (Container& paths, bool wantHidden)
{
	Threading::ScopedLock scopedLock (lock);

	ForEach (mountPoints, PackageEntry, entry)
		bool hidden = (entry->getOptions () & IPackageVolume::kHidden) != 0;
		if(hidden && !wantHidden)
			continue;

		Url* path = NEW Url (nullptr, Url::kFolder);
		path->setProtocol (getProtocol ());
		path->setHostName (entry->getName ());
		paths.add (path);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPackageVolume* PackageProtocolHandler::openVolume (StringRef name)
{
	Threading::ScopedLock scopedLock (lock);

	ForEach (mountPoints, PackageEntry, entry)
		if(entry->getName () == name)
			return return_shared<IPackageVolume> (entry);
	EndFor

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageProtocolHandler::isMounted (UrlRef path)
{
	Threading::ScopedLock scopedLock (lock);

	ForEach (mountPoints, PackageEntry, entry)
		if(entry->getPackage ()->getPath ().isEqualUrl (path, false))
			return true;
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageProtocolHandler::unmountAll ()
{
	Threading::ScopedLock scopedLock (lock);

	ForEach (mountPoints, PackageEntry, entry)
		ASSERT (entry->getUseCount () == 0)
		mountPoints.remove (entry);
		entry->release ();
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API PackageProtocolHandler::getProtocol () const
{
	static const String packageProtocol = CCLSTR ("package");
	return packageProtocol;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFileSystem* CCL_API PackageProtocolHandler::getMountPoint (StringRef name)
{
	Threading::ScopedLock scopedLock (lock);

	if(name.isEmpty ())
		return rootFileSystem;
	else
		return SuperClass::getMountPoint (name);
}

//************************************************************************************************
// PackageHandler
//************************************************************************************************

DEFINE_SINGLETON (PackageHandler)

//////////////////////////////////////////////////////////////////////////////////////////////////

PackageHandler::PackageHandler ()
: protocolHandler (NEW PackageProtocolHandler)
{
	UnknownPtr<IProtocolHandlerRegistry> registry (&System::GetFileSystem ());
	ASSERT (registry != nullptr)
	if(registry)
		registry->registerProtocol (protocolHandler);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PackageHandler::~PackageHandler ()
{
	UnknownPtr<IProtocolHandlerRegistry> registry (&System::GetFileSystem ());
	if(registry)
		registry->unregisterProtocol (protocolHandler);

	protocolHandler->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PackageHandler::setCryptoFactory (Security::Crypto::ICryptoFactory* factory)
{
	ASSERT (EncryptionStream::factoryInstance == nullptr)
	if(EncryptionStream::factoryInstance != nullptr) // can be set only once!
		return kResultUnexpected;

	EncryptionStream::factoryInstance = factory;

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UIDRef PackageHandler::getPackageClassForMimeType (StringRef mimeType) const
{
	if(FileTypes::Zip ().getMimeType ().compare (mimeType, false) == 0)
		return ClassID::ZipFile;
	if(FileTypes::Package ().getMimeType ().compare (mimeType, false) == 0)
		return ClassID::PackageFile;
	if(mimeType.contains (CCLSTR ("directory"), false) == 0)
		return ClassID::FolderPackage;
	return kNullUID;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PackageHandler::isPackage (UrlRef path)
{
	if(path.isFolder ())
	{
		Url infoPath (path);
		infoPath.descend (PackageInfo::kFileName);
		if(System::GetFileSystem ().fileExists (infoPath))
			return true;
	}
	else
	{
		if(path.getFileType () == FileTypes::Package ())
			return true;

		if(path.getFileType () == FileTypes::Zip ())
			return true;

		String fileName;
		path.getName (fileName);
		if(fileName == PackageInfo::kFileName)
			return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPackageFile* CCL_API PackageHandler::createPackage (UrlRef path, UIDRef cid)
{
	if(!cid.isValid ()) // decide by path
	{
		FolderPackage* folderPackage = FolderPackage::findPackage (path);
		if(folderPackage)
			return folderPackage;

		// Note: This is an ambiguous situation, use openPackage() instead or supply a class id!
		//if(path.getFileType () == FileTypes::Zip ())
		//	return NEW ZipFile;
		//else
			return NEW PackageFile (path);
	}
	else
	{
		if(cid == ClassID::FolderPackage)
			return NEW FolderPackage (path);
		else if(cid == ClassID::PackageFile)
			return NEW PackageFile (path);
		else if(cid == ClassID::ZipFile)
			return NEW ZipFile (path);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPackageFile* CCL_API PackageHandler::openPackage (UrlRef path, int options)
{
	AutoPtr<IPackageFile> package;

	// check for nested package first
	if(get_flag<int> (options, kNestedPackageSupported) && path.getProtocol () == PackageUrl::Protocol)
	{
		FileInfo info;
		if(File (path).getInfo (info) && get_flag<int> (info.flags, IPackageItem::kCompressed))
			if(AutoPtr<IMemoryStream> ms = File::loadBinaryFile (path))
				if(package = openPackageWithStream (*ms))
					return package.detach ();
	}

	// TODO: optimize format detection, avoid to open file three times!!!

	if(path.isFile ())
	{
		// 1) try package file
		package = NEW PackageFile (path);
		if(package->open ())
			return package.detach ();

		// 2) try .zip file
		package = NEW ZipFile (path);
		if(package->open ())
			return package.detach ();
	}

	// 3) try folder package
	package = FolderPackage::findPackage (path);
	if(package && package->open ())
		return package.detach ();

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPackageFile* CCL_API PackageHandler::openPackageWithStream (IStream& stream, UIDRef cid)
{
	AutoPtr<FileArchive> package;

	// 1) try .zip file (our primary use case)
	//    'openWithStream' would also succeed when a package file contains a zip file, so the cid must be used in this case
	if(cid.isValid () == false || cid == ClassID::ZipFile)
	{
		package = NEW ZipFile;
		if(package->openWithStream (stream))
			return package.detach ();
	}

	stream.rewind ();

	// 2) try package file
	package = NEW PackageFile;
	if(package->openWithStream (stream))
		return package.detach ();

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPackageFile* CCL_API PackageHandler::createPackageWithStream (IStream& stream, UIDRef cid)
{
	AutoPtr<FileArchive> package;

	if(cid == ClassID::PackageFile)
		package = NEW PackageFile;
	else if(cid == ClassID::ZipFile)
		package = NEW ZipFile;

	if(package)
		if(package->createWithStream (stream))
			return package.detach ();
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PackageHandler::mountPackageVolume (IPackageFile* package, StringRef packageID, int options)
{
	ASSERT (package != nullptr && !packageID.isEmpty ())
	if(!package || packageID.isEmpty ())
		return kResultInvalidArgument;

	// check if package identifier already in use...
	IFileSystem* existingPackage = protocolHandler->getMountPoint (packageID);
	if(existingPackage)
	{
		CCL_WARN ("A package with the same ID already exists: %s\n", MutableCString (packageID).str ())
		return kResultAlreadyExists;
	}

	// add to list of mounted packages
	protocolHandler->addPackage (packageID, package, options);

	if(System::IsInMainThread ())
		SignalSource (Signals::kPackageHandler).signal (Message (Signals::kPackageMounted, Variant (package, true)));
	else
		SignalSource (Signals::kPackageHandler).deferSignal (NEW Message (Signals::kPackageMounted, Variant (package, true)));
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PackageHandler::unmountPackageVolume (IPackageFile* package)
{
	if(!package)
		return kResultInvalidArgument;

	if(System::IsInMainThread ())
		SignalSource (Signals::kPackageHandler).signal (Message (Signals::kPackageUnmounted, Variant (package, true)));
	else
		SignalSource (Signals::kPackageHandler).deferSignal (NEW Message (Signals::kPackageUnmounted, Variant (package, true)));
	
	bool removed = protocolHandler->removePackage (package);
	ASSERT (removed == true)
	if(!removed)
		return kResultInvalidArgument;

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPackageVolume* CCL_API PackageHandler::openPackageVolume (StringRef packageID)
{
	return protocolHandler->openVolume (packageID);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PackageHandler::isMounted (UrlRef path)
{
	return protocolHandler->isMounted (path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PackageHandler::terminate ()
{
	protocolHandler->unmountAll ();
	return kResultOk;
}

//************************************************************************************************
// PackageRootFileSystem
//************************************************************************************************

PackageRootFileSystem::PackageRootFileSystem (PackageProtocolHandler& handler)
: handler (handler)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFileIterator* CCL_API PackageRootFileSystem::newIterator (UrlRef url, int mode)
{
	if((mode & IFileIterator::kFolders) == 0)
		return nullptr;

	ASSERT (url.isRootPath ())
	if(!url.isRootPath ())
		return nullptr;

	return NEW PackageRootIterator (handler, mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PackageRootFileSystem::getVolumeInfo (VolumeInfo& info, UrlRef rootUrl)
{
	if(rootUrl.getProtocol () != handler.getProtocol ())
		return false;

	if(rootUrl.getHostName ().isEmpty ())
		return false;

	AutoPtr<IPackageVolume> volume = handler.openVolume (rootUrl.getHostName ());
	if(!volume)
		return false;

	Url filePath = volume->getPackage ()->getPath ();

	info.type = VolumeInfo::kPackage;

	// use extension as sub type
	if(UnknownPtr<IFolderPackage> folderPackage = volume->getPackage ())
		info.subType = folderPackage->getRepresentedFileType ().getExtension ();
	else
		filePath.getExtension (info.subType);

	info.serialNumber = rootUrl.getHostName ();
	filePath.getName (info.label, false);

	FileInfo fileInfo;
	System::GetFileSystem ().getFileInfo (fileInfo, filePath);
	info.bytesTotal = fileInfo.fileSize;
	info.bytesFree = 0;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PackageRootFileSystem::isHiddenFile (UrlRef url)
{
	AutoPtr<IPackageVolume> volume = handler.openVolume (url.getHostName ());
	if(volume)
	{
		if(url.getPath ().isEmpty ())
			return (volume->getOptions () & IPackageVolume::kHidden) != 0;
		else
		{
			if(FileArchive* archive = unknown_cast<FileArchive> (volume->getPackage ()))
			{
				if(FileSystemItem* item = archive->lookupItem (url))
					return item->isHidden ();
			}
			else if(FolderPackage* folder = unknown_cast<FolderPackage> (volume->getPackage ()))
			{
				if(AutoPtr<IUrl> fullPath = folder->translateUrl (url))
					return System::GetFileSystem ().isHiddenFile (*fullPath);
			}
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PackageRootFileSystem::isLocalFile (UrlRef url)
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PackageRootFileSystem::isWriteProtected (UrlRef url)
{
	AutoPtr<IPackageVolume> volume = handler.openVolume (url.getHostName ());
	if(volume)
	{
		// delegate to target for folder packages
		UrlRef target = volume->getPackage ()->getPath ();
		if(target.isFolder ())
			return System::GetFileSystem ().isWriteProtected (target);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PackageRootFileSystem::moveFile (UrlRef dstPath, UrlRef srcPath, int mode, IProgressNotify* progress)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PackageRootFileSystem::copyFile (UrlRef dstPath, UrlRef srcPath, int mode, IProgressNotify* progress)
{
	return false;
}

//************************************************************************************************
// PackageRootIterator
//************************************************************************************************

PackageRootIterator::PackageRootIterator (PackageProtocolHandler& handler, int mode)
{
	bool wantHidden = (mode & kIgnoreHidden) == 0;

	paths.objectCleanup (true);
	handler.collectPaths (paths, wantHidden);
	iter = paths.newIterator ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IUrl* CCL_API PackageRootIterator::next ()
{
	return iter ? (Url*)iter->next () : nullptr;
}

