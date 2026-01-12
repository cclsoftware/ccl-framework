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
// Filename    : ccl/system/packaging/folderpackage.h
// Description : Folder Package
//
//************************************************************************************************

#ifndef _ccl_folderpackage_h
#define _ccl_folderpackage_h

#include "ccl/base/storage/fileresource.h"
#include "ccl/public/system/ipackagefile.h"

namespace CCL {

class FolderPackageFileSystem;

//************************************************************************************************
// FolderPackage
/** Represents a native file system package (folder). It should be used for development only! */
//************************************************************************************************

class FolderPackage: public FileResource,
					 public IPackageFile,
					 public IFolderPackage
{
public:
	DECLARE_CLASS_ABSTRACT (FolderPackage, FileResource)

	FolderPackage (UrlRef path);
	~FolderPackage ();

	class SubStream;
	PROPERTY_VARIABLE (int32, useCount, UseCount) ///< needs to be atomic!

	IUrl* translateUrl (UrlRef path) const;

	static FolderPackage* findPackage (UrlRef path); ///< path can be the data folder, the package info file inside, or the "dummy" file besides the folder

	// IPackageFile
	DELEGATE_FILERESOURCE_METHODS (FileResource)
	tresult CCL_API setOption (StringID id, VariantRef value) override;
	tresult CCL_API getOption (Variant& value, StringID id) const override;
	IFileSystem* CCL_API getFileSystem () override;
	IPackageItem* CCL_API getRootItem () override;
	int CCL_API embedd (UrlRef path, int fileIteratorMode, IUrlFilter* filter, IProgressNotify* progress) override;
	int CCL_API embeddToFolder (UrlRef destPath, UrlRef sourcePath, int fileIteratorMode, IUrlFilter* filter, IProgressNotify* progress) override;
	int CCL_API extractAll (UrlRef path, tbool deep, IUrlFilter* filter, IProgressNotify* progress) override;
	int CCL_API extractFolder (UrlRef sourcePath, UrlRef destPath, tbool deep, IUrlFilter* filter, IProgressNotify* progress) override;
	IPackageItem* CCL_API createItem (UrlRef url, IPackageItemWriter* writer, int* attributes = nullptr) override;
	IPackageItem* CCL_API copyItem (IPackageFile* sourcePackage, UrlRef sourcePath, const IUrl* destPath = nullptr) override;
	tbool CCL_API flush (IProgressNotify* progress = nullptr) override;

	// IFolderPackage
	void CCL_API setRepresentedFileType (const FileType& fileType) override;
	const FileType& CCL_API getRepresentedFileType () const override;

	CLASS_INTERFACE2 (IPackageFile, IFolderPackage, FileResource)

protected:
	FolderPackageFileSystem* fileSystem;
	IStream* dummyFile;
	FileType representedFileType;

	UrlRef getDataPath (Url& dataPath) const;

	// FileResource overrides:
	bool openFile (int mode) override;
	bool createFile (int mode) override;
	bool closeFile () override;
};

} // namespace CCL

#endif // _ccl_folderpackage_h
