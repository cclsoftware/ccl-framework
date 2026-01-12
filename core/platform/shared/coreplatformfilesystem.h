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
// Filename    : core/platform/shared/coreplatformfilesystem.h
// Description : File System Platform Abstraction
//
//************************************************************************************************

#ifndef _coreplatformfilesystem_h
#define _coreplatformfilesystem_h

#include "core/portable/corefilename.h"
#include "core/public/corestream.h"
#include "core/platform/corefeatures.h"

namespace Core {
namespace Platform {

using Portable::FileName;
using Portable::FindFileData;

//************************************************************************************************
// IFileIterator
//************************************************************************************************

struct IFileIterator
{
	typedef FindFileData Entry;
	
	virtual ~IFileIterator () {}

	virtual bool findNext (Entry& entry) = 0;
};

//************************************************************************************************
// FileIteratorBase
//************************************************************************************************

class FileIteratorBase: public IFileIterator
{
public:
	FileIteratorBase (CStringPtr dirname);

	const Entry* next ();

protected:
	FileName dirname;
	Entry result;
};

//************************************************************************************************
// IFileStream
//************************************************************************************************

struct IFileStream: IO::Stream
{
	virtual bool open (CStringPtr filename, int mode = IO::kReadMode|IO::kWriteMode) = 0;
	virtual bool create (CStringPtr filename) = 0;
	virtual void close () = 0;
	virtual bool isOpen () const = 0;
	virtual int64 getFileSize () = 0;
};

//************************************************************************************************
// IFileSystem
//************************************************************************************************

struct IFileSystem
{
	enum DirType
	{
		kTempDir,
		kDataDir,
		kSharedDataDir,
		kAppDir,
		kSharedAppDir,
		kAppSupportDir,
		kSharedAppSupportDir,
		kHomeDir,
		kWorkingDir
	};
	
	virtual ~IFileSystem () {}
	
	virtual void getDirectory (FileName& dirname, DirType type) = 0;
	virtual bool makeDirectory (CStringPtr dirname) = 0;
	virtual bool fileExists (CStringPtr filename) = 0;
	virtual bool dirExists (CStringPtr dirname) = 0;
	virtual int64 fileLastModified (CStringPtr filename) = 0;
	virtual bool deleteFile (CStringPtr filename) = 0;
	virtual bool removeDirectory (CStringPtr dirname) = 0;
	virtual bool renameFile (CStringPtr oldname, CStringPtr newname) = 0;
	virtual bool truncate (CStringPtr oldname, int64 length) = 0;
	virtual bool touchFile (CStringPtr filename) = 0;
};

//************************************************************************************************
// FileSystem
//************************************************************************************************

namespace FileSystem
{
	IFileSystem& instance ();
}

//************************************************************************************************
// FileIteratorBase implementation
//************************************************************************************************

inline FileIteratorBase::FileIteratorBase (CStringPtr dirname)
: dirname (dirname)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline const IFileIterator::Entry* FileIteratorBase::next ()
{
	Entry entry;
	while(findNext (entry))
	{
		if(entry.name == "." || entry.name == "..")
			continue;

		result.name = dirname;
		result.name.descend (entry.name);
		result.directory = entry.directory;
		result.hidden = entry.hidden;
		return &result;
	}
	return nullptr;
}

#if CORE_FILESYSTEM_IMPLEMENTATION == CORE_FEATURE_UNIMPLEMENTED
	
//************************************************************************************************
// FileIteratorStub
//************************************************************************************************

class FileIteratorStub: public FileIteratorBase
{
public:
	FileIteratorStub (CStringPtr dirname)
	: FileIteratorBase (dirname)
	{}

	// FileIteratorBase
	bool findNext (Entry& entry) { return false; }
};

typedef FileIteratorStub FileIterator;

//************************************************************************************************
// FileStreamStub
//************************************************************************************************

class FileStreamStub : public IFileStream
{
public:
	FileStreamStub (FILE* file = nullptr)
	{}
	
	// IFileStream
	int64 getPosition () { return 0; }
	int64 setPosition (int64 pos, int mode) { return 0; }
	int readBytes (void* buffer, int size) { return 0; }
	int writeBytes (const void* buffer, int size) { return 0; }
	bool open (CStringPtr filename, int mode = IO::kReadMode | IO::kWriteMode) { return false; }
	bool create (CStringPtr filename) { return false; }
	void close () {}
	bool isOpen () const { return false; }
	int64 getFileSize () { return 0; }
};

typedef FileStreamStub FileStream;

//************************************************************************************************
// FileSystemStub
//************************************************************************************************

class FileSystemStub: public IFileSystem
{
	void getDirectory (FileName& dirname, DirType type) {};
	bool makeDirectory (CStringPtr dirname) { return false; };
	bool fileExists (CStringPtr filename) { return false; };
	bool dirExists (CStringPtr dirname) { return false; }; 
	int64 fileLastModified (CStringPtr filename) { return 0; };
	bool deleteFile (CStringPtr filename) { return false; };
	bool removeDirectory (CStringPtr dirname) { return false; }; 
	bool renameFile (CStringPtr oldname, CStringPtr newname) { return false; };
	bool truncate (CStringPtr oldname, int64 length) { return false; };
	bool touchFile (CStringPtr filename) { return false; };
};

//////////////////////////////////////////////////////////////////////////////////////////////////

inline IFileSystem& FileSystem::instance () 
{
	static FileSystemStub theFileSystem;
	return theFileSystem;
};

#endif // CORE_FILESYSTEM_IMPLEMENTATION == CORE_FEATURE_UNIMPLEMENTED
	
} // namespace Platform
} // namespace Core

#endif // _coreplatformfilesystem_h
