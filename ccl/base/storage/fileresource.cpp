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
// Filename    : ccl/base/storage/fileresource.cpp
// Description : File Resource
//
//************************************************************************************************

#include "ccl/base/storage/fileresource.h"

#include "ccl/public/systemservices.h"
#include "ccl/public/system/inativefilesystem.h"

#define DEBUG_OPEN_FILES (0 && DEBUG)

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

#if DEBUG_OPEN_FILES
#include "ccl/base/collections/objectlist.h"
#include "ccl/public/text/cstring.h"

struct DebugFileList
{
	struct Entry : public Object
	{
		FileResource& file;
		int id;

		Entry (FileResource& file, int id)
		: file (file), id (id)
		{}

		void log ()
		{
			UrlDisplayString path (file.getPath ());
			Debugger::printf ("  (%d) %s  %s\n", id, file.isOpen () ? " open " : "closed", MutableCString (path).str ());
		}
	};

	ObjectList entries;
	int nextId;

	DebugFileList ()
	: nextId (0)
	{
		entries.objectCleanup ();
	}

	void addFile (FileResource* file)
	{
		entries.add (NEW Entry (*file, nextId++));
	}

	void removeFile (FileResource* file)
	{
		if(Entry* e = findEntry (file))
			if(entries.remove (e))
				e->release ();

		Debugger::println ("---");
		log ();
	}

	Entry* findEntry (FileResource* file)
	{
		ForEach (entries, Entry, e)
			if(&e->file == file)
				return e;
		EndFor
		return 0;
	}

	void onFileOpened (FileResource* file, int openCount)
	{
		if(Entry* e = findEntry (file))
		{
			Debugger::printf ("--- openFile (%d):", openCount);
			e->log ();
		}
		if(openCount == 1)
			log ();
	}

	void onFileClosed (FileResource* file, int openCount)
	{
		if(Entry* e = findEntry (file))
		{
			Debugger::printf ("--- closeFile (%d):", openCount);
			e->log ();
		}
		if(openCount == 0)
			log ();
	}

	void log ()
	{
		ForEach (entries, Entry, e)
			e->log ();
		EndFor
		Debugger::println ("");
	}
};
static DebugFileList debugFiles;
#endif

//************************************************************************************************
// FileResource
//************************************************************************************************

DEFINE_CLASS_ABSTRACT (FileResource, Object)
DEFINE_CLASS_NAMESPACE (FileResource, NAMESPACE_CCL)

//////////////////////////////////////////////////////////////////////////////////////////////////

FileResource::FileResource ()
: openCount (0)
{
	#if DEBUG_OPEN_FILES
	debugFiles.addFile (this);
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FileResource::FileResource (UrlRef _path)
: path (_path),
  openCount (0)
{
	#if DEBUG_OPEN_FILES
	debugFiles.addFile (this);
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FileResource::FileResource (const FileResource& other)
: path (other.path),
  openCount (0)
{
	#if DEBUG_OPEN_FILES
	debugFiles.addFile (this);
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FileResource::~FileResource ()
{
	ASSERT (openCount == 0)

	#if DEBUG_OPEN_FILES
	debugFiles.removeFile (this);
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileResource::setPath (UrlRef _path)
{
	//ASSERT (path.isEmpty () != 0)
	if(/*!path.isEmpty () ||*/ isOpen ())
		return false;

	path.assign(_path);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UrlRef CCL_API FileResource::getPath () const
{
	return path;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileResource::open (int mode)
{
	bool result = true;

	if(openCount == 0)
		result = openFile (mode);

	if(result)
		openCount++;

	#if DEBUG_OPEN_FILES
	debugFiles.onFileOpened (this, openCount);
	#endif

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileResource::create (int mode)
{
	// file can only be created by one user!
	ASSERT (openCount == 0)
	if(openCount != 0)
		return false;

	bool result = createFile (mode);
	if(result)
		openCount++;
	return result;
}


//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileResource::close ()
{
	if(!isOpen ())
		return true;

	ASSERT (openCount > 0)

	bool result = true;
	if(openCount-1 == 0)
		result = closeFile ();

	if(result)
		openCount--;

	#if DEBUG_OPEN_FILES
	debugFiles.onFileClosed (this, openCount);
	#endif

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileResource::isExisting () const
{
	return isOpen () || System::GetFileSystem ().fileExists (path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileResource::isOpen () const
{
	return openCount > 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileResource::deletePhysical (int mode)
{
	return System::GetFileSystem ().removeFile (path, mode);
}

//************************************************************************************************
// FileStreamResource
//************************************************************************************************

FileStreamResource::FileStreamResource ()
: file (nullptr),
  options (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

FileStreamResource::FileStreamResource (UrlRef path)
: FileResource (path),
  file (nullptr),
  options (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

FileStreamResource::FileStreamResource (const FileStreamResource& other)
: FileResource (other),
  file (nullptr),
  options (other.options)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

FileStreamResource::~FileStreamResource ()
{
	ASSERT (file == nullptr)
	if(file)
		file->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileStreamResource::openFile (int mode)
{
	ASSERT (openCount == 0 && file == nullptr)
	if(isOpen () || path.isEmpty ())
		return false;

	file = System::GetFileSystem ().openStream (path, mode | IStream::kOpenMode);
	
	return file != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileStreamResource::createFile (int mode)
{
	ASSERT (openCount == 0 && file == nullptr)
	if(isOpen () || path.isEmpty ())
		return false;

	file = System::GetFileSystem ().openStream (path, mode | options | IStream::kCreateMode);
	return file != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileStreamResource::closeFile ()
{
	if(file)
	{
		file->release ();
		file = nullptr;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStream* FileStreamResource::getStream ()
{
	return file;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileStreamResource::setIOOptions (int _options)
{
	UnknownPtr<INativeFileStream> s = file;
	if(s)
		s->setOptions (options = _options);
}
