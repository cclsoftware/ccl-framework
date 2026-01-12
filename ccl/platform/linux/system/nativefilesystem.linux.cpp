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
// Filename    : ccl/platform/linux/system/nativefilesystem.linux.cpp
// Description : Linux file system
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/linux/system/nativefilesystem.linux.h"
#include "ccl/platform/linux/system/mountinfo.h"

#include "ccl/base/storage/url.h"
#include "ccl/base/storage/propertyfile.h"
#include "ccl/base/collections/container.h"

#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/vfs.h>
#include <errno.h>
#include <unistd.h>

using namespace CCL;

//************************************************************************************************
// NativeFileSystem
//************************************************************************************************

NativeFileSystem& NativeFileSystem::instance ()
{
	static LinuxNativeFileSystem theInstance;
	return theInstance;
}

//************************************************************************************************
// LinuxNativeFileSystem
//************************************************************************************************

IFileIterator* CCL_API LinuxNativeFileSystem::newIterator (UrlRef url, int mode)
{
	if(url.getHostName ().isEmpty () && url.getPath ().isEmpty ())
		return NEW LinuxVolumesIterator;
	else
		return NEW PosixFileIterator (url, mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API LinuxNativeFileSystem::getVolumeInfo (VolumeInfo& info, UrlRef rootUrl)
{
	MountInfo mountInfo;
	mountInfo.load ();

	const MountInfo::Entry* entry = mountInfo.find (rootUrl);
	if(entry == nullptr)
		return false;

	MutableCString mountSource (entry->mountSource, Text::kUTF8);
	struct stat fileInfo {};
	if(::stat (mountSource, &fileInfo) < 0)
		return false;
	
	CStringPtr type = nullptr;
	if(S_ISCHR (fileInfo.st_mode))
		type = "char";
	else if(S_ISBLK (fileInfo.st_mode))
		type = "block";
	else
		return false; // not a device

	MutableCString udevPath;
	udevPath.appendFormat ("/run/udev/data/%c%u:%u", type[0], major (fileInfo.st_rdev), minor (fileInfo.st_rdev));

	Java::PropertyFile file;
	Url fileUrl;
	fileUrl.fromPOSIXPath (udevPath);
	if(!file.loadFromFile (fileUrl))
		return false;

	const StringDictionary& properties = file.getProperties ();
	info.subType = properties.lookupValue ("E:ID_FS_TYPE");
	info.label = properties.lookupValue ("E:ID_FS_LABEL");
	if(info.label.isEmpty ())
		info.label = UrlDisplayString (rootUrl);
	info.serialNumber = properties.lookupValue ("E:ID_FS_UUID");

	MutableCString mountPoint (entry->mountPoint, Text::kUTF8);
	struct statfs fileSystemInfo {};
	if(::statfs (mountPoint, &fileSystemInfo) < 0)
		return false;

	info.bytesTotal = fileSystemInfo.f_blocks * fileSystemInfo.f_bsize;
	info.bytesFree = fileSystemInfo.f_bavail * fileSystemInfo.f_bsize;

	info.type = VolumeInfo::kLocal;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStream* LinuxNativeFileSystem::openPlatformStream (UrlRef url, int mode)
{
	int handle = openFileDescriptor (url, mode);
	if(handle == -1)
	{
		onNativeError (errno, &url);
		return nullptr;
	}
	
	return NEW LinuxFileStream (this, reinterpret_cast<void*> (handle), mode);
}

//************************************************************************************************
// LinuxFileStream
//************************************************************************************************

LinuxFileStream::LinuxFileStream (LinuxNativeFileSystem* fileSystem, void* file, int options)
: PosixFileStream (fileSystem, file, options)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API LinuxFileStream::getPath (IUrl& path)
{ 
	char filePath[STRING_STACK_SPACE_MAX] = {0};
	MutableCString fdPath ("/proc/self/fd/");
	fdPath.appendInteger (intFromPointer (file));
	ssize_t length = ::readlink (fdPath, filePath, ARRAY_COUNT (filePath));
	if(length > 0 && length + 1 < STRING_STACK_SPACE_MAX)
	{
		filePath[length] = 0;
		return path.fromPOSIXPath (filePath, IUrl::kFile);
	}
	return false;
}

//************************************************************************************************
// LinuxVolumesIterator
//************************************************************************************************

LinuxVolumesIterator::LinuxVolumesIterator ()
{
	MountInfo mountInfo;
	mountInfo.load ();

	for(const MountInfo::Entry& entry : mountInfo.getEntries ())
	{
		if(entry.mountSource.startsWith ("/") && entry.mountPoint.startsWith ("/") 
			&& !entry.mountPoint.startsWith ("/boot/")
			&& !entry.mountPoint.startsWith ("/proc/")
			&& !entry.mountPoint.startsWith ("/snap/")
			&& !entry.mountPoint.startsWith ("/sys/")
			&& !entry.mountPoint.startsWith ("/tmp/")
			&& !entry.mountPoint.startsWith ("/var/snap/")
			&& !entry.mountPoint.startsWith ("/var/lib/snapd/"))
		{
			Url* path = NEW Url;
			path->fromDisplayString (entry.mountPoint);
			path->descend ("/", IUrl::kFolder);
			volumes->add (path);
		}
	}

	construct ();
}
