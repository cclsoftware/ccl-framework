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
// Filename    : ccl/system/packaging/filetree.h
// Description : File Tree
//
//************************************************************************************************

#ifndef _ccl_filetree_h
#define _ccl_filetree_h

#include "ccl/base/objectnode.h"
#include "ccl/public/text/cstring.h"
#include "ccl/public/system/ipackagefile.h"

namespace CCL {

class Url;
class Streamer;
interface IUrlFilter;
interface IProgressNotify;

//************************************************************************************************
// FileSystemItem
/** Base class for file and folder items. */
//************************************************************************************************

class FileSystemItem: public ObjectNode,
					  public IPackageItem
{
public:
	DECLARE_CLASS (FileSystemItem, ObjectNode)

	FileSystemItem (StringRef fileName = nullptr);

	PROPERTY_OBJECT (FileTime, time, Time)	///< last modification time
	void updateTime ();

	enum ExtraAttributes
	{
		kPublicAttrMask = kCompressed|kEncrypted|kUseExternalKey|kHidden,

		kDeleted = 1<<10,	///< item is marked as deleted

		kFileSystemItemLastFlag = 10
	};

	PROPERTY_VARIABLE (uint32, attributes, Attributes)

	bool isPlain () const;
	PROPERTY_FLAG (attributes, kDeleted, isDeleted)
	PROPERTY_FLAG (attributes, kCompressed, isCompressed)
	PROPERTY_FLAG (attributes, kEncrypted, isEncrypted)
	PROPERTY_FLAG (attributes, kUseExternalKey, useExternalKey)
	PROPERTY_FLAG (attributes, kHidden, isHidden)

	PROPERTY_MUTABLE_CSTRING (encodedFileName, EncodedFileName)
	PROPERTY_VARIABLE (TextEncoding, fileNameEncoding, FileNameEncoding) ///< used when writing ZIP files (not serialized with file tree)

	// IPackageItem
	tbool CCL_API isFile () const override;
	tbool CCL_API isFolder () const override;
	StringRef CCL_API getFileName () const override;
	int64 CCL_API getSizeOnDisk () const override;
	int CCL_API getItemAttributes () const override;
	tbool CCL_API getModifiedTime (FileTime& time) const override;
	int CCL_API countSubItems () const override;
	IPackageItem* CCL_API getSubItem (int index) const override;

	// (De)serialization
	virtual bool serialize (Streamer& stream, int version) const;
	virtual bool deserialize (Streamer& stream, int version);

	CLASS_INTERFACE (IPackageItem, ObjectNode)
};

//************************************************************************************************
// FileStreamItem
/** Represents a data stream. */
//************************************************************************************************

class FileStreamItem: public FileSystemItem
{
public:
	DECLARE_CLASS (FileStreamItem, FileSystemItem)

	FileStreamItem (StringRef fileName = nullptr);
	~FileStreamItem ();

	PROPERTY_VARIABLE (int64, fileDataOffset, FileDataOffset)	///< offset to file data inside archive
	PROPERTY_VARIABLE (int64, fileDataSize,   FileDataSize)		///< size of file data inside container (could be compressed!)
	PROPERTY_VARIABLE (int64, fileSizeOnDisk, FileSizeOnDisk)	///< original size of file on disk
	PROPERTY_VARIABLE (int64, fileHeaderSize, FileHeaderSize)	///< optional: size of header before file data
	PROPERTY_VARIABLE (uint32, crc32, Crc32)					///< optional: CRC-32 checksum
	PROPERTY_BOOL (startsWithHeader, StartsWithHeader)			///< optional: file data offset points to a file header

	Url* getLocalPath () const;
	void setLocalPath (Url* path);
	bool unlinkLocalFile ();

	PROPERTY_BOOL (temporaryFile, TemporaryFile)				///< if true, the local file is temporary only and will be deleted!
	PROPERTY_SHARED_AUTO (IPackageItemWriter, writer, Writer)

	// FileSystemItem
	tbool CCL_API isFile () const override;
	int64 CCL_API getSizeOnDisk () const override;
	tbool CCL_API getModifiedTime (FileTime& time) const override;
	bool serialize (Streamer& stream, int version) const override;
	bool deserialize (Streamer& stream, int version) override;

protected:
	Url* localPath;
};

//************************************************************************************************
// FolderItem
/** Represents a folder. */
//************************************************************************************************

class FolderItem: public FileSystemItem
{
public:
	DECLARE_CLASS (FolderItem, FileSystemItem)

	FolderItem ();

	PROPERTY_VARIABLE (int64, folderHeaderOffset, FolderHeaderOffset)	///< offset to local header for folder (ZIP only)

	void removeDeleted ();	///< recursively remove all items marked to be deleted

	// FileSystemItem
	tbool CCL_API isFolder () const override;
	int64 CCL_API getSizeOnDisk () const override;
	bool serialize (Streamer& stream, int version) const override;
	bool deserialize (Streamer& stream, int version) override;

protected:
	bool serializeChildren (Streamer& stream, int version) const;
	bool deserializeChildren (Streamer& stream, int version);
};

//************************************************************************************************
// RootFolderItem
/** Special item class for the root folder. */
//************************************************************************************************

class RootFolderItem: public FolderItem
{
public:
	DECLARE_CLASS (RootFolderItem, FolderItem)

	// FolderItem
	bool serialize (Streamer& stream, int version) const override;
	bool deserialize (Streamer& stream, int version) override;
};

//************************************************************************************************
// FileTreeFileSystem
/** Filesystem based on a file tree. */
//************************************************************************************************

class FileTreeFileSystem: public IFileSystem
{
public:
	FileTreeFileSystem ();
	virtual ~FileTreeFileSystem ();

	PROPERTY_BOOL (readOnly, ReadOnly)		///< if true, the file system is read-only
	PROPERTY_OBJECT (UID, compressionType, CompressionType)
	PROPERTY_OBJECT (UID, encryptionType,  EncryptionType)
	bool isCompressed () const;
	bool isEncrypted  () const;

	RootFolderItem& getRoot ();
	void setRoot (RootFolderItem* newRoot);
	FileSystemItem* lookupItem (UrlRef url, bool create = false);
	void getItemPath (String& path, FileSystemItem& item);
	void getItemUrl (Url& url, FileSystemItem& item);

	int createFromFolder (UrlRef folderPath, int fileIteratorMode = IFileIterator::kAll, IUrlFilter* filter = nullptr, 
						   IProgressNotify* progress = nullptr, FileSystemItem* current = nullptr);

	int extractToFolder (UrlRef folderPath, bool deep = true, IUrlFilter* filter = nullptr,
						  IProgressNotify* progress = nullptr, FileSystemItem* current = nullptr);

	// IFileSystem
	IStream* CCL_API openStream (UrlRef url, int mode = IStream::kOpenMode, IUnknown* context = nullptr) override;
	tbool CCL_API fileExists (UrlRef url) override;
	tbool CCL_API getFileInfo (FileInfo& info, UrlRef url) override;
	tbool CCL_API removeFile (UrlRef url, int mode = 0) override;
	tbool CCL_API renameFile (UrlRef url, StringRef newName, int mode = 0) override;
	IFileIterator* CCL_API newIterator (UrlRef url, int mode = IFileIterator::kAll) override;
	tbool CCL_API createFolder (UrlRef url) override;
	tbool CCL_API removeFolder (UrlRef url, int mode = 0) override;
	tbool CCL_API isCaseSensitive () override;

	// IUnknown - will be overwritten by subclass!
	IMPLEMENT_DUMMY_UNKNOWN (IFileSystem)

protected:
	RootFolderItem* rootItem;

	virtual IStream* openDataStream (FileStreamItem& item, int mode, IUnknown* context) = 0;
};

//************************************************************************************************
// FileTreeIterator
//************************************************************************************************

class FileTreeIterator: public Object,
						public IFileIterator
{
public:
	FileTreeIterator (FileTreeFileSystem& fileSystem, UrlRef path, int mode);
	~FileTreeIterator ();

	// IFileIterator
	const IUrl* CCL_API next () override;

	CLASS_INTERFACE (IFileIterator, Object)

protected:
	ObjectArray& paths;
	Iterator* iter;
};

} // namespace CCL

#endif // _ccl_filetree_h
