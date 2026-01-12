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
// Filename    : ccl/system/nativefilesystem.cpp
// Description : Native File System
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/system/nativefilesystem.h"

#include "ccl/base/storage/url.h"
#include "ccl/base/collections/objectlist.h"

#include "ccl/public/system/cclerror.h"
#include "ccl/public/system/threadlocal.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/text/translation.h"

namespace CCL {

//************************************************************************************************
// FileSystemThreadState
//************************************************************************************************

class FileSystemThreadState: public Object,
						     public Threading::ThreadSingleton<FileSystemThreadState>
{
public:
	PROPERTY_VARIABLE (int, firstError, FirstError)
	AutoPtr<Object> transaction;

	FileSystemThreadState ()
	: firstError (0)
	{
		CCL_PRINTF ("FileSystemThreadState ctor for thread %d\n", System::GetThreadSelfID ())
	}

	~FileSystemThreadState ()
	{
		CCL_PRINTF ("FileSystemThreadState dtor for thread %d\n", System::GetThreadSelfID ())
	}
};

} // namespace CCL

DEFINE_THREAD_SINGLETON (FileSystemThreadState)

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("FileErrors")
	XSTRING (FileInUse, "The file is in use")
	XSTRING (FileExists, "The file exists")
	XSTRING (FileNotFound, "The system cannot find the file specified")
	XSTRING (FileWriteProtected, "The file is write protected")
	XSTRING (AccesDenied, "Access is denied")
	XSTRING (InvalidArgument, "Invalid Argument")
	XSTRING (TooManyOpenFiles, "Too many open files")
	XSTRING (OutOfDiscSpace, "There is not enough space on the disk")
	XSTRING (UnknownError, "Unknown Error")
END_XSTRINGS

//************************************************************************************************
// NativeFileSystem
//************************************************************************************************

NativeFileSystem::NativeFileSystem ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStream* CCL_API NativeFileSystem::openStream (UrlRef url, int mode, IUnknown* context)
{
	// try to create folder structure first...
	if(mode & IStream::kWriteMode)
		createFolder (url);

	return openPlatformStream (url, mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API NativeFileSystem::fileExists (UrlRef url)
{
	ASSERT (false) // to be implemented by derived class!
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API NativeFileSystem::getFileInfo (FileInfo& info, UrlRef url)
{
	ASSERT (false) // to be implemented by derived class!
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API NativeFileSystem::removeFile (UrlRef url, int mode)
{
	ASSERT (false) // to be implemented by derived class!
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API NativeFileSystem::renameFile (UrlRef url, StringRef newName, int mode)
{
	Url newUrl (url);
	newUrl.setName (newName);
	return moveFile (newUrl, url, mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFileIterator* CCL_API NativeFileSystem::newIterator (UrlRef url, int mode)
{
	ASSERT (false) // to be implemented by derived class!
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool NativeFileSystem::createParentFolder (UrlRef url)
{
	Url parent (url);
	if(!parent.ascend ())
		return false;

	return createFolder (parent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API NativeFileSystem::createFolder (UrlRef url)
{
	if(url.isFile ())
		return createParentFolder (url);

	if(fileExists (url))
		return true;

	if(!createParentFolder (url))
		return false;

	return createPlatformFolder (url);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API NativeFileSystem::removeFolder (UrlRef folderUrl, int mode)
{
	if(mode & kDeleteRecursively)
	{
		AutoPtr<IFileIterator> iter = newIterator (folderUrl);
		if(iter)
		{
			const IUrl* url;
			while((url = iter->next ()) != nullptr)
			{
				if(url->getType () == Url::kFolder)
					removeFolder (*url, mode);
				else
					removeFile (*url, mode & ~kDeleteRecursively);
			}
		}
	}

	return removePlatformFolder (folderUrl, mode & ~kDeleteRecursively);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API NativeFileSystem::isCaseSensitive ()
{
	ASSERT (false) // to be implemented by derived class!
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API NativeFileSystem::getVolumeInfo (VolumeInfo& info, UrlRef url)
{
	ASSERT (false) // to be implemented by derived class!
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API NativeFileSystem::isLocalFile (UrlRef url)
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API NativeFileSystem::isHiddenFile (UrlRef url)
{
	ASSERT (false) // to be implemented by derived class!
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API NativeFileSystem::isWriteProtected (UrlRef url)
{
	if(url.isEmpty ())
		return false;

	// Test write access by creating a temporary file.
	Url fileUrl (url);
	while(fileUrl.isFile () || (fileExists (fileUrl) == false && fileUrl.isRootPath () == false))
		fileUrl.ascend ();

	fileUrl.descend ("tmp");
	System::GetFileUtilities ().makeUniqueFileName (*this, fileUrl);

	bool fileCreated = false;
	{
		AutoPtr<IStream> stream (openStream (fileUrl, IStream::kCreateMode, nullptr));
		if(stream)
			fileCreated = true;
	}

	if(fileCreated)
		removeFile (fileUrl);

	return fileCreated == false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API NativeFileSystem::getPathType (int& type, UrlRef baseFolder, StringRef fileName)
{
	ASSERT (false) // to be implemented by derived class!
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API NativeFileSystem::moveFile (UrlRef dstPath, UrlRef srcPath, int mode, IProgressNotify* progress)
{
	ASSERT (false) // to be implemented by derived class!
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API NativeFileSystem::copyFile (UrlRef dstPath, UrlRef srcPath, int mode, IProgressNotify* progress)
{
	ASSERT (false) // to be implemented by derived class!
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API NativeFileSystem::setFileTime (UrlRef url, const FileTime& modifiedTime)
{
	ASSERT (false) // to be implemented by derived class!
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ISearcher* CCL_API NativeFileSystem::createSearcher (ISearchDescription& description)
{
	ASSERT (false) // to be implemented by derived class!
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API NativeFileSystem::getWorkingDirectory (IUrl& url)
{
	ASSERT (false) // to be implemented by derived class!
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API NativeFileSystem::setWorkingDirectory (UrlRef url)
{
	ASSERT (false) // to be implemented by derived class!
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API NativeFileSystem::getFirstError (int& errorCode)
{
	FileSystemThreadState& state = FileSystemThreadState::instance ();
	if(state.getFirstError () != 0)
	{
		errorCode = state.getFirstError ();
		state.setFirstError (0);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String NativeFileSystem::getErrorString (int errorCode, const IUrl* url, int* nativeError)
{
	String string (getErrorString (errorCode));

	if(nativeError)
		string << " [" << String ().appendHexValue (*nativeError) << "]";

	if(url)
		string << " (" << UrlDisplayString (*url) << ")";

	return string;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String CCL_API NativeFileSystem::getErrorString (int errorCode)
{
	if(LocalString::hasTable () == false) // in case this function is called before translations are loaded
		return String (XSTR_REF (UnknownError).getKey ());

	switch(errorCode)
	{
	case kFileInUse:			return XSTR (FileInUse);
	case kFileExists:			return XSTR (FileExists);
	case kFileNotFound:			return XSTR (FileNotFound);
	case kFileWriteProtected:	return XSTR (FileWriteProtected);
	case kAccesDenied:			return XSTR (AccesDenied);
	case kInvalidArgument:		return XSTR (InvalidArgument);
	case kTooManyOpenFiles:		return XSTR (TooManyOpenFiles);
	case kOutOfDiscSpace:		return XSTR (OutOfDiscSpace);
	default:					return XSTR (UnknownError);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NativeFileSystem::onNativeError (int nativeError, const IUrl* url)
{
	int errorCode = translateNativeError (nativeError);
	if(errorCode == kUnknownError)
	{
		checkFirstError (errorCode);

		ccl_raise (getErrorString (errorCode, url, &nativeError));
	}
	else
		onError (errorCode, url);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NativeFileSystem::onError (int errorCode, const IUrl* url)
{
	checkFirstError (errorCode);

	ccl_raise (getErrorString (errorCode, url));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NativeFileSystem::checkFirstError (int errorCode)
{
	FileSystemThreadState& state = FileSystemThreadState::instance ();
	if(state.getFirstError () == 0)
		state.setFirstError (errorCode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API NativeFileSystem::beginTransaction ()
{
	ASSERT (false) // to be implemented by derived class!
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API NativeFileSystem::endTransaction (int, IProgressNotify*)
{
	ASSERT (false) // to be implemented by derived class!
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NativeFileSystem::setTransaction (Object* transaction)
{
	FileSystemThreadState::instance ().transaction = transaction;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Object* NativeFileSystem::getTransaction () const
{
	return FileSystemThreadState::instance ().transaction;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStream* NativeFileSystem::openPlatformStream (UrlRef url, int mode)
{
	ASSERT (false) // to be implemented by derived class!
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool NativeFileSystem::createPlatformFolder (UrlRef url)
{
	ASSERT (false) // to be implemented by derived class!
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool NativeFileSystem::removePlatformFolder (UrlRef url, int mode)
{
	ASSERT (false) // to be implemented by derived class!
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int NativeFileSystem::translateNativeError (int nativeError)
{
	ASSERT (false) // to be implemented by derived class!
	return kUnknownError;
}

//************************************************************************************************
// FileStream
//************************************************************************************************

FileStream::FileStream (NativeFileSystem* fileSystem, void* file, int options)
: fileSystem (fileSystem),
  file (file),
  options (options)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API FileStream::read (void* buffer, int size)
{
	ASSERT (false) // to be implemented by derived class!
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API FileStream::write (const void* buffer, int size)
{
	ASSERT (false) // to be implemented by derived class!
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 CCL_API FileStream::tell ()
{
	ASSERT (false) // to be implemented by derived class!
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileStream::isSeekable () const
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 CCL_API FileStream::seek (int64 pos, int mode)
{
	ASSERT (false) // to be implemented by derived class!
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void* CCL_API FileStream::getNativeFileStream ()
{
	return file;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API FileStream::setOptions (int options)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileStream::getPath (IUrl& path)
{
	ASSERT (false) // to be implemented by derived class!
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileStream::setEndOfFile (int64 eof)
{
	ASSERT (false) // to be implemented by derived class!
	return false;
}

//************************************************************************************************
// NativeFileIterator
//************************************************************************************************

NativeFileIterator::NativeFileIterator (UrlRef url, int mode)
: mode (mode),
  baseUrl (nullptr),
  current (nullptr),
  iter (nullptr)
{
	url.clone (baseUrl);
	url.clone (current);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeFileIterator::~NativeFileIterator ()
{
	baseUrl->release ();
	current->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IUrl* CCL_API NativeFileIterator::next ()
{
	ASSERT (false) // to be implemented by derived class!
	return nullptr;
}

//************************************************************************************************
// NativeVolumesIterator
//************************************************************************************************

NativeVolumesIterator::NativeVolumesIterator ()
: volumes (nullptr),
  iter (nullptr)
{
	volumes = NEW ObjectList;
	volumes->objectCleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeVolumesIterator::~NativeVolumesIterator ()
{
	if(iter)
		delete iter;
	if(volumes)
		delete volumes;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NativeVolumesIterator::construct ()
{
	iter = volumes->newIterator ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IUrl* CCL_API NativeVolumesIterator::next ()
{
	Url* url = iter ? (Url*)iter->next () : nullptr;
	return url;
}
