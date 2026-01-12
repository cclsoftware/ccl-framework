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
// Filename    : ccl/platform/cocoa/system/filemanager.ios.mm
// Description : iOS file system manager
//
//************************************************************************************************

#include "filemanager.ios.h"

#include "ccl/base/asyncoperation.h"
#include "ccl/public/text/translation.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/systemservices.h"
#include "ccl/platform/cocoa/macutils.h"

#include "ccl/public/base/ccldefpush.h"

#include <CoreServices/CoreServices.h>

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("IOSFileManager")
    XSTRING (FileProvider, "Storage")
    XSTRING (OtherApplication, "Other Application")
END_XSTRINGS

//************************************************************************************************
// UIDocument
/**
 Provides apple's implementation to synchronize files from and to cloud provider which implement apple's file
 provider extension interface. It can be used to trigger downloads and uploads.
*/
//************************************************************************************************

@interface CCL_ISOLATED (UIDocument): UIDocument
- (BOOL)readFromURL:(NSURL*)url error:(NSError* _Nullable*)outError;
- (BOOL)writeContents:(id)contents toURL:(NSURL*)url forSaveOperation:(UIDocumentSaveOperation)saveOperation originalContentsURL:(nullable NSURL*)originalContentsURL error:(NSError**)outError;
- (void)handleError:(NSError*)error userInteractionPermitted:(BOOL)userInteractionPermitted;
@end

//////////////////////////////////////////////////////////////////////////////////////////////////

@implementation CCL_ISOLATED (UIDocument)

- (BOOL)readFromURL:(NSURL*)url error:(NSError* _Nullable*)outError;
{
	return YES;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)writeContents:(id)contents toURL:(NSURL*)url forSaveOperation:(UIDocumentSaveOperation)saveOperation originalContentsURL:(nullable NSURL*)originalContentsURL error:(NSError**)outError
{
	return YES;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)handleError:(NSError*)error userInteractionPermitted:(BOOL)userInteractionPermitted
{
	NSError* underlyingError = [[error userInfo] objectForKey:@"NSUnderlyingError"];

	if(underlyingError && [[underlyingError domain] isEqualToString:@"trash"])
		[[NSFileManager defaultManager] removeItemAtURL:[self presentedItemURL] error:nil];
}

@end

//************************************************************************************************
// IOSFileManager
//************************************************************************************************

DEFINE_CLASS (IOSFileManager, FileManager)

DEFINE_EXTERNAL_SINGLETON (FileManager, IOSFileManager)

//////////////////////////////////////////////////////////////////////////////////////////////////

IOSFileManager::IOSFileManager ()
{
	// TODO
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IOSFileManager::~IOSFileManager ()
{
	// TODO
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult IOSFileManager::startWatching (UrlRef url, int flags)
{
	// TODO
	return kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult IOSFileManager::stopWatching (UrlRef url)
{
	// TODO
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult IOSFileManager::startUsing (UrlRef url)
{
	NSURL* nsurl = [NSURL alloc];
	MacUtils::urlToNSUrl (url, nsurl);
	BOOL success = [nsurl startAccessingSecurityScopedResource];
	if(success == NO)
		return kResultFailed;
		
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult IOSFileManager::stopUsing (UrlRef url)
{
	NSURL* nsurl = [NSURL alloc];
	MacUtils::urlToNSUrl (url, nsurl);
	[nsurl stopAccessingSecurityScopedResource];

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void IOSFileManager::setWriting (UrlRef url, bool state)
{
	if(state == false)
	{
		NSURL* nsurl = [NSURL alloc];
		MacUtils::urlToNSUrl (url, nsurl);
		CCL_ISOLATED (UIDocument)* document = [[CCL_ISOLATED (UIDocument) alloc] initWithFileURL:nsurl];

		[document saveToURL:nsurl forSaveOperation:UIDocumentSaveForOverwriting completionHandler:^(BOOL success)
		{
			[NSFileCoordinator removeFilePresenter:document];
			
			[document closeWithCompletionHandler:^(BOOL success) {
				[document release];
			}];
			
		}];
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* CCL_API IOSFileManager::triggerFileUpdate (UrlRef url)
{
	NSURL* nsurl = [NSURL alloc];
	MacUtils::urlToNSUrl (url, nsurl);

	CCL_ISOLATED (UIDocument)* document = [[CCL_ISOLATED (UIDocument) alloc] initWithFileURL:nsurl];

	// Keep asyncOperation alive until finishOnMainThread. AutoPtr cannot be used here
	// since we cannot control when GCD releases the object.
	AsyncOperation* asyncOperation (NEW AsyncOperation);
	asyncOperation->setState (AsyncOperation::kStarted);

	void(^finishOnMainThread) (IAsyncInfo::State state) = ^(IAsyncInfo::State state)
	{
		if([NSThread isMainThread] == YES)
		{
			asyncOperation->setState (state);
			asyncOperation->release ();
		}
		else
		{
			dispatch_sync (dispatch_get_main_queue (), ^{
				asyncOperation->setState (state);
				asyncOperation->release ();
			});
		}
	};

	[document openWithCompletionHandler:^(BOOL success)
	{
		if(success)
		{
			[document closeWithCompletionHandler:^(BOOL success)
			{
				[document release];
				if([[NSFileManager defaultManager] fileExistsAtPath:[nsurl path]])
					finishOnMainThread (AsyncOperation::kCompleted);
				else
					finishOnMainThread (AsyncOperation::kFailed);
			}];
		}
		else
		{
			[document release];
			finishOnMainThread (AsyncOperation::kFailed);
		}
	}];

	return return_shared<IAsyncOperation> (asyncOperation);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API IOSFileManager::getFileDisplayString (String& string, UrlRef url, int type) const
{
	if(type == Url::kStringDisplayPath)
	{
		Url documentPath;
		System::GetSystem ().getLocation (documentPath, System::kUserDocumentFolder);
		NSURL* nsurl = [NSURL alloc];
		MacUtils::urlToNSUrl (url, nsurl);
		NSArray* components = [[NSFileManager defaultManager] componentsToDisplayForPath:[nsurl path]];
		NSMutableString* displayString = [NSMutableString stringWithString:@""];

		bool (^buildPath)(NSUInteger, NSUInteger) = ^(NSUInteger index, NSUInteger offset)
		{
			if(index == NSNotFound)
				return false;

			NSUInteger rangeLength = [components count] - index - offset;
			if(rangeLength > 0)
			{
				NSRange indexRange = NSMakeRange (index + offset, rangeLength);
				NSIndexSet* indexSet = [NSIndexSet indexSetWithIndexesInRange:indexRange];
				@try
				{
					NSArray* pathComponents = [components objectsAtIndexes:indexSet];
					[displayString appendString:[pathComponents componentsJoinedByString:@"/"]];
				}
				@catch (NSException* e)
				{
					return false;
				}
			}

			return true;
		};

		// url points to folder in application's kUserDocumentFolder
		if(url.getPath ().contains (documentPath.getPath ()))
		{
			NSString* bundleName = [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleDisplayName"];

			// kUserDocumentFolder is in icloud container
			if([components containsObject:@"iCloud Drive"])
			{
				[displayString appendFormat:@"iCloud Drive/%@", bundleName];
			}
			// kUserDocumentFolder is on device ("on my iphone")
			else
			{
				NSString* deviceType = [[UIDevice currentDevice] localizedModel];
				[displayString appendFormat:@"%@/%@", deviceType, bundleName];
			}

			NSUInteger index = [components indexOfObject:@"Documents"];
			if(!buildPath (index, 1))
				return false;
		}
		// url points outside of kUserDocumentFolder
		else
		{
			// User generated folder in iCloud Drive, e.g. /iCloud Drive/myownfolder
			if([components containsObject:@"iCloud Drive"])
			{
				NSUInteger index = [components indexOfObject:@"iCloud Drive"];
				if(!buildPath (index, 0))
					return false;
			}
			// Other application's kUserDocumentFolder on iCloud, e.g. /iCloud Drive/Numbers
			else if([components containsObject:@"Mobile Documents"])
			{
				NSUInteger index = [components indexOfObject:@"Mobile Documents"];
				[displayString appendString:@"iCloud Drive/"];
				if(!buildPath (index, 2))
					return false;
			}
			// Other application's kUserDocumentFolder on device, e.g. /on my iphone/Numbers
			else if([components containsObject:@"Application"])
			{
				NSString* deviceType = [[UIDevice currentDevice] localizedModel];
				NSString* otherApplicationDisplayString = [XSTR (OtherApplication).createNativeString<NSString*> () autorelease];
				
				[displayString appendFormat:@"%@/%@", deviceType, otherApplicationDisplayString];

				if([components containsObject:@"Documents"])
				{
					NSUInteger index = [components indexOfObject:@"Documents"];
					[displayString appendString:@"/"];
					if(!buildPath (index, 1))
						return false;
				}
			}
			// User generated folder on iphone or 3rd party cloud provider
			else
			{
				NSString* fileProviderDisplayString = [XSTR (FileProvider).createNativeString<NSString*> () autorelease];
				
				[displayString appendString:fileProviderDisplayString];

				NSUInteger index = [components indexOfObject:@"File Provider Storage"];
				[displayString appendString:@"/"];
				if(!buildPath (index, 1))
					return false;
			}
		}

		string.empty ();
		string.appendNativeString (displayString);

		return true;
	}

	return SuperClass::getFileDisplayString (string, url, type);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API IOSFileManager::getFileLocationType (UrlRef url) const
{	
	UrlDisplayString displayPathString (url, Url::kStringDisplayPath);
	if(displayPathString.startsWith (CCLSTR ("iCloud"), false))
		return FileLocationType::kICloud;

	return SuperClass::getFileLocationType (url);
}
