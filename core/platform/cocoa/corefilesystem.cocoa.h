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
// Filename    : core/platform/cocoa/corefilesystem.cocoa.h
// Description : File System Cocoa implementation
//
//************************************************************************************************

#ifndef _corefilesystem_cocoa_h
#define _corefilesystem_cocoa_h

#include "core/platform/shared/posix/corefilesystem.posix.h"

#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

namespace Core {
namespace Platform {

//************************************************************************************************
// FileIterator
//************************************************************************************************

typedef PosixFileIterator FileIterator;

//************************************************************************************************
// FileStream
//************************************************************************************************

typedef PosixFileStream FileStream;

//************************************************************************************************
// CocoaFileSystem
//************************************************************************************************

class CocoaFileSystem: public PosixFileSystem
{
public:
	// PosixFileSystem
	void getDirectory (FileName& dirname, DirType type) override;
};

} // namespace Core
} // namespace Platform

#endif // _corefilesystem_cocoa_h
