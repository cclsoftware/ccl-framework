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
// Filename    : ccl/system/packaging/packagefile.h
// Description : Package File
//
//************************************************************************************************

#ifndef _ccl_packagefile_h
#define _ccl_packagefile_h

#include "ccl/system/packaging/filearchive.h"

namespace CCL {

//************************************************************************************************
// PackageFile
//************************************************************************************************

class PackageFile: public FileArchive
{
public:
	DECLARE_CLASS (PackageFile, FileArchive)

	PackageFile (UrlRef path = Url ());
	~PackageFile ();

	enum ChunkFlags
	{
		kEncrypted = 1<<0,
		kHasReservedBlock = 1<<1,

		kEncryptionAlgoMask	= 0xff000000,
		kEncryptionBasic	= 0x00000000,
		kEncryptionXTEA		= 0x01000000,
		kEncryptionAES		= 0x02000000
	};

	PROPERTY_VARIABLE (int, formatVersion, FormatVersion)
	PROPERTY_VARIABLE (int, chunkFlags, ChunkFlags)
	PROPERTY_FLAG (chunkFlags, kEncrypted, isEncrypted)
	PROPERTY_VARIABLE (int, reservedBlockSize, ReservedBlockSize)

	int getEncryptionAlgorithm () const;
	void setEncryptionAlgorithm (int algo);

protected:
	bool isFileTreeEncrypted () const;
	void setEncryptionOption (int algorithm, bool state);
	IStream* createEncryptionStream (IStream* srcStream, const uint8 key[16], int64 nonce) const;

	// FileArchive overrides:
	tresult CCL_API setOption (StringID id, VariantRef value) override;
	tresult CCL_API getOption (Variant& value, StringID id) const override;
	bool readFormat (IStream& stream) override;
	bool writeFormat (IStream& stream, IProgressNotify* progress) override;
	int64 beginFile (IStream& dstStream, FileStreamItem& item) override { return 0; }
	bool endFile (IStream& dstStream, FileStreamItem& item) override { return true; }
	bool beginFolder (IStream& dstStream, FolderItem& item) override { return true; }
	IStream* createReadTransform (IStream& srcStream, FileStreamItem& item, IUnknown* context) const override;
	IStream* createWriteTransform (IStream& dstStream, FileStreamItem& item, IUnknown* context) const override;
};

} // namespace CCL

#endif // _ccl_packagefile_h
