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
// Filename    : ccl/platform/shared/posix/system/nativefilesystem.posix.cpp
// Description : POSIX file system class (using stdio), used as common base for Cocoa, Android
//
//************************************************************************************************

#define DEBUG_LOG 0

#include <sys/stat.h>
#include <sys/time.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>

#include "ccl/platform/shared/posix/system/nativefilesystem.posix.h"
#include "ccl/base/storage/url.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/systemservices.h"

#if CCL_PLATFORM_MAC || CCL_PLATFORM_IOS
#include <sys/mount.h>
#endif

#if CCL_PLATFORM_ANDROID || CCL_PLATFORM_LINUX
#include <sys/vfs.h>
#endif

using namespace CCL;

//************************************************************************************************
// PosixNativeFileSystem
//************************************************************************************************

void PosixNativeFileSystem::translateMode (int mode, int& oflag)
{
	oflag = 0;
	bool wantRead = mode & CCL::IStream::kReadMode;
	bool wantWrite = mode & CCL::IStream::kWriteMode;
	if(wantRead && !wantWrite)
		oflag |= O_RDONLY;
	if(!wantRead && wantWrite)
		oflag |= O_WRONLY;
	if(wantRead && wantWrite)
		oflag |= O_RDWR;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int PosixNativeFileSystem::openFileDescriptor (UrlRef url, int mode)
{
	POSIXPath path (url);
	
	int handle = 0;
	int fileFlags = 0;
	translateMode (mode, fileFlags);
	if(mode & IStream::kCreate)
	{
		fileFlags |= O_CREAT | O_TRUNC;
		mode_t oldmask = umask (0);
		handle = ::open (path, fileFlags, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH); // make newly created files world readable/writable;
		umask (oldmask);
	}
	else
		handle = ::open (path, fileFlags);

	return handle;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStream* PosixNativeFileSystem::openPlatformStream (UrlRef url, int mode)
{
	int handle = openFileDescriptor (url, mode);
	if(handle == -1)
	{
		onNativeError (errno, &url);
		return nullptr;
	}
	
	return NEW PosixFileStream (this, reinterpret_cast<void*> (handle), mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PosixNativeFileSystem::getFileInfo (FileInfo& info, UrlRef url)
{
	POSIXPath path (url);

	struct stat buf;
	int result = ::stat (path, &buf);
	if(result != 0)
		return false;

	info.fileSize = buf.st_size;
	info.createTime = UnixTime::toLocal (buf.st_ctime); // last status change, not exactly the same
	info.modifiedTime = UnixTime::toLocal (buf.st_mtime);
	info.accessTime = UnixTime::toLocal (buf.st_atime);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PosixNativeFileSystem::removeFile (UrlRef url, int mode)
{
	POSIXPath path (url);
	return ::remove (path) == 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFileIterator* CCL_API PosixNativeFileSystem::newIterator (UrlRef url, int mode)
{
	//todo: PosixVolumesIterator
	return NEW PosixFileIterator (url, mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixNativeFileSystem::createPlatformFolder (UrlRef url)
{
	POSIXPath path (url);
	return ::mkdir (path, 0777) == 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixNativeFileSystem::removePlatformFolder (UrlRef url, int mode)
{
	POSIXPath path (url);
	return ::rmdir (path) == 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PosixNativeFileSystem::fileExists (UrlRef url)
{
	POSIXPath path (url);
	if(::access (path, F_OK) != 0)
		return false;
	
	struct stat pathStat;
    ::stat (path, &pathStat);
    bool isFile = S_ISREG (pathStat.st_mode);
	
	return url.isFile () == isFile;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PosixNativeFileSystem::isWriteProtected (UrlRef url)
{
	POSIXPath path (url);
	return ::access (path, W_OK) != 0; // no write access
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PosixNativeFileSystem::getPathType (int& type, UrlRef baseFolder, StringRef fileName)
{
	Url fullUrl (baseFolder);
	fullUrl.descend (fileName);

	POSIXPath path (fullUrl);

	struct stat buf;
	int result = ::stat (path, &buf);
	if(result != 0)
		return false;
	
	type = (buf.st_mode & S_IFDIR) ? IUrl::kFolder : IUrl::kFile;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PosixNativeFileSystem::isHiddenFile (UrlRef url)
{
	String name;
	url.getName (name);
	return name.startsWith (".");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PosixNativeFileSystem::moveFile (UrlRef dstPath, UrlRef srcPath, int mode, IProgressNotify* progress)
{
	createParentFolder (dstPath); // create folder structure first

	if(!fileExists (srcPath))
		return false;

	if((mode & kDoNotOverwrite) && fileExists (dstPath))
		return false;

	// todo: kDisableWriteProtection flag
	POSIXPath oldPath (srcPath);
	POSIXPath newPath (dstPath);
	bool success = (::rename (oldPath, newPath) == 0);
	if(!success && errno == EXDEV && !(mode & kDoNotMoveAcrossVolumes))
	{
		if(copyFile (dstPath, srcPath, mode, progress))
			if(removeFile (srcPath))
				success = true;
	}
	return success;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PosixNativeFileSystem::copyFile (UrlRef dstPath, UrlRef srcPath, int mode, IProgressNotify* progress)
{
	createParentFolder (dstPath); // create folder structure first

	if(!fileExists (srcPath))
		return false;

	if((mode & kDoNotOverwrite) && fileExists (dstPath))
		return false;

	AutoPtr<IStream> srcStream = openStream (srcPath, IStream::kOpenMode);
	AutoPtr<IStream> dstStream = openStream (dstPath, IStream::kCreateMode);
	if(srcStream && dstStream)
		return System::GetFileUtilities ().copyStream (*dstStream, *srcStream, progress);

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PosixNativeFileSystem::getVolumeInfo (VolumeInfo& info, UrlRef url)
{
	POSIXPath path (url);

	struct statfs buf;
	if(::statfs (path, &buf) == 0)
	{
		// todo: type, label, serialNumber
		info.bytesTotal = buf.f_blocks * buf.f_bsize;
		info.bytesFree = buf.f_blocks * buf.f_bsize;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PosixNativeFileSystem::setFileTime (UrlRef url, const FileTime& modifiedTime)
{
	POSIXPath path (url);

	int64 timestamp = UnixTime::fromLocal (modifiedTime);

	timeval t { time_t (timestamp), 0 };
	timeval times[2] = { t, t }; // access and modification times 
	return ::utimes (path, times) == 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PosixNativeFileSystem::isCaseSensitive ()
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PosixNativeFileSystem::getWorkingDirectory (IUrl& url)
{
	POSIXPath path;
	::getcwd (path, path.size ());
	return url.fromPOSIXPath (path, IUrl::kFolder);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PosixNativeFileSystem::setWorkingDirectory (UrlRef url)
{
	POSIXPath path (url);
	return ::chdir (path) == 0 ? true : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int PosixNativeFileSystem::translateNativeError (int nativeError)
{
	switch(nativeError)
	{
	case ENOENT : return kFileNotFound;
	case EPERM :
	case EACCES : return kAccesDenied;
	case EBUSY : return kFileInUse;
	case EEXIST : return kFileExists;
	case ENOTDIR : return kNotDirectory;
	case EISDIR : return kIsDirectory;
	case EINVAL : return kInvalidArgument;
	case ENFILE :
	case EMFILE : return kTooManyOpenFiles;
	case ENOSPC : return kOutOfDiscSpace;
		
	default : return kUnknownError;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PosixNativeFileSystem::beginTransaction ()
{
	// not implemented
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PosixNativeFileSystem::endTransaction (int mode, CCL::IProgressNotify* progress)
{
	// not implemented
	return false;
}

//************************************************************************************************
// PosixFileStream
//************************************************************************************************

PosixFileStream::~PosixFileStream ()
{
	if(::close (intFromPointer (file)) == -1)
		onNativeError (errno, nullptr);
	file = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int PosixFileStream::intFromPointer (void* ptr)
{
	return *((int*)&ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API PosixFileStream::read (void* buffer, int size)
{
	if(size == 0)
		return 0;
	
	ssize_t bytesRead = ::read (intFromPointer (file), buffer, size);
	if(bytesRead == -1)
		onNativeError (errno, nullptr);
	
	return (int)bytesRead;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API PosixFileStream::write (const void* buffer, int size)
{
	if(size == 0)
		return 0;
	
	int fileHandle = intFromPointer (file);
	ssize_t bytesWritten = ::write (fileHandle, buffer, size);
	if(bytesWritten == -1)
		onNativeError (errno, nullptr);
	if(options & CCL::INativeFileStream::kWriteThru)
		::fsync (fileHandle);
	return (int)bytesWritten;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 CCL_API PosixFileStream::tell ()
{
	return seek (0, kSeekCur);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 CCL_API PosixFileStream::seek (int64 pos, int mode)
{
	off_t position = ::lseek (intFromPointer (file), pos, mode);
	if(position == -1)
		onNativeError (errno, nullptr);
	
	return position;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PosixFileStream::setOptions (int options)
{
	this->options = options;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PosixFileStream::setEndOfFile (int64 eof)
{
	bool result = true;
	int64 oldPos = tell ();
	seek (eof, kSeekSet);
	if(::ftruncate (intFromPointer (file), eof) == -1)
	{
		onNativeError (errno, nullptr);
		result = false;
	}
	if(oldPos < eof)
		seek (oldPos, kSeekSet);
	
	return result;
}

//************************************************************************************************
// PosixFileIterator
//************************************************************************************************

PosixFileIterator::PosixFileIterator (UrlRef url, int mode)
: NativeFileIterator (url, mode)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

PosixFileIterator::~PosixFileIterator ()
{
	if(iter)
		::closedir ((DIR*)iter);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IUrl* CCL_API PosixFileIterator::next ()
{
	if(iter == nullptr)
	{
		POSIXPath dirPath (*baseUrl);
		iter = ::opendir (dirPath);
		if(iter == nullptr)
			return nullptr;
	}

	while(dirent* findData = ::readdir ((DIR*)iter))
	{
		CString name (findData->d_name);
		if(name == "." || name == "..") // skip these pseudo-folders, can lead to endless recursion in client code
			continue;
	
		bool wantFolders = (mode & kFolders) != 0;
		bool wantFiles = (mode & kFiles) != 0;
		bool wantHidden = (mode & kIgnoreHidden) == 0;

		if(name.startsWith (".") && !wantHidden)
			continue;

		int type = IUrl::kFile;
		if(findData->d_type == DT_DIR)
			type = IUrl::kFolder;
		else if(findData->d_type == DT_LNK || findData->d_type == DT_UNKNOWN)
			NativeFileSystem::instance ().getPathType (type, *baseUrl, findData->d_name);

		if(type == IUrl::kFolder && !wantFolders)
			continue;
		if(type == IUrl::kFile && !wantFiles)
			continue;

		current->assign (*baseUrl);
		current->descend (findData->d_name, type);

		return current;
	}
	return nullptr;
}
