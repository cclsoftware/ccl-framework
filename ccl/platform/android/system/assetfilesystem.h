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
// Filename    : ccl/platform/android/system/assetfileystem.h
// Description : Android Asset File System
//
//************************************************************************************************

#ifndef _ccl_assetfilesystem_h
#define _ccl_assetfilesystem_h

#include "ccl/base/storage/url.h"

#include "ccl/public/system/ifilesystem.h"

namespace CCL {
namespace Android {

//************************************************************************************************
// AssetFileSystem
/** File system for Android app assets. */
//************************************************************************************************

class AssetFileSystem: public Unknown,
					   public AbstractFileSystem
{
public:
	// IFileSystem
	IStream* CCL_API openStream (UrlRef url, int mode, IUnknown* context = nullptr) override;
	tbool CCL_API getFileInfo (FileInfo& info, UrlRef url) override;
	IFileIterator* CCL_API newIterator (UrlRef url, int mode = IFileIterator::kAll) override;
	tbool CCL_API fileExists (UrlRef url) override;

	CLASS_INTERFACE (IFileSystem, Unknown)
};

//************************************************************************************************
// AssetUrl
//************************************************************************************************

class AssetUrl: public Url
{
public:
	AssetUrl (StringRef path, int type = kFile);

	static const String Protocol;
};

} // namespace Android
} // namespace CCL

#endif // _ccl_assetfilesystem_h
