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
// Filename    : ccl/platform/cocoa/system/nativefilesystem.cocoa.mm
// Description : Mac OS file system class (using stdio)
//
//************************************************************************************************

#define DEBUG_LOG 0

#include <sys/stat.h>
#include <copyfile.h>
#include <errno.h>

#include "ccl/platform/cocoa/system/nativefilesystem.cocoa.h"
#include "ccl/platform/cocoa/macutils.h"

#include "ccl/base/storage/url.h"
#include "ccl/base/collections/container.h"

#include "ccl/system/nativefilesystem.h"

#include "ccl/public/base/iprogress.h"
#include "ccl/public/text/cstring.h"
#include "ccl/public/text/istringdict.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/system/ifilemanager.h"
#include "ccl/public/system/ifilesystemsecuritystore.h"
#include "ccl/public/system/isysteminfo.h"

#include "ccl/public/base/ccldefpush.h"

namespace CCL {
namespace MacOS {

class CopyContext;

} // namespace MacOS
} // namespace CCL

//************************************************************************************************
// NSURLSecurityScopeGuard
//************************************************************************************************

class NSURLSecurityScopeGuard
{
public:
    NSURLSecurityScopeGuard (NSURL* _url)
    : url (_url),
      secureScope (false)
    {
        if(url)
        {
            [url retain];
            secureScope = [url startAccessingSecurityScopedResource];
        }

    };
    
    ~NSURLSecurityScopeGuard ()
    {
        if(url)
        {
            if(secureScope)
                [url stopAccessingSecurityScopedResource];
    
            [url release];
        }
    };

private:
    NSURL* url;
    bool secureScope;
};


using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

static NSCalendar* sharedCalendar ()
{
	static NSCalendar* gregorian = nil;
	if(!gregorian) {
		gregorian = [[NSCalendar alloc] initWithCalendarIdentifier:NSCalendarIdentifierGregorian];
		[gregorian setTimeZone:[NSTimeZone defaultTimeZone]];
	}
	return gregorian;
}

//************************************************************************************************
// MacOS::CopyContext
//************************************************************************************************

struct MacOS::CopyContext
{
	CopyContext (IProgressNotify* notifier, off_t bytes)
	: finished (false), success (false), progress (notifier), totalBytes (bytes)
	{};
	
	static int copyfile_callback (int what, int stage, copyfile_state_t state, const char * src, const char * dst, void * ctx);
	
	IProgressNotify* progress;
	bool finished;
	bool success;
	off_t totalBytes;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

int MacOS::CopyContext::copyfile_callback (int what, int stage, copyfile_state_t state, const char * src, const char * dst, void * ctx)
{
	CopyContext* context = (CopyContext*) ctx;
	if(!context)
		return COPYFILE_QUIT;
	
	IProgressNotify* progress = context->progress;
	if(progress)
	{
		if(progress->isCanceled ())
			return COPYFILE_QUIT;
		
		if(what == COPYFILE_COPY_DATA)
		{
			if(context->totalBytes > 0)
			{
				off_t completedBytes;
				copyfile_state_get (state,COPYFILE_STATE_COPIED, &completedBytes);
				context->progress->updateProgress (IProgressNotify::State ((double) completedBytes / (double) context->totalBytes));
			}
		}
	}
	
	if(stage == COPYFILE_FINISH)
	{
		context->finished = true;
		context->success = true;
	}
	
	if(stage == COPYFILE_ERR)
	{
		context->finished = true;
		context->success = false;
	}
	
	return COPYFILE_CONTINUE;
}

//************************************************************************************************
// CocoaNativeFileSystem
//************************************************************************************************

NativeFileSystem& NativeFileSystem::instance ()
{
	static CocoaNativeFileSystem theInstance;
	return theInstance;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStream* CocoaNativeFileSystem::openPlatformStream (UrlRef url, int mode)
{
	NSURL* nsUrl = [NSURL alloc];
	MacUtils::urlToNSUrl (url, nsUrl);
    bool secureScopeResource = false;
	bool secureScopeParent = false;

	Variant data;
	System::GetFileSystemSecurityStore ().getSecurityData (data, url);
	String base64 = data.asString ();

    if(!base64.isEmpty ())
	{
		NSError* error;
		if([nsUrl checkResourceIsReachableAndReturnError:&error])
			secureScopeResource = ([nsUrl startAccessingSecurityScopedResource] == YES);
		else
		{
			Url parent = url;
			parent.ascend ();
			NSURL* parentNSUrl = [NSURL alloc];
			MacUtils::urlToNSUrl (parent, parentNSUrl);
			if([parentNSUrl checkResourceIsReachableAndReturnError:&error])
			{
				System::GetFileManager ().setFileUsed (parent, true);
				secureScopeParent = true;
			}
		}
	}
	
    
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
		
		if(System::GetSystem ().isProcessSandboxed ())
		{
			secureScopeResource = [nsUrl startAccessingSecurityScopedResource] == YES;
		}
	}
	else
	{		
		handle = ::open (path, fileFlags);
	}
	
	if(handle == -1)
	{
		onNativeError (errno, &url);
		return nullptr;
	}
	return NEW CocoaFileStream (this, reinterpret_cast<void*> (handle), mode, nsUrl, secureScopeResource, secureScopeParent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CocoaNativeFileSystem::getFileInfo (FileInfo& info, UrlRef url)
{
	NSURL* nsUrl = [NSURL alloc];
	MacUtils::urlToNSUrl (url, nsUrl);
    NSURLSecurityScopeGuard urlGuard (nsUrl);

	NSDictionary* resourceValues = [nsUrl resourceValuesForKeys:@[NSURLCreationDateKey, NSURLContentModificationDateKey, NSURLContentAccessDateKey, NSURLFileSizeKey] error:nil];

    if(resourceValues == nil || [resourceValues count] == 0)
		return false;
    
	NSCalendar* gregorianCalendar = sharedCalendar ();
 	[gregorianCalendar setTimeZone:[NSTimeZone defaultTimeZone]];
	unsigned unitFlags = NSCalendarUnitYear | NSCalendarUnitMonth |  NSCalendarUnitDay | NSCalendarUnitHour | NSCalendarUnitMinute | NSCalendarUnitSecond ;
	NSDateComponents* components;
	
	NSDate* date = nil;

	date = [resourceValues objectForKey:NSURLCreationDateKey];
	if(date)
	{
		components = [gregorianCalendar components:unitFlags fromDate:date];
		info.createTime.setTime (Time ((int)components.hour, (int)components.minute, (int)components.second, 0));
		info.createTime.setDate (Date ((int)components.year, (int)components.month, (int)components.day));
	}
	
	date = [resourceValues objectForKey:NSURLContentModificationDateKey];
	if(date)
	{
		components = [gregorianCalendar components:unitFlags fromDate:date];
		info.modifiedTime.setTime (Time ((int)components.hour, (int)components.minute, (int)components.second, 0));
		info.modifiedTime.setDate (Date ((int)components.year, (int)components.month, (int)components.day));
	}
	
	date = [resourceValues objectForKey:NSURLContentAccessDateKey];
	if(date)
	{
		components = [gregorianCalendar components:unitFlags fromDate:date];
		info.accessTime.setTime (Time ((int)components.hour, (int)components.minute, (int)components.second, 0));
		info.accessTime.setDate (Date ((int)components.year, (int)components.month, (int)components.day));
	}
	
	NSNumber* fileSizeNumber = [resourceValues objectForKey:NSURLFileSizeKey];
	info.fileSize = [fileSizeNumber longLongValue];
    
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CocoaNativeFileSystem::removeFile (UrlRef url, int mode)
{
	NSURL* nsUrl = [NSURL alloc];
	MacUtils::urlToNSUrl (url, nsUrl);
    NSURLSecurityScopeGuard urlGuard (nsUrl);
	NSFileManager* fileManager = [NSFileManager defaultManager];
	NSError* error = nil;
	bool success;

	if(mode & kDeleteToTrashBin)
	{
		success = [fileManager trashItemAtURL:nsUrl resultingItemURL:nil error:&error];
		// Dropbox does delete the file permenantly and fails with this error code.
		if(error.code == NSXPCConnectionInterrupted)
		{
			success = true;
		}
	}
	else
		success = [fileManager removeItemAtURL:nsUrl error:&error];
    
	if(error)
		onNativeError ((int)[error code], &url);
	
	return success;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CocoaNativeFileSystem::renameFile (UrlRef url, StringRef newName, int mode)
{
	Url newUrl (url);
	newUrl.setName (newName);
	
	NSError* error = nil;
	NSURL* nsUrl = [NSURL alloc];
	MacUtils::urlToNSUrl (url, nsUrl);
    NSURLSecurityScopeGuard urlGuard (nsUrl);
    
	NSURL* nsNewUrl = [NSURL alloc];
	MacUtils::urlToNSUrl (newUrl, nsNewUrl);
    NSURLSecurityScopeGuard newUrlGuard (nsNewUrl);
	
	bool success = [[NSFileManager defaultManager] moveItemAtURL:nsUrl toURL:nsNewUrl error:&error];
    
	if(!success)
		onNativeError ((int)error.code, &url);
	
	return success;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFileIterator* CCL_API CocoaNativeFileSystem::newIterator (UrlRef url, int mode)
{
	if(url.getHostName ().isEmpty () && url.getPath ().isEmpty ())
		return NEW CocoaVolumesIterator;
	else
		return NEW CocoaFileIterator (url, mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CocoaNativeFileSystem::createPlatformFolder (UrlRef url)
{
	NSURL* nsUrl = [NSURL alloc];
	MacUtils::urlToNSUrl (url, nsUrl);
	NSError* error = nil;
	mode_t oldmask = umask (0);
	
	NSDictionary* fileAttributes = @{ NSFilePosixPermissions: [NSNumber numberWithShort: S_IRWXU | S_IRWXG | S_IRWXO] };
	
	bool success = [[NSFileManager defaultManager] createDirectoryAtURL:nsUrl withIntermediateDirectories:YES attributes:fileAttributes error:&error];
	umask (oldmask);
	
	if(error)
		onNativeError ((int)[error code], &url);
	return success;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CocoaNativeFileSystem::removePlatformFolder (UrlRef url, int mode)
{
	if(mode & kDeleteToTrashBin)
		return removeFile (url, kDeleteToTrashBin) != 0;

	return PosixNativeFileSystem::removePlatformFolder (url, mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CocoaNativeFileSystem::fileExists (UrlRef url)
{
	#if CCL_PLATFORM_IOS
	NSURL* nsUrl = [NSURL alloc];
	MacUtils::urlToNSUrl (url, nsUrl);
    NSURLSecurityScopeGuard urlGuard (nsUrl);
    
	int type = url.getType ();
	BOOL isDirectory = false;
    
	BOOL isExisting = [[NSFileManager defaultManager] fileExistsAtPath:[nsUrl path] isDirectory:&isDirectory];
    
	if(!isExisting)
		return false;
	
	if(type == IUrl::kFile && isDirectory)
	{
        if(MacUtils::isBundle (nsUrl))
			return true; // treat bundles as files, not directories
		return false;
	}
	if(type == IUrl::kFolder && !isDirectory)
		return false;
	
	return true;
	
	#else
	POSIXPath path (url);
	struct stat statStruct;
	int result = stat (path, &statStruct);

	if(result != 0) return false;
	int type = url.getType ();
	if(type==IUrl::kFile && (statStruct.st_mode & S_IFDIR))
	{
		NSURL* nsUrl = [NSURL alloc];
		MacUtils::urlToNSUrl (url, nsUrl);
        NSURLSecurityScopeGuard urlGuard (nsUrl);
        
        if(MacUtils::isBundle (nsUrl))
            return true; // treat bundles as files, not directories
		
		return false;
	}
	if(type==IUrl::kFolder && !(statStruct.st_mode & S_IFDIR))
		return false;
		
	return true;
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CocoaNativeFileSystem::isWriteProtected (UrlRef url)
{
	NSURL* nsUrl = [NSURL alloc];
	MacUtils::urlToNSUrl (url, nsUrl);
    NSURLSecurityScopeGuard urlGuard (nsUrl);
	NSNumber* isWritable;
    
	if([nsUrl getResourceValue:&isWritable forKey:NSURLIsWritableKey error:nil])
		return ![isWritable boolValue];
	else
		return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CocoaNativeFileSystem::getPathType (int& type, UrlRef baseFolder, StringRef fileName)
{
	NSURL* nsUrl = [NSURL alloc];
	MacUtils::urlToNSUrl (baseFolder, nsUrl);
    NSURLSecurityScopeGuard urlGuard (nsUrl);
	NSString* fileNameString = fileName.createNativeString<NSString*>();
	nsUrl = [nsUrl URLByAppendingPathComponent:fileNameString];
	
	NSNumber* isDirectory;
	if([nsUrl getResourceValue:&isDirectory forKey:NSURLIsDirectoryKey error:nil])
	{
		type = [isDirectory boolValue] ? IUrl::kFolder : IUrl::kFile;
		if(type == IUrl::kFolder && MacUtils::isBundle (nsUrl))
			type = IUrl::kFile;
		return true;
	}
	else
		return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CocoaNativeFileSystem::isHiddenFile (UrlRef url)
{
	NSURL* nsUrl = [NSURL alloc];
	MacUtils::urlToNSUrl (url, nsUrl);
    NSURLSecurityScopeGuard urlGuard (nsUrl);
	
	NSNumber* isHidden;
	if([nsUrl getResourceValue:&isHidden forKey:NSURLIsHiddenKey error:nil])
		return [isHidden boolValue];
	else
		return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CocoaNativeFileSystem::moveFile (UrlRef dstPath, UrlRef srcPath, int mode, IProgressNotify* progress)
{
    NSURL* nsDstPath = [NSURL alloc];
    MacUtils::urlToNSUrl (dstPath, nsDstPath);
    NSURLSecurityScopeGuard dstUrlGuard (nsDstPath);
    
	createParentFolder (dstPath); // create folder structure first
	bool overwrite = (mode & NativeFileSystem::kDoNotOverwrite) == 0;
	if(!overwrite && fileExists (dstPath))
		return false;
	
	bool shouldMoveAcrossVolumes = (mode & NativeFileSystem::kDoNotMoveAcrossVolumes) == 0;
	bool isAcrossVolumes = false;
	
	VolumeInfo volume1;
	if(!getVolumeInfo (volume1, srcPath))
		return false;
	VolumeInfo volume2;
	if(!getVolumeInfo (volume2, dstPath))
		return false;
	isAcrossVolumes = (volume1.label != volume2.label);
	
	if(!shouldMoveAcrossVolumes && isAcrossVolumes)
		return false;
	if(!isAcrossVolumes)
	{
		// do a real move instead of copy and delete source (but no progress notification)
		if(dstPath.getPath ().compareWithOptions (srcPath.getPath (), Text::kIgnoreCase) == Text::kEqual)
		{
			// only the case changed
			if(!(volume1.flags & kVolumeIsCasePreserving))
				return true; // nothing to do
		}
		else
			if(fileExists (dstPath))
				removeFile (dstPath);

		NSError* error = nil;
		NSURL* nsSrcPath = [NSURL alloc];
		MacUtils::urlToNSUrl (srcPath, nsSrcPath);
        NSURLSecurityScopeGuard srcUrlGuard (nsSrcPath);
        
		bool success = [[NSFileManager defaultManager] moveItemAtURL:nsSrcPath toURL:nsDstPath error:&error];
        
		if(!success)
			onNativeError ((int)error.code, &srcPath);
		return success;
	}
	
	copyfile_flags_t flags = COPYFILE_ALL | COPYFILE_RECURSIVE;
	if(overwrite == false)
		flags |= COPYFILE_EXCL;
	
	NSURL* srcUrl = [NSURL alloc];
	MacUtils::urlToNSUrl (srcPath, srcUrl);
    NSURLSecurityScopeGuard srcUrlGuard (srcUrl);
	
	NSNumber* fileSize;
	[srcUrl getResourceValue:&fileSize forKey:NSURLFileSizeKey error:nil];
	copyfile_state_t copyState = copyfile_state_alloc ();
	
	copyfile_callback_t callbackFunction = &MacOS::CopyContext::copyfile_callback;
	copyfile_state_set (copyState, COPYFILE_STATE_STATUS_CB, (void*) callbackFunction);
	MacOS::CopyContext context (progress, [fileSize longLongValue]);
	copyfile_state_set (copyState, COPYFILE_STATE_STATUS_CTX, &context);
	
	POSIXPath destinationPath(dstPath);
	POSIXPath sourcePath(srcPath);
	if(copyfile (sourcePath, destinationPath, copyState, flags) != 0)
		onNativeError (errno, &srcPath);
	else
		while(!context.finished)
			sleep (100000);
	copyfile_state_free (copyState);
	
	if(context.success)
		removeFile (srcPath);
    
	return context.success;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CocoaNativeFileSystem::copyFile (UrlRef dstPath, UrlRef srcPath, int mode, IProgressNotify* progress)
{
	createParentFolder (dstPath); // create folder structure first
	
	Url parent = dstPath;
	parent.ascend ();
	System::GetFileManager ().setFileUsed (parent, true);
	
	bool overwrite = (mode & NativeFileSystem::kDoNotOverwrite) == 0;
	if(!overwrite && fileExists (dstPath))
		return false;
	
	copyfile_flags_t flags = COPYFILE_ALL | COPYFILE_RECURSIVE;
	if(overwrite == false)
		flags |= COPYFILE_EXCL;
	
	NSURL* srcUrl = [NSURL alloc];
	MacUtils::urlToNSUrl (srcPath, srcUrl);
    NSURLSecurityScopeGuard srcUrlGuard (srcUrl);
    
    NSURL* dstUrl = [NSURL alloc];
    MacUtils::urlToNSUrl (dstPath, dstUrl);
    NSURLSecurityScopeGuard dstUrlGuard (dstUrl);
	
	NSNumber* fileSize = nil;
	if(![srcUrl getResourceValue:&fileSize forKey:NSURLFileSizeKey error:nil])
		return false;
	if(fileSize == nil)
		return false;
	
	copyfile_state_t copyState = copyfile_state_alloc ();
	
	copyfile_callback_t callbackFunction = &MacOS::CopyContext::copyfile_callback;
	copyfile_state_set (copyState, COPYFILE_STATE_STATUS_CB, (void*) callbackFunction);
	MacOS::CopyContext context (progress, [fileSize longLongValue]);
	copyfile_state_set (copyState, COPYFILE_STATE_STATUS_CTX, &context);
	
	POSIXPath destinationPath(dstPath);
	POSIXPath sourcePath(srcPath);
	if(copyfile (sourcePath, destinationPath, copyState, flags) != 0)
		onNativeError (errno, &srcPath);
	else
		while(!context.finished)
			usleep (100000);
	copyfile_state_free (copyState);
	
	if(context.success && (flags & COPYFILE_MOVE))
		removeFile (srcPath);
	
	System::GetFileManager ().setFileUsed (parent, false);
	
	return context.success;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CocoaNativeFileSystem::getVolumeInfo (VolumeInfo& info, UrlRef url)
{
	Url rootUrl (url);
	while(!fileExists (rootUrl))
		if(!rootUrl.ascend ()) break;
	
	NSURL* nsUrl = [NSURL alloc];
	MacUtils::urlToNSUrl (rootUrl, nsUrl);
    NSURLSecurityScopeGuard urlGuard (nsUrl);
		
	NSError* error = nil;
	NSDictionary* resourceValues = [nsUrl resourceValuesForKeys:@[ NSURLVolumeIsLocalKey, NSURLVolumeIsRemovableKey, NSURLVolumeIsInternalKey, NSURLVolumeSupportsCaseSensitiveNamesKey, NSURLVolumeSupportsCasePreservedNamesKey ] error:&error];
	
	if(error)
		return false;
	
	info.type = VolumeInfo::kUnknown;
	if(![((NSNumber*)[resourceValues objectForKey:NSURLVolumeIsLocalKey]) boolValue])
		info.type = VolumeInfo::kRemote;
	else if([((NSNumber*)[resourceValues objectForKey:NSURLVolumeIsRemovableKey]) boolValue])
		info.type = VolumeInfo::kRemovable;
	else if([((NSNumber*)[resourceValues objectForKey:NSURLVolumeIsInternalKey]) boolValue])
		info.type = VolumeInfo::kLocal;
	if([((NSNumber*)[resourceValues objectForKey:NSURLVolumeSupportsCaseSensitiveNamesKey]) boolValue])
		info.flags |= kVolumeIsCaseSensitive;
	else
		info.flags &= ~kVolumeIsCaseSensitive;

	if([((NSNumber*)[resourceValues objectForKey:NSURLVolumeSupportsCasePreservedNamesKey]) boolValue])
		info.flags |= kVolumeIsCasePreserving;
	else
		info.flags &= ~kVolumeIsCasePreserving;
	
	bool suppressSlowVolumeInfo = (info.type & kSuppressSlowVolumeInfo) != 0;
	
	info.label.empty();
	if(info.type == VolumeInfo::kLocal || !suppressSlowVolumeInfo)
	{
		resourceValues = [nsUrl resourceValuesForKeys:@[NSURLVolumeNameKey, NSURLVolumeTotalCapacityKey, NSURLVolumeAvailableCapacityKey, NSURLVolumeUUIDStringKey] error:&error];
		
		if(error)
		{
			onNativeError ((int)[error code], &url);
			return false;
		}
		
		info.label.appendNativeString ((NSString*)[resourceValues objectForKey:NSURLVolumeNameKey]);
		info.bytesTotal = [(NSNumber*)[resourceValues objectForKey:NSURLVolumeTotalCapacityKey] longLongValue];
		info.bytesFree = [(NSNumber*)[resourceValues objectForKey:NSURLVolumeAvailableCapacityKey] longLongValue];
		info.serialNumber.appendNativeString ([resourceValues objectForKey:NSURLVolumeUUIDStringKey]);
	}
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CocoaNativeFileSystem::setFileTime (UrlRef url, const FileTime& modifiedTime)
{
	NSCalendar* gregorianCalendar = [[NSCalendar alloc] initWithCalendarIdentifier:NSCalendarIdentifierGregorian];
	[gregorianCalendar setTimeZone:[NSTimeZone defaultTimeZone]];
	NSDateComponents* components = [[NSDateComponents alloc] init];
	components.year = modifiedTime.getDate ().getYear ();
	components.month = modifiedTime.getDate ().getMonth ();
	components.day = modifiedTime.getDate ().getDay ();
	components.hour = modifiedTime.getTime ().getHour ();
	components.minute = modifiedTime.getTime ().getMinute ();
	components.second = modifiedTime.getTime ().getSecond ();
	
	NSURL* nsUrl = [NSURL alloc];
	MacUtils::urlToNSUrl (url, nsUrl);
    NSURLSecurityScopeGuard urlGuard (nsUrl);
	
	NSError* error = nil;
	[nsUrl setResourceValue:[gregorianCalendar dateFromComponents:components] forKey:NSURLContentModificationDateKey error:&error];
	[components release];
	[gregorianCalendar release];
	
	if(error)
	{
		onNativeError ((int)[error code], &url);
		return false;
	}
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CocoaNativeFileSystem::isCaseSensitive ()
{
	#if CCL_PLATFORM_IOS
	return true;
	#else
	return false;  // this can only be determined on a per volume basis, typically on the Mac the filesystems are not case sensitive, just case preserving
	#endif
}

//************************************************************************************************
// CocoaFileStream
//************************************************************************************************

CocoaFileStream::CocoaFileStream (CocoaNativeFileSystem* fileSystem, void* file, int options, NSURL* _url, bool _secureScopeResource, bool _secureScopeParent)
: PosixFileStream (fileSystem, file, options),
  url (_url),
  secureScopeResource (_secureScopeResource),
  secureScopeParent (_secureScopeParent)
{
	if(url)
		[url retain];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CocoaFileStream::~CocoaFileStream ()
{
	if(url)
	{
		if(secureScopeResource)
			[url stopAccessingSecurityScopedResource];
		
		if(secureScopeParent)
		{
			Url parent;
			MacUtils::urlFromNSUrl (parent, url);
			parent.ascend ();
			
			if([url checkResourceIsReachableAndReturnError:nil])
				System::GetFileManager ().setFileUsed (parent, false);
		}
		
		[url release];
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CocoaFileStream::getPath (IUrl& path)
{
	if(url)
		return MacUtils::urlFromNSUrl (path, url);

	return false;
}

//************************************************************************************************
// CocoaFileIterator
//************************************************************************************************

CocoaFileIterator::CocoaFileIterator (UrlRef url, int mode)
: NativeFileIterator (url, mode)
{
	NSURL* nsUrl = [NSURL alloc];
	if(MacUtils::urlToNSUrl (*baseUrl, nsUrl))
	{
		nsUrl = [nsUrl URLByResolvingSymlinksInPath];
		NSDirectoryEnumerationOptions options = (mode & kIgnoreHidden) ? NSDirectoryEnumerationSkipsHiddenFiles : 0;
		NSArray* contents = [[NSFileManager defaultManager] contentsOfDirectoryAtURL:nsUrl includingPropertiesForKeys:@[ NSURLIsDirectoryKey, NSURLIsPackageKey] options:options error:nil ];
		NSMutableArray* sortedContents = [[NSMutableArray alloc] initWithArray:contents];
		[sortedContents sortUsingComparator:^NSComparisonResult (NSString* pathA, NSString* pathB)
		{
			NSString* fileNameA = [pathA lastPathComponent];
			NSString* fileNameB = [pathB lastPathComponent];
			return [fileNameA localizedCaseInsensitiveCompare: fileNameB];
		}];
		iter = [[sortedContents objectEnumerator] retain];
		[sortedContents release];
	}
	else
	{
		// baseUrl is an empty string in this case?
		ASSERT(0)
	}
	
	#if DEBUG_LOG
	MutableCString cStr (baseUrl->getPath ());
	CCL_PRINT ("File iterator on: ")
	CCL_PRINTLN (cStr.str ())
	if(iter == 0)
	{
		CCL_PRINT ("Folder not found: ");
		CCL_PRINTLN (cStr.str ())
	}
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CocoaFileIterator::~CocoaFileIterator ()
{
	if(iter)
	{
		[(id)iter release];
		iter = 0;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IUrl* CCL_API CocoaFileIterator::next ()
{
	if(iter == 0)
		return 0;

	NSEnumerator<NSURL*>* enumerator = (NSEnumerator<NSURL*>*)iter;
	bool done = false;
	bool isFolder = false;
	String fileName;
	do
	{
		NSURL* file = [enumerator nextObject];
		if(file == 0)
			return 0;
		
		done = true;
		fileName.empty ();
		fileName.appendNativeString ([file lastPathComponent]);

		NSNumber* isDirectory = nil;
		[[file URLByResolvingSymlinksInPath] getResourceValue:&isDirectory forKey:NSURLIsDirectoryKey error:nil];

		isFolder = [isDirectory boolValue];

		// present bundles as files
		if((mode & kBundlesAsFiles) && isFolder && MacUtils::isBundle (file))
				isFolder = false;
		
		bool wantFolders = (mode & kFolders) != 0;
		bool wantFiles = (mode & kFiles) != 0;
		if((isFolder && !wantFolders) || (!isFolder && !wantFiles))
			done = false;
		
		CCL_PRINT ("  -> ")
		CCL_PRINT (isFolder ? "Dir  " : "File ")
		CCL_PRINTLN (fileName)
	}
	while(!done);
	
	current->assign (*baseUrl);
	current->descend (fileName, isFolder ? IUrl::kFolder : IUrl::kFile);
	return current;
}

//************************************************************************************************
// CocoaVolumesIterator
//************************************************************************************************

CocoaVolumesIterator::CocoaVolumesIterator ()
{
	NSArray* volumes = [[NSFileManager defaultManager] mountedVolumeURLsIncludingResourceValuesForKeys:nil options:NSVolumeEnumerationSkipHiddenVolumes];
	for(NSURL* volume in volumes)
	{
		Url* path = NEW Url;
		path->fromPOSIXPath ([[@"/" stringByAppendingString:volume.path] UTF8String], IUrl::kFolder);
		this->volumes->add (path);
	}
	
	construct ();
}
