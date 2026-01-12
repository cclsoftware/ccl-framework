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
// Filename    : ccl/platform/cocoa/macutils.mm
// Description : some Mac/iOS specific helper functions
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/cocoa/macutils.h"

#include "ccl/base/storage/url.h"
#include "ccl/base/storage/urlencoder.h"

#include "ccl/public/storage/iurl.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/ifilesystemsecuritystore.h"
#include "ccl/public/systemservices.h"

#include <objc/runtime.h>
#include <objc/message.h>

#if CCL_PLATFORM_MAC
BOOL automaticallyNotifiesObserversOfTouchBar (id self, SEL _cmd)
{
	return NO;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void setTouchBar (id self, SEL _cmd, NSTouchBar* touchBar)
{
	[self willChangeValueForKey:@"touchBar"];
	struct objc_super viewSuper =
	{
		.receiver = self,
		.super_class = class_getSuperclass ([self class])
	};
                                
    void(*callSuper) (objc_super*, SEL, NSTouchBar*) = (void(*)(objc_super*, SEL, NSTouchBar*))objc_msgSendSuper;
    callSuper (&viewSuper, _cmd, touchBar);

    [self didChangeValueForKey:@"touchBar"];
}
#endif

namespace CCL {
namespace MacUtils {
		
//////////////////////////////////////////////////////////////////////////////////////////////////
// Path conversion
//////////////////////////////////////////////////////////////////////////////////////////////////

bool urlFromCFURL (IUrl& url, CFURLRef cfUrlRef, int type)
{
	if(cfUrlRef == 0)
		return false;
		
	CFObj<CFURLRef> absUrl = CFURLCopyAbsoluteURL (cfUrlRef);
	CFObj<CFStringRef> pathString = CFURLCopyFileSystemPath (absUrl, kCFURLPOSIXPathStyle);
	
	char str[IUrl::kMaxLength] = {0};
	CFStringGetCString (pathString, str, IUrl::kMaxLength, kCFStringEncodingUTF8);
	
	return url.fromPOSIXPath (str, type) ? true : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool urlToCFURL (const IUrl& url, CFURLRef& cfUrlRef, int type)
{
	String path;
	url.toDisplayString (path);
	
	CFObj<CFStringRef> filePath = path.createNativeString<CFStringRef> ();
	cfUrlRef = CFURLCreateWithFileSystemPath (kCFAllocatorDefault, filePath, kCFURLPOSIXPathStyle, url.getType () == IUrl::kFolder);
	return cfUrlRef != NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool urlFromNSUrl (IUrl& url, const NSURL* nsUrl, int type, bool storeBookmark)
{
	if([nsUrl isFileURL])
	{
		String urlString;
		urlString.appendNativeString ([nsUrl path]);
		urlString.normalize (Text::kNormalizationC);
		url.fromNativePath (StringChars (urlString), type);
		if(System::GetSystem ().isProcessSandboxed () && storeBookmark)
		{
			[nsUrl startAccessingSecurityScopedResource];				
			NSError* error = nil;
			NSURLBookmarkCreationOptions options = 0;
			#if CCL_PLATFORM_MAC
			options |= NSURLBookmarkCreationWithSecurityScope;
			NSNumber* isWritable = nil;
			if([nsUrl getResourceValue:&isWritable forKey:NSURLIsWritableKey error:nil])
				if([isWritable boolValue] == NO)
					options |= NSURLBookmarkCreationSecurityScopeAllowOnlyReadAccess;
			#elif CCL_PLATFORM_IOS
			options |= NSURLBookmarkCreationMinimalBookmark;
			#endif
			
			NSData* bookmark = nil;
			
			if([nsUrl checkResourceIsReachableAndReturnError:nil])
				bookmark = [nsUrl bookmarkDataWithOptions:options includingResourceValuesForKeys:nil relativeToURL:nil error:&error];
			
			if(bookmark)
			{
				NSString* base64String = [bookmark base64EncodedStringWithOptions:0];
				String base64;
				base64.appendNativeString (base64String);
				System::GetFileSystemSecurityStore ().setSecurityData (url, base64);
			}
			ASSERT (error == nil)
			[nsUrl stopAccessingSecurityScopedResource];
		}
	}
	else
	{
		String urlString;
		urlString.appendNativeString ([nsUrl absoluteString]);
		urlString.normalize (Text::kNormalizationC);
		UrlEncoder encoder;
		url.setUrl (encoder.decode (urlString), type);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool urlToNSUrl (const IUrl& url, NSURL*& nsUrl, int type)
{
	if(url.isNativePath ())
	{
		NSString* filePath = [NSString stringWithFormat:@"/%@", [url.getPath ().createNativeString<NSString*> () autorelease]];

		Variant securityData;
		System::GetFileSystemSecurityStore ().getSecurityData (securityData, url);
		String base64 = securityData.asString ();

		if(!base64.isEmpty ())
		{
			NSError* error = nil;
			NSString* base64String = [base64.createNativeString<NSString*> () autorelease];
			NSData* bookmark = [[[NSData alloc] initWithBase64EncodedString:base64String options:0] autorelease];
			BOOL isStale = NO;
			NSURLBookmarkResolutionOptions bookMarkoptions = 0;
			#if CCL_PLATFORM_MAC
			bookMarkoptions = NSURLBookmarkResolutionWithSecurityScope;
			#endif
			nsUrl = [NSURL URLByResolvingBookmarkData:bookmark options:bookMarkoptions relativeToURL:nil bookmarkDataIsStale:&isStale error:&error];

			BOOL isInconsistent = YES;
			
			if(nsUrl != nil)
			{
				NSMutableString* normalizedPath = [NSMutableString stringWithString:[nsUrl path]];
				::CFStringNormalize ((CFMutableStringRef)normalizedPath, kCFStringNormalizationFormC);
				isInconsistent = ![normalizedPath isEqualToString:filePath];
			}
			
			BOOL couldNotBeResolved = nsUrl == nil;
			BOOL renewBookmark = isStale || couldNotBeResolved || isInconsistent;
			
			if(renewBookmark)
			{
				nsUrl = [[[NSURL alloc] initFileURLWithPath:filePath isDirectory:url.getType () == IUrl::kFolder] autorelease];
				
				NSData* newBookmark = [nsUrl bookmarkDataWithOptions:bookMarkoptions includingResourceValuesForKeys:nil relativeToURL:nil error:&error];
				
				if(newBookmark != nil)
				{
					NSString* base64String = [newBookmark base64EncodedStringWithOptions:0];
					String base64;
					base64.appendNativeString (base64String);
					System::GetFileSystemSecurityStore ().setSecurityData (url, base64);
				}
			}
		}
		else
			nsUrl = [[nsUrl initFileURLWithPath:filePath isDirectory:url.getType () == IUrl::kFolder] autorelease];
		
		return (nsUrl != 0);
	}
	else
	{
		String urlString;
		url.getUrl (urlString, true);
		NSString* nsUrlString = [urlString.createNativeString<NSString*> () autorelease];
		nsUrl = [[nsUrl initWithString:nsUrlString] autorelease];
		return (nsUrl != nil);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Bundle tools
//////////////////////////////////////////////////////////////////////////////////////////////////
	
NSBundle* bundleFromId (NSString* bundleId)
{
	for(NSBundle* bundle in [NSBundle allFrameworks])
		if([bundle.bundleIdentifier isEqualToString:bundleId])
			return bundle;
				
	return nil;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool isBundle (NSURL* url)
{
	NSNumber* isPackage = nil;
	if([url getResourceValue:&isPackage forKey:NSURLIsPackageKey error:nil])
		if([isPackage boolValue])
			return true;

	if([[url pathExtension] isEqualToString:@"bundle"])
		return true;

	//frameworks are bundles but not packages
	if([[url pathExtension] isEqualToString:@"framework"])
		return true;

	return false;
}

} // namespace MacUtils
} // namespace CCL
