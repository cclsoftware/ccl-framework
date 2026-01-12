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
// Filename    : ccl/system/packaging/filearchive.h
// Description : File Archive
//
//************************************************************************************************

#ifndef _ccl_filearchive_h
#define _ccl_filearchive_h

#include "ccl/base/storage/fileresource.h"

#include "ccl/system/packaging/filetree.h"

namespace CCL {
namespace Threading {
interface ILockable; }

class SectionStream;

//************************************************************************************************
// FileArchive
//************************************************************************************************

class FileArchive: public FileStreamResource,
				   public FileTreeFileSystem,
				   public IPackageFile
{
public:
	DECLARE_CLASS_ABSTRACT (FileArchive, FileStreamResource)
	DECLARE_METHOD_NAMES (FileArchive)

	FileArchive (UrlRef path = Url ());

	PROPERTY_BOOL (crc32Enabled, Crc32Enabled)

	class SubStream;
	PROPERTY_VARIABLE (int32, useCount, UseCount) ///< needs to be atomic!
	PROPERTY_VARIABLE (int, threadSafety, ThreadSafety)	///< if true, sub-streams could be openend by concurrent threads

	bool openWithStream (IStream& stream); ///< open archive from existing stream (read-only, can't be used with thread-safe option!)
	bool createWithStream (IStream& stream);

	// IPackageFile
	DELEGATE_FILERESOURCE_METHODS (FileStreamResource)
	tresult CCL_API setOption (StringID id, VariantRef value) override;
	tresult CCL_API getOption (Variant& value, StringID id) const override;
	IFileSystem* CCL_API getFileSystem () override;
	IPackageItem* CCL_API getRootItem () override;
	int CCL_API embedd (UrlRef path, int fileIteratorMode = IFileIterator::kAll, IUrlFilter* filter = nullptr, IProgressNotify* progress = nullptr) override;
	int CCL_API embeddToFolder (UrlRef destPath, UrlRef sourcePath, int fileIteratorMode = IFileIterator::kAll, IUrlFilter* filter = nullptr, IProgressNotify* progress = nullptr) override;
	int CCL_API extractAll (UrlRef path, tbool deep = true, IUrlFilter* filter = nullptr, IProgressNotify* progress = nullptr) override;
	int CCL_API extractFolder (UrlRef sourcePath, UrlRef destPath, tbool deep = true, IUrlFilter* filter = nullptr, IProgressNotify* progress = nullptr) override;
	IPackageItem* CCL_API createItem (UrlRef url, IPackageItemWriter* writer, int* attributes = nullptr) override;
	IPackageItem* CCL_API copyItem (IPackageFile* sourcePackage, UrlRef sourcePath, const IUrl* destPath = nullptr) override;
	tbool CCL_API flush (IProgressNotify* progress = nullptr) override;

	CLASS_INTERFACE2 (IPackageFile, IFileSystem, FileStreamResource)

protected:
	Url* tempFolder;
	bool isCreated;
	float compressionLevel;
	bool failOnInvalidFile;
	bool detailedProgressEnabled;
	uint8 externalEncryptionKey[16];
	Threading::ILockable* lock;

	class ExternalArchiveReference;

	void destruct (); ///< to be called by dtor of derived class
	UrlRef getTempFolder ();

	virtual SectionStream* openSectionStream (FileStreamItem& item);

	bool flushAll (IStream& dstStream, IProgressNotify* progress);
	bool flushFolderData (IStream& dstStream, FolderItem& folder, IProgressNotify* progress);
	bool copyFileData (IStream& dstStream, FileStreamItem& fileItem, IProgressNotify* progress);
	bool copyFileDataFromPackage (IStream& dstStream, FileStreamItem& fileItem, IProgressNotify* progress);
	bool writeFileData (IStream& dstStream, FileStreamItem& fileItem, IProgressNotify* progress);

	// Format-specific methods:
	virtual bool readFormat (IStream& stream) = 0;
	virtual bool writeFormat (IStream& stream, IProgressNotify* progress) = 0;
	virtual int64 beginFile (IStream& dstStream, FileStreamItem& item) = 0;
	virtual bool endFile (IStream& dstStream, FileStreamItem& item) = 0;
	virtual bool beginFolder (IStream& dstStream, FolderItem& item) = 0;
	virtual IStream* createReadTransform (IStream& srcStream, FileStreamItem& item, IUnknown* context) const = 0;
	virtual IStream* createWriteTransform (IStream& dstStream, FileStreamItem& item, IUnknown* context) const = 0;

	// FileStreamResource overrides:
	bool openFile (int mode) override;
	bool createFile (int mode) override;
	bool closeFile () override;

	// FileTreeFileSystem overrides:
	IStream* openDataStream (FileStreamItem& item, int mode, IUnknown* context) override;

	// IObject
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

} // namespace CCL

#endif // _ccl_filearchive_h
