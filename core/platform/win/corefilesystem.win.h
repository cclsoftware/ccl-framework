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
// Filename    : core/platform/win/corefilesystem.win.h
// Description : File System Windows implementation
//
//************************************************************************************************

#ifndef _corefilesystem_win_h
#define _corefilesystem_win_h

#include "core/platform/shared/posix/corefilesystem.posix.h"

#include <direct.h>
#include <sys/stat.h>

namespace Core {
namespace Platform {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Missing POSIX functions
//////////////////////////////////////////////////////////////////////////////////////////////////

typedef int* DIR;
struct dirent 
{
	unsigned char d_type;
	char d_name[256];
};

const unsigned char DT_DIR = 0;

int closedir (DIR* handle);
DIR* opendir (const char *name);
dirent* readdir (DIR *dirp);
int mkdir (const char *path, int mode);
int truncate (const char *path, off_t length);

//************************************************************************************************
// Win32FileIterator
//************************************************************************************************

class Win32FileIterator: public FileIteratorBase
{
public:
	Win32FileIterator (CStringPtr dirname);
	~Win32FileIterator ();

	// FileIteratorBase
	bool findNext (Entry& entry) override;

protected:
	IntPtr findHandle;
};

typedef Win32FileIterator FileIterator;

//************************************************************************************************
// FileStream
//************************************************************************************************

typedef PosixFileStream FileStream;

//************************************************************************************************
// Win32FileSystem
//************************************************************************************************

class Win32FileSystem: public PosixFileSystem
{
public:
	// PosixFileSystem
	void getDirectory (FileName& dirname, DirType type) override;
	bool truncate (CStringPtr oldname, int64 length) override;
};

} // namespace Core
} // namespace Platform

#endif // _corefilesystem_win_h
