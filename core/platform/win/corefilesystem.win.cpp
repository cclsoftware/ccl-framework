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
// Filename    : core/platform/win/corefilesystem.win.cpp
// Description : File System Windows implementation
//
//************************************************************************************************

#include "corefilesystem.win.h"

#include <windows.h>
#include <io.h>
#include <fcntl.h>

namespace Core {
namespace Platform {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Missing POSIX functions
//////////////////////////////////////////////////////////////////////////////////////////////////

int closedir (DIR* handle) { return _findclose ((IntPtr)handle); }
DIR* opendir (const char* name) { return nullptr; }
dirent* readdir (DIR* dirp) { return nullptr; }
int mkdir (const char* path, int mode) { return _mkdir (path); }
int truncate (const char* path, off_t length) { return 0; }

} // namespace Platform
} // namespace Core

using namespace Core;
using namespace Platform;

//************************************************************************************************
// FileSystem
//************************************************************************************************

IFileSystem& FileSystem::instance ()
{
	static Win32FileSystem theFileSystem;
	return theFileSystem;
}

//************************************************************************************************
// Win32FileIterator
//************************************************************************************************

Win32FileIterator::Win32FileIterator (CStringPtr dirname)
: FileIteratorBase (dirname),
  findHandle (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Win32FileIterator::~Win32FileIterator ()
{
	if(findHandle)
		_findclose (findHandle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Win32FileIterator::findNext (Entry& entry)
{
	_finddata_t findData = {0};
	if(findHandle == 0)
	{
		FileName filespec (dirname);
		filespec.descend ("*.*");
		findHandle = _findfirst (filespec, &findData);
		if(findHandle <= 0) // can be -1 for error
			return false;
		if(findData.name[0] == 0) // directory doesn't exist (exit here, avoid crash in _findnext)
			return false;
	}
	else
	{
		if(_findnext (findHandle, &findData) != 0)
			return false;
	}

	entry.name = findData.name;
	entry.directory = (findData.attrib & _A_SUBDIR) != 0;
	entry.hidden = (findData.attrib & _A_HIDDEN) != 0;
	return true;
}

//************************************************************************************************
// Win32FileSystem
//************************************************************************************************

void Win32FileSystem::getDirectory (FileName& dirname, DirType type)
{
	switch(type)
	{
	case kTempDir:
		dirname = ::getenv ("TEMP");
		break;
	case kDataDir:
		dirname = ::getenv ("APPDATA");
		break;
	case kSharedDataDir:
		dirname = ::getenv ("PROGRAMDATA");
		break;
	case kAppDir:
	case kSharedAppDir:
		dirname = ::getenv ("PROGRAMFILES");
		break;
	case kAppSupportDir:
		::GetModuleFileNameA (NULL, dirname.getBuffer (), dirname.getSize ());
		dirname.ascend ();
		break;
	case kSharedAppSupportDir:
		dirname = ::getenv ("COMMONPROGRAMFILES");
		break;
	case kHomeDir:
		dirname = ::getenv ("HOMEDRIVE");
		dirname += ::getenv ("HOMEPATH");
		break;
	case kWorkingDir:
		::getcwd (dirname.getBuffer (), dirname.getSize ());
		break;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Win32FileSystem::truncate (CStringPtr filename, int64 length)
{
	if(length < 1)
		return false;

	int fd = 0;
	if(_sopen_s (&fd, filename, _O_RDWR, _SH_DENYNO, _S_IREAD|_S_IWRITE) == 0)
	{
		int err = _chsize_s (fd, length);
		_close (fd);
		return err == 0;
	}
	return false;
}
