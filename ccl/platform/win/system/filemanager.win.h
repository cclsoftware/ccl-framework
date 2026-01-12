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
// Filename    : ccl/platform/win/filemanager.win.h
// Description : Windows file manager
//
//************************************************************************************************

#ifndef _ccl_filemanager_win_h
#define _ccl_filemanager_win_h

#include "ccl/system/filemanager.h"

namespace CCL {

//************************************************************************************************
// WindowsFileManager
//************************************************************************************************

class WindowsFileManager: public FileManager
{
public:
	DECLARE_CLASS_ABSTRACT (WindowsFileManager, FileManager)

	WindowsFileManager ();
	~WindowsFileManager ();

	// FileManager
	tbool CCL_API getFileDisplayString (String& string, UrlRef url, int type) const override; 
	StringID CCL_API getFileLocationType (UrlRef url) const override; 
	void CCL_API terminate () override;

protected:
	class MonitorThread;
	MonitorThread* monitorThread;

	// FileManager
	tresult startWatching (UrlRef url, int flags) override;
	tresult stopWatching (UrlRef url) override;
};

} // namespace CCL

#endif // _ccl_filemanager_win_h
