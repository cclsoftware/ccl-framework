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
// Filename    : ccl/system/packaging/zipfile.h
// Description : Zip File
//
//************************************************************************************************

#ifndef _ccl_zipfile_h
#define _ccl_zipfile_h

#include "ccl/system/packaging/filearchive.h"

namespace CCL {

//************************************************************************************************
// ZipFile
//************************************************************************************************

class ZipFile: public FileArchive
{
public:
	DECLARE_CLASS (ZipFile, FileArchive)

	ZipFile (UrlRef path = Url ());
	~ZipFile ();

protected:
	bool isZip64;

	void setEncrypted (bool state);
	IStream* createEncryptionStream (IStream* srcStream, const FileStreamItem& item) const;

	FileSystemItem* createFileSystemItem (StringRef fileName);
	int writeDirEntries (IStream& stream, FolderItem& folder);

	void encodeFileName (FileSystemItem& item, StringRef unicodePath) const;
	bool isFolderHeaderNeeded (const FolderItem& item) const;

	// FileArchive overrides:
	tresult CCL_API setOption (StringID id, VariantRef value) override;
	tresult CCL_API getOption (Variant& value, StringID id) const override;
	SectionStream* openSectionStream (FileStreamItem& item) override;
	bool readFormat (IStream& stream) override;
	bool writeFormat (IStream& stream, IProgressNotify* progress) override;
	int64 beginFile (IStream& dstStream, FileStreamItem& item) override;
	bool endFile (IStream& dstStream, FileStreamItem& item) override;
	bool beginFolder (IStream& dstStream, FolderItem& item) override;
	IStream* createReadTransform (IStream& srcStream, FileStreamItem& item, IUnknown* context) const override;
	IStream* createWriteTransform (IStream& dstStream, FileStreamItem& item, IUnknown* context) const override;
};

} // namespace CCL

#endif // _ccl_zipfile_h
