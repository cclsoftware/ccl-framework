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
// Filename    : ccl/platform/linux/filemanager.linux.h
// Description : Linux file manager
//
//************************************************************************************************

#ifndef _ccl_filemanager_linux_h
#define _ccl_filemanager_linux_h

#include "ccl/system/filemanager.h"

namespace CCL {

class LinuxFileSystemMonitorThread;

//************************************************************************************************
// LinuxFileManager
//************************************************************************************************

class LinuxFileManager: public FileManager
{
public:
	LinuxFileManager ();
	~LinuxFileManager ();

	// FileManager
	void CCL_API terminate () override;
	
protected:
	friend class LinuxFileSystemMonitorThread;
	LinuxFileSystemMonitorThread* thread;

	// FileManager
	tresult startWatching (UrlRef url, int flags) override;
	tresult stopWatching (UrlRef url) override;
};

} // namespace CCL

#endif // _ccl_filemanager_linux_h
