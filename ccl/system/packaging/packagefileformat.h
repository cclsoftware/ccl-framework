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
// Filename    : ccl/system/packaging/packagefileformat.h
// Description : Package File Format
//
//************************************************************************************************

#ifndef _ccl_packagefileformat_h
#define _ccl_packagefileformat_h

#include "ccl/public/base/streamer.h"
#include "ccl/public/text/cstring.h"

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Package File Format
//////////////////////////////////////////////////////////////////////////////////////////////////

/*
  0 +---------------------------------+
	| Package Signature				  |	8 Bytes
	+---------------------------------+
	| Reserved Block (optional)       | variable
	+---------------------------------+
	| Data Block 					  |	variable
	| (File 1..N)					  |
	+---------------------------------+
	| File Tree 					  |	variable
	|								  |
	+---------------------------------+
	| Package Chunk					  |	64 Bytes
	|								  |
EOF +---------------------------------+

*/

//////////////////////////////////////////////////////////////////////////////////////////////////

struct PackageChunk // 64 Bytes
{
	int64 fileTreePosition;			///< absolute position of file tree
	uint32 fileTreeSize;			///< size of file tree
	UIDBytes encryptionType;		///< Encryption type
	UIDBytes compressionType;		///< Compression type
	uint32 flags;					///< kBasicEncrypted, etc.
	uint32 version;					///< version
	uint32 chunkSize;				///< size of this chunk (including signature)
	FOURCC signature1;				///< Package signature 1
	FOURCC signature2;				///< Package signature 2

	bool serialize (Streamer& s) const
	{
		s.write (fileTreePosition);
		s.write (fileTreeSize);
		s.write (encryptionType);
		s.write (compressionType);
		s.write (flags);
		s.write (version);
		s.write (chunkSize);
		s.write (signature1);
		s.write (signature2);
		return true;
	}

	bool deserialize (Streamer& s)
	{
		s.read (fileTreePosition);
		s.read (fileTreeSize);
		s.read (encryptionType);
		s.read (compressionType);
		s.read (flags);
		s.read (version);
		s.read (chunkSize);
		s.read (signature1);
		s.read (signature2);
		return true;
	}
};

struct ReservedBlockHeader
{
	FOURCC signature;			///< Reserved block signature
	uint32 totalSize;			///< total size (including signature)
	uint32 usedSize;			///< used size (data starts after file name)
	MutableCString comment;		///< comment (UTF-8)
	MutableCString fileName;	///< file name (UTF-8)

	ReservedBlockHeader ()
	: totalSize (0),
	  usedSize (0)
	{
		signature.fcc = 0;
	}

	int getHeaderSize () const
	{
		return 12 + comment.length ()+1 + fileName.length ()+1;
	}

	bool serialize (Streamer& s) const
	{
		s.write (signature);
		s.write (totalSize);
		s.write (usedSize);
		s.writeCString (comment);
		s.writeCString (fileName);
		return true;
	}

	bool deserialize (Streamer& s)
	{
		s.read (signature);
		s.read (totalSize);
		s.read (usedSize);
		s.readCString (comment);
		s.readCString (fileName);
		return true;
	}
};

enum PackageFileConstants
{
	kPackageFormatV1 = 1, // initial format version (UTF-16 file names)
	kPackageFormatV2 = 2, // file tree encryption, UTF-8 file names
	kPackageFormatV3 = 3, // hidden attribute implemented (no file format change)

	kPackageChunkSize = sizeof(PackageChunk),
	kReservedBlockOffset = 8 ///< after package signature
};

static DEFINE_FOURCC (kPackageSignature1, 'P', 'A', 'C', 'K')
static DEFINE_FOURCC (kPackageSignature2, 'A', 'G', 'E', 'F')
static DEFINE_FOURCC (kReservedBlockSignature, 'R', 'S', 'V', 'D')

static inline bool isValidFormatVersion (int version)
{
	return version >= kPackageFormatV1 && version <= kPackageFormatV3;
}

static inline void encryptionTypeToKey (uint8 key[16], UIDRef encryptionType)
{
	UIDBytes t (encryptionType);
	t.data1 = MAKE_LITTLE_ENDIAN (t.data1);
	t.data2 = MAKE_LITTLE_ENDIAN (t.data2);
	t.data3 = MAKE_LITTLE_ENDIAN (t.data3);
	::memcpy (key, &t, 16);
}

} // namespace CCL

#endif // _ccl_packagefileformat_h
