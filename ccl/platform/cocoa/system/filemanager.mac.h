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
// Filename    : ccl/platform/cocoa/filemanager.mac.h
// Description : Mac file system manager
//
//************************************************************************************************

#ifndef _ccl_filemanager_mac_h
#define _ccl_filemanager_mac_h

#include "ccl/platform/cocoa/system/filemanager.cocoa.h"

namespace CCL {

class CocoaFSEventStream;

//************************************************************************************************
// MacFileManager
//************************************************************************************************

class MacFileManager: public CocoaFileManager
{
public:
	MacFileManager ();
	~MacFileManager ();

	// CocoaFileManager
	tresult startWatching (UrlRef url, int flags);
	tresult stopWatching (UrlRef url);

	tbool CCL_API getFileDisplayString (String& string, UrlRef url, int type) const;
	StringID CCL_API getFileLocationType (UrlRef url) const;
	void CCL_API terminate ();

protected:
	CocoaFSEventStream* eventStream;
};

} // namespace CCL

#endif // _ccl_filemanager_mac_h
