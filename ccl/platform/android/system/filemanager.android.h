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
// Filename    : ccl/platform/android/system/filemanager.android.h
// Description : Android file manager
//
//************************************************************************************************

#ifndef _ccl_filemanager_android_h
#define _ccl_filemanager_android_h

#include "ccl/system/filemanager.h"

namespace CCL {

//************************************************************************************************
// AndroidFileManager
//************************************************************************************************

class AndroidFileManager: public FileManager
{
public:
	AndroidFileManager ();

	// FileManager
	tbool CCL_API getFileDisplayString (String& string, UrlRef url, int type) const override;
	StringID CCL_API getFileLocationType (UrlRef url) const override;
};

} // namespace CCL

#endif // _ccl_filemanager_android_h
