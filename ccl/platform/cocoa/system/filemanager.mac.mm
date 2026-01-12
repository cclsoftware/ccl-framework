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
// Filename    : ccl/platform/cocoa/filemanager.mac.mm
// Description : Mac file system manager
//
//************************************************************************************************

#include "filemanager.mac.h"

#include "ccl/platform/cocoa/macutils.h"

#include "ccl/public/system/ifilesystemsecuritystore.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/text/istringdict.h"

#include <CoreServices/CoreServices.h>

namespace CCL {

//************************************************************************************************
// CocoaFSEventStream
//************************************************************************************************

class CocoaFSEventStream
{
public:
	CocoaFSEventStream (MacFileManager* filemanager);
	~CocoaFSEventStream ();
	
	bool addPath (NSString* path, NSRegularExpression* pathFilter);
	void removePath (NSString* path);
	
	void onFileChanged (NSString* path, FSEventStreamEventFlags changeFlags);
	
protected:
	MacFileManager* filemanager;
	FSEventStreamRef stream;
	FSEventStreamCreateFlags flags;
	FSEventStreamContext context;
	CFAbsoluteTime latency;
	NSMutableArray* pathsToWatch;
	NSMutableArray* filterForPath;

	bool start ();
	void stop ();
};

static void streamCallback (ConstFSEventStreamRef streamRef, void* clientCallBackInfo, size_t numEvents, void* eventPaths, const FSEventStreamEventFlags eventFlags[], const FSEventStreamEventId eventIds[])
{
	CocoaFSEventStream* stream = (CocoaFSEventStream*)clientCallBackInfo;
	if(!stream)
		return;
	
	NSArray* paths = (NSArray*)eventPaths;
	for(int i = 0; i < numEvents; i++)
		stream->onFileChanged ((NSString*)paths[i], eventFlags[i]);
}

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

CocoaFSEventStream::CocoaFSEventStream (MacFileManager* _filemanager)
: stream (nullptr),
  filemanager (_filemanager),
  flags (kFSEventStreamCreateFlagUseCFTypes|kFSEventStreamCreateFlagFileEvents),
  latency (3.0)
{
	context.version = 0;
	context.release = 0;
	context.retain = 0;
	context.info = this;
	
	pathsToWatch = [[NSMutableArray alloc] initWithCapacity:1];
	filterForPath = [[NSMutableArray alloc] initWithCapacity:1];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CocoaFSEventStream::~CocoaFSEventStream ()
{
	stop ();
	
	if(pathsToWatch)
		[pathsToWatch release];
		
	if(filterForPath)
		[filterForPath release];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CocoaFSEventStream::addPath (NSString* newPath, NSRegularExpression* pathFilter)
{
	for(NSString* path in pathsToWatch)
		if([path isEqualToString:newPath])
			return false;
			
	[pathsToWatch addObject:newPath];
	[filterForPath addObject:pathFilter];
	
	stop ();
	return start ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaFSEventStream::removePath (NSString* remove)
{
	NSUInteger index = NSNotFound;
	for(NSString* path in pathsToWatch)
		if([path isEqualToString:remove])
		{
			index = [pathsToWatch indexOfObject:path];
			break;
		}
		
	if(index != NSNotFound)
	{
		[pathsToWatch removeObjectAtIndex:index];
		[filterForPath removeObjectAtIndex:index];
		
		stop ();
		if([pathsToWatch count] > 0)
			start ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaFSEventStream::onFileChanged (NSString* path, FSEventStreamEventFlags changeFlags)
{
	if(!filemanager)
		return;
	
	if(!(changeFlags & (kFSEventStreamEventFlagItemCreated|kFSEventStreamEventFlagItemRemoved|kFSEventStreamEventFlagItemModified|kFSEventStreamEventFlagItemRenamed)))
		return;
	
	NSURL* parentUrl = [[NSURL fileURLWithPath:path] URLByDeletingLastPathComponent];
	NSString* parentPath = [parentUrl path];

	NSUInteger index = NSNotFound;
	for(NSString* rootPath in pathsToWatch)
    {
        // Use rangeOfString over hasPrefix to compare case insensitive.
        NSRange prefixRange = [parentPath rangeOfString:rootPath
                                options:(NSAnchoredSearch | NSCaseInsensitiveSearch)];
        if(prefixRange.location == 0 && prefixRange.length > 0)
        {
			index = [pathsToWatch indexOfObject:rootPath];
			// If this results in an early return later, look for a better match
			NSRegularExpression* filter = [filterForPath objectAtIndex:index];
			if([filter numberOfMatchesInString:path options:0 range:NSMakeRange (0, [path length])] != 0)
				break;
		}
    }
	
	if(index == NSNotFound)
		return;
		
	NSRegularExpression* filter = [filterForPath objectAtIndex:index];
	if([filter numberOfMatchesInString:path options:0 range:NSMakeRange (0, [path length])] == 0)
		return;
		
	String urlString;
	urlString.appendNativeString (path);
	Url url;
	url.fromNativePath (StringChars (urlString), changeFlags & kFSEventStreamEventFlagItemIsDir ? IUrl::kFolder : IUrl::kFile);

	if(changeFlags & kFSEventStreamEventFlagItemCreated)
		filemanager->signalFileCreated (url, true);
	
	if(changeFlags & kFSEventStreamEventFlagItemRemoved)
		filemanager->signalFileRemoved (url, true);
	
	if(changeFlags & kFSEventStreamEventFlagItemModified)
		filemanager->signalFileChanged (url, true);
		
	if(changeFlags & kFSEventStreamEventFlagItemRenamed)
		filemanager->signalFileChanged (url, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CocoaFSEventStream::start ()
{
	System::ThreadSleep (100); // give the file system some time to process changes which might have happened immediately before on the same call stack
	stream = FSEventStreamCreate (kCFAllocatorDefault, &streamCallback, &context, (CFMutableArrayRef)pathsToWatch, kFSEventStreamEventIdSinceNow, latency, flags);
	FSEventStreamScheduleWithRunLoop (stream, CFRunLoopGetCurrent (), kCFRunLoopDefaultMode);
	BOOL success = FSEventStreamStart (stream);
	ASSERT (success)
	
	return success;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaFSEventStream::stop ()
{
	if(!stream)
		return;
		
	FSEventStreamStop (stream);
	FSEventStreamInvalidate (stream);
	FSEventStreamRelease (stream);
	
	stream = 0;
}

//************************************************************************************************
// MacFileManager
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (FileManager, MacFileManager)

//////////////////////////////////////////////////////////////////////////////////////////////////

MacFileManager::MacFileManager ()
{
	eventStream = NEW CocoaFSEventStream (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MacFileManager::~MacFileManager ()
{
	ASSERT (eventStream == nullptr)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API MacFileManager::terminate ()
{
	delete eventStream;
	eventStream = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult MacFileManager::startWatching (UrlRef url, int flags)
{
	ASSERT (System::IsInMainThread ())
	if(!System::IsInMainThread ())
		return kResultWrongThread;
	
	NSMutableString* filterString = [NSMutableString stringWithString:@"^"];

	NSURL* rootUrl = [NSURL alloc];
	MacUtils::urlToNSUrl (url, rootUrl);

	if(url.isFolder ())
	{
		if(!(flags & IFileManager::kDeep))
		{
			// match only files in rootURL directory, not in subdirectories
			[filterString appendString:[rootUrl path]];
			[filterString appendString:@"\\/[^\\/]+$"];
		}
	}
	else
	{
		// match only one file in a directory
		[filterString appendString:[rootUrl path]];
		[filterString appendString:@"$"];
		rootUrl = [rootUrl URLByDeletingLastPathComponent];
	}

	NSRegularExpression* filter = [NSRegularExpression regularExpressionWithPattern:filterString options:NSRegularExpressionCaseInsensitive error:nil];
	if(eventStream->addPath ([rootUrl path], filter))
		return kResultOk;

	return kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult MacFileManager::stopWatching (UrlRef url)
{
	ASSERT (System::IsInMainThread ())
	if(!System::IsInMainThread ())
		return kResultWrongThread;
	
	NSURL* rootUrl = [NSURL alloc];
	MacUtils::urlToNSUrl (url, rootUrl);
	
	if(url.isFile ())
		rootUrl = [rootUrl URLByDeletingLastPathComponent];
	
	eventStream->removePath ([rootUrl path]);
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult MacFileManager::startUsing (UrlRef url)
{
	NSURL* nsurl = [NSURL alloc];
	MacUtils::urlToNSUrl (url, nsurl);
	BOOL success = [nsurl startAccessingSecurityScopedResource];
	if(success == NO)
		return kResultFailed;
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult MacFileManager::stopUsing (UrlRef url)
{
	NSURL* nsurl = [NSURL alloc];
	MacUtils::urlToNSUrl (url, nsurl);
	[nsurl stopAccessingSecurityScopedResource];
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API MacFileManager::getFileDisplayString (String& string, UrlRef url, int type) const
{
	if(type == Url::kStringDisplayPath)
	{
		NSURL* nsurl = [NSURL alloc];
		MacUtils::urlToNSUrl (url, nsurl);
		NSArray* components = [[NSFileManager defaultManager] componentsToDisplayForPath:[nsurl path]];
		NSMutableString* displayString = [NSMutableString stringWithString:@""];

		if([components containsObject:@"iCloud Drive"])
		{
			NSUInteger index = [components indexOfObject:@"iCloud Drive"];
			NSRange indexRange = NSMakeRange (index, [components count] - index);
			NSIndexSet* indexSet = [NSIndexSet indexSetWithIndexesInRange:indexRange];
			NSArray* pathComponents = [components objectsAtIndexes:indexSet];

			[displayString appendString:[pathComponents componentsJoinedByString:@"/"]];
		}
		else if([components containsObject:@"Mobile Documents"])
		{
			NSUInteger index = [components indexOfObject:@"Mobile Documents"];
			if([components count] - index >= 3 && [components objectAtIndex:index + 1] == [components objectAtIndex:index + 2])
			{
				NSRange indexRange = NSMakeRange (index + 2, [components count] - index - 2);
				NSIndexSet* indexSet = [NSIndexSet indexSetWithIndexesInRange:indexRange];
				NSArray* pathComponents = [components objectsAtIndexes:indexSet];
				[displayString appendFormat:@"iCloud Drive/%@", [pathComponents componentsJoinedByString:@"/"]];
			}
			else
			{
				return FileManager::getFileDisplayString (string, url, type);
			}
		}
		else if([components containsObject:@"Containers"])
		{
			NSUInteger index = [components indexOfObject:@"Containers"];
			int indexOffset = 4;
			if(index + indexOffset <= [components count] && [[components objectAtIndex:index + 2] isEqualToString:@"Data"])
			{
				NSRange indexRange = NSMakeRange (index + indexOffset, [components count] - (index + indexOffset));
				NSIndexSet* indexSet = [NSIndexSet indexSetWithIndexesInRange:indexRange];
				NSArray* pathComponents = [components objectsAtIndexes:indexSet];
				[displayString appendFormat:@"Local/%@", [pathComponents componentsJoinedByString:@"/"]];
			}
			else
			{
				return FileManager::getFileDisplayString (string, url, type);
			}
		}
		else
		{
			return FileManager::getFileDisplayString (string, url, type);
		}
		
		string.empty ();
		string.appendNativeString (displayString);

		return true;
	}

	return FileManager::getFileDisplayString (string, url, type);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API MacFileManager::getFileLocationType (UrlRef url) const
{
	NSURL* nsurl = [NSURL alloc];
	MacUtils::urlToNSUrl (url, nsurl);
	NSArray* components = [[NSFileManager defaultManager] componentsToDisplayForPath:[nsurl path]];

	if([components containsObject:@"iCloud Drive"])
	{
		return  FileLocationType::kICloud;
	}
	else if([components containsObject:@"Google Drive"])
	{
		return FileLocationType::kGoogleDrive;
	}
	else if([components containsObject:@"Dropbox"])
	{
		return FileLocationType::kDropBox;
	}
	else if([components containsObject:@"OneDrive"])
	{
		return FileLocationType::kOneDrive;
	}

	return FileManager::getFileLocationType (url);
}
