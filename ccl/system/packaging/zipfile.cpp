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
// Filename    : ccl/system/packaging/zipfile.cpp
// Description : Zip File
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/system/packaging/zipfile.h"
#include "ccl/system/packaging/zipfileformat.h"
#include "ccl/system/packaging/sectionstream.h"

#include "ccl/base/collections/container.h"

#include "ccl/public/base/variant.h"
#include "ccl/public/base/memorystream.h"
#include "ccl/public/base/idatatransformer.h"

using namespace CCL;

//************************************************************************************************
/*
	ZIP Protection (proprietary, not compatible to strong PKZIP encryption)

	Files using this encryption cannot be unpacked by general purpose ZIP tools because
	checksum validation will fail. CRC-32 is calculated from the original unencrypted data.

	From APPNOTE.TXT: Header ID's of 0 thru 31 are reserved for use by PKWARE.
	The remaining ID's can be used by third party vendors for proprietary usage.
	In case two different programs should appropriate the same Header ID value, it is strongly
	recommended that each program place a unique signature of at least two bytes in size
	(and preferably 4 bytes or bigger) at the start of each data area.
*/
//************************************************************************************************

namespace ZipProtection
{
	static const uint16 kDataID = 0xCC50;	///< "CCL" encoded as CC + Roman Numeral L (=50)
	static const uint32 kDataSize = 8;

	static DEFINE_FOURCC (kSignature, 'P', 'r', 'o', 't') ///< protection signature

	struct Data
	{
		FOURCC signature;			///< protection signature
		uint16 encryptionType;		///< encryption type
		uint16 lastFileNameChar;	///< last character of original file name (UTF-16)

		Data ()
		: encryptionType (0),
		  lastFileNameChar (0)
		{
			signature.fcc = 0;
		}

		bool isValid () const
		{
			return signature == kSignature;
		}

		bool read (Streamer& s)
		{
			return	s.read (signature) &&
					s.read (encryptionType) &&
					s.read (lastFileNameChar);
		}

		bool write (Streamer& s) const
		{
			return	s.write (signature) &&
					s.write (encryptionType) &&
					s.write (lastFileNameChar);
		}
	};

	static const uint16 kEncryptionTypeAES = 0x0001;	///< AES encryption type
	static const uchar kReplacementChar = '_';			///< file name replacement character

	bool isEncryptionSupported (uint16 encryptionType)
	{
		return encryptionType == kEncryptionTypeAES;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

enum ZipDOSFileAttributes
{
	kDOSAttributeReadOnly = 0x01,
	kDOSAttributeHidden = 0x02,
	kDOSAttributeSystem = 0x04,
	kDOSAttributeDirectory = 0x10
};

inline bool hasDOSFileAttributes (uint16 versionMadeBy)
{
	// upper byte of 'version made by' indicates file attribute compatibility (0: MS-DOS)
	return (versionMadeBy & 0xFF00) == 0;
}

static const uint16 kZipVersion = 20;
static const uint16 kZip64Version = 45;
static const uint32 k4GBLimit = 0xffffffff;

inline uint32 zip64_limit (int64 value)
{
	return value >= k4GBLimit ? k4GBLimit : (uint32)value;
}

//************************************************************************************************
// ZipFile
//************************************************************************************************

DEFINE_CLASS (ZipFile, FileArchive)
DEFINE_CLASS_NAMESPACE (ZipFile, NAMESPACE_CCL)
DEFINE_CLASS_UID (ZipFile, 0x706b59b6, 0xec0, 0x4243, 0x90, 0x7d, 0x9, 0x45, 0xbc, 0x5a, 0x7b, 0x69)

//////////////////////////////////////////////////////////////////////////////////////////////////

ZipFile::ZipFile (UrlRef path)
: FileArchive (path),
  isZip64 (false)
{
	setCrc32Enabled (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ZipFile::~ZipFile ()
{
	destruct ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ZipFile::setEncrypted (bool state)
{
	// use class member from file tree to work with isEncrypted()
	encryptionType = state ? myClass ().getClassID () : kNullUID;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ZipFile::setOption (StringID id, VariantRef value)
{
	if(id == PackageOption::kAESEncrypted)
	{
		setEncrypted (value.asBool ());
		return kResultOk;
	}
	return SuperClass::setOption (id, value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ZipFile::getOption (Variant& value, StringID id) const
{
	if(id == PackageOption::kAESEncrypted)
	{
		value = isEncrypted ();
		return kResultOk;
	}
	return SuperClass::getOption (value, id);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SectionStream* ZipFile::openSectionStream (FileStreamItem& item)
{
	AutoPtr<SectionStream> s = SuperClass::openSectionStream (item);
	if(s)
	{
		if(item.isStartsWithHeader ())
		{
			IStream* file = s->getSourceStream ();
			ASSERT (file != nullptr)

			int64 localHeaderOffset = item.getFileDataOffset ();
			file->seek (localHeaderOffset, IStream::kSeekSet);
			Streamer stream (*file, Zip::kZipByteOrder);
			Zip::LocalFileHeader localHeader = {0};
			if(!localHeader.read (stream))
				return nullptr;
			if(localHeader.signature != Zip::kLocalFileHeaderSignature)
				return nullptr;

			// this is the position where the actual data starts
			int64 fileDataOffset = stream->seek (localHeader.getAdditionalSize (), IStream::kSeekCur);
			if(fileDataOffset <= 0)
				return nullptr;

			s->setSourceOffset (fileDataOffset);
		}
	}
	return s.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ZipFile::readFormat (IStream& _stream)
{
	Streamer stream (_stream, Zip::kZipByteOrder);
	CoreStream coreStream (_stream);
	ASSERT (stream->isSeekable ())
	if(!stream->isSeekable ())
		return false;

	Zip::CentralDirEndRecord endRecord = {0};
	Zip::CentralDirEndRecord64 endRecord64 = {0};
	Zip::CentralDirEndRecordLocator64 endRecordLocator64 = {0};
	bool anyFileCompressed = false, anyFileEncrypted = false;

	// check for Zip64
	isZip64 = Zip::findZip64Locator (endRecordLocator64, coreStream);
	if(isZip64)
	{
		stream->seek (endRecordLocator64.dirEndRecordOffset, IStream::kSeekSet);
		if(!endRecord64.read (stream))
		{
			CCL_PRINTLN ("[Zip] Zip64 Central Directory not found!")
			return false;
		}
	}
	// search for central directory
	else if(!Zip::findCentralDirectoryEnd (endRecord, coreStream))
	{
		CCL_PRINTLN ("[Zip] Central Directory not found!")

		// try to recover without central directory
		// (Zip 64, protection, file attributes, etc. aren't supported during recovery)
		#if 1
		stream->seek (0, IStream::kSeekSet);
		int numEntries = 0;
		Zip::LocalFileHeader localHeader = {0};
		while(localHeader.read (stream))
		{
			if(localHeader.signature != Zip::kLocalFileHeaderSignature)
				break;

			// read file name
			char* fileNameBuffer = NEW char[localHeader.fileNameLength + 1];
			VectorDeleter<char> fileNameDeleter (fileNameBuffer);
			if(stream.read (fileNameBuffer, localHeader.fileNameLength) != localHeader.fileNameLength)
				break;
			fileNameBuffer[localHeader.fileNameLength] = 0;

			// this is the position where the actual data starts
			int64 fileDataOffset = stream->tell () + localHeader.extraFieldLength;

			// ignore entry if we can not decompress it
			if(Zip::isCompressionSupported (localHeader.compressionMethod))
			{
				if(localHeader.compressionMethod != Zip::kCompressionMethodNone)
					anyFileCompressed = true;

				String fileName;
				TextEncoding fileNameEncoding = localHeader.isUTF8Encoded () ? Text::kUTF8 : Text::kDOSLatinUS;
				fileName.appendCString (fileNameEncoding, fileNameBuffer);

				// add to file tree
				FileSystemItem* item = createFileSystemItem (fileName);
				ASSERT (item != nullptr)
				if(FileStreamItem* streamItem = ccl_cast<FileStreamItem> (item))
				{
					DateTime modifiedTime;
					Zip::getDateTime (modifiedTime, localHeader.lastModifiedDate, localHeader.lastModifiedTime);

					streamItem->setTime (modifiedTime);
					streamItem->setFileDataOffset (fileDataOffset);
					streamItem->setFileDataSize (localHeader.compressedSize);
					streamItem->setFileSizeOnDisk (localHeader.uncompressedSize);
					streamItem->isCompressed (localHeader.compressionMethod == Zip::kCompressionMethodDeflated);

					numEntries++;
				}
			}

			// seek to position where next local header is expected
			stream->seek (fileDataOffset + localHeader.compressedSize, IStream::kSeekSet);
		}

		setOption (PackageOption::kCompressed, anyFileCompressed); // keep state for getOption()!
		return numEntries > 0;
		#else
		return false;
		#endif
	}

	int64 dirPosition = isZip64 ? endRecord64.dirOffset : endRecord.dirOffset;
	uint32 dirSize = isZip64 ? (uint32)endRecord64.dirSize : endRecord.dirSize;
	int numEntries = isZip64 ? (int)endRecord64.numEntriesThisDisk : endRecord.numEntriesThisDisk;

	// load central directory into memory stream
	MemoryStream dirMemory;
	if(!dirMemory.allocateMemory (dirSize))
		return false;
	if(stream->seek (dirPosition, IStream::kSeekSet) != dirPosition)
		return false;
	if(stream->read (dirMemory.getMemoryAddress (), dirSize) != dirSize)
		return false;
	dirMemory.setBytesWritten (dirSize);

	// parse central directory entries
	Streamer dirStream (dirMemory, Zip::kZipByteOrder);
	CCL_PRINTF ("[Zip] %d entries follow...\n", numEntries)
	for(int i = 0; i < numEntries; i++)
	{
		// read header
		Zip::CentralDirFileHeader header = {0};
		if(!header.read (dirStream))
			break;
		if(header.signature != Zip::kCentralDirFileHeaderSignature)
			break;

		int offset = header.getAdditionalSize ();

		// read file name
		char* fileNameBuffer = NEW char[header.fileNameLength + 1];
		VectorDeleter<char> fileNameDeleter (fileNameBuffer);
		if(dirStream.read (fileNameBuffer, header.fileNameLength) != header.fileNameLength)
			break;

		fileNameBuffer[header.fileNameLength] = 0;
		offset -= header.fileNameLength;

		int64 localHeaderOffset = header.localHeaderOffset;
		int64 compressedSize = header.compressedSize;
		int64 uncompressedSize = header.uncompressedSize;
		ZipProtection::Data protectionHeader;

		// parse extra fields
		if(header.extraFieldLength > 0)
		{
			uint16 consumed = 0;
			while(consumed < header.extraFieldLength)
			{
				Zip::ExtraField extraField = {0};
				extraField.read (dirStream);
				int64 oldPos = dirStream->tell ();

				// Zip 64
				if(extraField.headerID == Zip::kZip64ExtraID)
				{
					Zip::ExtraFieldZip64 info64 = {0};
					info64.read (dirStream, extraField.size);

					// overwrite with values from Zip64 Extended Information
					localHeaderOffset = info64.localHeaderOffset;
					compressedSize = info64.compressedSize;
					uncompressedSize = info64.uncompressedSize;
				}
				// Zip Protection
				else if(extraField.headerID == ZipProtection::kDataID)
				{
					protectionHeader.read (dirStream);
				}

				// skip unread bytes
				int bytesRead = (int)(dirStream->tell () - oldPos);
				int bytesToSkip = extraField.size - bytesRead;
				if(bytesToSkip != 0)
					dirStream->seek (bytesToSkip, IStream::kSeekCur);

				consumed += Zip::kExtraFieldPrologSize + extraField.size;
			}

			offset -= header.extraFieldLength;
		}

		CCL_PRINTF ("[Zip] Entry %02d: \"%s\" offset = %d compressed size = %d\n", i, fileNameBuffer, localHeaderOffset, compressedSize)

		// position directory stream for next entry
		ASSERT (offset >= 0)
		if(offset != 0)
			dirStream->seek (offset, IStream::kSeekCur);

		// ignore entry if we can not decompress it
		if(!Zip::isCompressionSupported (header.compressionMethod))
		{
			CCL_DEBUGGER ("[Zip] Compression method not supported!")
			continue;
		}

		// ignore entry if we can not decrypt it
		if(protectionHeader.isValid () && !ZipProtection::isEncryptionSupported (protectionHeader.encryptionType))
		{
			CCL_DEBUGGER ("[Zip] Protection method not supported!")
			continue;
		}

		if(header.compressionMethod != Zip::kCompressionMethodNone)
			anyFileCompressed = true;
		if(protectionHeader.isValid ())
			anyFileEncrypted = true;

		// position where data starts assuming there are no extra fields in the local file header
		int64 estimatedFileDataOffset = localHeaderOffset + Zip::kLocalFileHeaderSize + header.fileNameLength;
		bool startsWithHeader = false;

		// jump to local file header
		// ATTENTION: This has a negative performance impact, because we're seeking back and forth in the whole file!
		#if 0
		if(stream->seek (localHeaderOffset, IStream::kSeekSet) != localHeaderOffset)
			break;

		Zip::LocalFileHeader localHeader = {0};
		if(!localHeader.read (stream))
			break;
		if(localHeader.signature != Zip::kLocalFileHeaderSignature)
			break;

		ASSERT (header.isUTF8Encoded () == localHeader.isUTF8Encoded ())

		// this is the position where the actual data starts
		int64 fileDataOffset = stream->seek (localHeader.getAdditionalSize (), IStream::kSeekCur);
		if(fileDataOffset <= 0)
			break;

		ASSERT (fileDataOffset == estimatedFileDataOffset)

		#elif 0
		// The unreliable way: use estimated offset, fails with extra headers!
		int64 fileDataOffset = estimatedFileDataOffset;

		#else
		// The reliable way: take the extra field length in the local header
		// into account when the sub-stream is opened for the first time, see openSectionStream()
		int64 fileDataOffset = localHeaderOffset;
		startsWithHeader = true;
		#endif

		String fileName;
		TextEncoding fileNameEncoding = header.isUTF8Encoded () ? Text::kUTF8 : Text::kDOSLatinUS;
		fileName.appendCString (fileNameEncoding, fileNameBuffer);
		fileName.replace ("\t", " "); // replace tabs with spaces (imitate xml parser behavior)

		// restore original file name for protected file
		if(protectionHeader.isValid ())
			if(fileName.lastChar () == ZipProtection::kReplacementChar && protectionHeader.lastFileNameChar != 0)
			{
				fileName.truncate (fileName.length ()-1);
				uchar temp[2] = {protectionHeader.lastFileNameChar, '\0'};
				fileName.append (temp);
			}

		// file attributes
		bool hidden = hasDOSFileAttributes (header.versionMadeBy) && (header.externalAttributes & kDOSAttributeHidden) != 0;

		// add to file tree
		FileSystemItem* item = createFileSystemItem (fileName);
		ASSERT (item != nullptr)
		if(FileStreamItem* streamItem = ccl_cast<FileStreamItem> (item))
		{
			DateTime modifiedTime;
			Zip::getDateTime (modifiedTime, header.lastModifiedDate, header.lastModifiedTime);

			streamItem->setTime (modifiedTime);
			streamItem->setStartsWithHeader (startsWithHeader);
			streamItem->setFileDataOffset (fileDataOffset);
			streamItem->setFileDataSize (compressedSize);
			streamItem->setFileSizeOnDisk (uncompressedSize);
			streamItem->setCrc32 (header.crc32);
			streamItem->isCompressed (header.compressionMethod == Zip::kCompressionMethodDeflated);
			streamItem->isEncrypted (protectionHeader.isValid ());
			streamItem->useExternalKey (streamItem->isEncrypted ());
			streamItem->isHidden (hidden);
		}
		else if(FolderItem* folderItem = ccl_cast<FolderItem> (item))
		{
			folderItem->isHidden (hidden);
		}
	}

	setOption (PackageOption::kCompressed, anyFileCompressed); // keep state for getOption()!
	setOption (PackageOption::kAESEncrypted, anyFileEncrypted);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FileSystemItem* ZipFile::createFileSystemItem (StringRef fileName)
{
	Url path;
	if(fileName.endsWith (CCLSTR ("/")))
	{
		String folderName (fileName);
		folderName.truncate (folderName.length ()-1);
		path.setPath (folderName, Url::kFolder);
	}
	else
		path.setPath (fileName, Url::kFile);

	return lookupItem (path, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ZipFile::writeFormat (IStream& stream, IProgressNotify* progress)
{
	// TODO: PackageOption to enforce Zip64!
	isZip64 = false;

	// File Data
	if(!flushAll (stream, progress))
		return false;

	// Central Directory
	MemoryStream dirMemory;
	int numEntries = writeDirEntries (dirMemory, getRoot ());

	int64 dirOffset = stream.tell ();
	uint32 dirSize = dirMemory.getBytesWritten ();
	if(stream.write (dirMemory.getMemoryAddress (), dirSize) != dirSize)
		return false;

	Streamer streamer (stream, Zip::kZipByteOrder);
	if(isZip64)
	{
		// Zip64 End of central directory record
		Zip::CentralDirEndRecord64 endRecord64 = {0};
		endRecord64.signature = Zip::kCentralDirEnd64Signature;
		endRecord64.endRecordSize = Zip::kCentralDirEnd64Size - 12; // (size should not include the leading 12 bytes)
		endRecord64.versionMadeBy = kZip64Version;
		endRecord64.versionNeeded = kZip64Version;
		endRecord64.numEntriesThisDisk = numEntries;
		endRecord64.numEntriesAllDisks = numEntries;
		endRecord64.dirSize = dirSize;
		endRecord64.dirOffset = dirOffset;

		int64 dirEndRecordOffset = stream.tell ();
		if(!endRecord64.write (streamer))
			return false;

		// Zip64 end of central directory locator
		Zip::CentralDirEndRecordLocator64 endRecordLocator64 = {0};
		endRecordLocator64.signature = Zip::CentralDirEndRecordLocator64Signature;
		endRecordLocator64.totalDiskCount = 1;
		endRecordLocator64.dirEndRecordOffset = dirEndRecordOffset;

		if(!endRecordLocator64.write (streamer))
			return false;
	}

	// Central Directory End Record
	Zip::CentralDirEndRecord endRecord = {0};
	endRecord.signature = Zip::kCentralDirEndSignature;
	endRecord.numEntriesThisDisk = (uint16)numEntries;
	endRecord.numEntriesAllDisks = (uint16)numEntries;
	endRecord.dirSize = dirSize;
	endRecord.dirOffset = zip64_limit (dirOffset);

	if(!endRecord.write (streamer))
		return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ZipFile::writeDirEntries (IStream& stream, FolderItem& baseFolder)
{
	int count = 0;
	IterForEach (baseFolder.newIterator (), FileSystemItem, _item)
		if(FileStreamItem* fileItem = ccl_cast<FileStreamItem> (_item))
		{
			count++;

			Zip::CentralDirFileHeader fileHeader = {0};
			fileHeader.signature = Zip::kCentralDirFileHeaderSignature;
			fileHeader.versionMadeBy = isZip64 ? kZip64Version : kZipVersion;
			fileHeader.versionNeeded = isZip64 ? kZip64Version : kZipVersion;
			fileHeader.compressionMethod = fileItem->isCompressed () ? Zip::kCompressionMethodDeflated : Zip::kCompressionMethodNone;
			Zip::setDate (fileHeader.lastModifiedDate, fileItem->getTime ().getDate ());
			Zip::setTime (fileHeader.lastModifiedTime, fileItem->getTime ().getTime ());
			fileHeader.crc32 = fileItem->getCrc32 ();
			fileHeader.compressedSize = zip64_limit (fileItem->getFileDataSize ());
			fileHeader.uncompressedSize = zip64_limit (fileItem->getFileSizeOnDisk ());
			fileHeader.externalAttributes = fileItem->isHidden () ? kDOSAttributeHidden : 0;

			fileHeader.extraFieldLength = isZip64 ? Zip::kExtraFieldPrologSize + Zip::kExtraFieldZip64DataSize : 0;
			if(fileItem->isEncrypted ())
				fileHeader.extraFieldLength += Zip::kExtraFieldPrologSize + ZipProtection::kDataSize;

			StringID fileName = fileItem->getEncodedFileName ();
			ASSERT (!fileName.isEmpty ())
			fileHeader.fileNameLength = (uint16)fileName.length ();
			fileHeader.isUTF8Encoded (fileItem->getFileNameEncoding () == Text::kUTF8);

			int64 localHeaderOffset = fileItem->getFileDataOffset () - fileItem->getFileHeaderSize ();
			fileHeader.localHeaderOffset = zip64_limit (localHeaderOffset);

			Streamer streamer (stream, Zip::kZipByteOrder);
			fileHeader.write (streamer);

			stream.write (fileName.str (), fileHeader.fileNameLength);

			// write Zip64 Extended Information
			if(isZip64)
			{
				Zip::ExtraField extraField = {Zip::kZip64ExtraID, Zip::kExtraFieldZip64DataSize};
				extraField.write (streamer);

				Zip::ExtraFieldZip64 info64 = {0};
				info64.uncompressedSize = fileItem->getFileSizeOnDisk ();
				info64.compressedSize = fileItem->getFileDataSize ();
				info64.localHeaderOffset = localHeaderOffset;
				info64.write (streamer);
			}

			// write Zip Protection Information
			if(fileItem->isEncrypted ())
			{
				Zip::ExtraField extraField = {ZipProtection::kDataID, ZipProtection::kDataSize};
				extraField.write (streamer);

				ZipProtection::Data protectionHeader;
				protectionHeader.signature = ZipProtection::kSignature;
				protectionHeader.encryptionType = ZipProtection::kEncryptionTypeAES;
				protectionHeader.lastFileNameChar = fileItem->getFileName ().lastChar ();
				protectionHeader.write (streamer);
			}
		}
		else if(FolderItem* folderItem = ccl_cast<FolderItem> (_item))
		{
			if(isFolderHeaderNeeded (*folderItem))
			{
				count++;

				Zip::CentralDirFileHeader fileHeader = {0};
				fileHeader.signature = Zip::kCentralDirFileHeaderSignature;
				fileHeader.versionMadeBy = isZip64 ? kZip64Version : kZipVersion;
				fileHeader.versionNeeded = isZip64 ? kZip64Version : kZipVersion;
				Zip::setDate (fileHeader.lastModifiedDate, folderItem->getTime ().getDate ());
				Zip::setTime (fileHeader.lastModifiedTime, folderItem->getTime ().getTime ());
				fileHeader.externalAttributes = kDOSAttributeDirectory;
				if(folderItem->isHidden ())
					fileHeader.externalAttributes |= kDOSAttributeHidden;

				StringID fileName = folderItem->getEncodedFileName ();
				ASSERT (!fileName.isEmpty ())
				fileHeader.fileNameLength = (uint16)fileName.length ();
				fileHeader.isUTF8Encoded (folderItem->getFileNameEncoding () == Text::kUTF8);

				fileHeader.localHeaderOffset = zip64_limit (folderItem->getFolderHeaderOffset ());
				// LATER TODO: Zip 64 extra field for local header offset (ignored when reading)?

				Streamer streamer (stream, Zip::kZipByteOrder);
				fileHeader.write (streamer);
				stream.write (fileName.str (), fileHeader.fileNameLength);
			}

			count += writeDirEntries (stream, *folderItem);
		}
	EndFor
	return count;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ZipFile::encodeFileName (FileSystemItem& item, StringRef unicodePath) const
{
	TextEncoding fileNameEncoding = Text::kDOSLatinUS;
	MutableCString fileName (unicodePath, fileNameEncoding);

	// check if UTF-8 is required for file name
	// TODO: PackageOption to enforce UTF-8!
	String testPath;
	testPath.appendCString (fileNameEncoding, fileName);
	if(testPath != unicodePath)
	{
		fileNameEncoding = Text::kUTF8;
		fileName = MutableCString (unicodePath, fileNameEncoding);
	}

	item.setEncodedFileName (fileName);
	item.setFileNameEncoding (fileNameEncoding);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ZipFile::isFolderHeaderNeeded (const FolderItem& item) const
{
	// TODO: PackageOption to include all folders?
	return item.countChildren () == 0 || item.isHidden ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 ZipFile::beginFile (IStream& dstStream, FileStreamItem& item)
{
	String unicodePath;
	getItemPath (unicodePath, item);

	// modify file name for protected file
	if(item.isEncrypted ())
	{
		unicodePath.truncate (unicodePath.length ()-1);
		uchar temp[2] = {ZipProtection::kReplacementChar, '\0'};
		unicodePath << temp;
	}

	encodeFileName (item, unicodePath);

	// write placeholder for local header
	Zip::LocalFileHeader placeHolder = {0};
	Streamer streamer (dstStream, Zip::kZipByteOrder);
	if(!placeHolder.write (streamer))
		return -1;

	StringID fileName = item.getEncodedFileName ();
	ASSERT (!fileName.isEmpty ())

	int fileNameLength = fileName.length ();
	if(dstStream.write (fileName.str (), fileNameLength) != fileNameLength)
		return -1;

	return Zip::kLocalFileHeaderSize + fileNameLength;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ZipFile::endFile (IStream& dstStream, FileStreamItem& item)
{
	ASSERT (dstStream.isSeekable ())
	int64 fileDataEnd = dstStream.tell ();

	int64 headerStart = item.getFileDataOffset () - item.getFileHeaderSize ();
	if(dstStream.seek (headerStart, IStream::kSeekSet) != headerStart)
		return false;

	// check if Zip64 needed
	if(isZip64 == false)
		if(headerStart >= k4GBLimit || fileDataEnd >= k4GBLimit)
			isZip64 = true;

	StringID fileName = item.getEncodedFileName ();
	ASSERT (!fileName.isEmpty ())

	Zip::LocalFileHeader localHeader = {0};
	localHeader.signature = Zip::kLocalFileHeaderSignature;
	localHeader.compressionMethod = item.isCompressed () ? Zip::kCompressionMethodDeflated : Zip::kCompressionMethodNone;
	Zip::setTime (localHeader.lastModifiedTime, item.getTime ().getTime ());
	Zip::setDate (localHeader.lastModifiedDate, item.getTime ().getDate ());
	localHeader.crc32 = item.getCrc32 ();
	localHeader.compressedSize = zip64_limit (item.getFileDataSize ());
	localHeader.uncompressedSize = zip64_limit (item.getFileSizeOnDisk ());
	localHeader.versionNeeded = isZip64 ? kZip64Version : kZipVersion;
	localHeader.fileNameLength = (uint16)fileName.length ();
	localHeader.isUTF8Encoded (item.getFileNameEncoding () == Text::kUTF8);

	Streamer streamer (dstStream, Zip::kZipByteOrder);
	if(!localHeader.write (streamer))
		return false;

	return dstStream.seek (fileDataEnd, IStream::kSeekSet) == fileDataEnd;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ZipFile::beginFolder (IStream& dstStream, FolderItem& item)
{
	if(isFolderHeaderNeeded (item) == false)
		return true;

	String unicodePath;
	getItemPath (unicodePath, item);
	unicodePath << CCLSTR ("/");

	item.updateTime ();
	encodeFileName (item, unicodePath);
	item.setFolderHeaderOffset (dstStream.tell ());

	StringID fileName = item.getEncodedFileName ();
	ASSERT (!fileName.isEmpty ())
	int fileNameLength = fileName.length ();

	Zip::LocalFileHeader localHeader = {0};
	localHeader.signature = Zip::kLocalFileHeaderSignature;
	Zip::setTime (localHeader.lastModifiedTime, item.getTime ().getTime ());
	Zip::setDate (localHeader.lastModifiedDate, item.getTime ().getDate ());
	localHeader.versionNeeded = isZip64 ? kZip64Version : kZipVersion;
	localHeader.fileNameLength = (uint16)fileNameLength;
	localHeader.isUTF8Encoded (item.getFileNameEncoding () == Text::kUTF8);

	Streamer streamer (dstStream, Zip::kZipByteOrder);
	if(!localHeader.write (streamer))
		return false;
	if(dstStream.write (fileName.str (), fileNameLength) != fileNameLength)
		return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStream* ZipFile::createEncryptionStream (IStream* srcStream, const FileStreamItem& item) const
{
	int64 nonce = item.getFileName ().getHashCode ();

	return NEW AESEncryptionStream (srcStream, externalEncryptionKey, nonce);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStream* ZipFile::createReadTransform (IStream& srcStream, FileStreamItem& item, IUnknown* context) const
{
	if(item.isCompressed ())
	{
		ASSERT (!item.isEncrypted ())

		AutoPtr<IDataTransformer> decompressor = System::CreateDataTransformer (ClassID::ZlibCompression, IDataTransformer::kDecode);
		ASSERT (decompressor != nullptr)

		// configure window bits for inflate
		UnknownPtr<IZLibTransformer> zlibTransform (decompressor);
		ASSERT (zlibTransform != nullptr)
		zlibTransform->setWindowBits (-zlibTransform->getMaxWindowBits ());

		IStream* outStream = System::CreateTransformStream (&srcStream, decompressor, false);
		ASSERT (outStream != nullptr)
		return outStream;
	}
	else
	{
		ASSERT (!item.isCompressed () && item.isEncrypted () && isEncrypted ())

		return createEncryptionStream (&srcStream, item);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStream* ZipFile::createWriteTransform (IStream& dstStream, FileStreamItem& item, IUnknown* context) const
{
	if(item.isCompressed ())
	{
		ASSERT (!item.isEncrypted ())

		AutoPtr<IDataTransformer> compressor = System::CreateDataTransformer (ClassID::ZlibCompression, IDataTransformer::kEncode);
		ASSERT (compressor != nullptr)

		// configure compression level
		UnknownPtr<IDataCompressor> dataCompressor (compressor);
		ASSERT (dataCompressor != nullptr)
		dataCompressor->setCompressionLevel (compressionLevel);

		// configure window bits for deflate
		UnknownPtr<IZLibTransformer> zlibTransform (compressor);
		ASSERT (zlibTransform != nullptr)
		zlibTransform->setWindowBits (-zlibTransform->getMaxWindowBits ());

		IStream* outStream = System::CreateTransformStream (&dstStream, compressor, true);
		ASSERT (outStream != nullptr)
		return outStream;
	}
	else
	{
		ASSERT (!item.isCompressed () && item.isEncrypted () && isEncrypted ())

		return createEncryptionStream (&dstStream, item);
	}
}
