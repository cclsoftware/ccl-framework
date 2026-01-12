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
// Filename    : ccl/system/packaging/filearchive.cpp
// Description : File Archive
//
//************************************************************************************************

#include "ccl/system/packaging/filearchive.h"
#include "ccl/system/packaging/sectionstream.h"
#include "ccl/system/packaging/bufferedstream.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/base/variant.h"
#include "ccl/public/base/iprogress.h"
#include "ccl/public/base/idatatransformer.h"

#include "ccl/public/system/ilockable.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/systemservices.h"

#include "ccl/base/collections/container.h"

namespace CCL {

//************************************************************************************************
// FileArchive::SubStream
//************************************************************************************************

class FileArchive::SubStream: public SectionStream
{
public:
	SubStream (FileArchive& _archive, FileStreamItem& item, IStream* sourceStream, Threading::ILockable* lock)
	: SectionStream (sourceStream, item.getFileDataOffset (), item.getFileDataSize (), kReadMode, lock),
	  archive (&_archive)
	{
		AtomicAddInline (archive->useCount, 1);
	}

	~SubStream ()
	{
		AtomicAddInline (archive->useCount, -1);
	}

protected:
	SharedPtr<FileArchive> archive;
};

//************************************************************************************************
// FileArchive::ExternalArchiveReference
//************************************************************************************************

class FileArchive::ExternalArchiveReference: public Object,
											 public IPackageItemWriter
{
public:
	DECLARE_CLASS_ABSTRACT (ExternalArchiveReference, Object)

	PROPERTY_SHARED_AUTO (FileArchive, sourceArcchive, SourceArcchive)
	PROPERTY_SHARED_AUTO (FileStreamItem, sourceItem, SourceItem)

	enum ExtraAttributes
	{
		kExternalArchiveItem = 1<< (FileSystemItem::kFileSystemItemLastFlag + 1), ///< extra flag in FileStreamItem, to mark items with an ExternalArchiveReference
	};

	// IPackageItemWriter: only used to store this in a FileStreamItem
	tresult CCL_API writeData (IStream& dstStream, IProgressNotify* progress = nullptr) override { return kResultUnexpected; }

	CLASS_INTERFACE (IPackageItemWriter, Object)
};

DEFINE_CLASS_ABSTRACT_HIDDEN (FileArchive::ExternalArchiveReference, Object)

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
////////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("PackageFile")
	XSTRING (PackagingFile, "Packaging: %(1)")
END_XSTRINGS

//************************************************************************************************
// FileArchive
//************************************************************************************************

DEFINE_CLASS_ABSTRACT (FileArchive, FileStreamResource)
DEFINE_CLASS_NAMESPACE (FileArchive, NAMESPACE_CCL)

//////////////////////////////////////////////////////////////////////////////////////////////////

FileArchive::FileArchive (UrlRef path)
: FileStreamResource (path),
  tempFolder (nullptr),
  compressionLevel (1.f),
  isCreated (false),
  crc32Enabled (false),
  failOnInvalidFile (false),
  detailedProgressEnabled (false),
  useCount (0),
  threadSafety (PackageOption::kThreadSafetyOff),
  lock (nullptr)
{
	::memset (externalEncryptionKey, 0, 16);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileArchive::destruct ()
{
	if(isOpen ())
		close ();
	else
		closeFile ();

	if(tempFolder)
		tempFolder->release ();

	safe_release (lock);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API FileArchive::setOption (StringID id, VariantRef value)
{
	if(id == PackageOption::kCompressed)
	{
		if(value.asBool ())
			compressionType.assign (ClassID::ZlibCompression);
		else
			compressionType.assign (kNullUID);
		return kResultOk;
	}
	else if(id == PackageOption::kCompressionLevel)
	{
		compressionLevel = value.asFloat ();
		return kResultOk;
	}
	else if(id == PackageOption::kExternalEncryptionKey)
	{
		MutableCString string (value.asString ());
		ASSERT (string.length () == 32)
		for(int i = 0, j = 0; i < 16; i++, j+=2)
		{
			char hexString[3] = {string[j], string[j+1], 0};
			int value = 0;
			::sscanf (hexString, "%02X", &value);
			externalEncryptionKey[i] = (uint8)(value & 0xFF);
		}
		return kResultOk;
	}
	else if(id == PackageOption::kThreadSafe)
	{
		setThreadSafety (value.asInt ());
		return kResultOk;
	}
	else if(id == PackageOption::kFailOnInvalidFile)
	{
		failOnInvalidFile = value.asBool ();
		return kResultOk;
	}
	else if(id == PackageOption::kDetailedProgressEnabled)
	{
		detailedProgressEnabled = value;
		return kResultOk;
	}
	return kResultInvalidArgument;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API FileArchive::getOption (Variant& value, StringID id) const
{
	if(id == PackageOption::kCompressed)
	{
		value = isCompressed ();
		return kResultOk;
	}
	else if(id == PackageOption::kCompressionLevel)
	{
		value = compressionLevel;
		return kResultOk;
	}
	else if(id == PackageOption::kThreadSafe)
	{
		value = getThreadSafety ();
		return kResultOk;
	}
	return kResultInvalidArgument;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFileSystem* CCL_API FileArchive::getFileSystem ()
{
	return this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPackageItem* CCL_API FileArchive::getRootItem ()
{
	return &getRoot ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UrlRef FileArchive::getTempFolder ()
{
	if(!tempFolder)
	{
		tempFolder = NEW Url;
		System::GetFileUtilities ().makeUniqueTempFolder (*tempFolder);
	}
	return *tempFolder;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileArchive::openWithStream (IStream& stream)
{
	ASSERT (!isOpen () && path.isEmpty ())
	if(isOpen () || !path.isEmpty ())
		return false;

	if(!readFormat (stream))
		return false;

	take_shared (file, &stream);
	isCreated = false;
	setReadOnly (true);
	openCount++;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileArchive::createWithStream (IStream& stream)
{
	ASSERT (!isOpen () && path.isEmpty ())
	if(isOpen () || !path.isEmpty ())
		return false;

	take_shared (file, &stream);
	isCreated = true;
	openCount++;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileArchive::openFile (int mode)
{
	if(!SuperClass::openFile (mode))
		return false;

	ASSERT (file != nullptr)
	if(!readFormat (*file))
		return false;

	isCreated = false;
	setReadOnly (true);
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////w/////

bool FileArchive::createFile (int mode)
{
	if(!SuperClass::createFile (mode))
		return false;

	isCreated = true;
	setReadOnly (false);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileArchive::closeFile ()
{
	// Note: flush () must be called explicitely before closing the file!

	if(tempFolder)
		System::GetFileSystem ().removeFolder (*tempFolder, kDeleteRecursively); // must delete with subfolders!

	setRoot (nullptr);
	setReadOnly (true);
	isCreated = false;
	return SuperClass::closeFile ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API FileArchive::embedd (UrlRef path, int fileIteratorMode, IUrlFilter* filter, IProgressNotify* progress)
{
	return createFromFolder (path, fileIteratorMode, filter, progress);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API FileArchive::embeddToFolder (UrlRef destPath, UrlRef sourcePath, int fileIteratorMode, IUrlFilter* filter, IProgressNotify* progress)
{
	FileSystemItem* targetItem = lookupItem (destPath, true);
	return createFromFolder (sourcePath, fileIteratorMode, filter, progress, targetItem);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API FileArchive::extractAll (UrlRef path, tbool deep, IUrlFilter* filter, IProgressNotify* progress)
{
	return extractToFolder (path, deep != 0, filter, progress);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API FileArchive::extractFolder (UrlRef sourcePath, UrlRef destPath, tbool deep, IUrlFilter* filter, IProgressNotify* progress)
{
	ASSERT (sourcePath.isFolder () == true)
	if(!sourcePath.isFolder ())
		return 0;

	FileSystemItem* item = lookupItem (sourcePath);
	return item ? extractToFolder (destPath, deep != 0, filter, progress, item) : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPackageItem* CCL_API FileArchive::createItem (UrlRef url, IPackageItemWriter* writer, int* attributes)
{
	ASSERT (writer != nullptr)
	if(!writer)
		return nullptr;

	ASSERT (url.isFile () == true)
	if(!url.isFile ())
		return nullptr;

#if 1
	FileStreamItem* item = ccl_cast<FileStreamItem> (lookupItem (url, true));
	ASSERT (item != nullptr)
	if(item)
	{
		item->setWriter (writer);

		// set Compression/Encryption state
		int itemAttr = 0;
		if(attributes)
			itemAttr = (*attributes) & FileSystemItem::kPublicAttrMask;
		else
		{
			if(isCompressed ())
				itemAttr |= FileSystemItem::kCompressed;
			if(isEncrypted ())
				itemAttr |= FileSystemItem::kEncrypted;
		}
		item->setAttributes (itemAttr);
	}

	writer->release ();
	return item;
#else
	AutoPtr<IStream> stream = openStream (url, IStream::kCreateMode);
	FileSystemItem* item = lookupItem (url);
	if(stream && item)
	{
		tresult result = writer->writeData (*stream);
		writer->release ();
		if(result != kResultOk)
		{
			item->isDeleted (true);
			item = 0;
		}
	}
	return item;
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPackageItem* CCL_API FileArchive::copyItem (IPackageFile* sourcePackage, UrlRef sourcePath, const IUrl* destPath)
{
	FileArchive* sourceArchive = unknown_cast<FileArchive> (sourcePackage);
	ASSERT (sourceArchive)
	ASSERT (sourcePath.isFile ())
	if(!sourceArchive || !sourcePath.isFile ())
		return nullptr;

	FileStreamItem* sourceItem = ccl_cast<FileStreamItem> (sourceArchive->lookupItem (sourcePath));
	ASSERT (sourceItem)
	if(!sourceItem)
		return nullptr;

	// compression / encryption of source archive must be compatible, if used for this item
	if((sourceItem->isCompressed () && sourceArchive->getCompressionType () != getCompressionType ()) ||
		(sourceItem->isEncrypted () && (sourceArchive->getEncryptionType () != getEncryptionType () || ::memcmp (sourceArchive->externalEncryptionKey, externalEncryptionKey, sizeof(externalEncryptionKey)) != 0)))
		return nullptr;

	if(!destPath)
		destPath = &sourcePath;

	FileStreamItem* item = ccl_cast<FileStreamItem> (lookupItem (*destPath, true));
	ASSERT (item != nullptr)
	if(item)
	{
		AutoPtr<ExternalArchiveReference> reference (NEW ExternalArchiveReference);
		reference->setSourceArcchive (sourceArchive);
		reference->setSourceItem (sourceItem);
		item->setWriter (reference);
		item->setAttributes (sourceItem->getAttributes () | ExternalArchiveReference::kExternalArchiveItem);
	}
	return item;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileArchive::flush (IProgressNotify* progress)
{
	bool result = false;
	if(isOpen () && isCreated)
	{
		if(progress)
			progress->beginProgress ();

		ASSERT (file != nullptr)
		BufferedStream bStream (file, 1 << 17);
		bStream.setStreamOptions (INativeFileStream::kWriteFlushed);

		result = writeFormat (bStream, progress);

		if(progress)
			progress->endProgress ();
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileArchive::flushAll (IStream& dstStream, IProgressNotify* progress)
{
	getRoot ().removeDeleted ();
	bool result = flushFolderData (dstStream, getRoot (), progress);
	getRoot ().removeDeleted (); // maybe some items failed to open while flushing
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileArchive::flushFolderData (IStream& dstStream, FolderItem& folder, IProgressNotify* progress)
{
	int numItems = folder.countSubItems ();
	int i = 0;

	IterForEach (folder.newIterator (), FileSystemItem, item)
		FileStreamItem* fileItem = ccl_cast<FileStreamItem> (item);
		if(fileItem)
		{
			if(progress)
			{
				if(progress->isCanceled ())
					return false;

				if(detailedProgressEnabled)
				{
					progress->setProgressText (String ().appendFormat (XSTR (PackagingFile), fileItem->getFileName ()));
					progress->updateProgress (i / float(numItems));
				}
				else
					progress->updateAnimated (String ().appendFormat (XSTR (PackagingFile), fileItem->getFileName ()));
			}

			bool result = false;
			if(fileItem->getWriter ())
			{
				if(fileItem->getAttributes () & ExternalArchiveReference::kExternalArchiveItem)
					result = copyFileDataFromPackage (dstStream, *fileItem, progress);
				else
					result = writeFileData (dstStream, *fileItem, progress);

				fileItem->setWriter (nullptr); // release writer immediately to avoid circular references or use after free
			}
			else
				result = copyFileData (dstStream, *fileItem, progress);

			if(!result)
			{
				fileItem->isDeleted (true);
				if(failOnInvalidFile)
					return false;
				continue;
			}
		}
		else if(FolderItem* folderItem = ccl_cast<FolderItem> (item))
		{
			if(!beginFolder (dstStream, *folderItem))
				return false;

			if(!flushFolderData (dstStream, *folderItem, progress))
				return false;
		}
		i++;
	EndFor
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SectionStream* FileArchive::openSectionStream (FileStreamItem& item)
{
	AutoPtr<IStream> file2;

	// reopen original file if thread-safety is required!
	if(getThreadSafety () == PackageOption::kThreadSafetyReopen)
		file2 = System::GetFileSystem ().openStream (path);
	else
		file2.share (file);

	ASSERT (file2 != nullptr)
	if(file2 == nullptr)
		return nullptr;

	bool locked = getThreadSafety () == PackageOption::kThreadSafetyLocked;
	if(locked && lock == nullptr)
	{
		lock = System::CreateAdvancedLock (ClassID::ExclusiveLock);
		ASSERT (lock)
	}

	return NEW SubStream (*this, item, file2, locked ? lock : nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStream* FileArchive::openDataStream (FileStreamItem& item, int mode, IUnknown* context)
{
	bool writeMode = (mode & IStream::kWriteMode) != 0;
	if(writeMode && !item.getLocalPath ())
	{
		bool createMode = (mode & IStream::kCreate) != 0;
		ASSERT (createMode == true)
		ASSERT (isReadOnly () == false)
		// TODO: if item already exists and kCreate is not set, copy to temp folder first!

		String pathString;
		item.getChildPath (pathString);
		AutoPtr<Url> localPath = NEW Url (getTempFolder ());
		localPath->descend (pathString);
		item.setTemporaryFile (true);
		item.setLocalPath (localPath);
	}

	if(item.getLocalPath ())
		return System::GetFileSystem ().openStream (*item.getLocalPath (), mode, context);
	else
	{
		IStream* dataStream = openSectionStream (item);
		if(dataStream && !item.isPlain ())
		{
			// Decompression/Decryption
			IStream* transformStream = createReadTransform (*dataStream, item, context);
			ASSERT (transformStream != nullptr)
			dataStream->release (); // shared by transform stream!
			dataStream = transformStream;
		}
		return dataStream;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileArchive::writeFileData (IStream& dstStream, FileStreamItem& fileItem, IProgressNotify* progress)
{
	IPackageItemWriter* writer = fileItem.getWriter ();
	ASSERT (writer != nullptr)

	// update modification time
	fileItem.updateTime ();

	// file header (optional)
	int64 fileHeaderSize = beginFile (dstStream, fileItem);
	if(fileHeaderSize == -1) // error
		return false;

	int64 fileDataOffset = dstStream.tell ();
	AutoPtr<IStream> localDstStream (&dstStream);
	localDstStream->retain ();

	int64 localOffset = fileDataOffset;

	// Compression/Encryption
	if(!fileItem.isPlain ())
	{
		IStream* transformStream = createWriteTransform (dstStream, fileItem, nullptr/*no context!*/);
		ASSERT (transformStream != nullptr)
		if(!transformStream) // hmm...?
			return false;

		localDstStream.release ();
		localDstStream = transformStream;
		localOffset = 0; // this assumes the transformed stream initially "tells" zero
	}

	// write data
	uint32 crc32 = 0;
	tresult result = kResultFailed;
	AutoPtr<IProgressNotify> subProgress;
	if(detailedProgressEnabled)
		subProgress = progress ? progress->createSubProgress () : nullptr;

	ProgressNotifyScope scope (subProgress);
	if(isCrc32Enabled ())
	{
		Crc32Stream crcCalculator (localDstStream, IStream::kWriteMode);
		result = writer->writeData (crcCalculator, subProgress);
		crc32 = crcCalculator.getCrc32 ();
	}
	else
		result = writer->writeData (*localDstStream, subProgress);
	ASSERT (result == kResultOk)
	if(result != kResultOk)
		return false;

	int64 fileSizeOnDisk = localDstStream->tell () - localOffset;

	localDstStream.release (); // this should flush encryption/compression if necessary

	int64 fileDataSize = dstStream.tell () - fileDataOffset;
	ASSERT (fileItem.isCompressed () || fileDataSize == fileSizeOnDisk) // sanity check

	// update item (if something fails later the whole tree gets messed up!)
	fileItem.setFileDataOffset (fileDataOffset);
	fileItem.setFileDataSize   (fileDataSize);
	fileItem.setFileSizeOnDisk (fileSizeOnDisk);
	fileItem.setFileHeaderSize (fileHeaderSize);
	fileItem.setCrc32 (crc32);

	// finish file
	return endFile (dstStream, fileItem);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileArchive::copyFileDataFromPackage (IStream& dstStream, FileStreamItem& fileItem, IProgressNotify* progress)
{
	ExternalArchiveReference* writer = unknown_cast<ExternalArchiveReference> (fileItem.getWriter ());
	ASSERT (writer != nullptr)

	FileStreamItem* sourceItem = writer->getSourceItem ();
	FileArchive* sourceArchive = writer->getSourceArcchive ();
	ASSERT (sourceItem && sourceArchive)

	int sourceAttribs = sourceItem->getAttributes ();
	sourceItem->setAttributes (sourceAttribs & ~(IPackageItem::kEncrypted|IPackageItem::kCompressed)); // copy compressed / encrypted data directly
	AutoPtr<IStream> srcStream = (sourceArchive && sourceItem) ? sourceArchive->openDataStream (*sourceItem, IStream::kOpenMode, nullptr/*no context!*/) : nullptr;
	sourceItem->setAttributes (sourceAttribs);
	fileItem.setAttributes (sourceAttribs);

	ASSERT (srcStream != nullptr)
	if(!srcStream)
		return false;

	// update modification time
	fileItem.setTime (sourceItem->getTime ());

	// file header (optional)
	int64 fileHeaderSize = beginFile (dstStream, fileItem);
	if(fileHeaderSize == -1) // error
		return false;

	int64 fileDataOffset = dstStream.tell ();

	// write data
	tresult result = kResultFailed;
	AutoPtr<IProgressNotify> subProgress;
	if(detailedProgressEnabled)
		subProgress = progress ? progress->createSubProgress () : nullptr;

	ProgressNotifyScope scope (subProgress);
	int64 maxBytesToCopy = sourceItem->getSizeOnDisk ();

	bool copied = System::GetFileUtilities ().copyStream (dstStream, *srcStream, nullptr, maxBytesToCopy) != 0;
	ASSERT (copied)

	int64 fileSizeInArchive = dstStream.tell () - fileDataOffset;
	ASSERT (fileSizeInArchive == sourceItem->getFileDataSize ())

	fileItem.setFileDataOffset (fileDataOffset);
	fileItem.setFileDataSize   (fileSizeInArchive);
	fileItem.setFileSizeOnDisk (sourceItem->getFileSizeOnDisk ());
	fileItem.setFileHeaderSize (fileHeaderSize);
	fileItem.setCrc32 (sourceItem->getCrc32 ());

	// finish file
	return endFile (dstStream, fileItem);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileArchive::copyFileData (IStream& dstStream, FileStreamItem& fileItem, IProgressNotify* progress)
{
	AutoPtr<IStream> srcStream = openDataStream (fileItem, IStream::kOpenMode, nullptr/*no context!*/);
	ASSERT (srcStream != nullptr)
	if(!srcStream) // hmm...?
		return false;

	// update modification time
	if(fileItem.getLocalPath ())
	{
		FileInfo info;
		if(System::GetFileSystem ().getFileInfo (info, *fileItem.getLocalPath ()))
			fileItem.setTime (info.modifiedTime);
	}

	// file header (optional)
	int64 fileHeaderSize = beginFile (dstStream, fileItem);
	if(fileHeaderSize == -1) // error
		return false;

	int64 fileDataOffset = dstStream.tell ();
	AutoPtr<IStream> localDstStream (&dstStream);
	localDstStream->retain ();

	// Compression/Encryption
	if(!fileItem.isPlain ())
	{
		IStream* transformStream = createWriteTransform (dstStream, fileItem, nullptr/*no context!*/);
		ASSERT (transformStream != nullptr)
		if(!transformStream) // hmm...?
			return false;

		localDstStream.release ();
		localDstStream = transformStream;
	}

	// copy data
	AutoPtr<IProgressNotify> subProgress;//was: = progress ? progress->createSubProgress () : 0;
	int64 maxBytesToCopy = fileItem.getSizeOnDisk ();

	uint32 crc32 = 0;
	bool copied = false;
	if(isCrc32Enabled ())
	{
		Crc32Stream crcCalculator (localDstStream, IStream::kWriteMode);
		copied = System::GetFileUtilities ().copyStream (crcCalculator, *srcStream, subProgress, maxBytesToCopy) != 0;
		crc32 = crcCalculator.getCrc32 ();
	}
	else
		copied = System::GetFileUtilities ().copyStream (*localDstStream, *srcStream, subProgress, maxBytesToCopy) != 0;
	ASSERT (copied == true)
	if(!copied)
		return false;

	localDstStream.release (); // this should flush encryption/compression if necessary

	int64 fileDataSize = dstStream.tell () - fileDataOffset;
	int64 fileSizeOnDisk = srcStream->tell ();

	// update item (if something fails later the whole tree gets messed up!)
	fileItem.setFileDataOffset (fileDataOffset);
	fileItem.setFileDataSize   (fileDataSize);
	fileItem.setFileSizeOnDisk (fileSizeOnDisk);
	fileItem.setFileHeaderSize (fileHeaderSize);
	fileItem.setCrc32 (crc32);

	// unlink from local file
	srcStream.release ();
	fileItem.unlinkLocalFile ();

	// finish file
	return endFile (dstStream, fileItem);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (FileArchive)
	DEFINE_METHOD_NAME ("setOption")
	DEFINE_METHOD_NAME ("create")
	DEFINE_METHOD_NAME ("embedd")
	DEFINE_METHOD_NAME ("extract") // old API name
	DEFINE_METHOD_NAME ("extractAll")
	DEFINE_METHOD_NAME ("flush")
	DEFINE_METHOD_NAME ("close")
END_METHOD_NAMES (FileArchive)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileArchive::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "setOption")
	{
		MutableCString id (msg[0].asString ());
		returnValue = setOption (id, msg[1]);
		return true;
	}
	else if(msg == "create")
	{
		returnValue = create ();
		return true;
	}
	else if(msg == "embedd")
	{
		UnknownPtr<IUrl> path (msg[0].asUnknown ());
		ASSERT (path.isValid ())
		bool deep = msg.getArgCount () > 1 ? msg[1].asBool () : true;
		int fileIteratorMode = deep ? IFileIterator::kAll : IFileIterator::kFiles;
		returnValue = path.isValid () ? embedd (*path, fileIteratorMode) : -1;
		return true;
	}
	else if(msg == "extract" || msg == "extractAll")
	{
		if(isEncrypted () == true) // disable for protected archives
			returnValue = -1;
		else
		{
			UnknownPtr<IUrl> path (msg[0].asUnknown ());
			ASSERT (path.isValid ())
			bool deep = msg.getArgCount () > 1 ? msg[1].asBool () : true;
			returnValue = path.isValid () ? extractAll (*path, deep) : -1;
		}
		return true;
	}
	else if(msg == "flush")
	{
		returnValue = flush ();
		return true;
	}
	else if(msg == "close")
	{
		returnValue = close ();
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}
