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
// Filename    : core/platform/cocoa/corefilesystem.cocoa.cpp
// Description : File System Cocoa implementation
//
//************************************************************************************************

#include "corefilesystem.cocoa.h"

using namespace Core;
using namespace Platform;

//************************************************************************************************
// FileSystem
//************************************************************************************************

IFileSystem& FileSystem::instance () 
{ 
	static CocoaFileSystem theFileSystem;
	return theFileSystem; 
}

//************************************************************************************************
// CocoaFileSystem
//************************************************************************************************

void CocoaFileSystem::getDirectory (FileName& dirname, DirType type)
{
	switch(type)
	{
	case kTempDir: 
	case kHomeDir:
		dirname = ::getenv ("HOME");
		dirname += "/tmp";
		break;
	case kDataDir: 
	case kAppDir:
	case kAppSupportDir: 
		dirname = ::getenv ("HOME");
		dirname += "/Library/Application Support";
		break;
	case kSharedAppDir:
		dirname = "/Applications";
		break;
	case kSharedDataDir:
	case kSharedAppSupportDir: 
		dirname = "/Library/Application Support";
		break;
	case kWorkingDir:
		::getcwd (dirname.getBuffer (), dirname.getSize ());
		break;
	}
}
