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
// Filename    : ccl/system/packaging/packagefile.cpp
// Description : Package File
//
//************************************************************************************************

#include "ccl/system/packaging/packagefile.h"
#include "ccl/system/packaging/packagefileformat.h"
#include "ccl/system/packaging/sectionstream.h"

#include "ccl/public/base/variant.h"
#include "ccl/public/base/idatatransformer.h"
#include "ccl/public/base/memorystream.h"

using namespace CCL;

//************************************************************************************************
// PackageFile
//************************************************************************************************

DEFINE_CLASS (PackageFile, FileArchive)
DEFINE_CLASS_NAMESPACE (PackageFile, NAMESPACE_CCL)
DEFINE_CLASS_UID (PackageFile, 0x6b4597cd, 0xd7f6, 0x422a, 0x88, 0xcc, 0x5c, 0xa1, 0xca, 0x92, 0x3a, 0x89)

//////////////////////////////////////////////////////////////////////////////////////////////////

PackageFile::PackageFile (UrlRef path)
: FileArchive (path),
  formatVersion (kPackageFormatV1),
  chunkFlags (0),
  reservedBlockSize (0)
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PackageFile::~PackageFile ()
{
	destruct ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int PackageFile::getEncryptionAlgorithm () const
{
	return chunkFlags & kEncryptionAlgoMask;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageFile::setEncryptionAlgorithm (int algo)
{
	ASSERT ((algo & ~kEncryptionAlgoMask) == 0)

	chunkFlags |= (algo & kEncryptionAlgoMask);
	isEncrypted (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageFile::setEncryptionOption (int algorithm, bool state)
{
	if(state)
	{
		setEncryptionAlgorithm (algorithm);

		// encryption type holds the random key
		uint8 randomData[16];
		for(int i = 0; i < 16; i++)
			randomData[i] = 1 + (rand () % 0xFE);
		::memcpy (static_cast<void*> (&encryptionType), randomData, 16);
	}
	else
	{
		encryptionType.assign (kNullUID);
		isEncrypted (false);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PackageFile::setOption (StringID id, VariantRef value)
{
	if(id == PackageOption::kBasicEncrypted)
	{
		setEncryptionOption (kEncryptionBasic, value.asBool ());
		return kResultOk;
	}
	else if(id == PackageOption::kXTEAEncrypted)
	{
		setEncryptionOption (kEncryptionXTEA, value.asBool ());
		return kResultOk;
	}
	else if(id == PackageOption::kAESEncrypted)
	{
		setEncryptionOption (kEncryptionAES, value.asBool ());
		return kResultOk;
	}
	else if(id == PackageOption::kFormatVersion)
	{
		ASSERT (isValidFormatVersion (value))
		if(!isValidFormatVersion (value))
			return kResultInvalidArgument;

		formatVersion = value;
		return kResultOk;
	}
	else if(id == PackageOption::kReservedBlockSize)
	{
		reservedBlockSize = value;
		return kResultOk;
	}
	return SuperClass::setOption (id, value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PackageFile::getOption (Variant& value, StringID id) const
{
	if(id == PackageOption::kBasicEncrypted)
	{
		value = isEncrypted () && getEncryptionAlgorithm () == kEncryptionBasic;
		return kResultOk;
	}
	else if(id == PackageOption::kXTEAEncrypted)
	{
		value = isEncrypted () && getEncryptionAlgorithm () == kEncryptionXTEA;
		return kResultOk;
	}
	else if(id == PackageOption::kAESEncrypted)
	{
		value = isEncrypted () && getEncryptionAlgorithm () == kEncryptionAES;
		return kResultOk;
	}
	else if(id == PackageOption::kFormatVersion)
	{
		value = formatVersion;
		return kResultOk;
	}
	else if(id == PackageOption::kReservedBlockSize)
	{
		value = reservedBlockSize;
		return kResultOk;
	}
	return SuperClass::getOption (value, id);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageFile::isFileTreeEncrypted () const
{
	return isEncrypted () && formatVersion >= kPackageFormatV2;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStream* PackageFile::createEncryptionStream (IStream* srcStream, const uint8 key[16], int64 nonce) const
{
	switch(getEncryptionAlgorithm ())
	{
	case kEncryptionXTEA :
		return NEW XTEAEncryptionStream  (srcStream, key, nonce);
	case kEncryptionAES :
		return NEW AESEncryptionStream (srcStream, key, nonce);
	default :
		return NEW BasicEncryptionStream (srcStream, key);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageFile::readFormat (IStream& stream)
{
	// Package Header
	Streamer streamer (stream, kLittleEndian);
	FOURCC fcc = {0};
	streamer.read (fcc);
	if(fcc != kPackageSignature1)
		return false;
	streamer.read (fcc);
	if(fcc != kPackageSignature2)
		return false;

	// Package Chunk (End of File)
	PackageChunk chunk = {0};
	stream.seek (-kPackageChunkSize, IStream::kSeekEnd);
	chunk.deserialize (streamer);

	if(!(chunk.signature1 == kPackageSignature1 &&
		 chunk.signature2 == kPackageSignature2 &&
		 isValidFormatVersion (chunk.version)))
		return false;

	// Compression/Encryption/etc.
	compressionType.assign (chunk.compressionType);
	encryptionType.assign (chunk.encryptionType);
	formatVersion = chunk.version;
	chunkFlags = chunk.flags;

	// Reserved Block
	AutoPtr<FileStreamItem> reservedItem;
	if(chunk.flags & kHasReservedBlock)
	{
		stream.seek (kReservedBlockOffset, IStream::kSeekSet);
		ReservedBlockHeader header;
		header.deserialize (streamer);
		reservedBlockSize = header.totalSize; // keep for getOption()
		if(header.signature == kReservedBlockSignature && !header.fileName.isEmpty ())
		{
			int64 dataOffset = stream.tell ();
			String fileName;
			fileName.appendCString (Text::kUTF8, header.fileName);
			reservedItem = NEW FileStreamItem (fileName);
			reservedItem->setFileDataOffset (dataOffset);
			reservedItem->setFileDataSize (header.usedSize);
			reservedItem->setFileSizeOnDisk (header.usedSize);
		}
	}

	// File Tree
	int64 seekResult = stream.seek (chunk.fileTreePosition, IStream::kSeekSet);
	ASSERT (seekResult == chunk.fileTreePosition)
	if(isFileTreeEncrypted ())
	{
		uint8 key[16];
		encryptionTypeToKey (key, encryptionType);

		AutoPtr<IStream> encryptionStream (createEncryptionStream (&stream, key, *(int64*)key));
		Streamer streamer2 (*encryptionStream, kLittleEndian);
		if(!getRoot ().deserialize (streamer2, formatVersion))
			return false;
	}
	else if(!getRoot ().deserialize (streamer, formatVersion))
		return false;

	// add reserved block to file tree
	if(reservedItem)
		getRoot ().addChild (reservedItem.detach ());

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageFile::writeFormat (IStream& stream, IProgressNotify* progress)
{
	bool result = true;
	Streamer streamer (stream, kLittleEndian);

	// Package Header
	streamer.write (kPackageSignature1);
	streamer.write (kPackageSignature2);

	// Reserved Block
	if(reservedBlockSize > 0)
	{
		MemoryStream block;
		block.allocateMemory (reservedBlockSize);

		ReservedBlockHeader header;
		header.signature = kReservedBlockSignature;
		header.totalSize = reservedBlockSize;
		Streamer s (block, kLittleEndian);
		header.serialize (s);

		// fill up with random data
		int headerSize = block.getBytesWritten ();
		uint8* ptr = (uint8*)block.getMemoryAddress () + headerSize;
		for(int i = headerSize; i < reservedBlockSize; i++)
			*ptr++ = (uint8)(rand () % 0xFF);

		stream.write (block.getMemoryAddress (), reservedBlockSize);
	}

	// File Data
	result = flushAll (stream, progress);

	// File Tree
	int64 fileTreePosition = stream.tell ();
	if(isFileTreeEncrypted ())
	{
		uint8 key[16];
		encryptionTypeToKey (key, encryptionType);

		AutoPtr<IStream> encryptionStream (createEncryptionStream (&stream, key, *(int64*)key));
		Streamer streamer2 (*encryptionStream, kLittleEndian);
		getRoot ().serialize (streamer2, formatVersion);
	}
	else
		getRoot ().serialize (streamer, formatVersion);

	int fileTreeSize = (int)(stream.tell () - fileTreePosition);

	// Package Chunk
	PackageChunk chunk = {0};
	chunk.signature1 = kPackageSignature1;
	chunk.signature2 = kPackageSignature2;
	chunk.chunkSize = kPackageChunkSize;
	chunk.version = formatVersion;
	chunk.flags = chunkFlags;
	if(reservedBlockSize > 0)
		chunk.flags |= kHasReservedBlock;
	chunk.fileTreePosition = fileTreePosition;
	chunk.fileTreeSize = fileTreeSize;
	chunk.encryptionType = encryptionType;
	chunk.compressionType = compressionType;
	chunk.serialize (streamer);

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStream* PackageFile::createReadTransform (IStream& srcStream, FileStreamItem& item, IUnknown* context) const
{
	if(item.isEncrypted () && !isEncrypted ())
	{
		CCL_NOT_IMPL ("Encryption method not implemented!")
		return nullptr;
	}

	IStream* transformStream = nullptr;

	if(item.isEncrypted ())
	{
		uint8 key[16];
		if(item.useExternalKey ())
			::memcpy (key, externalEncryptionKey, 16);
		else
			encryptionTypeToKey (key, encryptionType);

		transformStream = createEncryptionStream (&srcStream, key, item.getFileName ().getHashCode ());
	}

	if(item.isCompressed ())
	{
		AutoPtr<IDataTransformer> decompressor = System::CreateDataTransformer (compressionType, IDataTransformer::kDecode);
		ASSERT (decompressor != nullptr)
		if(decompressor)
		{
			IStream* inStream = transformStream ? transformStream : &srcStream;
			IStream* outStream = System::CreateTransformStream (inStream, decompressor, false);
			if(transformStream)
				transformStream->release ();
			transformStream = outStream;
		}
	}

	ASSERT (transformStream != nullptr)
	return transformStream;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStream* PackageFile::createWriteTransform (IStream& dstStream, FileStreamItem& item, IUnknown* context) const
{
	if(item.isEncrypted () && !isEncrypted ())
	{
		CCL_NOT_IMPL ("Encryption method not implemented!")
		return nullptr;
	}

	IStream* transformStream = nullptr;

	if(item.isCompressed ())
	{
		AutoPtr<IDataTransformer> compressor = System::CreateDataTransformer (compressionType, IDataTransformer::kEncode);
		ASSERT (compressor != nullptr)
		if(compressor)
		{
			// configure compression level
			UnknownPtr<IDataCompressor> dataCompressor (compressor);
			ASSERT (dataCompressor != nullptr)
			dataCompressor->setCompressionLevel (compressionLevel);

			transformStream = System::CreateTransformStream (&dstStream, compressor, true);
		}
	}

	if(item.isEncrypted ())
	{
		uint8 key[16];
		if(item.useExternalKey ())
			::memcpy (key, externalEncryptionKey, 16);
		else
			encryptionTypeToKey (key, encryptionType);

		IStream* inStream = transformStream ? transformStream : &dstStream;
		IStream* outStream = createEncryptionStream (inStream, key, item.getFileName ().getHashCode ());

		if(transformStream)
			transformStream->release ();
		transformStream = outStream;
	}

	ASSERT (transformStream != nullptr)
	return transformStream;
}
