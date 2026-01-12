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
// Filename    : core/platform/shared/posix/corefilesystem.posix.cpp
// Description : File System POSIX implementation
//
//************************************************************************************************

#include "core/platform/corefeatures.h"

#if CORE_FILESYSTEM_IMPLEMENTATION == CORE_PLATFORM_IMPLEMENTATION
	#include CORE_PLATFORM_IMPLEMENTATION_HEADER (corefilesystem)
#elif CORE_FILESYSTEM_IMPLEMENTATION == CORE_FEATURE_IMPLEMENTATION_POSIX
	#include "core/platform/shared/posix/corefilesystem.posix.h"
#endif

#if CORE_FILESYSTEM_IMPLEMENTATION == CORE_FEATURE_IMPLEMENTATION_POSIX

#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

#endif // CORE_FILESYSTEM_IMPLEMENTATION == CORE_FEATURE_IMPLEMENTATION_POSIX

using namespace Core;
using namespace Platform;

//************************************************************************************************
// FileSystem
//************************************************************************************************

#if CORE_FILESYSTEM_IMPLEMENTATION == CORE_FEATURE_IMPLEMENTATION_POSIX
IFileSystem& FileSystem::instance ()
{
	static PosixFileSystem theFileSystem;
	return theFileSystem;
}
#endif

//************************************************************************************************
// PosixFileIterator
//************************************************************************************************

PosixFileIterator::PosixFileIterator (CStringPtr dirname)
: FileIteratorBase (dirname),
  findHandle (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

PosixFileIterator::~PosixFileIterator ()
{
	if(findHandle)
		closedir ((DIR*)findHandle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixFileIterator::findNext (Entry& entry)
{
	if(findHandle == 0)
	{
		findHandle = (IntPtr)opendir (dirname);
		if(findHandle == 0)
			return false;
	}

	if(dirent* findData = readdir ((DIR*)findHandle))
	{
		entry.name = findData->d_name;
		entry.directory = (findData->d_type == DT_DIR);
		entry.hidden = entry.name[0] == '.';
		return true;
	}
	return false;
}

//************************************************************************************************
// PosixFileSystem
//************************************************************************************************

void PosixFileSystem::getDirectory (FileName& dirname, DirType type)
{
	switch(type)
	{
	case kTempDir:
	case kDataDir:
	case kAppDir:
	case kAppSupportDir:
	case kHomeDir:
		dirname = ::getenv ("HOME");
		break;
	case kSharedDataDir:
	case kSharedAppDir:
	case kSharedAppSupportDir:
		ASSERT (false)
		break;
	case kWorkingDir:
		::getcwd (dirname.getBuffer (), dirname.getSize ());
		break;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixFileSystem::makeDirectory (CStringPtr dirname)
{
	return mkdir (dirname, -1) == 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixFileSystem::deleteFile (CStringPtr filename)
{
	return ::remove (filename) == 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixFileSystem::removeDirectory (CStringPtr dirname)
{
	return ::rmdir (dirname) == 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixFileSystem::renameFile (CStringPtr oldname, CStringPtr newname)
{
	return ::rename (oldname, newname) == 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixFileSystem::fileExists (CStringPtr filename)
{
	FILE* tmp = ::fopen (filename, "r");
	if(tmp)
	{
		::fclose (tmp);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixFileSystem::dirExists (CStringPtr dirname)
{
	struct stat buf;
	int result = ::stat (dirname, &buf);
	if(result != 0)
		return false;

	return (buf.st_mode & S_IFDIR) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 PosixFileSystem::fileLastModified (CStringPtr filename)
{
	struct stat buf;
	int result = ::stat (filename, &buf);
	if(result != 0)
		return 0;

	return buf.st_mtime;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixFileSystem::truncate (CStringPtr filename, int64 length)
{
	if(length < 1)
		return false;

	int64 len = ::truncate (filename, (off_t)length);
	return len == length;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixFileSystem::touchFile (CStringPtr filename)
{
	return false;
}

//************************************************************************************************
// PosixFileStream
//************************************************************************************************

PosixFileStream::PosixFileStream (FILE* file)
: file (file)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

PosixFileStream::~PosixFileStream ()
{
	close ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixFileStream::open (CStringPtr filename, int mode)
{
	ASSERT (file == nullptr)
	file = ::fopen (filename, (mode & IO::kWriteMode) ? "r+b" : "rb");
	return file != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixFileStream::create (CStringPtr filename)
{
	ASSERT (file == nullptr)
	file = ::fopen (filename, "w+b");
	return file != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PosixFileStream::close ()
{
	if(file)
		::fclose (file);
	file = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixFileStream::isOpen () const
{
	return file != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 PosixFileStream::getFileSize ()
{
	int64 oldPos = getPosition ();
	int64 size = setPosition (0, IO::kSeekEnd);
	setPosition (oldPos, IO::kSeekSet);
	return size;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 PosixFileStream::getPosition ()
{
	return ::ftell (file);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 PosixFileStream::setPosition (int64 pos, int mode)
{
	::fseek (file, (int)pos, mode);
	return (int64)::ftell (file);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int PosixFileStream::readBytes (void* buffer, int size)
{
	return (int)::fread (buffer, 1, size, file);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int PosixFileStream::writeBytes (const void* buffer, int size)
{
	return (int)::fwrite (buffer, 1, size, file);
}
