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
// Filename    : ccl/system/packaging/folderpackage.cpp
// Description : Folder Package
//
//************************************************************************************************

#include "ccl/system/packaging/folderpackage.h"
#include "ccl/system/packaging/sectionstream.h"
#include "ccl/system/virtualfilesystem.h" // for RelativeFileSystem

#include "ccl/base/storage/packageinfo.h"

#include "ccl/public/base/streamer.h"
#include "ccl/public/base/memorystream.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/system/inativefilesystem.h"

namespace CCL {

//************************************************************************************************
// FileArchive::SubStream
//************************************************************************************************

class FolderPackage::SubStream: public StreamAlias
{
public:
	SubStream (FolderPackage& _folderPackage, IStream* sourceStream)
	: StreamAlias (sourceStream),
	  folderPackage (&_folderPackage)
	{
		AtomicAddInline (folderPackage->useCount, 1);
	}

	~SubStream ()
	{
		AtomicAddInline (folderPackage->useCount, -1);
	}

protected:
	SharedPtr<FolderPackage> folderPackage;
};

//************************************************************************************************
// FolderPackageFileSystem
//************************************************************************************************

class FolderPackageFileSystem: public RelativeFileSystem
{
public:
	FolderPackageFileSystem (FolderPackage& owner, IFileSystem* fileSys, IUrl* baseUrl)
	: RelativeFileSystem (fileSys, baseUrl),
	  owner (&owner)
	{}

	PROPERTY_POINTER (FolderPackage, owner, Owner) // this is our owner, must not be shared!

	static FolderPackageFileSystem* fromFolder (FolderPackage& owner, UrlRef folderPath)
	{
		AutoPtr<Url> pathCopy = NEW Url (folderPath);
		IFileSystem* fileSystem = VirtualFileSystem::instance ().getMountPoint (folderPath);
		ASSERT (fileSystem != nullptr)
		if(!fileSystem)
			return nullptr;

		return NEW FolderPackageFileSystem (owner, fileSystem, pathCopy);
	}

	// RelativeFileSystem
	IStream* CCL_API openStream (UrlRef url, int mode = IStream::kOpenMode, IUnknown* context = nullptr) override
	{
		SharedPtr<FolderPackage> owner = this->owner;
		ASSERT (owner != nullptr)
		AutoPtr<IStream> sourceStream = RelativeFileSystem::openStream (url, mode, context);
		return sourceStream && owner ? NEW FolderPackage::SubStream (*owner, sourceStream) : sourceStream.detach ();
	}

	tresult CCL_API queryInterface (UIDRef iid, void** ptr) override
	{
		// delegate to owner to get back to package file interfaces
		if(owner && (iid == ccl_iid<IPackageFile> () || iid == ccl_iid<IFileResource> ()))
			return owner->queryInterface (iid, ptr);

		return RelativeFileSystem::queryInterface (iid, ptr);
	}
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

#define FOLDER_PACKAGE_TEXT CCLSTR ("Package data is located in \"{Package-name}.data\" folder!")

//************************************************************************************************
// FolderPackage
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (FolderPackage, FileResource)

//////////////////////////////////////////////////////////////////////////////////////////////////

FolderPackage::FolderPackage (UrlRef path)
: FileResource (path),
  fileSystem (nullptr),
  dummyFile (nullptr),
  useCount (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

FolderPackage::~FolderPackage ()
{
	close ();

	ASSERT (fileSystem == nullptr && dummyFile == nullptr)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API FolderPackage::setRepresentedFileType (const FileType& fileType)
{
	representedFileType = fileType;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const FileType& CCL_API FolderPackage::getRepresentedFileType () const
{
	return representedFileType;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API FolderPackage::setOption (StringID id, VariantRef value)
{
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API FolderPackage::getOption (Variant& value, StringID id) const
{
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFileSystem* CCL_API FolderPackage::getFileSystem ()
{
	return fileSystem;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPackageItem* CCL_API FolderPackage::getRootItem ()
{
	// not implemented!
	ASSERT (0)
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API FolderPackage::embedd (UrlRef path, int fileIteratorMode, IUrlFilter* filter, IProgressNotify* progress)
{
	// not implemented!
	ASSERT (0)
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API FolderPackage::embeddToFolder (UrlRef destPath, UrlRef sourcePath, int fileIteratorMode, IUrlFilter* filter, IProgressNotify* progress)
{
	// not implemented!
	ASSERT (0)
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API FolderPackage::extractAll (UrlRef path, tbool deep, IUrlFilter* filter, IProgressNotify* progress)
{
	// not implemented!
	ASSERT (0)
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API FolderPackage::extractFolder (UrlRef sourcePath, UrlRef destPath, tbool deep, IUrlFilter* filter, IProgressNotify* progress)
{
	// not implemented!
	ASSERT (0)
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FolderPackage::flush (IProgressNotify* progress)
{
	// nothing to do here ;-)
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPackageItem* CCL_API FolderPackage::createItem (UrlRef url, IPackageItemWriter* writer, int* attributes)
{
	CCL_NOT_IMPL ("FolderPackage::createItem not implemented!")

	if(writer)
		writer->release ();

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPackageItem* CCL_API FolderPackage::copyItem (IPackageFile* sourcePackage, UrlRef sourcePath, const IUrl* destPath)
{
	CCL_NOT_IMPL ("FolderPackage::copyItem not implemented!")
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UrlRef FolderPackage::getDataPath (Url& dataPath) const
{
	dataPath.assign (path);
	if(!path.isFolder ())
	{
		dataPath.ascend ();
		String name;
		path.getName (name, false);
		name.append (CCLSTR (".data"));
		dataPath.descend (name, Url::kFolder);
	}
	return dataPath;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FolderPackage::openFile (int mode)
{
	ASSERT (fileSystem == nullptr && dummyFile == nullptr)

	// TODO: use CFBundle on macOS???

	if(path.isFile ())
	{
		dummyFile = System::GetFileSystem ().openStream (path, IStream::kOpenMode);
		if(!dummyFile)
			return false;

		// verify package text
		bool verified = false;
		char buffer[1024] = {0}; // only inspect the first 1 KB to avoid scanning for null-terminated string in large binary files
		int numRead = dummyFile->read (buffer, 1024);
		if(numRead > 0)
		{
			MemoryStream ms (buffer, numRead);
			Streamer s (ms);
			String text;
			if(s.readByteOrder () && s.readString (text))
			{
				if(text == FOLDER_PACKAGE_TEXT)
					verified = true;
			}
		}

		if(verified == false)
		{
			dummyFile->release (),
			dummyFile = nullptr;
			return false;
		}
	}
	else
	{
		if(!System::GetFileSystem ().fileExists (path))
			return false;
	}

	Url dataPath;
	fileSystem = FolderPackageFileSystem::fromFolder (*this, getDataPath (dataPath));
	ASSERT (fileSystem != nullptr)
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FolderPackage::createFile (int mode)
{
	ASSERT (fileSystem == nullptr && dummyFile == nullptr)

	if(path.isFile ())
	{
		dummyFile = System::GetFileSystem ().openStream (path, IStream::kCreateMode);
		if(!dummyFile)
			return false;

		Streamer s (*dummyFile);
		s.writeByteOrder ();
		s.writeString (FOLDER_PACKAGE_TEXT);
	}
	else
	{
		if(!System::GetFileSystem ().createFolder (path))
			return false;
	}

	Url dataPath;
	fileSystem = FolderPackageFileSystem::fromFolder (*this, getDataPath (dataPath));
	ASSERT (fileSystem != nullptr)
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FolderPackage::closeFile  ()
{
	if(dummyFile)
		dummyFile->release (),
		dummyFile = nullptr;

	if(fileSystem)
	{
		ASSERT (useCount == 0)
		fileSystem->setOwner (nullptr);
		uint32 refCount = fileSystem->release ();
		ASSERT (refCount == 0)
		fileSystem = nullptr;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUrl* FolderPackage::translateUrl (UrlRef path) const
{
	return fileSystem ? fileSystem->translateUrl (path) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FolderPackage* FolderPackage::findPackage (UrlRef path)
{
	if(path.isFolder ())
		return NEW FolderPackage (path);

	// try package info file
	String fileName;
	path.getName (fileName);
	if(fileName == PackageInfo::kFileName)
	{
		Url folderPath (path);
		folderPath.ascend ();
		return NEW FolderPackage (folderPath);
	}

	// try dummy file
	AutoPtr<FolderPackage> package (NEW FolderPackage (path));
	if(package->openFile (IStream::kOpenMode))
	{
		package->closeFile ();
		package->retain ();
		return package;
	}
	return nullptr;
}
