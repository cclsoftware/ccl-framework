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
// Filename    : ccl/platform/cocoa/system/filemanager.ios.h
// Description : iOS file system manager
//
//************************************************************************************************

#ifndef _ccl_filemanager_ios_h
#define _ccl_filemanager_ios_h

#include "ccl/system/filemanager.h"

namespace CCL {

//************************************************************************************************
// IOSFileManager
//************************************************************************************************

class IOSFileManager: public FileManager
{
public:
	DECLARE_CLASS (IOSFileManager, FileManager)

	IOSFileManager ();
	~IOSFileManager ();

    // FileManager
	IAsyncOperation* CCL_API triggerFileUpdate (UrlRef url) override;
	tbool CCL_API getFileDisplayString (String& string, UrlRef url, int type) const override;
	StringID CCL_API getFileLocationType (UrlRef url) const override; 

protected:
	// FileManager
	tresult startWatching (UrlRef url, int flags) override;
	tresult stopWatching (UrlRef url) override;
	tresult startUsing (UrlRef url) override;
	tresult stopUsing (UrlRef url) override;
	void setWriting (UrlRef url, bool state) override;
};

} // namespace CCL

#endif // _ccl_filemanager_ios_h
