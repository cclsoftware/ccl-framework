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
// Filename    : ccl/public/system/ipackagefile.h
// Description : Package File Interface
//
//************************************************************************************************

#ifndef _ccl_ipackagefile_h
#define _ccl_ipackagefile_h

#include "ccl/public/storage/ifileresource.h"

#include "ccl/public/system/ifilesystem.h"

namespace CCL {

interface IUrlFilter;
interface IPackageItem;
interface IProgressNotify;
interface IPackageItemWriter;
class FileType;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Built-in Package Classes
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID
{
	DEFINE_CID (FolderPackage, 0xb94dd3d1, 0x7fc7, 0x40c3, 0xb5, 0xf3, 0xab, 0xc0, 0x66, 0x4b, 0xb, 0x58);
	DEFINE_CID (PackageFile, 0x6b4597cd, 0xd7f6, 0x422a, 0x88, 0xcc, 0x5c, 0xa1, 0xca, 0x92, 0x3a, 0x89);
	DEFINE_CID (ZipFile, 0x706b59b6, 0xec0, 0x4243, 0x90, 0x7d, 0x9, 0x45, 0xbc, 0x5a, 0x7b, 0x69);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Package File Options
//////////////////////////////////////////////////////////////////////////////////////////////////

/** Package file options. 
	\ingroup ccl_system */
namespace PackageOption
{
	/** Package file format version. */
	DEFINE_STRINGID (kFormatVersion, "formatVersion");

	/** Package is compressed. */
	DEFINE_STRINGID (kCompressed, "compressed");

	/** Package compression level. */
	DEFINE_STRINGID (kCompressionLevel, "compressionLevel");

	/** Package is encrypted using a very simple (unsafe) cipher algorithm. */
	DEFINE_STRINGID (kBasicEncrypted, "basicencrypted");

	/** Package is encrypted using the XTEA algorithm. */
	DEFINE_STRINGID (kXTEAEncrypted, "xteaencrypted");

	/** Package is encrypted using 128 bit AES. */
	DEFINE_STRINGID (kAESEncrypted, "aesencrypted");

	/** Encryption key not saved with the package file [string]. */
	DEFINE_STRINGID (kExternalEncryptionKey, "externalkey");

	/** Reserve block before file data [int]. */
	DEFINE_STRINGID (kReservedBlockSize, "reservedblocksize");
	
	/** Package sub-streams are thread-safe. [ThreadSafety]*/
	DEFINE_STRINGID (kThreadSafe, "threadsafe");

	/** All Package files must be valid on writing. */
	DEFINE_STRINGID (kFailOnInvalidFile, "failOnInvalid");

	/** Enable detailed progress notifications. */
	DEFINE_STRINGID (kDetailedProgressEnabled, "detailedProgressEnabled");

	/** Thread Safety Modes*/
	DEFINE_ENUM (ThreadSafetyMode)
	{		
		kThreadSafetyOff, 
		kThreadSafetyReopen,  ///< sub-streams can be accessed by concurrent threads - implemented as separate file stream per sub-stream
		kThreadSafetyLocked   ///< sub-streams can be accessed by concurrent threads - implemented as locked access
	};
}

//************************************************************************************************
// IPackageFile
/** A package file represents a "file system in a file". 
	\ingroup ccl_system */
//************************************************************************************************

interface IPackageFile: IFileResource
{
	/** Set package file option (e.g. enable/disable compression). */
	virtual tresult CCL_API setOption (StringID id, VariantRef value) = 0;

	/** Get package file option. */
	virtual tresult CCL_API getOption (Variant& value, StringID id) const = 0;

	/** Get file system interface to iterate, open and create individual streams. */
	virtual IFileSystem* CCL_API getFileSystem () = 0;

	/** Get root of content table (returns null if unsupported). */
	virtual IPackageItem* CCL_API getRootItem () = 0;

	/** Embedd files into package. This method builds the content table only. Use 'flush' to copy file data. */
	virtual int CCL_API embedd (UrlRef path, int fileIteratorMode = IFileIterator::kAll, 
								IUrlFilter* filter = nullptr, IProgressNotify* progress = nullptr) = 0;

	/** Embedd files into package in a subfolder. This method builds the content table only. Use 'flush' to copy file data. */
	virtual int CCL_API embeddToFolder (UrlRef destPath, UrlRef sourcePath, 
										int fileIteratorMode = IFileIterator::kAll, 
										IUrlFilter* filter = nullptr, IProgressNotify* progress = nullptr) = 0;
	
	/** Extract all files from package to local folder. */
	virtual int CCL_API extractAll (UrlRef path, tbool deep = true, IUrlFilter* filter = nullptr, IProgressNotify* progress = nullptr) = 0;

	/** Extract subfolder from package to local folder. */
	virtual int CCL_API extractFolder (UrlRef sourcePath, UrlRef destPath, tbool deep = true, IUrlFilter* filter = nullptr, IProgressNotify* progress = nullptr) = 0;

	/** Create item to be written during flush (takes ownership of writer!). */
	virtual IPackageItem* CCL_API createItem (UrlRef url, IPackageItemWriter* writer, int* attributes = nullptr) = 0;
	
	/** Create item as a copy of an item from another package. If destination path is not given, source path will be used. */
	virtual IPackageItem* CCL_API copyItem (IPackageFile* sourcePackage, UrlRef sourcePath, const IUrl* destPath = nullptr) = 0;

	/** Flush changes made to content table. This process might take a while. */
	virtual tbool CCL_API flush (IProgressNotify* progress = nullptr) = 0;

	DECLARE_IID (IPackageFile)
};

DEFINE_IID (IPackageFile, 0x49562048, 0x4702, 0x4fec, 0x9a, 0x24, 0xcf, 0x28, 0x3f, 0xc1, 0x75, 0x53)

//************************************************************************************************
// IFolderPackage
/** Additional interface for folder packages.
	\ingroup ccl_system */
//************************************************************************************************

interface IFolderPackage: IUnknown
{
	/** Set file type this folder represents (optional). */
	virtual void CCL_API setRepresentedFileType (const FileType& fileType) = 0;

	/** Get represented file type, can be empty. */
	virtual const FileType& CCL_API getRepresentedFileType () const = 0;

	DECLARE_IID (IFolderPackage)
};

DEFINE_IID (IFolderPackage, 0x1095ea61, 0x9cc4, 0x4906, 0x85, 0x18, 0x72, 0x90, 0xc6, 0x80, 0xed, 0xba)

//************************************************************************************************
// IPackageItem
/** Represents an item in content table of a package file. 
	\ingroup ccl_system */
//************************************************************************************************

interface IPackageItem: IUnknown
{
	/** Package item attributes. */
	enum Attributes
	{
		kPlain = 0,				///< item is stored "as is"
		kCompressed = 1<<1,		///< item is stored compressed
		kEncrypted = 1<<2,		///< item is stored encrypted
		kUseExternalKey = 1<<3,	///< item is encrypted with an external key
		kHidden = 1<<4			///< item is hidden
	};

	/** Check if item is a file. */
	virtual tbool CCL_API isFile () const = 0;

	/** Check if item is a folder. */
	virtual tbool CCL_API isFolder () const = 0;
	
	/** Get file (or folder) name. */
	virtual StringRef CCL_API getFileName () const = 0;
	
	/** Get uncompressed size of file data. */
	virtual int64 CCL_API getSizeOnDisk () const = 0;
	
	/** Get item attributes. */
	virtual int CCL_API getItemAttributes () const = 0;

	/** Get time of last modification. */
	virtual tbool CCL_API getModifiedTime (FileTime& time) const = 0;

	/** Get number of child items. */
	virtual int CCL_API countSubItems () const = 0;

	/** Get child item. */
	virtual IPackageItem* CCL_API getSubItem (int index) const = 0;

	DECLARE_IID (IPackageItem)
};

DEFINE_IID (IPackageItem, 0x66ad9682, 0x489a, 0x4656, 0x88, 0x4f, 0x5b, 0x68, 0xc8, 0x17, 0xe0, 0xb3)

//************************************************************************************************
// IPackageItemWriter
/** Callback interface for writing package item data. 
	\ingroup ccl_system */
//************************************************************************************************

interface IPackageItemWriter: IUnknown
{
	/** Write package item data. */
	virtual tresult CCL_API writeData (IStream& dstStream, IProgressNotify* progress = nullptr) = 0;

	DECLARE_IID (IPackageItemWriter)
};

DEFINE_IID (IPackageItemWriter, 0x6d190a9d, 0xca85, 0x42dc, 0xbf, 0x78, 0xd8, 0x8e, 0xe4, 0xd6, 0x29, 0x1c)

//************************************************************************************************
// IPackageItemFilter
/** Callback interface to adjust package items. 
	\ingroup ccl_system */
//************************************************************************************************

interface IPackageItemFilter: IUnknown
{
	/** Get attributes when packaging from a folder (external key, hidden). */
	virtual int CCL_API getPackageItemAttributes (UrlRef path) const = 0;

	DECLARE_IID (IPackageItemFilter)
};

DEFINE_IID (IPackageItemFilter, 0x366a4a30, 0xbc08, 0x4a69, 0xb8, 0xa4, 0xfa, 0x8f, 0x99, 0xd8, 0xd3, 0x8c)

} // namespace CCL

#endif // _ccl_ipackagefile_h
