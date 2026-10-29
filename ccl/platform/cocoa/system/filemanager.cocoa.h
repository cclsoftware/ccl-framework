//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
//
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/platform/cocoa/system/filemanager.cocoa.h
// Description : Shared Cocoa (Mac/iOS) file system manager base
//
//************************************************************************************************

#ifndef _ccl_filemanager_cocoa_h
#define _ccl_filemanager_cocoa_h

#include "ccl/system/filemanager.h"

namespace CCL {

//************************************************************************************************
// CocoaFileManager
//************************************************************************************************

class CocoaFileManager: public FileManager
{
public:
	// FileManager
	tbool CCL_API getFileSystemRepresentation (char pathBuffer[], int bufferSize, UrlRef url) const override;
	tresult startUsing (UrlRef url) override;
	tresult stopUsing (UrlRef url) override;
};

} // namespace CCL

#endif // _ccl_filemanager_cocoa_h
