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
// Filename    : core/system/corezipfileformat.h
// Description : Zip File Format
//
//************************************************************************************************

#ifndef _corezipfileformat_h
#define _corezipfileformat_h

#include "core/public/coremacros.h"
#include "core/public/coredatetime.h"
#include "core/public/corestreamaccessor.h"

namespace Core {

//************************************************************************************************
// Zip File Format
// see http://www.pkware.com/documents/casestudies/APPNOTE.TXT
//************************************************************************************************

/*
  Overall .ZIP file format:

    [local file header 1]
    [file data 1]
    [data descriptor 1]
    .
    .
    .
    [local file header n]
    [file data n]
    [data descriptor n]
    [archive decryption header]
    [archive extra data record]
    [central directory]
    [zip64 end of central directory record]
    [zip64 end of central directory locator]
    [end of central directory record]
*/

namespace Zip
{
	static const int kZipByteOrder = CORE_LITTLE_ENDIAN;

	//////////////////////////////////////////////////////////////////////////////////////////////

	/** End of central directory record */
	struct CentralDirEndRecord
	{
		uint32 signature;			///< end of central dir signature    4 bytes  (0x06054b50)
		uint16 thisDiskNumber;		///< number of this disk             2 bytes
		uint16 startDiskNumber;		///< number of the disk with the start of the central directory  2 bytes
		uint16 numEntriesThisDisk;	///< total number of entries in the central directory on this disk  2 bytes
		uint16 numEntriesAllDisks;	///< total number of entries in the central directory           2 bytes
		uint32 dirSize;				///< size of the central directory   4 bytes
		uint32 dirOffset;			///< offset of start of central directory with respect to the starting disk number 4 bytes
		uint16 commentLength;		///< .ZIP file comment length  2 bytes
		//char comment[];			///< .ZIP file comment (variable size)

		bool read (IO::BinaryAccessor& s);
		bool write (IO::BinaryAccessor& s) const;
	};

	static const uint32 kCentralDirEndRecordSize = 22;
	static const uint32 kCentralDirEndSignature = 0x06054b50;

	bool findCentralDirectoryEnd (CentralDirEndRecord& result, IO::Stream& stream);

	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Zip64 End of central directory record. */
	struct CentralDirEndRecord64
	{
		uint32 signature;			///< zip64 end of central dir signature 4 bytes  (0x06064b50)
		uint64 endRecordSize;		///< size of zip64 end of central directory record 8 bytes
									///< (should not include the leading 12 bytes: Size = SizeOfFixedFields + SizeOfVariableData - 12.)
		uint16 versionMadeBy;		///< version made by 2 bytes
		uint16 versionNeeded;		///< version needed to extract 2 bytes
		uint32 thisDiskNumber;		///< number of this disk 4 bytes
		uint32 startDiskNumber;		///< number of the disk with the start of the central directory 4 bytes
		uint64 numEntriesThisDisk;	///< total number of entries in the central directory on this disk 8 bytes
		uint64 numEntriesAllDisks;	///< total number of entries in the central directory 8 bytes
		uint64 dirSize;				///< size of the central directory 8 bytes
		uint64 dirOffset;			///< offset of start of central directory with respect to the starting disk number 8 bytes
		//char extensibleData[];	///< zip64 extensible data sector (variable size)

		bool read (IO::BinaryAccessor& s);
		bool write (IO::BinaryAccessor& s) const;
	};

	static const uint32 kCentralDirEnd64Size = 56;
	static const uint32 kCentralDirEnd64Signature = 0x06064b50;

	/** Zip64 end of central directory locator. */
	struct CentralDirEndRecordLocator64
	{
		uint32 signature;			///< zip64 end of central dir locator 4 bytes (0x07064b50)
		uint32 startDiskNumber;		///< number of the disk with the start of the zip64 end of central directory 4 bytes
		uint64 dirEndRecordOffset;	///< relative offset of the zip64 end of central directory record 8 bytes
		uint32 totalDiskCount;		///< total number of disks 4 bytes

		bool read (IO::BinaryAccessor& s);
		bool write (IO::BinaryAccessor& s) const;
	};

	static const uint32 CentralDirEndRecordLocator64Size = 20;
	static const uint32 CentralDirEndRecordLocator64Signature = 0x07064b50;

	bool findZip64Locator (CentralDirEndRecordLocator64& result, IO::Stream& stream);

	//////////////////////////////////////////////////////////////////////////////////////////////

	enum GeneralPurposeFlags
	{
		kEncrypted = 1<<0,			///< If set, indicates that the file is encrypted.
		kStrongEncrypted = 1<<6,	///< Strong encryption
		kUTF8Encoded = 1<<11		///< Language encoding flag (EFS). If this bit is set, the filename and comment fields for this file must be encoded using UTF-8.
	};

	/** File header in central directory. */
	struct CentralDirFileHeader
	{
		uint32 signature;			///< central file header signature   4 bytes  (0x02014b50)
        uint16 versionMadeBy;		///< version made by                 2 bytes
        uint16 versionNeeded;		///< version needed to extract       2 bytes
        uint16 gpBitFlag;			///< general purpose bit flag        2 bytes
        uint16 compressionMethod;	///< compression method              2 bytes
        uint16 lastModifiedTime;	///< last mod file time              2 bytes
        uint16 lastModifiedDate;	///< last mod file date              2 bytes
        uint32 crc32;				///< crc-32                          4 bytes
        uint32 compressedSize;		///< compressed size                 4 bytes
        uint32 uncompressedSize;	///< uncompressed size               4 bytes
        uint16 fileNameLength;		///< file name length                2 bytes
        uint16 extraFieldLength;	///< extra field length              2 bytes
        uint16 fileCommentLength;	///< file comment length             2 bytes
        uint16 startDiskNumber;		///< disk number start               2 bytes
        uint16 internalAttributes;	///< internal file attributes        2 bytes
        uint32 externalAttributes;	///< external file attributes        4 bytes
        uint32 localHeaderOffset;	///< relative offset of local header 4 bytes
        //char fileName[];			///< file name (variable size)
        //char extraField[];		///< extra field (variable size)
        //char fileComment[];		///< file comment (variable size)

		PROPERTY_FLAG (gpBitFlag, kUTF8Encoded, isUTF8Encoded)

		bool read (IO::BinaryAccessor& s);
		bool write (IO::BinaryAccessor& s) const;

		int getAdditionalSize () const { return fileNameLength + extraFieldLength + fileCommentLength; }
	};

	static const uint32 kCentralDirFileHeaderSize = 46;
	static const uint32 kCentralDirFileHeaderSignature = 0x02014b50;

	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Header ID mappings. */
	enum ExtraFieldIdentifiers
	{
		kZip64ExtraID = 0x0001	///< Zip64 extended information extra field
	};

	struct ExtraField
	{
		uint16 headerID;
		uint16 size;

		bool read (IO::BinaryAccessor& s);
		bool write (IO::BinaryAccessor& s) const;
	};

	static const uint32 kExtraFieldPrologSize = 4;

	/** Zip64 Extended Information Extra Field. */
	struct ExtraFieldZip64
	{
		uint64 uncompressedSize;	///< Original uncompressed file size 8 bytes
		uint64 compressedSize;		///< Size of compressed data 8 bytes
		uint64 localHeaderOffset;	///< Offset of local header record 8 bytes
		uint32 startDiskNumber;		///< Number of the disk on which this file starts 4 bytes

		bool read (IO::BinaryAccessor& s, int size);
		bool write (IO::BinaryAccessor& s) const;
	};

	static const uint32 kExtraFieldZip64DataSize = 28;

	//////////////////////////////////////////////////////////////////////////////////////////////

	enum CompressionMethod
	{
		kCompressionMethodNone = 0,
		kCompressionMethodDeflated = 8
	};

	inline bool isCompressionSupported (uint16 method)
	{
		return method == kCompressionMethodNone || method == kCompressionMethodDeflated;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Local file header */
	struct LocalFileHeader
	{
		uint32 signature;			///< local file header signature     4 bytes  (0x04034b50)
		uint16 versionNeeded;		///< version needed to extract       2 bytes
		uint16 gpBitFlag;			///< general purpose bit flag        2 bytes
		uint16 compressionMethod;	///< compression method              2 bytes
		uint16 lastModifiedTime;	///< last mod file time              2 bytes
		uint16 lastModifiedDate;	///< last mod file date              2 bytes
		uint32 crc32;				///< crc-32                          4 bytes
		uint32 compressedSize;		///< compressed size                 4 bytes
		uint32 uncompressedSize;	///< uncompressed size               4 bytes
		uint16 fileNameLength;		///< file name length                2 bytes
		uint16 extraFieldLength;	///< extra field length              2 bytes
		//char fileName[];			///< file name (variable size)
		//char extraField[];		///< extra field (variable size)

		PROPERTY_FLAG (gpBitFlag, kUTF8Encoded, isUTF8Encoded)

		bool read (IO::BinaryAccessor& s);
		bool write (IO::BinaryAccessor& s) const;

		int getAdditionalSize () const { return fileNameLength + extraFieldLength; }
	};

	static const uint32 kLocalFileHeaderSize = 30;
	static const uint32 kLocalFileHeaderSignature = 0x04034b50;

	//////////////////////////////////////////////////////////////////////////////////////////////

	// 16 Bit MS-DOS Date Format: DDDD DMMM MYYY YYYY (5 Bit Day, 4 Bit Month, 7 Bit Year)
	// 16 Bit MS-DOS Time Format: SSSS SMMM MMMH HHHH (5 Bit Second, 6 Bit Minute, 5 Bit Hour)

	inline void getDate (Date& date, uint16 zipDate)
	{
		int y = (zipDate >> 9) + 1980;
		int m = (zipDate >> 5) & 0x0F;
		int d = (zipDate & 0x1F);

		date (y, m, d);
	}

	inline void setDate (uint16& zipDate, const Date& date)
	{
		int y = date.getYear () - 1980;
		int m = date.getMonth ();
		int d = date.getDay ();

		zipDate = 0;
		zipDate |= (y & 0x7F) << 9;
		zipDate |= (m & 0x0F) << 5;
		zipDate |= (d & 0x1F);
	}

	inline void getTime (Time& time, uint16 zipTime)
	{
		int h = zipTime >> 11;
		int m = (zipTime >> 5) & 0x3F;
		int s = (zipTime & 0x1F) << 1; // every 2 seconds

		time (h, m, s);
	}

	inline void setTime (uint16& zipTime, const Time& time)
	{
		int h = time.getHour ();
		int m = time.getMinute ();
		int s = time.getSecond () >> 1; // every 2 seconds

		zipTime = 0;
		zipTime |= (h & 0x1F) << 11;
		zipTime |= (m & 0x3F) << 5;
		zipTime |= (s & 0x1F);
	}

	inline void getDateTime (DateTime& dateTime, uint16 zipDate, uint16 zipTime)
	{
		Date date;
		getDate (date, zipDate);
		Time time;
		getTime (time, zipTime);
		dateTime.setDate (date);
		dateTime.setTime (time);
	}

	inline void adjustDateTime (DateTime& dateTime) // adjust to 2 seconds resolution
	{
		Date date (dateTime.getDate ());
		Time time (dateTime.getTime ());

		uint16 zipDate = 0, zipTime = 0;
		setDate (zipDate, date);
		setTime (zipTime, time);
		getDate (date, zipDate);
		getTime (time, zipTime);

		dateTime.setDate (date);
		dateTime.setTime (time);
	}
}

} // namespace Core

#endif // _corezipfileformat_h
