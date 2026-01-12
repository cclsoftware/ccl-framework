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
// Filename    : core/system/corezipfileformat.impl.h
// Description : Zip File Format
//
//************************************************************************************************

#include "corezipfileformat.h"

#include "core/public/coreprimitives.h"

namespace Core {
namespace Zip {

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class Record, const uint32 kSignature, const uint32 kChunkSize>
bool findRecord (Record& result, IO::Stream& stream)
{
	char buffer[kChunkSize + 10] = {0};

	int64 position = stream.setPosition (0, IO::kSeekEnd);
	int64 minPosition = get_max<int64> (0, position - 1024);

	while(position > minPosition)
	{
		position = stream.setPosition (position - kChunkSize, IO::kSeekSet);
		::memcpy (buffer + kChunkSize, buffer, 4);
		if(stream.readBytes (buffer, kChunkSize) != kChunkSize)
			return false;
		uint32 signature = 0;
		for(uint32 i = 0; i < kChunkSize; i++)
		{
			// we know that the signature is the first member of the Record
			::memcpy (&signature, buffer + i, sizeof(uint32));
			if(MAKE_LITTLE_ENDIAN (signature) == kSignature)
			{
				stream.setPosition (position + i, IO::kSeekSet);
				IO::BinaryStreamAccessor accessor (stream, kZipByteOrder);
				return result.read (accessor);
			}
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool findCentralDirectoryEnd (CentralDirEndRecord& result, IO::Stream& stream)
{
	return findRecord<CentralDirEndRecord, kCentralDirEndSignature, kCentralDirEndRecordSize> (result, stream);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool findZip64Locator (CentralDirEndRecordLocator64& result, IO::Stream& stream)
{
	return findRecord<CentralDirEndRecordLocator64, CentralDirEndRecordLocator64Signature, CentralDirEndRecordLocator64Size> (result, stream);
}

} // namespace Zip
} // namespace Core

using namespace Core;

//************************************************************************************************
// Zip::CentralDirEndRecord
//************************************************************************************************

bool Zip::CentralDirEndRecord::read (IO::BinaryAccessor& s)
{
	if(!s.read (signature))
		return false;
	s.read (thisDiskNumber);
	s.read (startDiskNumber);
	s.read (numEntriesThisDisk);
	s.read (numEntriesAllDisks);
	s.read (dirSize);
	s.read (dirOffset);
	return s.read (commentLength);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Zip::CentralDirEndRecord::write (IO::BinaryAccessor& s) const
{
	if(!s.write (signature))
		return false;
	s.write (thisDiskNumber);
	s.write (startDiskNumber);
	s.write (numEntriesThisDisk);
	s.write (numEntriesAllDisks);
	s.write (dirSize);
	s.write (dirOffset);
	return s.write (commentLength);
}

//************************************************************************************************
// Zip::CentralDirEndRecord64
//************************************************************************************************

bool Zip::CentralDirEndRecord64::read (IO::BinaryAccessor& s)
{
	if(!s.read (signature))
		return false;
	s.read (endRecordSize);
	s.read (versionMadeBy);
	s.read (versionNeeded);
	s.read (thisDiskNumber);
	s.read (startDiskNumber);
	s.read (numEntriesThisDisk);
	s.read (numEntriesAllDisks);
	s.read (dirSize);
	return s.read (dirOffset);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Zip::CentralDirEndRecord64::write (IO::BinaryAccessor& s) const
{
	if(!s.write (signature))
		return false;
	s.write (endRecordSize);
	s.write (versionMadeBy);
	s.write (versionNeeded);
	s.write (thisDiskNumber);
	s.write (startDiskNumber);
	s.write (numEntriesThisDisk);
	s.write (numEntriesAllDisks);
	s.write (dirSize);
	return s.write (dirOffset);
}

//************************************************************************************************
// Zip::CentralDirEndRecordLocator64
//************************************************************************************************

bool Zip::CentralDirEndRecordLocator64::read (IO::BinaryAccessor& s)
{
	if(!s.read (signature))
		return false;
	s.read (startDiskNumber);
	s.read (dirEndRecordOffset);
	return s.read (totalDiskCount);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Zip::CentralDirEndRecordLocator64::write (IO::BinaryAccessor& s) const
{
	if(!s.write (signature))
		return false;
	s.write (startDiskNumber);
	s.write (dirEndRecordOffset);
	return s.write (totalDiskCount);
}

//************************************************************************************************
// Zip::CentralDirFileHeader
//************************************************************************************************

bool Zip::CentralDirFileHeader::read (IO::BinaryAccessor& s)
{
	if(!s.read (signature))
		return false;
	s.read (versionMadeBy);
	s.read (versionNeeded);
	s.read (gpBitFlag);
	s.read (compressionMethod);
	s.read (lastModifiedTime);
	s.read (lastModifiedDate);
	s.read (crc32);
	s.read (compressedSize);
	s.read (uncompressedSize);
	s.read (fileNameLength);
	s.read (extraFieldLength);
	s.read (fileCommentLength);
	s.read (startDiskNumber);
	s.read (internalAttributes);
	s.read (externalAttributes);
	return s.read (localHeaderOffset);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Zip::CentralDirFileHeader::write (IO::BinaryAccessor& s) const
{
	if(!s.write (signature))
		return false;
	s.write (versionMadeBy);
	s.write (versionNeeded);
	s.write (gpBitFlag);
	s.write (compressionMethod);
	s.write (lastModifiedTime);
	s.write (lastModifiedDate);
	s.write (crc32);
	s.write (compressedSize);
	s.write (uncompressedSize);
	s.write (fileNameLength);
	s.write (extraFieldLength);
	s.write (fileCommentLength);
	s.write (startDiskNumber);
	s.write (internalAttributes);
	s.write (externalAttributes);
	return s.write (localHeaderOffset);
}

//************************************************************************************************
// Zip::ExtraField
//************************************************************************************************

bool Zip::ExtraField::read (IO::BinaryAccessor& s)
{
	return s.read (headerID) && s.read (size);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Zip::ExtraField::write (IO::BinaryAccessor& s) const
{
	return s.write (headerID) && s.write (size);
}

//************************************************************************************************
// Zip::ExtraFieldZip64
//************************************************************************************************

bool Zip::ExtraFieldZip64::read (IO::BinaryAccessor& s, int size)
{
	if(!s.read (uncompressedSize))
		return false;
	if(!s.read (compressedSize))
		return false;
	if(!s.read (localHeaderOffset))
		return false;
	size -= 24; // 3 * 8 bytes

	// start disk number seems to be missing in some Zip64 files
	ASSERT (size >= 0)
	return size <= 0 ? true : s.read (startDiskNumber);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Zip::ExtraFieldZip64::write (IO::BinaryAccessor& s) const
{
	return s.write (uncompressedSize) && s.write (compressedSize) && s.write (localHeaderOffset) && s.write (startDiskNumber);
}

//************************************************************************************************
// Zip::LocalFileHeader
//************************************************************************************************

bool Zip::LocalFileHeader::read (IO::BinaryAccessor& s)
{
	if(!s.read (signature))
		return false;
	s.read (versionNeeded);
	s.read (gpBitFlag);
	s.read (compressionMethod);
	s.read (lastModifiedTime);
	s.read (lastModifiedDate);
	s.read (crc32);
	s.read (compressedSize);
	s.read (uncompressedSize);
	s.read (fileNameLength);
	return s.read (extraFieldLength);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Zip::LocalFileHeader::write (IO::BinaryAccessor& s) const
{
	if(!s.write (signature))
		return false;
	s.write (versionNeeded);
	s.write (gpBitFlag);
	s.write (compressionMethod);
	s.write (lastModifiedTime);
	s.write (lastModifiedDate);
	s.write (crc32);
	s.write (compressedSize);
	s.write (uncompressedSize);
	s.write (fileNameLength);
	return s.write (extraFieldLength);
}
