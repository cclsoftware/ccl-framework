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
// Filename    : core/platform/shared/posix/corefilesystem.posix.h
// Description : File System POSIX implementation
//
//************************************************************************************************

#ifndef _corefilesystem_posix_h
#define _corefilesystem_posix_h

#include "core/platform/shared/coreplatformfilesystem.h"

#include "core/platform/corefeatures.h"

namespace Core {
namespace Platform {

//************************************************************************************************
// PosixFileIterator
//************************************************************************************************

class PosixFileIterator: public FileIteratorBase
{
public:
	PosixFileIterator (CStringPtr dirname);
	~PosixFileIterator ();

	// FileIteratorBase
	bool findNext (Entry& entry) override;

protected:
	IntPtr findHandle;
};

#if CORE_FILESYSTEM_IMPLEMENTATION == CORE_FEATURE_IMPLEMENTATION_POSIX
typedef PosixFileIterator FileIterator;
#endif

//************************************************************************************************
// PosixFileStream
//************************************************************************************************

class PosixFileStream: public IFileStream
{
public:
	PosixFileStream (FILE* file = nullptr);
	~PosixFileStream ();

	// IFileStream
	int64 getPosition () override;
	int64 setPosition (int64 pos, int mode) override;
	int readBytes (void* buffer, int size) override;
	int writeBytes (const void* buffer, int size) override;
	bool open (CStringPtr filename, int mode = IO::kReadMode|IO::kWriteMode) override;
	bool create (CStringPtr filename) override;
	void close () override;
	bool isOpen () const override;
	int64 getFileSize () override;

protected:
	FILE* file;
};

#if CORE_FILESYSTEM_IMPLEMENTATION == CORE_FEATURE_IMPLEMENTATION_POSIX
typedef PosixFileStream FileStream;
#endif

//************************************************************************************************
// PosixFileSystem
//************************************************************************************************

class PosixFileSystem: public IFileSystem
{
public:
	// IFileSystem
	void getDirectory (FileName& dirname, DirType type) override;
	bool makeDirectory (CStringPtr dirname) override;
	bool fileExists (CStringPtr filename) override;
	bool dirExists (CStringPtr dirname) override;
	int64 fileLastModified (CStringPtr filename) override;
	bool deleteFile (CStringPtr filename) override;
	bool removeDirectory (CStringPtr dirname) override;
	bool renameFile (CStringPtr oldname, CStringPtr newname) override;
	bool truncate (CStringPtr oldname, int64 length) override;
	bool touchFile (CStringPtr filename) override;
};

} // namespace Core
} // namespace Platform

#endif // _corefilesystem_posix_h
